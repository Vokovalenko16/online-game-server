#pragma once

#include <memory>
#include <string>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/beast/http.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <boost/asio/ip/address.hpp>

#include "net/Future.h"
#include "json/value.h"

namespace mserver {
	namespace http {
		namespace beast = boost::beast;

		class GameCenterConnection
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			GameCenterConnection();
			GameCenterConnection(boost::asio::io_service& io, const boost::asio::ip::address& address, int port, const std::string& target);
			~GameCenterConnection();

			GameCenterConnection(GameCenterConnection&&);
			GameCenterConnection& operator =(GameCenterConnection&&);
			bool valid() const;

			Json::Value call_game_center(const Json::Value& req_val, std::string uri);
		};
	}
}
