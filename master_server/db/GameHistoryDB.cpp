#include "stdafx.h"

#include "GameHistoryDB.h"
#include "GameDB.h"
#include <stdexcept>
#include <boost/format.hpp>

namespace mserver {
	namespace db {
		const char SQL_INSERT_RECORD_QUERY[] = "INSERT INTO game_history(game_id, member_id, pass_time, rank) VALUES (?, ?, ?, ?)";
		const char SQL_RETRIEV_RECORD_BY_IDS[] = "SELECT * FROM game_history WHERE game_id = ? AND member_id = ?";
		const char SQL_UPDATE_PASSTIME_BY_IDS[] = "UPDATE game_history SET pass_time = ?, rank = ? WHERE game_id = ? AND member_id = ?";
		const char SQL_UPDATE_RANK_BY_IDS[] = "UPDATE game_history SET rank = ? WHERE game_id = ? AND member_id = ?";
		const char SQL_DELETE_RECORD_BY_IDS[] = "DELETE FROM game_history WHERE game_id = ? AND member_id = ?";
		const char SQL_GET_DETAILS_BY_MEMBER_ID[] = "SELECT member_id, login_name, game_id, pass_time, rank, level_name, game_name, game_mode, active, start_time, end_time, checkpoint_count, game_difficulty"
													" FROM game_history h"
													" LEFT JOIN games g ON g.id = h.game_id"
													" LEFT JOIN members m ON m.id = h.member_id"
													" WHERE h.member_id = ?";
		const char SQL_SELECT_COUNT_MATCHING[] = "SELECT count(pass_time) FROM game_history WHERE game_id = ? AND member_id = ?";
		const char SQL_UPDATE_PASSTIME_TO_MIN[] = "UPDATE game_history SET pass_time = ? WHERE pass_time > ? AND game_id = ? AND member_id = ?";
		const char SQL_SEL_PLAYER_ORDER_BY_PASSTIME[] = "SELECT member_id FROM game_history" 
											" LEFT JOIN games ON games.id = game_history.game_id "
											" WHERE game_id = ? AND game_mode = 1 ORDER BY pass_time ASC";
		const char SQL_UPDATE_PLAYER_TOTAL_SCORE[] = "UPDATE members SET total_score = total_score + ? WHERE id = ?";
		const char SQL_SELECT_BY_GAME_ID[] = "SELECT member_id, pass_time FROM game_history WHERE game_id = ? ORDER BY pass_time ASC";
		const char SQL_get_member_info[] = "SELECT login_name, total_score FROM members WHERE id = ?";

		bool GameHistoryDB::insert(db::DbSession& conn, const GameHistoryDB& record)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_INSERT_RECORD_QUERY
				, soci::use(record.game_id_)
				, soci::use(record.member_id_)
				, soci::use(record.pass_time_)
				, soci::use(record.rank_)
				);

			st.execute(true);
			return true;
		}

		boost::optional<GameHistoryDB> GameHistoryDB::find_by_ids(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id)
		{
			GameHistoryDB record;

			soci::statement st = (conn.get_session().prepare
				<< SQL_RETRIEV_RECORD_BY_IDS
				, soci::use(game_id)
				, soci::use(member_id)
				, soci::into(record.game_id_)
				, soci::into(record.member_id_)
				, soci::into(record.pass_time_)
				, soci::into(record.rank_)
				);
			if (!st.execute(true))
				return boost::none;
			else
				return record;
		}

		bool GameHistoryDB::update_player_history(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id, std::uint32_t pass_time, std::uint32_t rank)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_UPDATE_RANK_BY_IDS
				, soci::use(pass_time)
				, soci::use(rank)
				, soci::use(game_id)
				, soci::use(member_id)
				);
			st.execute(true);
			return true;
		}

		bool GameHistoryDB::remove(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_DELETE_RECORD_BY_IDS
				, soci::use(game_id)
				, soci::use(member_id)
				);
			st.execute(true);
			return true;
		}

		std::vector<protocol::GameHistoryInfo> GameHistoryDB::find_by_memeber_id(db::DbSession& conn, std::uint64_t member_id)
		{
			protocol::GameHistoryInfo row;
			soci::statement st = (conn.get_session().prepare
				<< SQL_GET_DETAILS_BY_MEMBER_ID
				, soci::use(member_id)
				, soci::into(row.memeber_id)
				, soci::into(row.login_name.value())
				, soci::into(row.game_id)
				, soci::into(row.pass_time.value())
				, soci::into(row.rank)
				, soci::into(row.level_name.value())
				, soci::into(row.game_name.value())
				, soci::into((int&)row.game_mode)
				, soci::into((int&)row.active)
				, soci::into(row.start_time.value())
				, soci::into(row.end_time.value())
				, soci::into(row.checkpoint_count)
				, soci::into(row.game_difficulty)
			);
			
			std::vector<protocol::GameHistoryInfo> res;

			if (st.execute(true))
			{
				do {
					res.push_back(row);
				} while (st.fetch());
			}

			return res;
		}

		void GameHistoryDB::update_player_pass_time(db::DbSession& conn, std::uint64_t game_id, std::uint64_t member_id, std::uint32_t passTime)
		{
			int isExist = 0;
			soci::statement st = (conn.get_session().prepare
				<< SQL_SELECT_COUNT_MATCHING
				, soci::use(game_id)
				, soci::use(member_id)
				, soci::into(isExist)
			);
			st.execute(true);

			if (isExist)
			{
				st = (conn.get_session().prepare
					<< SQL_UPDATE_PASSTIME_TO_MIN
					, soci::use(passTime)
					, soci::use(passTime)
					, soci::use(game_id)
					, soci::use(member_id)
					);
				st.execute(true);
			}
			else
			{
				st = (conn.get_session().prepare
					<< SQL_INSERT_RECORD_QUERY
					, soci::use(game_id)
					, soci::use(member_id)
					, soci::use(passTime)
					, soci::use(0)
					);
				st.execute(true);
			}
		}

		void GameHistoryDB::set_player_rank_and_update_total_score(db::DbSession& conn, std::uint64_t game_id)
		{
			auto game = GameDB::find_by_id(conn, game_id);
			if (game == boost::none)
				throw std::invalid_argument((boost::format{ "Not registered game_id(%||) is used." } % game_id).str());

			std::vector<int> member_ids;
			int member_id = 0;
			soci::statement st = (conn.get_session().prepare
				<< SQL_SEL_PLAYER_ORDER_BY_PASSTIME
				, soci::use(game_id)
				, soci::into(member_id)
			);

			if (st.execute(true))
			{
				do {
					member_ids.push_back(member_id);
				} while (st.fetch());
			}
			
			int rank = 0;

			for (const auto& m_id : member_ids)
			{
				rank++;
				st = (conn.get_session().prepare
					<< SQL_UPDATE_RANK_BY_IDS
					, soci::use(rank)
					, soci::use(game_id)
					, soci::use(m_id)
				);

				st.execute(true);
				
				if (game.value().game_mode_ != (std::uint32_t)protocol::GameMode::Expert)
				{
					int score = 1;
					if (rank == 1)
						score = 5;
					else if (rank == 2)
						score = 3;
					else if (rank == 3)
						score = 2;

					st = (conn.get_session().prepare
						<< SQL_UPDATE_PLAYER_TOTAL_SCORE
						, soci::use(score)
						, soci::use(m_id)
						);
					st.execute(true);
				}
			}
		}

		std::vector<protocol::LiveGameRank> GameHistoryDB::find_by_game_id(db::DbSession& conn, const std::uint64_t game_id)
		{
			std::vector<protocol::LiveGameRank> res;
			protocol::LiveGameRank row;
			std::uint32_t member_id;
			std::uint32_t rank = 0;
			std::string login_id;
			std::uint32_t grade = 0;

			soci::statement st = (conn.get_session().prepare
				<< SQL_SELECT_BY_GAME_ID
				, soci::use(game_id)
				, soci::into(member_id)
				, soci::into(row.passtime)
				);

			if (st.execute(true))
			{
				do {
					soci::statement st_join = (conn.get_session().prepare
						<< SQL_get_member_info
						, soci::use(member_id)
						, soci::into(login_id)
						, soci::into((int&)grade)
						);

					st_join.execute(true);
					rank++;
					row.rank = rank;
					row.login_id = login_id;
					row.grade = grade;
					res.push_back(row);
				} while (st.fetch());
			}

			return res;
		}
	}
}