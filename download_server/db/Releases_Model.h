#pragma once

#include "db/Database.h"
#include "protocol/Types.h"
#include <vector>
namespace mserver {
	namespace db {

		struct Releases_Model
		{
			std::uint64_t id;
			std::string name;
			int active;
			std::int32_t file_count;

			static Releases_Model get_latest_release(DbSession& session);
			static bool is_id_exist(DbSession& session, protocol::IDType id);

			static void insert_row(DbSession& session, const Releases_Model&);

			static void update_row(DbSession& session, const Releases_Model&);
			static void update_row(DbSession& dbSession, int id, int file_count);

			static bool set_active(DbSession& dbSession, int id, int active);
			static std::vector<Releases_Model> get_all_detail(DbSession&);
			static void delete_by_id(DbSession& session, protocol::IDType id);

		};
	}
}
