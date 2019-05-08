#include "stdafx.h"
#include "http/Handler.h"
#include "http/Authorizer.h"
#include "logger/Logger.h"
#include "json/value.h"
#include "json/reader.h"
#include "json/writer.h"

#include <unordered_map>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

namespace mserver {
	namespace http {

		namespace beast = boost::beast;

		beast::http::vector_body<char>::value_type to_vector_body(boost::beast::string_view str)
		{
			return beast::http::vector_body<char>::value_type(str.begin(), str.end());
		}

		Response Handler::bad_request(const Request& req, boost::beast::string_view why)
		{
			Response res{ Status::bad_request, req.version() };
			res.set(Field::server, BOOST_BEAST_VERSION_STRING);
			res.set(Field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = to_vector_body(why);
			res.prepare_payload();
			return res;
		};

		// Returns a not found response
		Response Handler::not_found(const Request& req, boost::beast::string_view target)
		{
			Response res{ Status::not_found, req.version() };
			res.set(Field::server, BOOST_BEAST_VERSION_STRING);
			res.set(Field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = to_vector_body("The resource '" + target.to_string() + "' was not found.");
			res.prepare_payload();
			return res;
		};

		// Returns a server error response
		Response Handler::server_error(const Request& req, boost::beast::string_view what)
		{
			Response res{ Status::internal_server_error, req.version() };
			res.set(Field::server, BOOST_BEAST_VERSION_STRING);
			res.set(Field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = to_vector_body("An error occurred: '" + what.to_string() + "'");
			res.prepare_payload();
			return res;
		};

		Response Handler::unauthorized(const Request& req, boost::beast::string_view why)
		{
			Response res{ Status::unauthorized, req.version() };
			res.set(Field::server, BOOST_BEAST_VERSION_STRING);
			res.set(Field::content_type, "text/html");
			res.keep_alive(req.keep_alive());
			res.body() = to_vector_body(why);
			res.prepare_payload();
			return res;
		};


		struct Handler::Impl
		{
			std::unordered_map<std::string, RequestHandler> resources;
			std::string master_token;
			std::shared_ptr<Authorizer> authorizer;
		};

		Handler::Handler() = default;
		Handler::~Handler() = default;
		Handler::Handler(const Handler&) = default;
		Handler::Handler(Handler&&) = default;
		Handler& Handler::operator =(const Handler&) = default;
		Handler& Handler::operator =(Handler&&) = default;

		Handler Handler::create(const std::string& master_token, std::shared_ptr<Authorizer> authorizer)
		{
			Handler result;
			result.impl_ = std::make_shared<Impl>();
			result.impl_->master_token = master_token;
			result.impl_->authorizer = authorizer;
			return result;
		}

		void Handler::register_resource(const std::string& path, RequestHandler request_handler)
		{
			impl_->resources[path] = std::move(request_handler);
		}

		void Handler::handle_request(Request&& req, ResponseSender& send)
		{
			if (req.target().empty() ||
				req.target()[0] != '/' ||
				req.target().find("..") != boost::beast::string_view::npos)
				return send(bad_request(req, "Illegal request-target"));

			if (impl_->authorizer && !impl_->authorizer->authorize(req, send))
				return;

			if (!impl_->master_token.empty())
			{
				auto token_iter = req.find("Security-Token");
				if (token_iter == req.end() || token_iter->value() != impl_->master_token)
					return send(unauthorized(req, "Invalid security token"));
			}

			auto iter = impl_->resources.find(req.target().to_string());
			if (iter == impl_->resources.end())
				return send(not_found(req, req.target()));

			Response resp{ Status::ok, req.version() };
			try
			{
				resp.set(Field::server, BOOST_BEAST_VERSION_STRING);
				resp.set(Field::content_type, "text/plain");
				resp.set(Field::access_control_allow_origin, "*");
				resp.keep_alive(req.keep_alive());
				iter->second(std::move(req), resp);
				resp.prepare_payload();
			}
			catch (std::exception& ex)
			{
				log::write(log::Level::Error, "error in HTTP handler: %||", ex.what());
				resp = server_error(req, "Internal sever error");
			}

			send(std::move(resp));
		}

		void Handler::register_json_method(const std::string& path, std::function<void(Json::Value&&, Json::Value&)> method)
		{
			return register_resource(path, [this, path, method = std::move(method)](http::Request&& req, http::Response& resp)
			{
				Json::Value json_req;
				{
					const auto& body = req.body();
					log::write(log::Level::Debug, "got request '%||', body=%||",
						path, std::string(body.begin(), body.end()));
					if (!Json::Reader().parse(body.data(), body.data() + body.size(), json_req, false))
						return bad_request(req, "JSON parse error.", resp);
				}

				Json::Value json_resp;
				method(std::move(json_req), json_resp);

				resp.set(http::Field::content_type, "text/json");
				resp.body() = Json::StyledWriter().write(json_resp);

				log::write(log::Level::Debug, "sending response for '%||', body=%||",
					path, std::string(resp.body().begin(), resp.body().end()));
			});
		}
	}
}
