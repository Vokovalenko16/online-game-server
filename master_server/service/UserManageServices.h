#pragma once

#include "db/Database.h"
#include "http/Handler.h"
#include "net/MessageServer.h"
#include "http/GameCenterConnection.h"
#include <memory>

namespace mserver {
	namespace service {

		class UserManageServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			UserManageServices(db::Database& database, http::Handler& http_handler, net::MessageServer& msg_server, http::GameCenterConnection& center_conn_);
			~UserManageServices();
		};
	}
}
