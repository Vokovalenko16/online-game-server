#include "stdafx.h"

#include "GameDB.h"

namespace mserver {
	namespace db {
		const char SQL_INSERT_GAME_QUERY[] =			"INSERT INTO games(level_name, game_name, game_mode, active, start_time, end_time, checkpoint_count)VALUES(?, ?, ?, ?, ?, ?, ?) ";
		const char SQL_RETRIEV_GAME_BY_ID[] =			"SELECT * FROM games WHERE id = ?";
		const char SQL_UPDATE_ACTIVE_BY_ID[] =			"UPDATE games SET active=? WHERE id=?";
		const char SQL_UPDATE_CEHCKPOINTCOUNT_BY_ID[] = "UPDATE games SET checkpoint_count=? WHERE id=?";
		std::uint64_t GameDB::insert(db::DbSession& conn, const GameDB& game)
		{
			int active = 0;
			if (game.active_ == true)
				active = 1;
			else
				active = 0;
			soci::statement st = (conn.get_session().prepare
				<< SQL_INSERT_GAME_QUERY
				, soci::use(game.level_name_)
				, soci::use(game.game_name_)
				, soci::use(game.game_mode_)
				, soci::use(active)
				, soci::use(game.start_time_)
				, soci::use(game.end_time_)
				, soci::use(game.checkpoint_count_)
				);

			st.execute(true);

			return conn.get_last_insert_rowid();
		}

		boost::optional<GameDB> GameDB::find_by_id(db::DbSession& conn, std::uint64_t id)
		{
			GameDB game;
			std::uint32_t active = 0;
			soci::statement st = (conn.get_session().prepare
				<< SQL_RETRIEV_GAME_BY_ID
				, soci::use(id)
				, soci::into(game.id_)
				, soci::into(game.level_name_)
				, soci::into(game.game_name_)
				, soci::into(game.game_mode_)
				, soci::into(active)
				, soci::into(game.start_time_)
				, soci::into(game.end_time_)
				, soci::into(game.checkpoint_count_)
				, soci::into(game.game_difficulty_)
				);
			if (!st.execute(true))
				return boost::none;
			else
			{
				if (active == 0)
					game.active_ = false;
				else
					game.active_ = true;
				return game;
			}
		}

		void GameDB::terminate_game(db::DbSession& conn, std::uint64_t id)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_UPDATE_ACTIVE_BY_ID
				, soci::use(0)
				, soci::use(id)
				);
			st.execute(true);
		}

		void GameDB::update_checkpoint_count(db::DbSession &conn, std::uint64_t id, std::uint32_t checkpointcount)
		{
			soci::statement st = (conn.get_session().prepare
				<< SQL_UPDATE_CEHCKPOINTCOUNT_BY_ID
				, soci::use(checkpointcount)
				, soci::use(id)
				);
			st.execute(true);
		}
	}
}