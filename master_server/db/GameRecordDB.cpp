#include "stdafx.h"

#include "GameRecordDB.h"

namespace mserver {
	namespace db {
		const char SQL_INSERT_RECORD_QUERY[] = "INSERT INTO game_records (game_id, member_id, pass_time, pass_point) VALUES (?, ?, ?, ?)";
		const char SQL_RETRIEV_RECORD_BY_IDS[] = "SELECT * FROM game_records WHERE game_id = ? AND member_id = ?";
		const char SQL_UPDATE_SCORE_BY_IDS[] = "UPDATE game_records SET pass_time = ?, pass_point = ? WHERE game_id = ? AND member_id = ?";
		const char SQL_DELETE_RECORD_BY_IDS[] = "DELETE FROM game_records WHERE game_id = ? AND member_id = ?";
		bool GameRecordDB::insert(db::DbSession& conn, const GameRecordDB& record)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_INSERT_RECORD_QUERY
				, soci::use(record.game_id_)
				, soci::use(record.member_id_)
				, soci::use(0)
				, soci::use(0)
				);

			st.execute(true);
			return true;
		}

		boost::optional<GameRecordDB> GameRecordDB::find_by_ids(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id)
		{
			GameRecordDB record;

			soci::statement st = (conn.get_session().prepare
				<< SQL_RETRIEV_RECORD_BY_IDS
				, soci::use(game_id)
				, soci::use(member_id)
				, soci::into(record.game_id_)
				, soci::into(record.member_id_)
				, soci::into(record.pass_time_)
				, soci::into(record.pass_point_)
				);
			if (!st.execute(true))
				return boost::none;
			else
				return record;
		}

		void GameRecordDB::update_player_score(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id, std::uint32_t pass_time, std::uint32_t pass_point)
		{
			if (mserver::db::GameRecordDB::find_by_ids(conn, game_id, member_id) == boost::none)
			{
				mserver::db::GameRecordDB record;
				record.game_id_ = game_id;
				record.member_id_ = member_id;
				mserver::db::GameRecordDB::insert(conn, record);
			}

			soci::statement st = (conn.get_session().prepare
				<< SQL_UPDATE_SCORE_BY_IDS
				, soci::use(pass_time)
				, soci::use(pass_point)
				, soci::use(game_id)
				, soci::use(member_id)
				);
			st.execute(true);
		}

		bool GameRecordDB::remove(db::DbSession &conn, std::uint64_t game_id, std::uint64_t member_id)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_DELETE_RECORD_BY_IDS
				, soci::use(game_id)
				, soci::use(member_id)
				);
			st.execute(true);
			return true;
		}
	}
}