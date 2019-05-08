#pragma once

#include "protocol/Types.h"
#include "PortManager.h"
#include <memory>
#include <string>
#include <cstdint>
#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace mserver {
	namespace game {

		class Game
		{
		public:
			enum class Status
			{
				Running,
				Terminated,
			};

		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			Game();
			~Game();

			Game(Game&&);
			Game& operator =(Game&&);
			static Game create(
				int master_server_port,
				PortManager::Reservation&& game_port,
				const boost::filesystem::path& dir,
				const std::string& level_name,
				const std::string& game_name,
				protocol::GameMode game_mode,
				const boost::posix_time::ptime  start_time,
				const boost::posix_time::ptime  end_time,
				bool is_need_cleanup = true
			);

			std::string level_name() const;
			std::string game_name() const;
			protocol::GameMode game_mode() const;
			Status status() const;
			std::string start_time() const;
			std::string end_time() const;
			boost::posix_time::ptime posix_start_time() const;
			boost::posix_time::ptime posix_end_time() const;
			std::uint32_t running_player_number() const;
			std::uint32_t waiting_time() const;
			void running_player_number(std::uint32_t in);
			std::uint64_t game_id() const;
			void waiting_time(std::uint32_t);
			void set_game_id(int);
			int checkpoint_count() const;
			void set_checkpoint_count(int);
			std::string console_output() const;
			int difficulty() const;
			int port() const;
			void launch(const std::uint64_t&) const;
		};
	}
}
