#pragma once

#include "http/Handler.h"
#include <boost/filesystem/path.hpp>
#include <string>

namespace mserver {
	namespace http {

		class StaticContentHandler
		{
		private:
			boost::filesystem::path root_dir_;

			void scan_directory(Handler&, const boost::filesystem::path& physical_path, const std::string& relative_path);
			void add_file(Handler&, const boost::filesystem::path& physical_path, const std::string& relative_path);

		public:
			StaticContentHandler(Handler& handler, const boost::filesystem::path& root_dir);
		};
	}
}
