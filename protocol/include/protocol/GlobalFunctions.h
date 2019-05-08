#include <string>
#include <vector>
#include <boost/filesystem/fstream.hpp>
#include <array>
#include "Message.h"
#include "CryptoPP/sha3.h"

namespace protocol {

	protocol::HashValType calc_block_hash(
		protocol::FileBlock block_buf,
		std::uint32_t read_cnt);

	protocol::HashValType calc_file_hash(
		const std::string& file_name);
}