#pragma once
#include "protocol/Types.h"
#include "db/Database.h"

#include <boost/optional.hpp>

#include <vector>

namespace mserver {
	namespace db {
		struct  user_money
		{
			std::string user;
			std::uint32_t money;
		};

		struct settle_by_user
		{
			std::string date_time;
			std::string item;
			std::uint32_t item_change;
			std::uint32_t star_change;
		};

		struct Equip_history
		{
			std::string transaction_id;
			std::string title;
			std::string user;
			std::string item;
			std::uint32_t item_change;
			std::uint32_t star_change;
			std::uint32_t status;
			std::string error_desc;
			std::string transaction;
			std::string signature;
			std::string date_time;
			std::uint32_t pending;

			static void insert(
				db::DbSession& session,
				const std::string& transaction_id,
				const std::string& title,
				const std::string& user,
				const std::string& item,
				std::int32_t item_change,
				std::int32_t star_change,
				std::uint32_t status,
				const std::string& error_desc,
				const std::string& transaction,
				const std::string& signature,
				std::uint32_t pending);
			static void insert(db::DbSession& session, Equip_history row);
			static bool is_transaction_finished(db::DbSession& session, const std::string& transaction_id);
			static boost::optional<Equip_history> not_finished_transaction(db::DbSession& dbSession);
			static void update_pending(db::DbSession& session, const std::string& transaction_id, std::uint32_t pending);
			static std::vector<user_money> get_settles(db::DbSession& session, const std::string& start_date, const std::string& end_date);
			static std::vector<settle_by_user> get_settles_by_user(db::DbSession& session, const std::string& user, const std::string& start_date, const std::string& end_date);
		};
	}
}
