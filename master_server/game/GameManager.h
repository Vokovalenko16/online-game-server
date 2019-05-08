#pragma once

#include "http/Handler.h"
#include "protocol/Types.h"
#include "db/Database.h"

#include <boost/optional.hpp>
#include <boost/asio/io_service.hpp>

#include <memory>
#include <cstdint>

namespace mserver {
	namespace game {

		class Game;

		class GameManager
		{
		private:
			struct Impl;
			std::shared_ptr<Impl> impl_;

		public:
			GameManager();
			~GameManager();

			static GameManager create(http::Handler& http_handler, db::Database& db, boost::asio::io_service& io);

			protocol::IndexType get_running_games_count();
			std::unordered_map<std::uint64_t, Game>& get_running_games();
			boost::optional<Game&> find_game_by_id(std::uint64_t);
			std::uint64_t add(Game&&);
			void set_game_checkpointcount(std::uint64_t game_id, std::uint32_t checkpointcount);
			bool process_connect_player(std::uint64_t game_id, std::uint64_t member_id, std::uint32_t running_player_number, std::uint32_t waiting_time);
			bool process_drop_player(std::uint64_t game_id, std::uint64_t member_id, std::uint32_t running_player_number, std::uint32_t waiting_time);
			void update_waiting_time(std::uint64_t game_id, std::uint32_t waiting_time);
			void update_player_score(std::uint64_t, std::uint64_t, std::uint32_t, std::uint32_t);
			bool is_cleard_checkpoint(std::uint64_t game_id, std::uint32_t pass_point);
			bool terminate_game(std::uint64_t game_id);
		};
	}
}