#include "stdafx.h"
#include "ChildServerAdminServices.h"
#include "json/value.h"
#include "json/writer.h"
#include "json/reader.h"

#include <unordered_map>
#include <exception>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#define BOOST_OPENSSL_RENAME_IDENTIFIERS
#include <boost/asio/ssl.hpp>

namespace mserver {
	namespace service {

		namespace asio = boost::asio;
		using tcp = boost::asio::ip::tcp;
		namespace ssl = boost::asio::ssl;
		namespace beast = boost::beast;

		struct ChildServer
		{
			std::string security_token;
			asio::ip::address host;
			int port;
		};

		struct ChildServerAdminServices::Impl
		{
			std::unordered_map<std::string, ChildServer> child_servers;
		};

		ChildServerAdminServices::ChildServerAdminServices(boost::asio::io_service& io, http::Handler& handler)
			: impl_(std::make_unique<Impl>())
		{
			auto& impl = *impl_;
			handler.register_json_method("/child_admin", [&impl, &io](Json::Value&& req, Json::Value& resp)
			{
				auto child_name = req["server_name"].asString();
				auto request_path = req["request_path"].asString();
				auto request_data = req["request_data"];

				auto iter = impl.child_servers.find(child_name);
				if (iter == impl.child_servers.end())
				{
					resp["succeeded"] = false;
					resp["message"] = "requested child server was not registered.";
					return;
				}

				try
				{
					ssl::context ctx{ ssl::context::tlsv12_client };

					// These objects perform our I/O
					tcp::resolver resolver{ io };
					ssl::stream<tcp::socket> stream{ io, ctx };

					// Make the connection on the IP address we get from a lookup
					stream.next_layer().connect(tcp::endpoint(iter->second.host, iter->second.port));

					// Perform the SSL handshake
					stream.handshake(ssl::stream_base::client);

					// Set up an HTTP POST request message
					beast::http::request<beast::http::vector_body<char>> child_req{ beast::http::verb::post, request_path, 11 };
					child_req.set(beast::http::field::host, iter->second.host);
					child_req.set(beast::http::field::user_agent, BOOST_BEAST_VERSION_STRING);
					child_req.set(beast::http::field::content_type, "application/json");
					child_req.set("Security-Token", iter->second.security_token);

					auto child_req_body = Json::StyledWriter().write(request_data);
					child_req.set(beast::http::field::content_length, child_req_body.size());
					child_req.body() = child_req_body;

					// Send the HTTP request to the remote host
					beast::http::write(stream, child_req);

					// This buffer is used for reading and must be persisted
					boost::beast::flat_buffer buffer;

					// Declare a container to hold the response
					beast::http::response<beast::http::vector_body<char>> child_res;

					// Receive the HTTP response
					beast::http::read(stream, buffer, child_res);

					// Gracefully close the stream
					boost::system::error_code ec;
					stream.shutdown(ec);

					// 
					{
						const auto& body = child_res.body();
						if (!Json::Reader().parse(body.data(), body.data() + body.size(), resp, false))
							throw std::runtime_error("invalid JSON data.");
					}
				}
				catch (std::exception& ex)
				{
					resp = Json::Value{};
					resp["succeeded"] = false;
					resp["message"] = ex.what();
					return;
				}
			});
		}

		ChildServerAdminServices::~ChildServerAdminServices() = default;

		void ChildServerAdminServices::add_child_server(const std::string& name, const std::string& security_token, const asio::ip::address& host, int port)
		{
			auto& child = impl_->child_servers[name];
			child.security_token = security_token;
			child.host = host;
			child.port = port;
		}
	}
}
