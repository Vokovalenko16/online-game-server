#pragma once

#include "db/Database.h"
#include <vector>

namespace mserver {
	namespace db {

		struct Files_Model
		{
			protocol::IDType id;
			protocol::IDType release_id;
			protocol::IndexType file_index;
			std::string physical_path;
			std::string logical_path;
			protocol::HashValType hash;
			protocol::FileLengthType file_length;

			Files_Model() : id(0), release_id(0), file_index(0)
			{}
			static Files_Model find_by_id(DbSession&, protocol::IDType file_id);
			static std::vector<protocol::IDType> get_file_ids_by_release_id(
				DbSession&, const protocol::IDType&, const protocol::IndexType &, const protocol::IndexType &);
			static void insert_rows(db::DbSession& db, const std::vector<Files_Model> & files);
			static void insert_row(db::DbSession& db, const Files_Model& file);
			static void get_all_data(db::DbSession& dbSession, std::vector<Files_Model>& files);
			static void delete_by_release_id(db::DbSession& dbSession, protocol::IDType release_id);
		};
	}
}
