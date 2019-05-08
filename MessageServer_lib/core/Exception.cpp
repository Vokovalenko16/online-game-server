#include "stdafx.h"
#include "Exception.h"
#include <boost/exception/diagnostic_information.hpp>

namespace mserver {
	namespace core {

		std::string Exception::diagnostic_information() const
		{
			return boost::diagnostic_information(*this);
		}
	}
}
