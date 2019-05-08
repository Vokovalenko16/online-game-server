#pragma once
#include "net/MessageServer.h"
#include "game/GameManager.h"
#include "db/Database.h"
#include "http/GameCenterConnection.h"
#include <memory>


namespace mserver {
	namespace service {

		struct MasterServerConfig;

		class ItemServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;
		public:
			ItemServices(boost::asio::io_service& io, db::Database&, http::Handler&, net::MessageServer&, http::GameCenterConnection& center_conn_);
			~ItemServices();
		};
	}
}
