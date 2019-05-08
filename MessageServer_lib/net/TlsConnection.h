#pragma once

#include "util/unique_shared_ptr.h"
#include "TlsContext.h"
#include <cstdint>
#include <memory>
#include <functional>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace mserver {
	namespace net {

		namespace detail {
			struct TlsConnectionImpl;
		}

		class TlsConnectionRef;
		using TcpSocket = boost::asio::basic_socket<boost::asio::ip::tcp
#if BOOST_VERSION < 106600
			, boost::asio::stream_socket_service<boost::asio::ip::tcp>
#endif
		>;

		class TlsConnection
		{
		private:
			util::unique_shared_ptr<detail::TlsConnectionImpl> impl_;

		public:
			TlsConnection();
			~TlsConnection();

			TlsConnection(TlsConnection&&);
			TlsConnection& operator =(TlsConnection&&);

			static TlsConnection create(boost::asio::io_service& io, const TlsContext& tls_ctx);
			void begin_server_handshake(std::function<void(boost::system::error_code)> completion);
			void begin_write(const void* data, size_t len, std::function<void(boost::system::error_code, size_t)> completion);
			void begin_read(void* data, size_t len, std::function<void(boost::system::error_code, size_t)> completion);

			TcpSocket& socket();
			std::uint32_t id() const;
			inline operator TlsConnectionRef() const;

			explicit operator bool() const { return (bool)impl_; }
		};

		class TlsConnectionRef
		{
			friend class TlsConnection;

		private:
			std::shared_ptr<detail::TlsConnectionImpl> impl_;
			explicit TlsConnectionRef(std::shared_ptr<detail::TlsConnectionImpl> impl)
				: impl_(std::move(impl))
			{}

		public:
			TcpSocket& socket();
		};

		inline TlsConnection::operator TlsConnectionRef() const
		{
			return TlsConnectionRef(impl_.shared());
		}
	}
}
