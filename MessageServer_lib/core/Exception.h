#pragma once

#include <boost/exception/exception.hpp>
#include <boost/exception/info.hpp>
#include <exception>
#include <string>

namespace mserver {
	namespace core {

		class Exception
			: public std::exception
			, public boost::exception
		{
		private:
			const char* msg_;

		public:
			explicit Exception(const char* msg)
				: msg_(msg)
			{}

			std::string diagnostic_information() const;

			virtual const char* what() const noexcept { return msg_; }
		};

		struct tag_errinfo_file_path;
		using errinfo_file_path = boost::error_info<tag_errinfo_file_path, std::string>;
	}
}
