#pragma once

#include <boost/asio/ssl/context.hpp>

namespace mserver {
	namespace net {
		namespace detail {

			struct TlsContextImpl
			{
				boost::asio::ssl::context ctx_;

				TlsContextImpl();
			};
		}
	}
}
