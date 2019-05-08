#include "stdafx.h"
#include "User.h"
#include "soci/statement.h"
#include <stdexcept>

namespace mserver {
	namespace db {

		std::vector<std::int64_t> User::get_all_id(db::DbSession& session)
		{
			std::vector<std::int64_t> result;

			std::int64_t id;
			soci::statement st = (session.get_session().prepare
				<< "SELECT id FROM users"
				, soci::into(id));

			if (st.execute(true))
				do { result.push_back(id); } while (st.fetch());

			return result;
		}

		User User::get(db::DbSession& session, std::int64_t id)
		{
			User user;
			soci::statement st = (session.get_session().prepare
				<< "SELECT id, login_name, active FROM users WHERE id=?"
				, soci::use(id)
				, soci::into(user.id)
				, soci::into(user.login_name)
				, soci::into(user.active));

			if (!st.execute(true))
				throw std::runtime_error("requested user ID was not found.");

			return user;
		}

		boost::optional<User> User::find_by_login_name(db::DbSession& session, const std::string& name)
		{
			User user;
			soci::statement st = (session.get_session().prepare
				<< "SELECT id, login_name, active FROM users WHERE login_name=?"
				, soci::use(name)
				, soci::into(user.id)
				, soci::into(user.login_name)
				, soci::into(user.active));

			if (!st.execute(true))
				return boost::none;
			else
				return user;
		}

		std::int64_t User::insert(db::DbSession& session, const User& user)
		{
			soci::blob empty_pwd{ session.get_session() };

			soci::statement st = (session.get_session().prepare
				<< "INSERT INTO users(login_name, active, password_hash_base16) VALUES(?, ?, ?)"
				, soci::use(user.login_name)
				, soci::use(user.active)
				, soci::use(empty_pwd));
			st.execute(true);

			return session.get_last_insert_rowid();
		}

		void User::update(db::DbSession& session, const User& user)
		{
			soci::statement st = (session.get_session().prepare
				<< "UPDATE users SET login_name=?, active=? WHERE id=?"
				, soci::use(user.login_name)
				, soci::use(user.active)
				, soci::use(user.id));
			st.execute(true);
		}

		std::string User::get_password_hash(db::DbSession& session, std::int64_t id)
		{
			std::string hash;
			soci::statement st = (session.get_session().prepare
				<< "SELECT password_hash_base16 FROM users WHERE id=?"
				, soci::use(id)
				, soci::into(hash));

			if (!st.execute(true))
				throw std::runtime_error("requested user ID was not found.");

			return hash;
		}

		void User::set_password_hash(db::DbSession& session, std::int64_t id, const std::string& password_hash)
		{
			soci::statement st = (session.get_session().prepare
				<< "UPDATE users SET password_hash_base16=? WHERE id=?"
				, soci::use(password_hash)
				, soci::use(id));
			st.execute(true);
		}
	}
}
