#pragma once

#include "db/Database.h"
#include "http/Handler.h"
#include "job/Manager.h"

namespace mserver {
	namespace service {

		class AdminClientService
		{
		private:
			db::Database db_;

		public:
			AdminClientService(http::Handler& server, db::Database& database, job::Manager& job_manager);
		};
	}
}