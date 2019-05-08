#pragma once

#include "db/Database.h"
#include <boost/optional.hpp>
#include <cstdint>
#include <string>
#include <vector>

namespace mserver {
	namespace db {

		struct User
		{
			std::int64_t id;
			std::string login_name;
			int active;

			static std::vector<std::int64_t> get_all_id(db::DbSession& session);
			static User get(db::DbSession& session, std::int64_t id);
			static boost::optional<User> find_by_login_name(db::DbSession& session, const std::string& name);
			static std::int64_t insert(db::DbSession& session, const User& user);
			static void update(db::DbSession& session, const User& user);
			static std::string get_password_hash(db::DbSession& session, std::int64_t id);
			static void set_password_hash(db::DbSession& session, std::int64_t id, const std::string& password_hash);
		};
	}
}
