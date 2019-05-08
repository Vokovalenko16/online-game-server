#pragma once

#include "TlsConnection.h"
#include "util/unique_shared_ptr.h"
#include <boost/asio/io_service.hpp>
#include <chrono>
#include <functional>

namespace mserver {
	namespace net {

		class TlsHandshaker
		{
		private:
			struct Impl;
			util::unique_shared_ptr<Impl> impl_;

		public:
			TlsHandshaker();
			~TlsHandshaker();
			TlsHandshaker(TlsHandshaker&&);
			TlsHandshaker& operator =(TlsHandshaker&&);

			static TlsHandshaker create(
				boost::asio::io_service& io,
				std::chrono::steady_clock::duration timeout,
				std::function<void (TlsConnection)> on_handshake);

			void begin_handshake(TlsConnection conn);
		};
	}
}
