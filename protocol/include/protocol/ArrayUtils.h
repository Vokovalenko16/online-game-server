#pragma once
#include <vector>
#include <array>
#include <algorithm>

namespace protocol {
	
	template<typename T, size_t N> 
	std::array<T, N> inline vec_to_array(const std::vector<T>& vec)
	{
		std::array<T, N> result;
		auto len = vec.size();
		if (len > N)
			len = N;
		std::copy(vec.begin(), vec.begin() + len, result.begin());
		std::fill(result.begin() + len, result.end(), 0);
		return result;
	}
}