#pragma once

#include <string>
#include <array>
#include <cstdint>

namespace protocol {

	std::string to_base16(const std::uint8_t* data, size_t len);
	std::string to_base16(const char* data, size_t len);
	void from_base16(const std::string& str, std::uint8_t* data, size_t len);

	template<size_t L> inline std::string to_base16(const std::array<std::uint8_t, L>& data)
	{
		return to_base16(data.data(), data.size());
	}

	template<size_t L> inline std::array<std::uint8_t, L> from_base16(const std::string& str)
	{
		std::array<std::uint8_t, L> result;
		from_base16(str, result.data(), result.size());
		return result;
	}
}
