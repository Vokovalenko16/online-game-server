#pragma once

#include "http/Handler.h"
#include "util/unique_shared_ptr.h"
#include "net/TlsContext.h"
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace mserver {
	namespace http {

		class Server
		{
		private:
			struct Impl;
			util::unique_shared_ptr<Impl> impl_;

		public:
			Server();
			~Server();
			Server(Server&&);
			Server& operator =(Server&&);

			static Server create(
				boost::asio::io_service& io,
				const boost::asio::ip::tcp::endpoint& ep,
				const net::TlsContext& tls_ctx,
				const Handler& handler);
		};
	}
}
