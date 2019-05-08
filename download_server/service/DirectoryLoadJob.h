#pragma once

#include "protocol/Types.h"
#include "db/Database.h"
#include "job/SimpleJob.h"

#include <string>
#include <boost/filesystem/path.hpp>

namespace mserver {
	namespace service {
		
		class DirectoryLoadJob : public job::SimpleJob
		{
		private:
			db::Database db_;
			protocol::IDType release_id_;
			std::string release_name_;
			std::string physical_path_;
			std::string logical_path_;
			protocol::IndexType file_index_;
		public:
			DirectoryLoadJob(
				db::Database&, 
				protocol::IDType, 
				std::string release_name, 
				std::string physical_path, 
				std::string logical_path);

			void run() override;

		private:
			void add_release();
			void update_release_file_count();

			void load_directory(
				const boost::filesystem::path& physical_path,
				const std::string& logical_path
			);

			void load_file(
				const boost::filesystem::path& physical_path,
				const std::string& logical_path
			);

			void roll_back();
		};
	}
}