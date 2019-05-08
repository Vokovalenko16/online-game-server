#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <boost/optional.hpp>
#include "db/Database.h"

namespace mserver {
	namespace db {

		struct Token
		{
			std::string value;
			std::string user_id;
			std::int64_t expiry_time;

			std::chrono::system_clock::time_point get_expiry_time() const
			{
				return std::chrono::system_clock::time_point(std::chrono::seconds(expiry_time));
			}

			void set_expiry_time(const std::chrono::system_clock::time_point& value)
			{
				expiry_time
					= std::chrono::duration_cast<std::chrono::seconds>(value.time_since_epoch())
					.count();
			}

			static boost::optional<Token> find_by_value(db::DbSession& session, const std::string& value);
			static boost::optional<Token> find_by_user(db::DbSession& session, const std::string& user_id);
			static void insert(db::DbSession& session, const Token& token);
			static void remove_by_value(db::DbSession& session, const std::string& value);
			static void remove_by_user(db::DbSession& session, const std::string& user_id);
		};
	}
}
