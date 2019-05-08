
#include "GlobalFunctions.h"

namespace protocol {
	namespace filesystem = boost::filesystem;

	protocol::HashValType calc_block_hash(
		protocol::FileBlock block_buf,
		std::uint32_t read_cnt)
	{
		protocol::HashValType result;

		CryptoPP::SHA3_256 sha3_256_block;

		sha3_256_block.Update(block_buf.data(), read_cnt);
		sha3_256_block.Final(result.data());

		return result;
	}

	protocol::HashValType calc_file_hash(
		const std::string& file_name)
	{
		protocol::HashValType result;

		filesystem::fstream fs;
		fs.open(file_name, std::ios::in | std::ios::binary);
		if (!fs.good())
			throw std::invalid_argument("failed to load file");
		fs.seekg(0, std::ios::beg);
		std::vector<std::uint8_t> buf(protocol::BlockSize);
		std::vector<std::uint8_t> hash_block(32);
		std::streamsize read_cnt;
		CryptoPP::SHA3_256 sha3_256_file;

		while (!fs.eof())
		{
			fs.read((char*)buf.data(), protocol::BlockSize);
			read_cnt = fs.gcount();
			sha3_256_file.Update(buf.data(), (std::size_t)read_cnt);
		}

		sha3_256_file.Final(result.data());
		return result;
	}
}