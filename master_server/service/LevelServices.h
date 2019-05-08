#pragma once
#include "MasterServerConfig.h"
#include "db/Database.h"
#include "http/Handler.h"
#include "job/Manager.h"
#include "net/MessageServer.h"

#include <memory>

namespace mserver {
	namespace service {

		class LevelService
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;
		public:

			LevelService(
				MasterServerConfig&, 
				db::Database&, 
				http::Handler& handler, 
				job::Manager& jobManager, 
				net::MessageServer&
			);
			~LevelService();
		};
	}
}