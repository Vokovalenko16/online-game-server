#include "stdafx.h"
#include "Equipment.h"

namespace mserver {
	namespace db {

		std::vector<protocol::EquipDisplay> Equipment::find_by_user(db::DbSession& session, const std::string& login_name)
		{
			std::vector<protocol::EquipDisplay> res;
			protocol::EquipDisplay row;

			soci::statement stmt = (session.get_session().prepare << 
				"SELECT"
					" m.login_name,"
					" e.equip_id,"
					" e.equip_cnt,"
					" l.name,"
					" l.price,"
					" l.category_id,"
					" l.comment,"
					" l.duration,"
					" l.cooldown," 
					" l.img_path,"
					" l.indexInCategory,"
					" c.name category_name"
				" FROM"
					" equipment e"
				" LEFT JOIN members m ON e.member_id = m.id"
				" LEFT JOIN equip_list l ON e.equip_id = l.id"
				" LEFT JOIN equip_category c ON l.category_id = c.id"
				" WHERE"
					" c.id != 4"
					" AND m.login_name = ?"
					" AND e.equip_cnt > 0"
				" ORDER BY"
					" l.category_id ASC, l.indexInCategory ASC"
				, soci::use(login_name)
				, soci::into(row.login_name.value())
				, soci::into(row.equip_name.value())
				, soci::into(row.equip_cnt)
				, soci::into(row.name.value())
				, soci::into(row.price)
				, soci::into(row.category_id)
				, soci::into(row.comment.value())
				, soci::into(row.duration)
				, soci::into(row.cooldown)
				, soci::into(row.img_path.value())
				, soci::into(row.indexInCategory)
				, soci::into(row.category_name.value())
			);

			if (stmt.execute(true))
			{
				do {
					res.push_back(row);
				} while (stmt.fetch());
			}

			return res;
		}

		std::vector<protocol::EquipDisplay> Equipment::find_by_user_and_category(
			db::DbSession& session,
			const std::string& login_name,
			const std::uint64_t& category_id)
		{
			std::vector<protocol::EquipDisplay> res;
			protocol::EquipDisplay row;

			soci::statement stmt = (session.get_session().prepare <<
				"SELECT"
				" m.login_name,"
				" e.equip_id,"
				" e.equip_cnt,"
				" l.name,"
				" l.price,"
				" l.category_id,"
				" l.comment,"
				" l.duration,"
				" l.cooldown,"
				" l.img_path,"
				" l.indexInCategory,"
				" c.name category_name"
				" FROM"
					" equipment e"
				" LEFT JOIN members m ON e.member_id = m.id"
				" LEFT JOIN equip_list l ON e.equip_id = l.id"
				" LEFT JOIN equip_category c ON l.category_id = c.id"
				" WHERE"
					" m.login_name = ?"
				" AND e.equip_cnt > 0"
				" AND c.id = ?"
				" ORDER BY"
					" l.indexInCategory ASC"
				, soci::use(login_name)
				, soci::use(category_id)
				, soci::into(row.login_name.value())
				, soci::into(row.equip_name.value())
				, soci::into(row.equip_cnt)
				, soci::into(row.name.value())
				, soci::into(row.price)
				, soci::into(row.category_id)
				, soci::into(row.comment.value())
				, soci::into(row.duration)
				, soci::into(row.cooldown)
				, soci::into(row.img_path.value())
				, soci::into(row.indexInCategory)
				, soci::into(row.category_name.value())
			);

			std::vector<protocol::IDType> fileIDs;
			if (stmt.execute(true))
			{
				do {
					res.push_back(row);
				} while (stmt.fetch());
			}

			return res;
		}

		void Equipment::insert(db::DbSession& session, const std::string& login_name, const std::string& equip_name, std::uint32_t equip_cnt)
		{
			bool exist = 0;
			std::uint64_t member_id = 0;
			std::uint64_t equip_id = 0;
			soci::statement st_chk = (session.get_session().prepare
				<< "SELECT member_id, equip_id FROM equipment LEFT JOIN members ON equipment.member_id = members.id LEFT JOIN equip_list ON equipment.equip_id = equip_list.id WHERE login_name LIKE ? AND equip_list.name LIKE ?"
				, soci::use(login_name)
				, soci::use(equip_name)
				, soci::into(member_id)
				, soci::into(equip_id)
				);

			exist = st_chk.execute(true);

			if (exist)
			{
				soci::statement st = (session.get_session().prepare
					<< "UPDATE equipment SET equip_cnt = equip_cnt + ? WHERE member_id = ? AND equip_id = ?"
					, soci::use(equip_cnt)
					, soci::use(member_id)
					, soci::use(equip_id)
					);
				st.execute(true);
			}
			else
			{
				soci::statement st = (session.get_session().prepare
					<< "SELECT id FROM members WHERE login_name = ?"
					, soci::use(login_name)
					, soci::into(member_id)
					);
				st.execute(true);
				
				st = (session.get_session().prepare
					<< "SELECT id FROM equip_list WHERE name = ?"
					, soci::use(equip_name)
					, soci::into(equip_id)
					);
				st.execute(true);

				st = (session.get_session().prepare
					<< "INSERT INTO equipment(member_id, equip_id, equip_cnt) VALUES(?, ?, ?)"
					, soci::use(member_id)
					, soci::use(equip_id)
					, soci::use(equip_cnt));
				st.execute(true);
			}
			
		}

		bool Equipment::decrease_item_cnt(db::DbSession& session, const std::string& login_name, const std::string& itemName)
		{
			std::uint32_t equip_cnt = 0;
			std::uint64_t member_id = 0;
			std::uint64_t equip_id = 0;
			soci::statement st = (session.get_session().prepare
				<< "SELECT member_id, equip_id, equip_cnt FROM equipment e LEFT JOIN members m ON e.member_id = m.id LEFT JOIN equip_list l ON e.equip_id = l.id WHERE m.login_name = ? AND l.name = ?"
				, soci::use(login_name)
				, soci::use(itemName)
				, soci::into(member_id)
				, soci::into(equip_id)
				, soci::into(equip_cnt)
				);

			st.execute(true);

			if (equip_cnt <= 0)
				return false;

			st = (session.get_session().prepare
				<< "UPDATE equipment SET equip_cnt = equip_cnt - 1 WHERE member_id = ? AND equip_id = ? AND equip_cnt > 0"
				, soci::use(member_id)
				, soci::use(equip_id)
				);
			st.execute(true);

			return true;
		}

		std::uint32_t Equipment::get_equip_price(db::DbSession& session, const std::string& equip_name)
		{
			std::uint32_t equip_price;
			soci::statement st = (session.get_session().prepare
				<< "SELECT price FROM equip_list WHERE name LIKE ?"
				, soci::use(equip_name)
				, soci::into(equip_price)
				);
			st.execute(true);
			return equip_price;
		}

		bool Equipment::check_duplicated_id(db::DbSession& session, std::string trans_id)
		{
			std::uint32_t trans_cnt = 0;
			soci::statement st = (session.get_session().prepare
				<< "SELECT count(*) as cnt_id FROM equip_history WHERE transaction_id LIKE ?"
				, soci::use(trans_id)
				, soci::into(trans_cnt)
				);
			st.execute(true);
			return trans_cnt > 0 ? false : true;
		}
	}
}
