#pragma once

#include "http/Handler.h"
#include <string>
#include <memory>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>

namespace mserver {
	namespace service {

		class ChildServerAdminServices
		{
		private:
			struct Impl;
			std::unique_ptr<Impl> impl_;

		public:
			ChildServerAdminServices(boost::asio::io_service& io, http::Handler& handler);
			~ChildServerAdminServices();

			void add_child_server(const std::string& name, const std::string& security_token,
				const boost::asio::ip::address& host, int port);
		};
	}
}
