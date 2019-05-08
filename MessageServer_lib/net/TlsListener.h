#pragma once

#include "util/unique_shared_ptr.h"
#include "TlsContext.h"
#include <boost/asio/io_service.hpp>
#include <functional>

namespace mserver {
	namespace net {

		class TlsConnection;

		class TlsListener
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			TlsListener();
			~TlsListener();

			TlsListener(TlsListener&&);
			TlsListener& operator =(TlsListener&&);

			static TlsListener create(
				boost::asio::io_service& io,
				int port,
				const TlsContext& tls_ctx,
				const std::function<void (TlsConnection)>& on_accept);
		};
	}
}
