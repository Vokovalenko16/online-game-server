#include "stdafx.h"
#include "Equip_history.h"

#include <boost/format.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include <stdexcept>

namespace mserver {
	namespace db {

		const char SQL_insert[] = "INSERT INTO equip_history VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
		const char SQL_is_transaction_finished[] = "SELECT pending FROM equip_history WHERE transaction_id = ?";
		const char SQL_get_member_id[] = "SELECT id FROM members WHERE login_name = ?";
		const char SQL_get_equip_id[] = "SELECT id FROM equip_list WHERE name = ?";
		const char SQL_update_pending[] = "UPDATE equip_history SET pending = ? WHERE transaction_id = ?";
		const char SQL_sel_not_finished_transaction[] = "SELECT * FROM equip_history WHERE pending = 0";
		const char SQL_sel_settles_between_dates[] =
			"SELECT\n"
			"	user,\n"
			"	sum(star_change)\n"
			"FROM\n"
			"	equip_history\n"
			"WHERE\n"
			"	error_desc = 'success!'\n"
			"AND DATE(date_time) BETWEEN ?\n"
			"AND ?\n"
			"GROUP BY user";
		const char SQL_sel_settles_by_user[] =
			"SELECT\n"
			"	date_time,\n"
			"	item,\n"
			"	item_change,\n"
			"	star_change\n"
			"FROM\n"
			"	equip_history\n"
			"WHERE\n"
			"	error_desc = 'success!'\n"
			"AND user = ? "
			"AND DATE(date_time) BETWEEN ?\n"
			"AND ?";

		void Equip_history::insert(
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
			std::uint32_t pending)
		{
			
			using namespace boost::posix_time;
			ptime now = second_clock::local_time();
			auto date_time = to_iso_extended_string(now);
			
			soci::statement stmt = (session.get_session().prepare << SQL_insert
				, soci::use(transaction_id)
				, soci::use(title)
				, soci::use(user)
				, soci::use(item)
				, soci::use(item_change)
				, soci::use(star_change)
				, soci::use(status)
				, soci::use(error_desc)
				, soci::use(transaction)
				, soci::use(signature)
				, soci::use(date_time)
				, soci::use(pending)
				);
			stmt.execute(true);
		}

		void Equip_history::insert(db::DbSession& session, Equip_history row)
		{
			using namespace boost::posix_time;
			ptime now = second_clock::local_time();
			auto date_time = to_iso_extended_string(now);

			soci::statement stmt = (session.get_session().prepare << SQL_insert
				, soci::use(row.transaction_id)
				, soci::use(row.title)
				, soci::use(row.user)
				, soci::use(row.item)
				, soci::use(row.item_change)
				, soci::use(row.star_change)
				, soci::use(row.status)
				, soci::use(row.error_desc)
				, soci::use(row.transaction)
				, soci::use(row.signature)
				, soci::use(date_time)
				, soci::use(row.pending)
				);
			stmt.execute(true);
		}

		bool Equip_history::is_transaction_finished(db::DbSession& session, const std::string& transaction_id)
		{
			std::uint32_t res = 0;
			soci::statement stmt = (session.get_session().prepare << SQL_is_transaction_finished
				, soci::use(transaction_id)
				, soci::into(res)
				);
			if (!stmt.execute(true))
				throw std::invalid_argument((boost::format{"transaction(%||) is not registered database."} % transaction_id).str());
			return res;
		}

		boost::optional<Equip_history> Equip_history::not_finished_transaction(db::DbSession& session)
		{
			Equip_history res;
			soci::statement stmt = (session.get_session().prepare << SQL_sel_not_finished_transaction
				, soci::into(res.transaction_id)
				, soci::into(res.title)
				, soci::into(res.user)
				, soci::into(res.item)
				, soci::into(res.item_change)
				, soci::into(res.star_change)
				, soci::into(res.status)
				, soci::into(res.error_desc)
				, soci::into(res.transaction)
				, soci::into(res.signature)
				, soci::into(res.date_time)
				, soci::into(res.pending)
				);
			if (!stmt.execute(true))
				return boost::none;
			return res;
		}

		void  Equip_history::update_pending(db::DbSession& session, const std::string& transaction_id, std::uint32_t pending)
		{
			soci::statement stmt = (session.get_session().prepare << SQL_update_pending
				, soci::use(pending)
				, soci::use(transaction_id)
				);
			stmt.execute(true);
		}

		std::vector<user_money> Equip_history::get_settles(db::DbSession& session, const std::string& start_date, const std::string& end_date)
		{
			std::vector<user_money> res;
			user_money row;
			soci::statement stmt = (session.get_session().prepare << SQL_sel_settles_between_dates
				, soci::into(row.user)
				, soci::into(row.money)
				, soci::use(start_date)
				, soci::use(end_date)
				);
			if (stmt.execute(true))
			{
				do {
					res.push_back(row);
				} while (stmt.fetch());
			}
			return res;
		}

		std::vector<settle_by_user> Equip_history::get_settles_by_user(db::DbSession& session, const std::string& user, const std::string& start_date, const std::string& end_date)
		{
			std::vector<settle_by_user> res;
			settle_by_user row;
			soci::statement stmt = (session.get_session().prepare << SQL_sel_settles_by_user
				, soci::into(row.date_time)
				, soci::into(row.item)
				, soci::into(row.item_change)
				, soci::into(row.star_change)
				, soci::use(user)
				, soci::use(start_date)
				, soci::use(end_date)
				);
			if (stmt.execute(true))
			{
				do {
					res.push_back(row);
				} while (stmt.fetch());
			}
			return res;
		}
	}
}
