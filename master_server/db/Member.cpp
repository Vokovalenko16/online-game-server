#include "stdafx.h"

#include "Member.h"

namespace mserver {
	namespace db {
		const char SQL_INSERT_MEMBER_QUERY[] = "INSERT INTO members (login_name, total_score) VALUES(?, ?) ";
		const char SQL_RETRIEV_ID_BY_LOGIN_NAME[] = "SELECT id FROM members WHERE login_name = ?";
		const char SQL_SEL_total_score[] = "SELECT total_score FROM members WHERE id = ?";
		const char SQL_SEL_rank[] = "SELECT count(id) + 1 FROM members WHERE total_score > ?";
		const char SQL_SEL_ranks[] = "SELECT login_name, total_score FROM members ORDER BY total_score DESC LIMIT ?, ?";
		const char SQL_SEL_total_count[] = "SELECT count(id) FROM members";
		std::uint64_t Member::insert(db::DbSession& conn, const Member& member)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_INSERT_MEMBER_QUERY
				, soci::use(member.login_name_)
				, soci::use(0)
				);

			st.execute(true);

			db::Equipment equip;
			equip.insert(conn, member.login_name_, "oItemDrink1", 3);
			equip.insert(conn, member.login_name_, "oItemDrink2", 3);
			equip.insert(conn, member.login_name_, "oItemDrink3", 3);
			equip.insert(conn, member.login_name_, "oItemShoes1", 3);
			equip.insert(conn, member.login_name_, "oItemShoes2", 3);
			equip.insert(conn, member.login_name_, "oItemShoes3", 3);
			equip.insert(conn, member.login_name_, "oItemClimbShoes", 3);
			equip.insert(conn, member.login_name_, "oItemPosition", 3);
			equip.insert(conn, member.login_name_, "oItemDirection", 3);
			equip.insert(conn, member.login_name_, "oItemReturn", 3);

			return conn.get_last_insert_rowid();
		}

		boost::optional<std::uint64_t> Member::find_by_login_name(db::DbSession& conn, std::string login_name)
		{
			std::uint64_t id;
			soci::statement st = (conn.get_session().prepare
				<< SQL_RETRIEV_ID_BY_LOGIN_NAME
				, soci::use(login_name)
				, soci::into(id)
				);
			if (!st.execute(true))
				return boost::none;
			else
				return id;
		}

		boost::optional<protocol::Body<protocol::MessageType::GetScoreAndRankResp>> Member::find_score_and_rank(
			db::DbSession& conn, 
			std::uint64_t member_id)
		{
			protocol::Body<M::GetScoreAndRankResp> resp;

			soci::statement st = (conn.get_session().prepare
				<< SQL_SEL_total_score
				, soci::use(member_id)
				, soci::into(resp.score)
			);

			if (!st.execute(true))
				return boost::none;

			st = (conn.get_session().prepare
				<< SQL_SEL_rank
				, soci::use(resp.score)
				, soci::into(resp.rank)
				);
			st.execute(true);

			return resp;
		}

		protocol::Body<M::GetRanksResp> Member::find_ranks(db::DbSession& conn, std::uint64_t start, std::uint32_t row_cnt)
		{
			protocol::Body<M::GetRanksResp> res;
			protocol::NameRank row;
			row.rank = start;
			soci::statement st = (conn.get_session().prepare
				<< SQL_SEL_ranks
				, soci::use(start)
				, soci::use(row_cnt)
				, soci::into(row.login_name.value())
				, soci::into(row.score)
				);

			if (st.execute(true))
			{
				do {
					row.rank++;
					res.records.push_back(row);
				} while (st.fetch());
			}

			return res;
		}

		protocol::Body<M::GetTotalMemberCountResp> Member::find_total_count(db::DbSession& conn)
		{
			protocol::Body<M::GetTotalMemberCountResp> res;

			soci::statement st = (conn.get_session().prepare
				<< SQL_SEL_total_count
				, soci::into(res.total_cnt)
				);

			st.execute(true);

			return res;
		}

	}
}