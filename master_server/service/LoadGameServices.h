#pragma once
#include "net/MessageServer.h"
#include "game/GameManager.h"
#include <memory>


namespace mserver {
	namespace service {

		struct MasterServerConfig;

		class LoadGameServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;
		public:
			LoadGameServices(const MasterServerConfig&, net::MessageServer&, game::GameManager&);
			~LoadGameServices();
		};
	}
}
