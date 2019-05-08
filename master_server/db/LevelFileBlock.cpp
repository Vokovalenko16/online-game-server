#include "stdafx.h"

#include "LevelFileBlock.h"

namespace mserver {
	namespace db {

		std::uint64_t Level_File_Block::insert_blocks(
			db::DbSession& conn,
			std::uint64_t file_id,
			const std::vector<protocol::HashValType>& blocks)
		{
			std::uint64_t block_idx = 0;
			soci::blob hash{ conn.get_session() };

			soci::statement st = (conn.get_session().prepare
				<< "INSERT INTO level_file_blocks (file_id, block_index, hash) VALUES (?, ?, ?)"
				, soci::use(file_id)
				, soci::use(block_idx)
				, soci::use(hash)
			);

			for (const auto& block : blocks)
			{
				hash.write(0, (const char*)block.data(), protocol::HashValLen);
				st.execute(true);
				block_idx++;
			}


			return conn.get_last_insert_rowid();
		}

		std::vector<Level_File_Block> Level_File_Block::get_file_blocks_by_file_id(
			db::DbSession& conn, std::uint64_t file_id, std::uint32_t block_idx, std::uint32_t block_cnt)
		{
			std::vector<Level_File_Block> res;
			Level_File_Block block;
			soci::blob hash_blob(conn.get_session());

			soci::statement stmt = (conn.get_session().prepare 
				<< "SELECT * FROM level_file_blocks WHERE file_id = :file_id AND block_index >= :block_index AND "
				"block_index < :block_index + :block_count ORDER BY block_index ASC"
				, soci::use(file_id), soci::use(block_idx), soci::use(block_cnt)
				, soci::into(block.file_id), soci::into(block.block_idx), soci::into(hash_blob));

			std::vector<protocol::IDType> fileIDs;
			if (stmt.execute(true))
			{
				do {
					if (hash_blob.get_len() < protocol::HashValLen)
						throw std::runtime_error("hash val length is mismatched");
					hash_blob.read(0, (char*)block.hash.data(), protocol::HashValLen);
					res.push_back(block);
				} while (stmt.fetch());
			}

			return res;
		}
	}
}