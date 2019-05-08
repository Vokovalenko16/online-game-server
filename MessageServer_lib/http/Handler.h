#pragma once

#include <memory>
#include <string>
#include "Types.h"

namespace Json {
	class Value;
}

namespace mserver {
	namespace http {

		struct Authorizer;

		class Handler
		{
		private:
			struct Impl;
			std::shared_ptr<Impl> impl_;

		public:
			Handler();
			~Handler();

			Handler(const Handler&);
			Handler(Handler&&);
			Handler& operator =(const Handler&);
			Handler& operator =(Handler&&);

			static Handler create(const std::string& master_token, std::shared_ptr<Authorizer> authorizer = nullptr);
			void register_resource(const std::string& path, RequestHandler request_handler);
			void handle_request(Request&& req, ResponseSender& send);

			static Response bad_request(const Request& req, boost::beast::string_view why);
			static Response not_found(const Request& req, boost::beast::string_view target);
			static Response server_error(const Request& req, boost::beast::string_view what);
			static Response unauthorized(const Request& req, boost::beast::string_view why);

			static void bad_request(const Request& req, boost::beast::string_view why, Response& resp)
			{
				resp = bad_request(req, why);
			}
			static void not_found(const Request& req, boost::beast::string_view target, Response& resp)
			{
				resp = not_found(req, target);
			}
			static void server_error(const Request& req, boost::beast::string_view what, Response& resp)
			{
				resp = server_error(req, what);
			}
			static void unauthorized(const Request& req, boost::beast::string_view what, Response& resp)
			{
				resp = unauthorized(req, what);
			}

			void register_json_method(const std::string& path, std::function<void(Json::Value&& req, Json::Value& resp)> method);
		};
	}
}
