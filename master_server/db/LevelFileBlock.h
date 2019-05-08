#pragma once
#include "db/Database.h"
#include "protocol/Types.h"

#include <cstdint>
#include <vector>

namespace mserver {
	namespace db {
		class Level_File_Block {
		public:
			std::uint64_t file_id;
			std::uint32_t block_idx;
			protocol::HashValType hash;

		public:

			static std::uint64_t insert_blocks(
				db::DbSession&, 
				std::uint64_t file_id, 
				const std::vector<protocol::HashValType>&);
			static std::vector<Level_File_Block> get_file_blocks_by_file_id(
				db::DbSession&, std::uint64_t file_id, std::uint32_t block_idx, std::uint32_t block_cnt);
		};
	}
}