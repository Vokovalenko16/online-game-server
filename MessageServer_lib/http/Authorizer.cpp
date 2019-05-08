#include "stdafx.h"
#include "Authorizer.h"
#include "CryptoPP/base64.h"
#include "CryptoPP/sha.h"
#include "protocol/base16.h"

#include <cstdint>
#include <boost/beast/version.hpp>
#include <boost/format.hpp>

namespace mserver {
	namespace http {

		namespace beast = boost::beast;

		struct BasicAuthorizer : Authorizer
		{
			std::string user_id;
			std::string pwd_hash;

			bool check(const Request& req)
			{
				auto auth_i = req.find(Field::authorization);
				if (auth_i == req.end())
					return false;

				auto auth_field = auth_i->value();
				while (!auth_field.empty() && auth_field.front() == ' ')
					auth_field.remove_prefix(1);

				if (!auth_field.starts_with("Basic"))
					return false;
				auth_field.remove_prefix(5);

				while (!auth_field.empty() && auth_field.front() == ' ')
					auth_field.remove_prefix(1);

				CryptoPP::StringSource b64{
					auth_field.to_string(),
					true, new CryptoPP::Base64Decoder };

				std::string decoded_auth_field;
				CryptoPP::StringSink decoded_auth{ decoded_auth_field };

				b64.CopyTo(decoded_auth);

				auto colon_index = decoded_auth_field.find(':');
				if (colon_index == decoded_auth_field.npos)
					return false;

				if (decoded_auth_field.substr(0, colon_index) != user_id)
					return false;

				CryptoPP::SHA256 hasher;
				std::array<std::uint8_t, CryptoPP::SHA256::DIGESTSIZE> hash;
				hasher.CalculateDigest((byte*)hash.data(), (const byte*)decoded_auth_field.data() + colon_index + 1, decoded_auth_field.size() - colon_index - 1);

				return protocol::from_base16<CryptoPP::SHA256::DIGESTSIZE>(pwd_hash) == hash;
			}

			virtual bool authorize(const Request& req, ResponseSender& send) override
			{
				if (!check(req))
				{
					auto host_i = req.find(Field::host);

					Response resp{ Status::unauthorized, req.version() };
					resp.set(Field::server, BOOST_BEAST_VERSION_STRING);
					resp.set(Field::content_type, "text/html");
					resp.set(Field::www_authenticate, (boost::format("Basic realm=\"%||\"")
						% (host_i == req.end() ? "" : host_i->value())).str());
					resp.keep_alive(req.keep_alive());

					std::string msg = "unauthorized";
					resp.body() = beast::http::vector_body<char>::value_type(msg.begin(), msg.end());
					resp.prepare_payload();

					send(std::move(resp));
					return false;
				}

				return true;
			}
		};

		std::shared_ptr<Authorizer> create_basic_authorizer(const std::string& user_id, const std::string& pwd_hash)
		{
			auto res = std::make_shared<BasicAuthorizer>();
			res->user_id = user_id;
			res->pwd_hash = pwd_hash;
			return res;
		}
	}
}
