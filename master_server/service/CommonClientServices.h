#pragma once

#include "net/MessageServer.h"
#include "soci/soci.h"
#include "soci/backends/sqlite/soci-sqlite3.h"

namespace mserver {
	namespace service {

		class CommonClientServices
		{
		public:
			CommonClientServices(net::MessageServer& server);
		};
	}
}
