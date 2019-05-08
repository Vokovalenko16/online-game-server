#pragma once

#include <boost/asio/io_service.hpp>

namespace mserver {
	namespace util {

		void run_io_workers(boost::asio::io_service& io, int threads_per_core = 2);
	}
}
