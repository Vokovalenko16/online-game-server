#include "stdafx.h"
#include "Releases_Model.h"

namespace mserver {
	namespace db {

		using soci::details::execute;
		using soci::into;
		using soci::use;

		Releases_Model Releases_Model::get_latest_release(DbSession& session)
		{
			auto& sql = session.get_session();
			Releases_Model res;

			execute((
				sql << "SELECT id, name, file_count FROM releases WHERE active = 1 ORDER BY id desc LIMIT 1",
				into(res.id), into(res.name), into(res.file_count)
				));

			return res;
		}

		void Releases_Model::delete_by_id(DbSession& session, protocol::IDType id)
		{
			auto& sql = session.get_session();
			execute((
				sql << "delete from releases where id = :id", use((int)id)
				));
		}

		void Releases_Model::insert_row(DbSession& session, const Releases_Model& row)
		{
			auto& sql = session.get_session();
			execute((
				sql << "insert into releases (id, name) values (:id, :name)", use((int)row.id), use(row.name)
				));
		}

		void Releases_Model::update_row(DbSession& session, const Releases_Model& release)
		{
			auto& sql = session.get_session();

			execute((
				sql << "update releases set name = :name, active = :active, file_count = :file_count where id = :id",
				use(release.name), use((int)release.active), use((int)release.file_count), use((int)release.id)
				));
		}

		void Releases_Model::update_row(DbSession& dbSession, int id, int file_count)
		{
			auto& sql = dbSession.get_session();

			execute((
				sql << "update releases set file_count = :file_count where id = :id", use(file_count), use(id)
				));
		}

		bool Releases_Model::is_id_exist(DbSession& session, protocol::IDType id)
		{
			auto& sql = session.get_session();
			int is_exist = 0;
			{
				execute((
					sql << "select count(id) from releases where id = :id", use((int)id), into(is_exist)
					));
			}

			return is_exist;
		}

		bool Releases_Model::set_active(DbSession& dbSession, int id, int active)
		{
			soci::statement st = (dbSession.get_session().prepare
				<< "UPDATE releases SET active = :active WHERE id = :id"
				, use(active)
				, use(id)
			);
			if (st.execute(true))
				return true;
			else
				return false;
		}

		std::vector<Releases_Model> Releases_Model::get_all_detail(DbSession& conn)
		{
			std::vector<Releases_Model> result;
			Releases_Model one;
			soci::statement st = (conn.get_session().prepare
				<< "SELECT id, name, active, file_count FROM releases"
				, soci::into(one.id)
				, soci::into(one.name)
				, soci::into(one.active)
				, soci::into(one.file_count)
			);

			if (st.execute(true))
				do { result.push_back(one); } while (st.fetch());

			return result;
		}
	}
}
