#include "stdafx.h"
#include "TlsConnection.h"
#include "TlsContextImpl.h"
#include <vector>
#include <mutex>
#include <atomic>

#define BOOST_OPENSSL_RENAME_IDENTIFIERS
#include <boost/asio/ssl.hpp>

namespace mserver {
	namespace net {

		namespace asio = boost::asio;
		namespace ssl = asio::ssl;
		using asio::ip::tcp;
		using ConnPtr = std::shared_ptr<detail::TlsConnectionImpl>;
		using boost::system::error_code;
		using CompletionHandler = std::function<void(error_code)>;

		namespace {
			std::atomic_uint_least32_t next_connection_id;
		}

		struct detail::TlsConnectionImpl
		{
			// immutable members
			std::weak_ptr<detail::TlsConnectionImpl> self_weak_;
			std::uint32_t id_;
			asio::io_service* io_;

			// mutable members : protected by mutex
			std::mutex mutex_;
			ssl::stream<tcp::socket> sock_;

			// 
			ConnPtr shared_ptr() const { return self_weak_.lock(); }

			TlsConnectionImpl(asio::io_service& io, const TlsContext& tls_ctx)
				: id_(++next_connection_id)
				, io_(&io)
				, sock_(*io_, tls_ctx.impl()->ctx_)
			{
			}

			void dispose()
			{
				std::unique_lock<std::mutex> lock(mutex_);
				sock_.lowest_layer().close();
			}
		};


		TlsConnection::TlsConnection() = default;
		TlsConnection::~TlsConnection() = default;

		TlsConnection::TlsConnection(TlsConnection&&) = default;
		TlsConnection& TlsConnection::operator =(TlsConnection&&) = default;

		TlsConnection TlsConnection::create(boost::asio::io_service& io, const TlsContext& tls_ctx)
		{
			TlsConnection res;
			res.impl_ = util::make_unique_shared<detail::TlsConnectionImpl>(io, tls_ctx);
			res.impl_->self_weak_ = res.impl_.weak();
			return res;
		}

		void TlsConnection::begin_server_handshake(std::function<void(boost::system::error_code)> completion)
		{
			std::unique_lock<std::mutex> lock(impl_->mutex_);
			impl_->sock_.async_handshake(
				ssl::stream_base::server,
				[completion = std::move(completion), keep_impl = impl_.shared()](error_code ec) { completion(ec); });
		}

		TcpSocket& TlsConnection::socket()
		{
			return impl_->sock_.lowest_layer();
		}

		std::uint32_t TlsConnection::id() const
		{
			return impl_->id_;
		}

		void TlsConnection::begin_write(const void* data, size_t len, std::function<void(boost::system::error_code, size_t)> completion)
		{
			impl_->sock_.async_write_some(
				asio::buffer(data, len),
				[self = impl_.shared(), completion](auto ec, auto len) { completion(ec, len); });
		}

		void TlsConnection::begin_read(void* data, size_t len, std::function<void(boost::system::error_code, size_t)> completion)
		{
			impl_->sock_.async_read_some(
				asio::buffer(data, len),
				[self = impl_.shared(), completion](auto ec, auto len) { completion(ec, len); });
		}

		TcpSocket& TlsConnectionRef::socket()
		{
			return impl_->sock_.lowest_layer();
		}
	}
}
