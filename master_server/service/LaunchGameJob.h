#pragma once

#include "protocol/Types.h"
#include "game/PortManager.h"
#include "game/GameManager.h"
#include "job/SimpleJob.h"
#include <utility>
#include <boost/filesystem/path.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace mserver {
	namespace service {

		class LaunchGameJob : public job::SimpleJob
		{
		private:
			int master_server_port_;
			boost::filesystem::path game_dir_;
			boost::filesystem::path level_file_;
			protocol::GameMode game_mode_;
			boost::filesystem::path dest_dir_;
			game::PortManager::Reservation game_port_;
			game::GameManager& game_manager_;
			std::string game_name_;
			boost::posix_time::ptime  start_time_;
			boost::posix_time::ptime  end_time_;
			bool b_copy_game_;

		public:
			LaunchGameJob(
				int master_server_port,
				const boost::filesystem::path& game_dir,
				const boost::filesystem::path& level_file,
				protocol::GameMode game_mode,
				const boost::filesystem::path& dest_dir,
				game::PortManager::Reservation&& game_port,
				game::GameManager& game_manager,
				const std::string game_name,
				const boost::posix_time::ptime start_tm,
				const boost::posix_time::ptime end_tm,
				bool b_copy_game = true)
				: job::SimpleJob{ "Launch Game " + level_file.filename().string() }
				, master_server_port_(master_server_port)
				, game_dir_(game_dir)
				, level_file_(level_file)
				, game_mode_(game_mode)
				, dest_dir_(dest_dir)
				, game_port_(std::move(game_port))
				, game_manager_(game_manager)
				, game_name_(game_name)
				, start_time_(start_tm)
				, end_time_(end_tm)
				, b_copy_game_(b_copy_game)
			{}

			virtual void run() override;

		private:
			void copy_game_files(const boost::filesystem::path& src, const boost::filesystem::path& dest);
			void write_game_config();
			void remove_directory(const boost::filesystem::path& path);
		};
	}
}
