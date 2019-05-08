#pragma once

#include "net/MessageServer.h"
#include "db/Database.h"

namespace mserver {
	namespace service {

		class CommonClientServices
		{
		public:
			CommonClientServices(net::MessageServer& server, db::Database& database);
		};
	}
}
