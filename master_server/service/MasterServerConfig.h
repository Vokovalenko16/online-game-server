#pragma once

#include <string>

namespace mserver {
	namespace service {

		struct MasterServerConfig
		{
			std::string public_ip_addr;
			int server_port;
			int http_port;
			int download_server_http_port;
			int idle_timeout_seconds;
			std::string web_root_dir;
			std::string game_dir;
			std::string temp_dir;
			std::string levels_dir;
			int game_port_min;
			int game_port_max;
			std::string db_path;
			std::string dl_server_path;
			std::string game_center_addr;
			int game_center_port;
			std::string game_center_target;
			bool b_copy_game;
			std::string admin_password;
		};
	}
}
