#pragma once
#include "protocol/Types.h"
#include "db/Database.h"
#include <vector>

namespace mserver {
	namespace db {

		struct Equip_list
		{
			std::uint64_t id;
			std::string name;
			std::uint16_t price;
			std::uint64_t category_id;
			std::string comment;
			std::uint32_t duration;
			std::uint32_t cooldown;
			std::string img_path;
			std::uint8_t indexInCategory;

			static std::vector<protocol::ItemInfo> get_all_detail(db::DbSession& session);
			static bool is_having_stock(db::DbSession& session, const std::string& name);
			static void update_stock(db::DbSession& session, const std::string& name, std::uint32_t count);
		};
	}
}
