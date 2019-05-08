#include "stdafx.h"
#include "GameManager.h"
#include "Game.h"
#include "json/value.h"
#include "db/GameDB.h"
#include "db/GameRecordDB.h"
#include "db/GameHistoryDB.h"

#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/format.hpp>

#include <mutex>
#include <unordered_map>
#include <stdexcept>
#include <utility>

namespace mserver {
	namespace game {

		using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;
		using boost::system::error_code;

		inline std::string status_to_string(Game::Status status)
		{
			switch (status)
			{
			case Game::Status::Running:
				return "running";

			case Game::Status::Terminated:
				return "terminated";

			default:
				return "unknown";
			}
		}

		struct GameManager::Impl
		{
			db::Database& db_;
			boost::asio::io_service& io_;
			std::mutex mutex_;
			int next_id_ = 1;
			std::unordered_map<std::uint64_t, Game> running_games_;
			std::unordered_map<std::uint64_t, std::unique_ptr<Timer>> terminate_timers_;

			Impl(http::Handler& http_handler, db::Database& db, boost::asio::io_service& io)
				: db_(db)
				, io_(io)
			{
				http_handler.register_json_method("/game/list", [this](Json::Value&& req, Json::Value& resp)
				{
					auto& games_json = resp["games"];
					games_json = Json::ValueType::arrayValue;

					{
						std::unique_lock<std::mutex> lock(mutex_);
						for (const auto& entry : running_games_)
						{
							auto& entry_json = games_json[games_json.size()];
							entry_json["id"] = entry.first;
							entry_json["port"] = entry.second.port();
							entry_json["level_name"] = entry.second.level_name();
							entry_json["game_mode"] = (int)entry.second.game_mode();
							entry_json["status"] = status_to_string(entry.second.status());
						}
					}

					resp["succeeded"] = true;
				});

				http_handler.register_json_method("/game/console", [this](Json::Value&& req, Json::Value& resp)
				{
					int id = req["id"].asInt();

					bool succeeded = false;
					std::string console_output;

					{
						std::unique_lock<std::mutex> lock(mutex_);
						auto iter = running_games_.find(id);
						if (iter != running_games_.end())
						{
							succeeded = true;
							console_output = iter->second.console_output();
						}
					}

					resp["id"] = id;
					resp["succeeded"] = succeeded;
					resp["console_output"] = std::move(console_output);
				});

				http_handler.register_json_method("/game/terminate", [this](Json::Value&& req, Json::Value& resp)
				{
					int id = req["id"].asInt();

					resp["id"] = id;
					resp["succeeded"] = terminate_game(id);
				});
			}

			bool terminate_game(std::uint64_t game_id)
			{
				bool succeeded = false;
				Game game_to_terminate;

				{
					std::unique_lock<std::mutex> lock(mutex_);
					auto iter = running_games_.find(game_id);
					if (iter != running_games_.end())
					{
						succeeded = true;
						game_to_terminate = std::move(iter->second);
						running_games_.erase(iter);
					}
				}

				game_to_terminate = Game{};

				if (succeeded)
				{
					auto dbSession = db_.open();

					soci::transaction trans(dbSession.get_session());

					db::GameHistoryDB::set_player_rank_and_update_total_score(dbSession, game_id);

					trans.commit();
				}

				return succeeded;
			}
		};

		GameManager::GameManager() = default;
		GameManager::~GameManager() = default;

		GameManager GameManager::create(http::Handler& http_handler, db::Database& db, boost::asio::io_service& io)
		{
			GameManager res;
			res.impl_ = std::make_shared<Impl>(http_handler, db, io);
			return res;
		}

		protocol::IndexType GameManager::get_running_games_count()
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			return impl_->running_games_.size();
		}

		std::unordered_map<std::uint64_t, Game>& GameManager::get_running_games()
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			return impl_->running_games_;
		}

		boost::optional<Game&> GameManager::find_game_by_id(std::uint64_t game_id)
		{
			// TODO: insert return statement here
			for (auto& entry : impl_->running_games_)
			{
				if (entry.second.game_id() == game_id)
					return entry.second;
			}
			return boost::none;
		}

		std::uint64_t GameManager::add(Game&& g)
		{
			//Add game to SQLite Table
			mserver::db::GameDB gameDB(0, g.level_name(), g.game_name(), g.game_mode(), true, g.start_time(), g.end_time()
				,g.checkpoint_count(), g.difficulty());
			
			auto conn = impl_->db_.open();
			std::uint64_t game_id = mserver::db::GameDB::insert(conn, gameDB);			

			using namespace boost::posix_time;
			auto terminate_timer = std::make_unique<Timer>(impl_->io_);
	
			ptime now = second_clock::local_time();
			auto duration = g.posix_end_time() - now;

			if (duration.total_seconds() < 0)
				throw std::invalid_argument((boost::format{ "Game end time(%||) is not invalid." } % g.end_time()).str());

			terminate_timer->expires_after(std::chrono::seconds(duration.total_seconds()));
			
			terminate_timer->async_wait([this, game_id](const boost::system::error_code& ec) {
				terminate_game(game_id);
				impl_->terminate_timers_.erase(game_id);
			});
			impl_->terminate_timers_[game_id] = std::move(terminate_timer);

			//Add game to running list
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			g.launch(game_id);
			impl_->running_games_[game_id] = std::move(g);
			return game_id;
		}

		void GameManager::set_game_checkpointcount(std::uint64_t game_id, std::uint32_t checkpointcount)
		{
			auto conn = impl_->db_.open();
			mserver::db::GameDB::update_checkpoint_count(conn, game_id, checkpointcount);

			for (auto& game : impl_->running_games_)
				if (game.second.game_id() == game_id)
					game.second.set_checkpoint_count(checkpointcount);
		}

		bool GameManager::process_connect_player(std::uint64_t game_id, std::uint64_t member_id, std::uint32_t running_player_number, std::uint32_t waiting_time)
		{
			if (impl_->running_games_.find(game_id) == impl_->running_games_.end())
				return false;
			impl_->running_games_[game_id].running_player_number(running_player_number);
			impl_->running_games_[game_id].waiting_time(waiting_time);
			auto conn = impl_->db_.open();
			mserver::db::GameRecordDB record;
			record.game_id_ = game_id;
			record.member_id_ = member_id;
			if (mserver::db::GameRecordDB::find_by_ids(conn, game_id, member_id) != boost::none)
				mserver::db::GameRecordDB::remove(conn, game_id, member_id);
			return mserver::db::GameRecordDB::insert(conn, record);
		}

		bool GameManager::process_drop_player(std::uint64_t game_id, std::uint64_t member_id, std::uint32_t running_player_number, std::uint32_t waiting_time)
		{
			if (impl_->running_games_.find(game_id) == impl_->running_games_.end())
				return false;
			impl_->running_games_[game_id].running_player_number(running_player_number);
			impl_->running_games_[game_id].waiting_time(waiting_time);
			auto conn = impl_->db_.open();
			return mserver::db::GameRecordDB::remove(conn, game_id, member_id);
		}

		void GameManager::update_waiting_time(std::uint64_t game_id, std::uint32_t waiting_time)
		{
			if (impl_->running_games_.find(game_id) == impl_->running_games_.end())
				return;
			impl_->running_games_[game_id].waiting_time(waiting_time);
		}

		void GameManager::update_player_score(std::uint64_t game_id, std::uint64_t member_id, std::uint32_t pass_time, std::uint32_t pass_point)
		{
			//update game_record table
			auto conn = impl_->db_.open();
			soci::transaction trans(conn.get_session());

			mserver::db::GameRecordDB::update_player_score(conn, game_id, member_id, pass_time, pass_point);

			//update game_history_table
			if (is_cleard_checkpoint(game_id, pass_point))
			{
				mserver::db::GameHistoryDB::update_player_pass_time(conn, game_id, member_id, pass_time);
			}
			
			trans.commit();
		}

		bool GameManager::is_cleard_checkpoint(std::uint64_t game_id, std::uint32_t pass_point)
		{
			auto res = find_game_by_id(game_id);
			if (res == boost::none)
				return false;
			return pass_point == res->checkpoint_count();
		}

		bool GameManager::terminate_game(std::uint64_t game_id)
		{
			return impl_->terminate_game(game_id);
		}
	}
}
