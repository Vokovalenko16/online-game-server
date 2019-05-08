#include "stdafx.h"
#include "GameAdminServices.h"
#include "LaunchGameJob.h"
#include "game/PortManager.h"

#include "job/Manager.h"
#include "json/value.h"
#include "logger/Logger.h"

#include <mutex>
#include <random>
#include <chrono>
#include <vector>
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

namespace mserver {
	namespace service {

		namespace fs = boost::filesystem;

		struct GameAdminServices::Impl
		{
			const MasterServerConfig& config_;
			job::Manager job_manager_;
			std::mutex mutex_;
			std::mt19937 random_;
			game::PortManager port_manager_;
			game::GameManager& game_manager_;

			std::string generate_temp_dir_name_locked()
			{
				return (boost::format{"%08x-%08x"} % random_() % random_()).str();
			}

			Impl(
				const MasterServerConfig& config, 
				http::Handler& http_handler, 
				job::Manager& job_manager, 
				game::GameManager& game_manager)
				: config_(config)
				, job_manager_(job_manager)
				, game_manager_(game_manager)
				, random_((int)std::chrono::steady_clock().now().time_since_epoch().count())
			{
				port_manager_ = game::PortManager::create(config.game_port_min, config.game_port_max);
				

				fs::create_directory(config_.temp_dir);

				http_handler.register_json_method("/game/list_levels", [this](Json::Value&& req, Json::Value& resp)
				{
					resp["succeeded"] = true;
					auto& levels_json = resp["levels"];
					levels_json = Json::ValueType::arrayValue;

					for (auto iter = fs::directory_iterator(config_.levels_dir); iter != fs::directory_iterator(); ++iter)
					{
						auto& level_json = levels_json[levels_json.size()];
						level_json["name"] = iter->path().filename().string();
					}
				});

				http_handler.register_json_method("/game/create", [this](Json::Value&& req, Json::Value& resp)
				{
					auto level_file = fs::path{ config_.levels_dir } / req["level"].asString();
					if (!fs::exists(level_file))
					{
						resp["succeeded"] = false;
						resp["message"] = "level file was not found.";
						return;
					}

					fs::path temp_dir;
					{
						// create a temporary directory
						std::unique_lock<std::mutex> lock(mutex_);
						do
						{
							temp_dir = fs::path{ config_.temp_dir } / generate_temp_dir_name_locked();
						} while (fs::exists(temp_dir));
						fs::create_directory(temp_dir);
					}
					auto start_time = boost::posix_time::time_from_string(req["start_tm"].asString());
					auto end_time = boost::posix_time::time_from_string(req["end_tm"].asString());
					auto game_name = req["game_name"].asString();
					
					game::PortManager::Reservation game_port{ port_manager_ };
					log::write(log::Level::Info, "creating game for level '%||' on port %||",
						level_file.filename().string(),
						game_port.get());

					resp["job_id"] = job_manager_.start(std::make_shared<LaunchGameJob>(
						config_.server_port,
						config_.game_dir,
						level_file,
						(protocol::GameMode)req["game_mode"].asUInt(),
						temp_dir,
						std::move(game_port),
						game_manager_,
						game_name,
						start_time,
						end_time,
						config_.b_copy_game));
					resp["succeeded"] = true;
				});
			}
		};

		GameAdminServices::GameAdminServices(
			const MasterServerConfig& config, 
			http::Handler& http_handler, 
			job::Manager& job_manager, 
			game::GameManager& game_manager)
			: impl_(std::make_unique<Impl>(config, http_handler, job_manager, game_manager))
		{
		}

		GameAdminServices::~GameAdminServices() = default;
	}
}
