#include "stdafx.h"
#include "Game.h"
#include "protocol/Types.h"
#include "logger/Logger.h"
#include "core/Exception.h"

#include <system_error>
#include <thread>
#include <mutex>
#include <boost/filesystem.hpp>
#include <boost/process.hpp>
#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>

namespace mserver {
	namespace game {

		namespace fs = boost::filesystem;
		namespace bp = boost::process;

		const char GameExecutableFileName[] =
#if defined (WIN32)
			"orienteering.exe"
#elif defined (LINUX)
			"orienteering"
#endif
			;

		struct Game::Impl
		{
			int master_server_port_;
			PortManager::Reservation game_port_;
			fs::path game_dir_;
			protocol::GameMode game_mode_;
			std::string level_name_;
			std::string game_name_;
			boost::posix_time::ptime  start_time_;
			boost::posix_time::ptime  end_time_;
			std::uint64_t game_id_;
			int checkpoint_count_;
			int difficulty_;
			bool is_need_cleanup_;
			std::uint32_t running_player_number_;
			std::uint32_t waiting_time_;
			bp::ipstream child_out_;
			bp::child child_process_;
			std::thread console_thread_;

			std::mutex mutex_;
			boost::circular_buffer<char> console_buf_;
			Status status_ = Status::Running;

			Impl(
				int master_server_port,
				PortManager::Reservation&& game_port,
				const fs::path& dir, 
				const std::string& level_name, 
				const std::string& game_name,
				protocol::GameMode game_mode,
				const boost::posix_time::ptime start_time,
				const boost::posix_time::ptime end_time,
				const bool is_need_cleanup)
				:master_server_port_ (master_server_port)
				,game_port_(std::move(game_port))
				, game_dir_(dir)
				, level_name_(level_name)
				, game_name_(game_name)
				, console_buf_(65536)
				, game_mode_(game_mode)
				, start_time_(start_time)
				, end_time_(end_time)
				, is_need_cleanup_(is_need_cleanup)
				, game_id_(0)
				, checkpoint_count_(0)
				, difficulty_(0)
				, running_player_number_(0)
				, waiting_time_(0)
			{
			}

			void launch(std::uint64_t game_id)
			{
				game_id_ = game_id;

				log::write(log::Level::Info, "launching game directory '%||' on port %||",
					game_dir_.string(), game_port_.get());

				child_process_ = bp::child{
					(game_dir_ / GameExecutableFileName).string(),
					"-c",
					game_dir_.string(),
					"-dedicated",
					"-mission",
					"levels/" + level_name_ + "/level.mis",
					"-master_server_addr",
					"127.0.0.1",
					"-master_server_port",
					(boost::format("%||") % master_server_port_).str(),
					"-game_id",
					(boost::format("%||") % game_id).str(),
					"-game_mode",
					(boost::format{"%||"} % (std::uint32_t)game_mode_).str(),
					bp::std_out > child_out_
				};

				console_thread_ = std::thread([this]()
				{
					read_console_output();
				});
			}
			~Impl()
			{
				std::error_code ec;
				child_process_.terminate(ec);
				
				if(console_thread_.joinable())
					console_thread_.join();

				try
				{
					if(is_need_cleanup_)
						cleanup_directory(game_dir_);
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "error while cleaning up game %||: %||",
						level_name_, ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error while cleaning up game %||: %||",
						level_name_, ex.what());
				}
			}

			void read_console_output()
			{
				try
				{
					while (!child_out_.eof())
					{
						std::string line;
						std::getline(child_out_, line);
						if (!line.empty() && line.back() == '\r')
							line.pop_back();

						std::unique_lock<std::mutex> lock(mutex_);
						console_buf_.insert(console_buf_.end(), line.begin(), line.end());
						console_buf_.push_back('\n');
					}

					{
						std::unique_lock<std::mutex> lock(mutex_);
						status_ = Status::Terminated;
					}
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "error in game %||: %||",
						level_name_, ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error in game %||: %||",
						level_name_, ex.what());
				}
			}

			void cleanup_directory(const fs::path& dir)
			{
				for (auto iter = fs::directory_iterator(dir); iter != fs::directory_iterator(); ++iter)
				{
					if (iter->status().type() == fs::file_type::directory_file)
						cleanup_directory(iter->path());
					else
						fs::remove(iter->path());
				}

				fs::remove(dir);
			}
		};

		Game::Game() = default;
		Game::~Game() = default;
		Game::Game(Game&&) = default;
		Game& Game::operator =(Game&&) = default;

		Game Game::create(
			int master_server_port,
			PortManager::Reservation&& game_port,
			const boost::filesystem::path& dir,
			const std::string& level_name,
			const std::string& game_name,
			protocol::GameMode game_mode,
			const boost::posix_time::ptime  start_time,
			const boost::posix_time::ptime  end_time,
			bool is_need_cleanup)
		{
			Game res;
			res.impl_ = std::make_unique<Impl>(master_server_port, std::move(game_port), dir, level_name, game_name, game_mode, start_time, end_time, is_need_cleanup);
			return res;
		}

		std::string Game::level_name() const
		{
			return impl_->level_name_;
		}

		std::string Game::game_name() const
		{
			return impl_->game_name_;
		}

		void Game::launch(const std::uint64_t &game_id) const
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			impl_->launch(game_id);
		}

		protocol::GameMode Game::game_mode() const
		{
			return impl_->game_mode_;
		}

		std::string Game::start_time() const
		{
			return boost::posix_time::to_iso_string(impl_->start_time_);
		}

		std::string Game::end_time() const
		{
			return boost::posix_time::to_iso_string(impl_->end_time_);
		}

		boost::posix_time::ptime Game::posix_start_time() const
		{
			return impl_->start_time_;
		}

		boost::posix_time::ptime Game::posix_end_time() const
		{
			return impl_->end_time_;
		}

		std::uint32_t Game::running_player_number() const
		{
			return impl_->running_player_number_;
		}

		std::uint32_t Game::waiting_time() const
		{
			return impl_->waiting_time_;
		}

		void Game::waiting_time(std::uint32_t waiting_time)
		{
			impl_->waiting_time_ = waiting_time;
		}

		Game::Status Game::status() const
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			return impl_->status_;
		}

		std::string Game::console_output() const
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			return std::string(impl_->console_buf_.begin(), impl_->console_buf_.end());
		}

		int Game::port() const
		{
			return impl_->game_port_.get();
		}

		std::uint64_t Game::game_id() const
		{
			return impl_->game_id_;
		}
		void 
			Game::set_game_id(int id)
		{
			impl_->game_id_ = id;
		}

		int Game::checkpoint_count() const
		{
			return impl_->checkpoint_count_;
		}

		void Game::set_checkpoint_count(int checkpoint_count)
		{
			impl_->checkpoint_count_ = checkpoint_count;
		}

		int Game::difficulty() const
		{
			return impl_->difficulty_;
		}

		void Game::running_player_number(std::uint32_t in)
		{
			impl_->running_player_number_ = in;
		}
	}
}
