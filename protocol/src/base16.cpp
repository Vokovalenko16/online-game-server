#include "../include/protocol/base16.h"
#include <stdexcept>

namespace protocol {

	namespace {
		inline char hex_to_char(int x)
		{
			if (x < 10) return '0' + x;
			else return 'A' + (x - 10);
		}

		inline int char_to_hex(char c)
		{
			if (c >= '0' && c <= '9') return c - '0';
			if (c >= 'a' && c <= 'f') return c - 'a' + 10;
			if (c >= 'A' && c <= 'F') return c - 'A' + 10;
			throw std::invalid_argument("invalid character in base16-encoded string.");
		}
	}

	std::string to_base16(const std::uint8_t* data, size_t len)
	{
		std::string result;
		result.resize(len * 2);

		for (size_t i = 0; i < len; ++i)
		{
			int x = data[i];
			result[i * 2 + 0] = hex_to_char(x >> 4);
			result[i * 2 + 1] = hex_to_char(x & 0x0f);
		}

		return result;
	}
	std::string to_base16(const char* data, size_t len)
	{
		std::string result;
		result.resize(len * 2);

		for (size_t i = 0; i < len; ++i)
		{
			int x = data[i] + 0x8f;
			result[i * 2 + 0] = hex_to_char(x >> 4);
			result[i * 2 + 1] = hex_to_char(x & 0x0f);
		}

		return result;
	}

	void from_base16(const std::string& str, std::uint8_t* data, size_t len)
	{
		if (str.size() != len * 2)
			throw std::invalid_argument("length of base16-encoded string is not correct.");

		for (size_t i = 0; i < len; ++i)
		{
			data[i]
				= (char_to_hex(str[i * 2 + 0]) << 4)
				| char_to_hex(str[i * 2 + 1]);
		}
	}
}
