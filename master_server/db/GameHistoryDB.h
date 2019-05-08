#pragma once
#include "db/Database.h"

#include <cstdint>
#include <string>
#include <boost/optional.hpp>
#include <vector>

namespace mserver {
	namespace db {

		struct GameHistoryDB
		{
		public:
			std::uint64_t game_id_;
			std::uint64_t member_id_;
			std::uint32_t pass_time_;
			std::uint32_t rank_;

			static bool insert(db::DbSession&, const GameHistoryDB&);
			static boost::optional<GameHistoryDB> find_by_ids(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id);
			static std::vector<protocol::GameHistoryInfo> find_by_memeber_id(db::DbSession&, std::uint64_t member_id);
			static bool update_player_history(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id, std::uint32_t pass_time, std::uint32_t score);
			static std::vector<protocol::LiveGameRank> find_by_game_id(db::DbSession&, std::uint64_t game_id);
			static bool remove(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id);
			static void update_player_pass_time(db::DbSession&, std::uint64_t gameId, std::uint64_t memberId, std::uint32_t passTime);
			static void set_player_rank_and_update_total_score(db::DbSession&, std::uint64_t game_id);
		};
	}
}