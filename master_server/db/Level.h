#pragma once
#include "protocol/Types.h"
#include "protocol/detail/NetString.h"
#include "db/Database.h"

#include <cstdint>
#include <string>
#include <vector>
#include <boost/optional.hpp>

namespace mserver {
	namespace db {

		struct Level
		{
		public:
			std::uint64_t id;
			std::string name;
			std::uint32_t file_length;
			protocol::HashValType hash;

			static std::uint64_t insert(db::DbSession&, const Level&);

			static std::vector<Level> get_all_detail(db::DbSession&);
			static boost::optional<Level> find_by_id(db::DbSession&, std::uint64_t id);
			static boost::optional<Level> find_by_name(db::DbSession&, const std::string& level_name);
		};
	}
}
