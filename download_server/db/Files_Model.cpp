#include "stdafx.h"
#include "Files_Model.h"

#include <boost/format.hpp>
#include <vector>
#include <stdexcept>
namespace mserver {
	namespace db {

		using soci::details::execute;
		using soci::into;
		using soci::use;
		using soci::blob;
		using soci::statement;

		Files_Model Files_Model::find_by_id(DbSession& session, protocol::IDType file_id)
		{
			auto& sql = session.get_session();
			Files_Model res;

			soci::blob hash_blob(sql);
			execute((
				sql <<
				"SELECT id, release_id, file_index, physical_path, logical_path, hash, file_length FROM files WHERE id=:file_id",
				use(file_id),
				into(res.id), into(res.release_id), into(res.file_index), into(res.physical_path), into(res.logical_path),
				into(hash_blob), into(res.file_length)
				));
			if (hash_blob.get_len() < protocol::HashValLen)
				throw std::runtime_error("hash val length is mismatched");
			hash_blob.read(0, (char*)res.hash.data(), protocol::HashValLen);

			return res;
		}

		std::vector<protocol::IDType> Files_Model::get_file_ids_by_release_id(DbSession &session, const protocol::IDType &release_id, const protocol::IndexType &file_index_start, const protocol::IndexType &file_count)
		{
			auto& sql = session.get_session();

			protocol::IDType id;
			soci::statement stmt = (sql.prepare <<
				"SELECT id FROM files WHERE release_id=:release_id LIMIT :file_index_start, :file_count",
				use(release_id), use(file_index_start), use(file_count), into(id));

			std::vector<protocol::IDType> fileIDs;
			if (stmt.execute(true))
			{
				do {
					fileIDs.push_back(id);
				} while (stmt.fetch());
				return fileIDs;

			}
			else
			{
				throw std::runtime_error(
					(boost::format{ "No matching release id(%id) in 'files' table" } % release_id).str()
				);
			}

		}

		void Files_Model::insert_rows(db::DbSession& dbSession, const std::vector<Files_Model> & files)
		{
			auto& sql = dbSession.get_session();
			blob b(sql);

			for (const auto& file : files)
			{
				b.write(0, (const char*)file.hash.data(), file.hash.size());

				execute((
					sql << "INSERT INTO files (release_id, file_index, physical_path, logical_path, hash, file_length)"
					" VALUES (:release_id, :file_index, :physical_path, :logical_path, :hash, :file_length)",
					use(file.release_id),
					use(file.file_index),
					use(file.physical_path),
					use(file.logical_path),
					use(b),
					use(file.file_length)
					));

				std::string query = sql.get_last_query();
				int a = 3;
			}
		}

		void Files_Model::insert_row(db::DbSession& dbSession, const Files_Model& file)
		{
			auto& sql = dbSession.get_session();
			blob b(sql);

			b.write(0, (const char*)file.hash.data(), file.hash.size());

			execute((
				sql << "insert into files (release_id, file_index, physical_path, logical_path, hash, file_length)"
				" values (:release_id, :file_index, :physical_path, :logical_path, :hash, :file_length)",
				use(file.release_id),
				use(file.file_index),
				use(file.physical_path),
				use(file.logical_path),
				use(b),
				use(file.file_length)
				));
		}

		void Files_Model::get_all_data(db::DbSession& dbSession, std::vector<Files_Model>& files)
		{
			auto& sql = dbSession.get_session();

			Files_Model file;
			blob hash_b(sql);

			statement st = (sql.prepare << "select * from files",
				into(file.id),
				into(file.release_id),
				into(file.file_index),
				into(file.physical_path),
				into(file.logical_path),
				into(hash_b),
				into(file.file_length)
				);

			st.execute();

			while (st.fetch())
			{
				hash_b.read(0, (char*)file.hash.data(), file.hash.size());
				files.push_back(file);
			}
		}

		void Files_Model::delete_by_release_id(db::DbSession& dbSession, protocol::IDType release_id)
		{
			auto& sql = dbSession.get_session();

			execute((
				sql << "delete from files where release_id = :release_id", use(release_id)
				));
		}
	}
}
