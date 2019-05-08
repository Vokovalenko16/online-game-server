#include "GameCenterConnection.h"
#include "Handler.h"
#include "json/writer.h"
#include "json/reader.h"

namespace mserver {
	namespace http {

		namespace asio = boost::asio;
		using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>
		namespace http = boost::beast::http;    // from <boost/beast/http.hpp>

		struct GameCenterConnection::Impl
		{
			asio::io_service* io_;
			asio::ip::address address_;
			int port_;
			std::string target_;

			Impl(asio::io_service& io, const asio::ip::address& address, int port, std::string target)
				: io_(&io)
				, address_(address)
				, port_(port)
				, target_(target)
			{
			}
		};

		GameCenterConnection::GameCenterConnection(boost::asio::io_service& io, const asio::ip::address& address, int port, const std::string& target)
			: impl_(std::make_unique<Impl>(io, address, port, target))
		{
		}

		GameCenterConnection::GameCenterConnection() = default;
		GameCenterConnection::~GameCenterConnection() = default;
		GameCenterConnection::GameCenterConnection(GameCenterConnection&&) = default;
		GameCenterConnection& GameCenterConnection::operator =(GameCenterConnection&&) = default;

		bool GameCenterConnection::valid() const
		{
			return impl_ != nullptr;
		}

		Json::Value GameCenterConnection::call_game_center(
			const Json::Value &req_val, std::string uri)
		{
			if (!impl_)
				throw std::runtime_error("no connection to game center");

			tcp::resolver resolver_(*impl_->io_);
			tcp::socket socket_(*impl_->io_);
			boost::beast::flat_buffer buffer_; // (Must persist between reads)
			boost::beast::http::request<boost::beast::http::vector_body<char>> req_;
			boost::beast::http::response<boost::beast::http::vector_body<char>> res_;

			// Set up an HTTP GET request message
			req_.version(11);
			req_.method(http::verb::post);
			req_.target(impl_->target_ + uri);
			req_.set(http::field::host, impl_->address_);
			req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
			req_.set(http::field::content_type, "application/json");
			auto req_body = Json::StyledWriter().write(req_val);
			req_.set(http::field::content_length, req_body.size());
			req_.body() = req_body;

			// Make the connection on the IP address we get from a lookup
			socket_.connect(tcp::endpoint(impl_->address_, impl_->port_));

			// Send the HTTP request to the remote host
			http::write(
				socket_,
				req_);

			// Receive the HTTP response
			http::read(socket_, buffer_, res_);
			// Write the message to standard out
			Json::Value resp_val;
			{
				const auto& body = res_.body();
				if (!Json::Reader().parse(body.data(), body.data() + body.size(), resp_val, false))
					throw std::runtime_error("invalid JSON data.");
			}

			auto str = resp_val.toStyledString();
			// Gracefully close the socket
			boost::system::error_code ec;
			socket_.shutdown(tcp::socket::shutdown_both, ec);

			return resp_val;
		}
	}
}
