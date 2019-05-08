#include "stdafx.h"
#include "CommonClientServices.h"
#include "logger/Logger.h"
#include <iostream>

namespace mserver {
	namespace service {

		CommonClientServices::CommonClientServices(net::MessageServer& server)
		{
			server.register_handler(protocol::MessageType::Ping, [](protocol::Message& msg, net::NetSession&)
			{
				protocol::MessageWriter writer(msg);
				return protocol::MessageType::Ping;
			});
		}
	}
}
