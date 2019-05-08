#pragma once

#include "MasterServerConfig.h"
#include "http/Handler.h"
#include "job/Manager.h"
#include "game/GameManager.h"
#include <memory>

namespace mserver {
	namespace service {

		class GameAdminServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			GameAdminServices(
				const MasterServerConfig& config, http::Handler& http_handler, job::Manager& job_manager, game::GameManager&);
			~GameAdminServices();
		};
	}
}
