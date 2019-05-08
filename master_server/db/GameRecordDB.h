#pragma once
#include "db/Database.h"

#include <cstdint>
#include <string>
#include <boost/optional.hpp>

namespace mserver {
	namespace db {

		struct GameRecordDB
		{
		public:
			std::uint64_t game_id_;
			std::uint64_t member_id_;
			std::uint32_t pass_time_;
			std::uint32_t pass_point_;

			static bool insert(db::DbSession&, const GameRecordDB&);
			static boost::optional<GameRecordDB> find_by_ids(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id);
			static void update_player_score(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id, std::uint32_t pass_time, std::uint32_t pass_point);
			static bool remove(db::DbSession&, std::uint64_t game_id, std::uint64_t member_id);
		};
	}
}