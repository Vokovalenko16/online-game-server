#pragma once

#include "db/Database.h"
#include "net/MessageServer.h"
#include <memory>
#include <boost/asio/io_service.hpp>

namespace mserver {
	namespace service {

		class TokenServices
		{
		public:
			struct Impl;
		private:
			std::shared_ptr<Impl> impl_;

		public:
			TokenServices(db::Database& database, net::MessageServer&);
			~TokenServices();
		};
	}
}
