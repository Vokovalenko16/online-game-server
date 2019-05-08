#include "stdafx.h"
#include "http/Server.h"
#include "logger/Logger.h"
#include "core/Exception.h"
#include "net/TlsContextImpl.h"
#include <memory>
#include <chrono>
#include <boost/asio.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/core/flat_buffer.hpp>

#define BOOST_OPENSSL_RENAME_IDENTIFIERS
#include <boost/asio/ssl.hpp>

namespace mserver {
	namespace http {

		namespace asio = boost::asio;
		namespace beast = boost::beast;
		namespace ssl = boost::asio::ssl;
		using asio::ip::tcp;
		using boost::system::error_code;

		template<class Derived> class Session
		{
			Derived& derived() { return static_cast<Derived&>(*this); }
			static std::shared_ptr<Derived> derived(std::shared_ptr<Session<Derived>> me)
			{
				return std::static_pointer_cast<Derived>(me);
			}

			class Queue : public ResponseSender
			{
				static const int Limit = 8;

				// The type-erased, saved work item
				struct Work
				{
					virtual ~Work() = default;
					virtual void operator()() = 0;
				};

				std::weak_ptr<Session> self_;
				std::vector<std::unique_ptr<Work>> items_;

			public:
				void initialize(std::weak_ptr<Session> self)
				{
					self_ = std::move(self);
					static_assert(Limit > 0, "queue limit must be positive");
					items_.reserve(Limit);
				}

				// Returns `true` if we have reached the queue limit
				bool is_full() const
				{
					return items_.size() >= Limit;
				}

				// Called when a message finishes sending
				// Returns `true` if the caller should initiate a read
				bool on_write()
				{
					assert(!items_.empty());
					auto const was_full = is_full();
					items_.erase(items_.begin());
					if (!items_.empty())
						(*items_.front())();
					return was_full;
				}

				// Called by the HTTP handler to send a response.
				virtual void operator()(Response&& msg) override
				{
					// This holds a work item
					struct WorkImpl : Work
					{
						std::weak_ptr<Session> self_;
						Response msg_;

						WorkImpl(
							std::weak_ptr<Session> self,
							Response&& msg)
							: self_(std::move(self))
							, msg_(std::move(msg))
						{
						}

						void operator()()
						{
							auto self = self_.lock();
							if (!self)
								return;

							auto& session = *self;
							beast::http::async_write(session.derived().stream(), msg_,
								asio::bind_executor(
									session.strand_,
									[self = std::move(self), need_eof = msg_.need_eof()](error_code ec, size_t)
									{ Session::on_write(self, ec, need_eof); }
							));
						}
					};

					// Allocate and store the work
					items_.emplace_back(new WorkImpl(self_, std::move(msg)));

					// If there was no previous work, start this one
					if (items_.size() == 1)
						(*items_.front())();
				}
			};

		protected:
			Handler handler_;
			tcp::endpoint remote_ep_;
			asio::strand<asio::io_context::executor_type> strand_;
			asio::steady_timer timer_;
			beast::flat_buffer buffer_;
			Request req_;
			Queue queue_;

			void fail(error_code ec, const char* what)
			{
				log::write(
					log::Level::Error,
					"%|| error in HTTP session while servicing %||:%||: (%||)",
					what,
					remote_ep_.address().to_string(),
					remote_ep_.port(),
					ec);
			}

		public:
			// Take ownership of the socket
			Session(const Handler& handler, asio::io_context::executor_type executor, tcp::endpoint remote_ep)
				: handler_(handler)
				, remote_ep_(remote_ep)
				, strand_(executor)
				, timer_(executor.context(),
					(std::chrono::steady_clock::time_point::max)())
			{
			}

			using Ptr = std::shared_ptr<Session<Derived>>;

			static void init_queue(const Ptr& me)
			{
				me->queue_.initialize(me);
			}

			static void do_read(const Ptr& me)
			{
				// Set the timer
				me->timer_.expires_after(std::chrono::seconds(15));

				// Read a request
				me->req_ = Request{};
				beast::http::async_read(
					me->derived().stream(), me->buffer_, me->req_,
					asio::bind_executor(
						me->strand_,
						[me](error_code ec, size_t) { on_read(me, ec); }));
			}

			// Called when the timer expires.
			static void on_timer(const Ptr& me, boost::system::error_code ec)
			{
				if (ec && ec != asio::error::operation_aborted)
					return me->fail(ec, "timer");

				// Verify that the timer really expired since the deadline may have moved.
				if (me->timer_.expiry() <= std::chrono::steady_clock::now())
				{
					// Closing the socket cancels all outstanding operations. They
					// will complete with asio::error::operation_aborted
					Derived::do_timeout(derived(me));
					return;
				}

				// Wait on the timer
				me->timer_.async_wait(asio::bind_executor(
					me->strand_,
					[me](error_code ec) { on_timer(me, ec); }));
			}

			static void on_read(const Ptr& me, boost::system::error_code ec)
			{
				// Happens when the timer closes the socket
				if (ec == asio::error::operation_aborted)
					return;

				// This means they closed the connection
				if (ec == beast::http::error::end_of_stream)
					return Derived::do_close(derived(me));

				if (ec)
					return me->fail(ec, "read");

				try
				{
					// Send the response
					me->handler_.handle_request(std::move(me->req_), me->queue_);
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "error in HTTP server while handling request from %||:%||: %||",
						me->remote_ep_.address().to_string(),
						me->remote_ep_.port(),
						ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error in HTTP server while handling request from %||:%||: %||",
						me->remote_ep_.address().to_string(),
						me->remote_ep_.port(),
						ex.what());
				}

				// If we aren't at the queue limit, try to pipeline another request
				if (!me->queue_.is_full())
					do_read(me);
			}

			static void on_write(const Ptr& me, boost::system::error_code ec, bool close)
			{
				// Happens when the timer closes the socket
				if (ec == asio::error::operation_aborted)
					return;

				if (ec)
					return me->fail(ec, "write");

				if (close)
				{
					// This means we should close the connection, usually because
					// the response indicated the "Connection: close" semantic.
					return Derived::do_close(derived(me));
				}

				// Inform the queue that a write completed
				if (me->queue_.on_write())
				{
					// Read another request
					do_read(me);
				}
			}
		};

		class PlainSession : public Session<PlainSession>
		{
		private:
			tcp::socket socket_;

		public:
			using Ptr = std::shared_ptr<PlainSession>;

			tcp::socket& stream() { return socket_; }

			PlainSession(tcp::socket socket, const Handler& handler)
				: Session<PlainSession>(handler, socket.get_executor(), socket.remote_endpoint())
				, socket_(std::move(socket))
			{
			}

			// Start the asynchronous operation
			static void run(const Ptr& me)
			{
				init_queue(me);

				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				on_timer(me, {});

				do_read(me);
			}

			static void do_timeout(const Ptr& me)
			{
				boost::system::error_code ec;
				me->socket_.shutdown(tcp::socket::shutdown_both, ec);
				me->socket_.close(ec);
			}

			static void do_close(const Ptr& me)
			{
				// Send a TCP shutdown
				boost::system::error_code ec;
				me->socket_.shutdown(tcp::socket::shutdown_send, ec);

				// At this point the connection is closed gracefully
			}
		};


		// Handles an SSL HTTP connection
		class TlsSession
			: public Session<TlsSession>
		{
			ssl::stream<tcp::socket> stream_;
			bool eof_ = false;

		public:
			// Create the http_session
			TlsSession(
				tcp::socket socket,
				ssl::context& ctx,
				const Handler& handler)
				: Session<TlsSession>(handler, socket.get_executor(), socket.remote_endpoint())
				, stream_(std::move(socket), ctx)
			{
			}

			using Ptr = std::shared_ptr<TlsSession>;

			// Called by the base class
			ssl::stream<tcp::socket>& stream()
			{
				return stream_;
			}

			// Start the asynchronous operation
			static void run(const Ptr& me)
			{
				init_queue(me);

				// Run the timer. The timer is operated
				// continuously, this simplifies the code.
				on_timer(me, {});

				// Set the timer
				me->timer_.expires_after(std::chrono::seconds(15));

				// Perform the SSL handshake
				// Note, this is the buffered version of the handshake.
				me->stream_.async_handshake(
					ssl::stream_base::server,
					me->buffer_.data(),
					boost::asio::bind_executor(
						me->strand_,
						[me](error_code ec, size_t bytes_used) { on_handshake(me, ec, bytes_used); }
				));
			}

			static void on_handshake(const Ptr& me, error_code ec, size_t bytes_used)
			{
				// Happens when the handshake times out
				if (ec == boost::asio::error::operation_aborted)
					return;

				if (ec)
					return me->fail(ec, "handshake");

				// Consume the portion of the buffer used by the handshake
				me->buffer_.consume(bytes_used);

				do_read(me);
			}

			static void do_close(const Ptr& me)
			{
				me->eof_ = true;

				// Set the timer
				me->timer_.expires_after(std::chrono::seconds(15));

				// Perform the SSL shutdown
				me->stream_.async_shutdown(
					boost::asio::bind_executor(
						me->strand_,
						[me](error_code ec) { on_shutdown(me, ec); }
				));
			}

			static void on_shutdown(const Ptr& me, error_code ec)
			{
				// Happens when the shutdown times out
				if (ec == boost::asio::error::operation_aborted)
					return;

				if (ec)
					return me->fail(ec, "shutdown");

				// At this point the connection is closed gracefully
			}

			static void do_timeout(const Ptr& me)
			{
				// If this is true it means we timed out performing the shutdown
				if (me->eof_)
					return;

				// Start the timer again
				me->timer_.expires_at(
					(std::chrono::steady_clock::time_point::max)());
				on_timer(me, {});

				do_close(me);
			}
		};


		struct Server::Impl
		{
			tcp::acceptor acceptor_;
			Handler handler_;
			net::TlsContext tls_ctx_;

			Impl(asio::io_service& io, const Handler& handler, const net::TlsContext& tls_ctx)
				: acceptor_(io)
				, handler_(handler)
				, tls_ctx_(tls_ctx)
			{
			}

			void dispose()
			{
				acceptor_.close();
			}

			static void start(const std::shared_ptr<Impl>& me, const tcp::endpoint& ep)
			{
				me->acceptor_.open(ep.protocol());
				me->acceptor_.set_option(tcp::acceptor::reuse_address(true));
				me->acceptor_.bind(ep);
				me->acceptor_.listen();

				begin_accept(me);
			}

			static void begin_accept(const std::shared_ptr<Impl>& me)
			{
				me->acceptor_.async_accept([me](error_code ec, tcp::socket socket)
				{
					on_accept(me, ec, std::move(socket));
				});
			}

			static void on_accept(const std::shared_ptr<Impl>& me, error_code ec, tcp::socket&& socket)
			{
				tcp::endpoint remote_ep;

				try
				{
					remote_ep = socket.remote_endpoint();

					const auto tls_ctx_impl = me->tls_ctx_.impl();
					if (tls_ctx_impl)
					{
						auto session = std::make_shared<TlsSession>(std::move(socket), tls_ctx_impl->ctx_, me->handler_);
						TlsSession::run(session);
					}
					else
					{
					}
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "error in HTTP server while handling connection from %||:%||: %||",
						remote_ep.address().to_string(),
						remote_ep.port(),
						ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "error in HTTP server while handling connection from %||:%||: %||",
						remote_ep.address().to_string(),
						remote_ep.port(),
						ex.what());
				}

				begin_accept(me);
			}
		};

		Server::Server() = default;
		Server::~Server() = default;
		Server::Server(Server&&) = default;
		Server& Server::operator =(Server&&) = default;

		Server Server::create(
			boost::asio::io_service& io,
			const boost::asio::ip::tcp::endpoint& ep,
			const net::TlsContext& tls_ctx,
			const Handler& handler)
		{
			Server result;
			result.impl_ = util::make_unique_shared<Impl>(io, handler, tls_ctx);
			Impl::start(result.impl_.shared(), ep);
			return result;
		}
	}
}
