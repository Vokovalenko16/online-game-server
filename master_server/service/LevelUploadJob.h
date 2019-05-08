#pragma once
#include "db/Database.h"
#include "job/SimpleJob.h"
#include <boost/filesystem.hpp>

namespace mserver {
	namespace service {

		namespace fs = boost::filesystem;

		class LevelUploadJob : public job::SimpleJob
		{
		private:
			db::Database& db_;
			fs::path level_file_;
		public:
			LevelUploadJob(db::Database& db, const fs::path& level_file);

			virtual void run() override;

		private:
			void register_level_file();
		};
	}
}