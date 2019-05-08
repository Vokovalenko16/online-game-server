#pragma once

#include <string>
#include <array>
#include <algorithm>

namespace protocol {

	namespace detail {

		template<typename T, typename Iter> struct ToArrayConvertible
		{
			Iter first;
			size_t length;

			ToArrayConvertible(Iter first, size_t length)
				: first(first)
				, length(length)
			{}

			template<size_t N> operator std::array<T, N>() const
			{
				size_t len = length;
				if (len > N) len = N;
				std::array<T, N> result;
				std::copy(first, first + len, result.begin());
				std::fill(result.begin() + len, result.end(), T());
				return result;
			}
		};
	}

	template<size_t N> inline std::string from_array(const std::array<char, N>& data)
	{
		size_t len = std::find(data.begin(), data.end(), '\0') - data.begin();
		return std::string(data.data(), len);
	}

	inline detail::ToArrayConvertible<char, std::string::const_iterator> to_array(const std::string& str)
	{
		return detail::ToArrayConvertible<char, std::string::const_iterator>(
			str.begin(),
			str.size());
	}
}
