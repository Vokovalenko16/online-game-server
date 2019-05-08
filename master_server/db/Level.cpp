#include "stdafx.h"

#include "Level.h"

namespace mserver {
	namespace db {

		std::uint64_t Level::insert(db::DbSession& conn, const Level& level)
		{
			soci::blob hash{ conn.get_session() };

			hash.write(0, (const char*)level.hash.data(), protocol::HashValLen);

			soci::statement st = (conn.get_session().prepare 
				<< "INSERT INTO levels(name, file_length, hash) VALUES(?, ?, ?) "
				, soci::use(level.name)
				, soci::use(level.file_length)
				, soci::use(hash)
			);

			st.execute(true);

			return conn.get_last_insert_rowid();
		}

		boost::optional<Level> Level::find_by_id(db::DbSession& conn, std::uint64_t id)
		{
			soci::blob hash{ conn.get_session() };
			Level level;

			soci::statement st = (conn.get_session().prepare 
				<< "SELECT * FROM levels WHERE id = ?"
				, soci::use(id)
				, soci::into(level.id)
				, soci::into(level.name)
				, soci::into(level.file_length)
				, soci::into(hash)
			);

			if (!st.execute(true))
				return boost::none;
			else
				return level;
		}

		boost::optional<Level> Level::find_by_name(db::DbSession& conn, const std::string& level_name)
		{
			soci::blob hash{ conn.get_session() };
			Level level;
			soci::statement st = (conn.get_session().prepare
				<< "SELECT id, name, file_length, hash FROM levels WHERE name = ?"
				, soci::use(level_name)
				, soci::into(level.id)
				, soci::into(level.name)
				, soci::into(level.file_length)
				, soci::into(hash)
			);

			if (st.execute(true))
			{
				hash.write(0, (const char*)level.hash.data(), protocol::HashValLen);
				return level;
			}
			else
				return boost::none;
		}

		std::vector<Level> Level::get_all_detail(db::DbSession& conn)
		{
			std::vector<Level> result;
			Level one;
			soci::blob b{ conn.get_session() };
			soci::statement st = (conn.get_session().prepare
				<< "SELECT id, name, file_length, hash FROM levels"
				, soci::into(one.id)
				, soci::into(one.name)
				, soci::into(one.file_length)
				, soci::into(b)
			);

			if (st.execute(true))
				do {
					b.read(0, (char*)one.hash.data(), one.hash.size());
					result.push_back(one);
				} while (st.fetch());

			return result;
		}
	}
}