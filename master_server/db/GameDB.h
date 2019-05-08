#pragma once
#include "db/Database.h"
#include "protocol/Types.h"

#include <cstdint>
#include <string>
#include <boost/optional.hpp>

namespace mserver {
	namespace db {

		struct GameDB
		{
		public:
			std::uint64_t id_;
			std::string level_name_;
			std::string game_name_;
			std::uint32_t game_mode_;
			bool active_;
			std::string start_time_;
			std::string end_time_;
			std::uint32_t checkpoint_count_;
			std::uint32_t game_difficulty_;
			
			GameDB() {};
			GameDB(std::uint64_t id, std::string level_name, std::string game_name, protocol::GameMode game_mode
				, bool active, std::string start_time, std::string end_time, std::uint32_t checkpoint_count
				, std::uint32_t game_difficulty)
				:id_(id)
				,level_name_(level_name)
				,game_name_(game_name)
				,active_(active)
				,start_time_(start_time)
				,end_time_(end_time)
				,checkpoint_count_(checkpoint_count)
				,game_difficulty_(game_difficulty)
			{
				if (game_mode == protocol::GameMode::RealTime)
					game_mode_ = 1;
				else if (game_mode == protocol::GameMode::Expert)
					game_mode_ = 2;
				else if (game_mode == protocol::GameMode::Treasure)
					game_mode_ = 3;
			}

			static std::uint64_t insert(db::DbSession&, const GameDB&);
			static boost::optional<GameDB> find_by_id(db::DbSession&, std::uint64_t id);
			static void terminate_game(db::DbSession&, std::uint64_t id);
			static void update_checkpoint_count(db::DbSession&, std::uint64_t id, std::uint32_t checkpointcount);
		};
	}
}