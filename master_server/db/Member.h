#pragma once
#include "db/Database.h"
#include "db/Equipment.h"
#include "protocol/Types.h"
#include "protocol/GameCommands.h"

#include <boost/optional.hpp>

#include <cstdint>
#include <string>

namespace mserver {
	namespace db {

		using M = protocol::MessageType;

		struct Member
		{
		public:
			std::uint64_t id_;
			std::string login_name_;

			static std::uint64_t insert(db::DbSession&, const Member&);
			static boost::optional<std::uint64_t> find_by_login_name(db::DbSession&, std::string login_name);
			static boost::optional<protocol::Body<M::GetScoreAndRankResp>> find_score_and_rank(db::DbSession&, std::uint64_t);
			static protocol::Body<M::GetRanksResp> find_ranks(db::DbSession&, std::uint64_t start, std::uint32_t row_cnt);
			static protocol::Body<M::GetTotalMemberCountResp> find_total_count(db::DbSession&);
		};
	}
}