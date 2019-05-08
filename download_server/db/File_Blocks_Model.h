#pragma once

#include "db/Database.h"
#include <vector>

namespace mserver {
	namespace db {

		struct File_Blocks_Model
		{
			protocol::IDType file_id;
			protocol::IndexType block_index;
			protocol::HashValType hash;

			File_Blocks_Model();
			static std::vector<File_Blocks_Model> get_file_blocks_by_file_id(
				DbSession& session,
				protocol::IDType file_id,
				protocol::IndexType block_index,
				protocol::IndexType block_count);

			static void insert_rows(DbSession& dbSession, const std::vector<File_Blocks_Model>& blocks);
			static void insert_rows(
				DbSession& dbSession, protocol::IDType file_id, const std::vector<protocol::HashValType>& hashes);
			static void empty_table(DbSession& dbSession);
		};
	}
}
