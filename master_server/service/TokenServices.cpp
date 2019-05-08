#include "stdafx.h"
#include "TokenServices.h"
#include "CryptoPP/osrng.h"
#include "CryptoPP/sha.h"
#include "protocol/TokenCommands.h"
#include "protocol/base16.h"
#include "db/Token.h"
#include "logger/Logger.h"
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace mserver {
	namespace service {

		using protocol::Body;
		using protocol::MessageType;
		using protocol::HashValType;
		using net::NetSession;

		const std::chrono::system_clock::duration TokenExpiryTime = std::chrono::hours(24);

		const std::uint8_t TokenSecret[] =
		{
			0x80, 0x0A, 0x5A, 0x78, 0x86, 0x90, 0x49, 0xCA,
			0xB1, 0xCF, 0xA6, 0x78, 0x40, 0xEB, 0xBF, 0x4D,
			0xFA, 0x7A, 0x4C, 0x67, 0xD6, 0x7C, 0x4B, 0x07,
			0x91, 0xAC, 0x58, 0x27, 0x01, 0x91, 0x02, 0x8B,
		};

		struct TokenServices::Impl
		{
			db::Database db_;
			std::mutex mutex_;
			CryptoPP::AutoSeededRandomPool random_;

			Impl(db::Database& database)
				: db_(database)
			{}
		};

		TokenServices::TokenServices(db::Database& database, net::MessageServer& msg_server)
		{
			impl_ = std::make_unique<Impl>(database);

			msg_server.register_typed_handler([this](Body<MessageType::TokenReq>&& req, NetSession& session)
			{
				const auto& user_id = session.get<std::string>("user_id");
				if (user_id.empty())
					throw std::runtime_error("Not logged in");

				std::array<byte, 32> random;
				{
					std::unique_lock<std::mutex> lock(impl_->mutex_);
					impl_->random_.GenerateBlock(random.data(), random.size());
				}

				CryptoPP::SHA256 hasher;
				hasher.Update((const byte*)TokenSecret, sizeof(TokenSecret));
				hasher.Update(random.data(), random.size());

				HashValType token_value;
				hasher.Final((byte*)token_value.data());

				{
					auto session = impl_->db_.open();
					soci::transaction tran(session.get_session());

					db::Token token;
					token.value = protocol::to_base16(token_value);
					token.user_id = user_id;
					token.set_expiry_time(std::chrono::system_clock::now() + TokenExpiryTime);

					db::Token::remove_by_user(session, token.user_id);
					db::Token::remove_by_value(session, token.value);
					db::Token::insert(session, token);

					tran.commit();
				}

				log::write(log::Level::Debug, "client %|| issued token for user '%||'",
					session.client_id(),
					user_id);

				Body<MessageType::TokenResp> resp;
				resp.token = token_value;
				return resp;
			});

			msg_server.register_typed_handler([this](Body<MessageType::VerifyTokenReq>&& req, NetSession& session)
			{
				boost::optional<db::Token> token;

				{
					auto session = impl_->db_.open();
					soci::transaction tran(session.get_session());

					token = db::Token::find_by_user(session, req.user_id);
				}

				if (!token || std::chrono::system_clock::now() > token->get_expiry_time())
				{
					log::write(log::Level::Warning, "user '%||' either did not requested a token or has expired",
						req.user_id.value());
					throw std::runtime_error("invalid token.");
				}

				// verify the token
				auto token_value = protocol::from_base16<protocol::HashValLen>(token->value);

				CryptoPP::SHA256 hasher;
				hasher.Update((const byte*)req.nonce.data(), req.nonce.size());
				hasher.Update((const byte*)token_value.data(), token_value.size());

				protocol::HashValType calculatedResp;
				hasher.Final((byte*)calculatedResp.data());

				if (calculatedResp != req.response)
				{
					log::write(log::Level::Warning, "token from client %|| for user '%||' is not correct",
						session.client_id(),
						req.user_id.value());
					throw std::runtime_error("invalid token.");
				}

				Body<MessageType::VerifyTokenResp> resp;
				resp.token = token_value;
				return resp;
			});
		}

		TokenServices::~TokenServices() = default;
	}
}
