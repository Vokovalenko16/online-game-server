#include "stdafx.h"
#include "Future.h"

namespace mserver {
	namespace net {
		namespace detail {

			const std::exception_ptr work_canceled_error = std::make_exception_ptr(
				std::runtime_error{ "work canceled." });
		}
	}
}