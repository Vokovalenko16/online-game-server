#pragma once

#include "Types.h"
#include "protocol/detail/NetString.h"

#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/array.hpp>

namespace protocol {

	constexpr protocol::IndexType MaxCountFileIDsInMessage = (protocol::MaxMessageLen - MessageHeaderLen) / sizeof(protocol::IDType) - 10;
	constexpr protocol::IndexType MaxCountHashValuesInMessage = 100;

	struct FileInfo
	{
		protocol::IDType			FileID_;
		protocol::FileLengthType	FileLength;
		protocol::HashValType		HashVal;
		std::string					FilePath_;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & FileID_;
			ar & FileLength;
			ar & HashVal;
			ar & FilePath_;
		}
	};

	struct LevelInfo
	{
		std::uint64_t id;
		protocol::NetString<20> name;
		std::uint32_t file_length;
		protocol::HashValType hash;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & id;
			ar & name;
			ar & file_length;
			ar & hash;
		}
	};
}