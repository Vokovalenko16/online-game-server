#pragma once
#include "net/MessageServer.h"
#include "game/GameManager.h"
#include "db/Database.h"
#include <memory>


namespace mserver {
	namespace service {
		class ScoreServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;
		public:
			ScoreServices(db::Database&, net::MessageServer&, game::GameManager&);
			~ScoreServices();
		};
	}
}
