#pragma once

#include <vector>
#include <cstdint>

namespace mserver {
	namespace util {

		void load_file(const char* path, std::vector<std::uint8_t>& buf);
	}
}
