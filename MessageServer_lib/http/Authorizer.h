#pragma once

#include "Types.h"
#include <memory>
#include <string>

namespace mserver {
	namespace http {

		struct Authorizer
		{
			virtual ~Authorizer() {}
			virtual bool authorize(const Request&, ResponseSender& send) = 0;
		};

		std::shared_ptr<Authorizer> create_basic_authorizer(const std::string& user_id, const std::string& pwd_hash);
	}
}
