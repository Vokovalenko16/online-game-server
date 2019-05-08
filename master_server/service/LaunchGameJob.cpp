#include "stdafx.h"
#include "LaunchGameJob.h"
#include "game/Game.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>


namespace mserver {
	namespace service {

		namespace fs = boost::filesystem;

		void LaunchGameJob::run()
		{
			try
			{
				/*using namespace boost::posix_time;
				ptime current_time;
				do 
				{
					current_time = second_clock::local_time();
					std::this_thread::sleep_for(std::chrono::milliseconds(100));
				} while (current_time < start_time);*/
				if (b_copy_game_)
				{
					copy_game_files(game_dir_, dest_dir_);

					update_status(job::Status{ "copying level file" });
					fs::remove_all(dest_dir_ / "levels");
					fs::create_directory(dest_dir_ / "levels");
					fs::copy(level_file_, dest_dir_ / "levels" / level_file_.filename());
				}
				else
				{
					dest_dir_ = game_dir_;
				}
				

				update_status(job::Status{ "launching game server" });
				write_game_config();

				game_manager_.add(game::Game::create(
					master_server_port_,
					std::move(game_port_),
					dest_dir_,
					level_file_.stem().string(),
					game_name_, game_mode_, start_time_, end_time_, b_copy_game_));
			}
			catch (std::exception&)
			{
				try
				{
					update_status(job::Status{ "cleaning up" }, false);
					if(b_copy_game_)
						remove_directory(dest_dir_);
				}
				catch (std::exception&)
				{ }

				throw;
			}
		}

		void LaunchGameJob::copy_game_files(const boost::filesystem::path& src, const boost::filesystem::path& dest)
		{
			update_status(job::Status{ "copying " + src.string() });
			fs::create_directory(dest);

			for (auto iter = fs::directory_iterator(src); iter != fs::directory_iterator(); ++iter)
			{
				auto child_dest = dest / iter->path().filename();

				switch (iter->status().type())
				{
				case fs::file_type::directory_file:
					copy_game_files(iter->path(), child_dest);
					break;

				case fs::file_type::regular_file:
					update_status(job::Status{ "copying " + iter->path().string() });
					fs::copy_file(iter->path(), child_dest);
					break;
				}
			}
		}

		void LaunchGameJob::write_game_config()
		{
			fs::fstream file;
			file.open(dest_dir_ / "server_prefs.cs", std::ios::out | std::ios::app);
			if (!file.good())
				throw std::runtime_error("failed to open game server prefs file");

			file << "$Pref::Server::Port = " << game_port_.get() << ";\n";
			file << "$Conf::Server::Start = \"" << boost::posix_time::to_iso_string(start_time_) << "\";\n";
			file << "$Conf::Server::End = \"" << boost::posix_time::to_iso_string(end_time_) << "\";\n";
			file.flush();

			if (file.fail())
				throw std::runtime_error("failed to write server prefs file");
		}

		void LaunchGameJob::remove_directory(const boost::filesystem::path& path)
		{
			for (auto iter = fs::directory_iterator(path); iter != fs::directory_iterator(); ++iter)
			{
				if (iter->status().type() == fs::file_type::directory_file)
					remove_directory(iter->path());
				else
					fs::remove(iter->path());
			}

			fs::remove(path);
		}
	}
}
