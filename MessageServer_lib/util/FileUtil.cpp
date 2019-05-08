#include "stdafx.h"
#include "FileUtil.h"
#include "core/Exception.h"
#include <fstream>

namespace mserver {
	namespace util {

		void load_file(const char* path, std::vector<std::uint8_t>& buf)
		{
			std::fstream file;
			file.open(path, std::ios::in | std::ios::binary);
			if (!file.good())
			{
				throw core::Exception("failed to load file.")
					<< core::errinfo_file_path(path);
			}

			file.seekg(0, std::ios::end);
			auto original_size = buf.size();
			buf.resize(original_size + (size_t)file.tellg());

			file.seekg(0, std::ios::beg);
			file.read((char*)buf.data() + original_size, buf.size() - original_size);
			if (file.fail() || file.gcount() != buf.size() - original_size)
			{
				throw core::Exception("failed to load file.")
					<< core::errinfo_file_path(path);
			}
		}
	}
}
