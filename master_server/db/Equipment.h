#pragma once
#include "protocol/Types.h"
#include "protocol/detail/NetString.h"
#include "db/Database.h"

#include <boost/serialization/serialization.hpp>

#include <vector>

namespace mserver {
	namespace db {

		struct Equipment
		{
			std::uint64_t member_id;
			std::uint64_t equip_id;
			std::uint32_t equip_cnt;

			static std::vector<protocol::EquipDisplay> find_by_user(db::DbSession& session, const std::string& login_name);
			static std::vector<protocol::EquipDisplay> find_by_user_and_category(
					db::DbSession& session, const std::string& login_name, const std::uint64_t& category_id);
			static void insert(db::DbSession& session, const std::string& login_name, const std::string& equip_name, std::uint32_t equip_cnt);
			static std::uint32_t get_equip_price(db::DbSession& session, const std::string& equip_name);
			static bool check_duplicated_id(db::DbSession& session, std::string trans_id);
			static bool decrease_item_cnt(db::DbSession& session, const std::string& login_name, const std::string& itemName);
		};
	}
}
