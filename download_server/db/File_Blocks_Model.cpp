#include "stdafx.h"
#include "File_Blocks_Model.h"
#include <vector>

namespace mserver {
	namespace db {

		using soci::details::execute;
		using soci::into;
		using soci::use;
		using soci::blob;

		std::vector<File_Blocks_Model> File_Blocks_Model::get_file_blocks_by_file_id(
			DbSession& session,
			protocol::IDType file_id,
			protocol::IndexType block_index,
			protocol::IndexType block_count)
		{
			auto& sql = session.get_session();

			std::vector<File_Blocks_Model> res;
			File_Blocks_Model block;
			soci::blob hash_blob(sql);

			soci::statement stmt = (sql.prepare <<
				"SELECT * FROM file_blocks WHERE file_id = :file_id AND block_index >= :block_index AND "
				"block_index < :block_index + :block_count ORDER BY block_index ASC",
				use(file_id), use(block_index), use(block_count),
				into(block.file_id), into(block.block_index), into(hash_blob));

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

		void File_Blocks_Model::insert_rows(DbSession& dbSession, const std::vector<File_Blocks_Model>& file_blocks)
		{
			auto& sql = dbSession.get_session();
			blob hash_b(sql);

			for (const auto& file_block : file_blocks)
			{
				hash_b.write(0, (const char*)file_block.hash.data(), file_block.hash.size());

				execute((
					sql << "insert into file_blocks (file_id, block_index, hash)"
					" values (:file_id, :block_index, :hash)",
					use(file_block.file_id),
					use(file_block.block_index),
					use(hash_b)
					));
			}

		}

		void File_Blocks_Model::empty_table(DbSession& dbSession)
		{
			auto& sql = dbSession.get_session();

			execute((
				sql << "delete from file_blocks"));
		}

		void File_Blocks_Model::insert_rows(
			DbSession& dbSession,
			protocol::IDType file_id,
			const std::vector<protocol::HashValType>& hashes)
		{
			auto& sql = dbSession.get_session();
			int block_index = 0;
			blob hash_b(sql);

			soci::statement st = (sql.prepare << "insert into file_blocks (file_id, block_index, hash)"
				" values (:file_id, :block_index, :hash)",
				use(file_id),
				use(block_index),
				use(hash_b)
				);

			for (const auto& hash : hashes)
			{
				hash_b.write(0, (const char*)hash.data(), protocol::HashValLen);
				st.execute(true);
				block_index++;
			}
		}

		File_Blocks_Model::File_Blocks_Model()
			: file_id(0)
			, block_index(0)
		{}
	}
}
