#pragma once

#include <functional>
#include <boost/beast/http/message.hpp>
#include <boost/beast/http/vector_body.hpp>
#include <boost/beast/core/string.hpp>
#include <boost/beast/http/status.hpp>
#include <boost/beast/http/field.hpp>
#include <boost/beast/http/verb.hpp>

namespace mserver {
	namespace http {

		using Request = boost::beast::http::request<boost::beast::http::vector_body<char>>;
		using Response = boost::beast::http::response<boost::beast::http::vector_body<char>>;
		using Status = boost::beast::http::status;
		using Field = boost::beast::http::field;
		using Verb = boost::beast::http::verb;

		struct ResponseSender
		{
			virtual void operator()(Response&&) = 0;
		};

		using RequestHandler = std::function<void(Request&& req, Response& resp)>;
	}
}
