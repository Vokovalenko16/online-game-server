#pragma once
#include "net/MessageServer.h"
#include "game/GameManager.h"
#include <memory>


namespace mserver {
	namespace service {
		class GameServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;
		public:
			GameServices(net::MessageServer&, game::GameManager&);
			~GameServices();
		};
	}
}
