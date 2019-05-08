#pragma once

#include <string>
#include <stdexcept>
#include <cstdint>
#include <boost/serialization/split_member.hpp>

namespace protocol {

	template<size_t MaxLen> class NetString
	{
	private:
		std::string value_;

	public:
		BOOST_SERIALIZATION_SPLIT_MEMBER()

		template<typename Ar> void save(Ar& ar, unsigned int /*version*/) const
		{
			std::uint32_t len = value_.size();
			if (len > MaxLen)
				len = MaxLen;

			ar << len;
			for (std::uint32_t i = 0; i < len; ++i)
				ar << value_[i];
		}

		template<typename Ar> void load(Ar& ar, unsigned int /*version*/)
		{
			std::uint32_t len;
			ar >> len;
			if (len > MaxLen)
				throw std::runtime_error("length of NetString exceeded maximum limit.");

			std::string str;
			str.resize(len);
			for (auto& c : str)
				ar >> c;

			value_ = std::move(str);
		}

		const std::string& value() const { return value_; }
		std::string& value() { return value_; }

		operator const std::string&() const { return value_; }
		NetString<MaxLen>& operator =(const std::string& str) { value_ = str; return *this; }
		NetString<MaxLen>& operator =(std::string&& str) { value_ = std::move(str); return *this; }
	};
}
