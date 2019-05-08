#include "stdafx.h"
#include "Equip_list.h"

namespace mserver {
	namespace db {
		const char SQL_is_having_stock[] = "SELECT stock > 0 FROM equip_list l WHERE l.name = ? AND l.is_stock_available = 1";
		const char SQL_update_stock[] = "UPDATE equip_list SET stock = stock - ? WHERE name = ? AND is_stock_available = 1 AND stock > 0";

		std::vector<protocol::ItemInfo> Equip_list::get_all_detail(db::DbSession& session)
		{
			std::vector<protocol::ItemInfo> res;
			protocol::ItemInfo row;

			soci::statement stmt = (session.get_session().prepare << 
				"SELECT id, name, price, category_id, comment, duration, cooldown, img_path, indexInCategory FROM equip_list WHERE stock ORDER BY category_id ASC, indexInCategory ASC"
				, soci::into(row.id)
				, soci::into(row.name.value())
				, soci::into(row.price)
				, soci::into(row.category_id)
				, soci::into(row.comment.value())
				, soci::into(row.duration)
				, soci::into(row.cooldown)
				, soci::into(row.img_path.value())
				, soci::into(row.indexInCategory)
			);

			if (stmt.execute(true))
			{
				do {
					res.push_back(row);
				} while (stmt.fetch());
			}

			return res;
		}

		bool Equip_list::is_having_stock(db::DbSession& session, const std::string& name)
		{
			int res = 1;
			soci::statement stmt = (session.get_session().prepare << SQL_is_having_stock
				, soci::into(res)
				, soci::use(name)
				);

			stmt.execute(true);
			return (bool)res;
		}

		void Equip_list::update_stock(db::DbSession& session, const std::string& name, std::uint32_t count)
		{
			soci::statement stmt = (session.get_session().prepare << SQL_update_stock
				, soci::use(count)
				, soci::use(name)
				);
		}
	}
}
