#include "stdafx.h"
#include "Token.h"

namespace mserver {
	namespace db {

		boost::optional<Token> Token::find_by_value(db::DbSession& session, const std::string& value)
		{
			Token token;
			soci::statement st = (session.get_session().prepare
				<< "SELECT value, user_id, expiry_time FROM tokens WHERE value=?"
				, soci::use(value)
				, soci::into(token.value)
				, soci::into(token.user_id)
				, soci::into(token.expiry_time));

			if (!st.execute(true))
				return boost::none;
			else
				return token;
		}

		boost::optional<Token> Token::find_by_user(db::DbSession& session, const std::string& user_id)
		{
			Token token;
			soci::statement st = (session.get_session().prepare
				<< "SELECT value, user_id, expiry_time FROM tokens WHERE user_id=?"
				, soci::use(user_id)
				, soci::into(token.value)
				, soci::into(token.user_id)
				, soci::into(token.expiry_time));

			if (!st.execute(true))
				return boost::none;
			else
				return token;
		}

		void Token::insert(db::DbSession& session, const Token& token)
		{
			soci::statement st = (session.get_session().prepare
				<< "INSERT INTO tokens(value, user_id, expiry_time) VALUES(?, ?, ?)"
				, soci::use(token.value)
				, soci::use(token.user_id)
				, soci::use(token.expiry_time));
			st.execute(true);
		}

		void Token::remove_by_value(db::DbSession& session, const std::string& value)
		{
			soci::statement st = (session.get_session().prepare
				<< "DELETE FROM tokens WHERE value=?"
				, soci::use(value));
			st.execute(true);
		}

		void Token::remove_by_user(db::DbSession& session, const std::string& user_id)
		{
			soci::statement st = (session.get_session().prepare
				<< "DELETE FROM tokens WHERE user_id=?"
				, soci::use(user_id));
			st.execute(true);
		}
	}
}
