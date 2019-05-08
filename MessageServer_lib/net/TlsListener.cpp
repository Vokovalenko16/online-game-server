#include "stdafx.h"
#include "TlsListener.h"
#include "TlsConnection.h"
#include "logger/Logger.h"
#include "core/Exception.h"
#include <boost/asio.hpp>
#include <mutex>
#include <chrono>

namespace mserver {
	namespace net {

		namespace asio = boost::asio;
		using asio::ip::tcp;
		using boost::system::error_code;

		const auto ErrorRetryDelay = std::chrono::seconds(60);

		struct TcpProtocolListener
		{
			using SPtr = std::shared_ptr<TcpProtocolListener>;

			asio::io_service* io_;
			tcp::endpoint ep_;
			TlsContext tls_ctx_;
			std::function<void(TlsConnection)> on_accept_;

			std::mutex mutex_;
			bool disposed_ = false;
			asio::basic_waitable_timer<std::chrono::steady_clock> retry_timer_;
			std::unique_ptr<tcp::acceptor> acceptor_;
			TlsConnection current_accepting_;

			TcpProtocolListener(
				asio::io_service& io,
				const tcp::endpoint& ep,
				const TlsContext& tls_ctx,
				const std::function<void(TlsConnection)>& on_accept)
				: io_(&io)
				, ep_(ep)
				, tls_ctx_(tls_ctx)
				, on_accept_(on_accept)
				, retry_timer_(*io_)
			{
			}

			void dispose()
			{
				std::unique_lock<std::mutex> lock(mutex_);
				acceptor_ = nullptr;
				retry_timer_.cancel();
				disposed_ = true;
			}

			static void start(SPtr me)
			{
				std::unique_lock<std::mutex> lock(me->mutex_);
				start_locked(me);
			}

			static void start_locked(const SPtr& me)
			{
				try
				{
					if (!me->acceptor_)
					{
						auto acceptor = std::make_unique<tcp::acceptor>(*me->io_);
						acceptor->open(me->ep_.protocol());
						acceptor->set_option(tcp::acceptor::reuse_address(true));
						acceptor->bind(me->ep_);
						acceptor->listen();
						me->acceptor_ = std::move(acceptor);
					}

					me->current_accepting_ = TlsConnection::create(*me->io_, me->tls_ctx_);
					auto original_acceptor = me->acceptor_.get();
					me->acceptor_->async_accept(me->current_accepting_.socket(), [me, original_acceptor](error_code ec)
					{
						std::unique_lock<std::mutex> lock(me->mutex_);
						if (me->disposed_ || original_acceptor != me->acceptor_.get())
							return;

						on_accept_locked(me, ec);
					});

					return;
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "error accepting connection asynchronously: %||", ex.what());
					log::write(log::Level::Error, "%||", ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error accepting connection asynchronously: %||", ex.what());
				}

				log::write(log::Level::Info, "will retry connection accept after 1 minute.");
				me->acceptor_ = nullptr;
				me->retry_timer_.expires_from_now(ErrorRetryDelay);
				me->retry_timer_.async_wait([me](error_code ec)
				{
					std::unique_lock<std::mutex> lock(me->mutex_);
					if (me->disposed_ || ec)
						return;

					start_locked(me);
				});
			}

			static void on_accept_locked(const SPtr& me, error_code ec)
			{
				try
				{
					if (!ec)
					{
						const auto& remote_ep = me->current_accepting_.socket().remote_endpoint();
						log::write(log::Level::Info, "new TCP connection from %||:%||, assigned ID %||",
							remote_ep.address().to_string(), remote_ep.port(), me->current_accepting_.id());

						initialize_connection_locked(me, std::move(me->current_accepting_));
					}
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error initializing connection: %||", ex.what());
				}

				start_locked(me);
			}

			static void initialize_connection_locked(const SPtr& me, TlsConnection connection)
			{
				me->io_->post([c = std::move(connection), callback = me->on_accept_]() mutable
				{
					try
					{
						callback(std::move(c));
					}
					catch (std::exception& ex)
					{
						log::write(log::Level::Error, "error in on_accept callback: %||", ex.what());
					}
				});
			}
		};

		struct TlsListener::Impl
		{
			util::unique_shared_ptr<TcpProtocolListener> listener_v4_;
			util::unique_shared_ptr<TcpProtocolListener> listener_v6_;

			Impl(asio::io_service& io, int port, const TlsContext& tls_ctx,
				const std::function<void(TlsConnection)>& on_accept)
				: listener_v4_(util::make_unique_shared<TcpProtocolListener>(io, tcp::endpoint(tcp::v4(), port), tls_ctx, on_accept))
				, listener_v6_(util::make_unique_shared<TcpProtocolListener>(io, tcp::endpoint(tcp::v6(), port), tls_ctx, on_accept))
			{
				TcpProtocolListener::start(listener_v4_.shared());
				TcpProtocolListener::start(listener_v6_.shared());
			}
		};

		TlsListener::TlsListener() = default;
		TlsListener::~TlsListener() = default;
		TlsListener::TlsListener(TlsListener&&) = default;
		TlsListener& TlsListener::operator =(TlsListener&&) = default;

		TlsListener TlsListener::create(
			boost::asio::io_service& io,
			int port,
			const TlsContext& tls_ctx,
			const std::function<void(TlsConnection)>& on_accept)
		{
			TlsListener res;
			res.impl_ = std::make_unique<Impl>(io, port, tls_ctx, on_accept);
			return res;
		}
	}
}
