#include "stdafx.h"
#include "TlsHandshaker.h"
#include "logger/Logger.h"
#include <boost/asio/basic_waitable_timer.hpp>
#include <mutex>

namespace mserver {
	namespace net {

		using TimeoutTimer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;
		using boost::system::error_code;

		struct TlsHandshaker::Impl
		{
			boost::asio::io_service* io_;
			std::chrono::steady_clock::duration timeout_;
			std::function<void(TlsConnection)> on_handshake_;

			void dispose() {}
		};

		struct HandshakeContext
		{
			std::mutex mutex;
			TlsConnection conn;
			TimeoutTimer timeout_timer;

			HandshakeContext(boost::asio::io_service& io)
				: timeout_timer(io)
			{}
		};

		TlsHandshaker::TlsHandshaker() = default;
		TlsHandshaker::~TlsHandshaker() = default;
		TlsHandshaker::TlsHandshaker(TlsHandshaker&&) = default;
		TlsHandshaker& TlsHandshaker::operator =(TlsHandshaker&&) = default;

		TlsHandshaker TlsHandshaker::create(
			boost::asio::io_service& io,
			std::chrono::steady_clock::duration timeout,
			std::function<void(TlsConnection)> on_handshake)
		{
			TlsHandshaker res;
			res.impl_ = util::make_unique_shared<Impl>();
			res.impl_->io_ = &io;
			res.impl_->timeout_ = timeout;
			res.impl_->on_handshake_ = on_handshake;
			return res;
		}

		void TlsHandshaker::begin_handshake(TlsConnection conn)
		{
			auto context = std::make_shared<HandshakeContext>(*impl_->io_);
			context->conn = std::move(conn);
			context->timeout_timer.expires_from_now(impl_->timeout_);
			context->timeout_timer.async_wait([context](error_code ec)
			{
				if (ec)
					return;

				std::unique_lock<std::mutex> lock(context->mutex);
				if (context->conn)
				{
					log::write(log::Level::Error, "connection %||: TLS handshake timeout.",
						context->conn.id());
					context->conn = TlsConnection{};
				}
			});

			context->conn.begin_server_handshake([context, impl = impl_.shared()](error_code ec)
			{
				std::unique_lock<std::mutex> lock(context->mutex);
				if (!context->conn)
					return;

				context->timeout_timer.cancel();

				if (ec)
				{
					log::write(log::Level::Error, "connection %||: TLS handshake error (%||).",
						context->conn.id(), ec.message());
					return;
				}

				log::write(log::Level::Debug, "connection %||: TLS handshake completed.",
					context->conn.id());

				if (impl->on_handshake_)
					impl->on_handshake_(std::move(context->conn));
			});
		}
	}
}
