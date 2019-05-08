#include "stdafx.h"
#include "MessageServer.h"
#include "protocol/Message.h"

#include "logger/Logger.h"
#include "core/Exception.h"
#include <chrono>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>
#include <sstream>
#include <boost/asio/basic_waitable_timer.hpp>
#include <boost/serialization/string.hpp>
#include <boost/asio/ip/tcp.hpp>

namespace mserver {
	namespace net {

		using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;
		using boost::system::error_code;

		struct MessageTypeHasher
		{
			std::hash<std::uint32_t> base_;

			size_t operator ()(protocol::MessageType x) const noexcept
			{
				return base_((std::uint32_t)x);
			}
		};

		struct MessageServer::Impl
		{
			boost::asio::io_service* io_;
			std::chrono::steady_clock::duration idle_timeout_;
			std::shared_timed_mutex handler_mutex_;
			std::unordered_map<protocol::MessageType, MessageHandler, MessageTypeHasher> handlers_;

			void dispose() {}
		};

		struct MessageClient;
		using ClientPtr = std::shared_ptr<MessageClient>;

		inline std::string endpoint_to_string(const boost::asio::ip::tcp::endpoint& ep)
		{
			std::ostringstream ss;
			ss << ep.address().to_string() << ":" << ep.port();
			return ss.str();
		}

		struct MessageClient
		{
		private:
			std::shared_ptr<MessageServer::Impl> server;
			std::mutex mutex;
			TlsConnection conn;
			Timer timeout_timer;
			protocol::Message msg_buf;
			NetSession net_session;
			size_t recv_pos;
			size_t transmit_pos;

		public:
			MessageClient(boost::asio::io_service& io, std::shared_ptr<MessageServer::Impl> server, TlsConnection conn)
				: server(std::move(server))
				, conn(std::move(conn))
				, timeout_timer(io)
				, net_session(endpoint_to_string(this->conn.socket().remote_endpoint()))
			{}

			static void start(const ClientPtr& me)
			{
				std::unique_lock<std::mutex> lock(me->mutex);
				MessageClient::reset_timeout_locked(me);
				MessageClient::begin_receive_message_locked(me);
			}

		private:
			static void reset_timeout_locked(const ClientPtr& me)
			{
				me->timeout_timer.expires_from_now(me->server->idle_timeout_);
				me->timeout_timer.async_wait([me](error_code ec)
				{
					if (ec)
						return;

					std::unique_lock<std::mutex> lock(me->mutex);
					if (!me->conn)
						return;

					log::write(log::Level::Error, "client %||: connection timed out.",
						me->conn.id());
					me->conn = TlsConnection{};
				});
			}

			void close_locked()
			{
				timeout_timer.cancel();
				conn = TlsConnection{};
			}

			void report_error_and_close_locked(error_code ec)
			{
				log::write(log::Level::Error, "client %||: error: '%||'.",
					conn.id(), ec.message());
				close_locked();
			}

			void client_disconnected_locked()
			{
				log::write(log::Level::Info, "client %||: disconnected.",
					conn.id());
				close_locked();
			}

			static void begin_receive_message_locked(const ClientPtr& me)
			{
				if (!me->conn)
					return;

				me->recv_pos = 0;
				continue_receive_header_locked(me);
			}

			static void continue_receive_header_locked(const ClientPtr& me)
			{
				me->conn.begin_read(
					me->msg_buf.raw_data + me->recv_pos,
					sizeof(protocol::MessageHeader) - me->recv_pos,
					[me](error_code ec, size_t len) { on_receive_header(me, ec, len); });
			}

			static void on_receive_header(const ClientPtr& me, error_code ec, size_t len)
			{
				std::unique_lock<std::mutex> lock(me->mutex);
				if (!me->conn)
					return;

				if (ec)
					return me->report_error_and_close_locked(ec);
				if (len == 0)
					return me->client_disconnected_locked();

				try
				{
					me->recv_pos += len;
					if (me->recv_pos < sizeof(protocol::MessageHeader))
						return continue_receive_header_locked(me);

					if (me->msg_buf.header.length > protocol::MaxMessageLen)
					{
						log::write(log::Level::Error, "client %||: protocol error: message length(%||) exceeds maximum.",
							me->conn.id(), me->msg_buf.header.length);
						return me->close_locked();
					}

					return continue_receive_body_locked(me);
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.what());
				}
			}

			static void continue_receive_body_locked(const ClientPtr& me)
			{
				if (me->msg_buf.header.length == me->recv_pos)
					return process_message_locked(me);

				me->conn.begin_read(
					me->msg_buf.raw_data + me->recv_pos,
					me->msg_buf.header.length - me->recv_pos,
					[me](error_code ec, size_t len) { on_receive_body(me, ec, len); });
			}

			static void on_receive_body(const ClientPtr& me, error_code ec, size_t len)
			{
				std::unique_lock<std::mutex> lock(me->mutex);
				if (!me->conn)
					return;

				if (ec)
					return me->report_error_and_close_locked(ec);
				if (len == 0)
					return me->client_disconnected_locked();

				try
				{
					me->recv_pos += len;
					return continue_receive_body_locked(me);
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.what());
				}
			}

			static void process_message_locked(const ClientPtr& me)
			{
				reset_timeout_locked(me);

				//log::write(log::Level::Debug, "client %||: received message %||.",
				//	me->conn.id(), me->msg_buf.header.length);

				protocol::MessageType response_type;

				{
					std::shared_lock<std::shared_timed_mutex> lock_handlers(me->server->handler_mutex_);
					auto handler_iter = me->server->handlers_.find(me->msg_buf.header.type);

					bool succeeded = false;
					std::string error_desc;
					try
					{
						if (handler_iter != me->server->handlers_.end())
						{
							response_type = handler_iter->second(me->msg_buf, me->net_session);
							succeeded = true;
						}
						else
							error_desc = "unrecognized message type.";
					}
					catch (std::exception& ex)
					{
						error_desc = ex.what();
					}

					if (!succeeded)
					{
						log::write(log::Level::Warning, "client %||: error handling message (id=%||, type=%||, error='%||')",
							me->conn.id(),
							me->msg_buf.header.id,
							(int)me->msg_buf.header.type,
							error_desc);

						protocol::MessageWriter w(me->msg_buf);
						w & error_desc;

						response_type = protocol::MessageType::ErrorResponse;
					}
				}

				me->msg_buf.header.type = response_type;

				me->transmit_pos = 0;
				continue_transmit_locked(me);
			}

			static void continue_transmit_locked(const ClientPtr& me)
			{
				if (!me->conn)
					return;

				if (me->transmit_pos == me->msg_buf.header.length)
					return begin_receive_message_locked(me);

				return me->conn.begin_write(
					me->msg_buf.raw_data + me->transmit_pos,
					me->msg_buf.header.length - me->transmit_pos,
					[me](error_code ec, size_t len) { on_transmit(me, ec, len); }
				);
			}

			static void on_transmit(const ClientPtr& me, error_code ec, size_t len)
			{
				std::unique_lock<std::mutex> lock(me->mutex);
				if (!me->conn)
					return;

				if (ec)
					return me->report_error_and_close_locked(ec);
				if (len == 0)
					return me->client_disconnected_locked();

				try
				{
					me->transmit_pos += len;
					return continue_transmit_locked(me);
				}
				catch (core::Exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.diagnostic_information());
				}
				catch (std::exception& ex)
				{
					log::write(log::Level::Error, "client %||: error: '%||'.",
						me->conn.id(), ex.what());
				}
			}
		};


		MessageServer::MessageServer() = default;
		MessageServer::~MessageServer() = default;
		MessageServer::MessageServer(MessageServer&&) = default;
		MessageServer& MessageServer::operator =(MessageServer&&) = default;

		MessageServer MessageServer::create(boost::asio::io_service& io, std::chrono::steady_clock::duration idle_timeout)
		{
			MessageServer res;
			res.impl_ = util::make_unique_shared<Impl>();
			res.impl_->io_ = &io;
			res.impl_->idle_timeout_ = idle_timeout;
			return res;
		}

		void MessageServer::service(TlsConnection conn)
		{
			auto client = std::make_shared<MessageClient>(*impl_->io_, impl_.shared(), std::move(conn));
			MessageClient::start(client);
		}

		void MessageServer::register_handler(protocol::MessageType type, MessageHandler handler)
		{
			std::unique_lock<std::shared_timed_mutex> lock(impl_->handler_mutex_);
			impl_->handlers_[type] = std::move(handler);
		}
	}
}
