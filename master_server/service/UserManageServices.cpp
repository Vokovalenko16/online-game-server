#include "stdafx.h"
#include "UserManageServices.h"
#include "db/User.h"
#include "db/Member.h"
#include "soci/transaction.h"
#include "json/value.h"
#include "protocol/Message.h"
#include "protocol/LoginCommands.h"
#include "logger/Logger.h"
#include "CryptoPP/osrng.h"
#include "CryptoPP/sha.h"
#include "CryptoPP/filters.h"
#include "CryptoPP/base64.h"
#include <mutex>
#include <sstream>
#include <boost/optional.hpp>

namespace mserver {
	namespace service {

		struct UserManageServices::Impl
		{
			db::Database db;
			std::mutex mutex;
			CryptoPP::AutoSeededRandomPool random;
			mserver::http::GameCenterConnection* center_conn_;
		};

		UserManageServices::UserManageServices(db::Database& database, http::Handler& http_handler, net::MessageServer& msg_server, http::GameCenterConnection& center_conn_ )
			: impl_(std::make_unique<Impl>())
		{
			impl_->db = database;
			impl_->center_conn_ = std::move(&center_conn_);
			http_handler.register_json_method("/user/list", [this](Json::Value&& req, Json::Value& resp)
			{
				std::vector<std::int64_t> id_list;
				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					id_list = db::User::get_all_id(conn);

					tran.commit();
				}

				resp["succeeded"] = true;

				auto& users_json = resp["user_ids"];
				users_json = Json::ValueType::arrayValue;
				for (auto id : id_list)
				{
					users_json.append(id);
				}
			});

			http_handler.register_json_method("/user/info", [this](Json::Value&& req, Json::Value& resp)
			{
				auto id = req["id"].asInt64();

				db::User user;
				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					user = db::User::get(conn, id);

					tran.commit();
				}

				resp["succeeded"] = true;
				auto& user_json = resp["user"];
				user_json["id"] = user.id;
				user_json["login_name"] = user.login_name;
				user_json["active"] = user.active ? true : false;
			});
			
			http_handler.register_json_method("/user/create", [this](Json::Value&& req, Json::Value& resp)
			{
				db::User user;
				user.login_name = req["login_name"].asString();
				user.active = req["active"].asBool();

				std::int64_t id;

				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					id = db::User::insert(conn, user);

					tran.commit();
				}

				resp["succeeded"] = true;
				resp["id"] = id;
			});

			http_handler.register_json_method("/user/modify", [this](Json::Value&& req, Json::Value& resp)
			{
				db::User user;
				user.id = req["id"].asInt64();

				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					user = db::User::get(conn, user.id);
					{
						const auto& login_name = req["login_name"];
						if (!login_name.isNull())
							user.login_name = login_name.asString();

						const auto& active = req["active"];
						if (!active.isNull())
							user.active = active.asBool();
					}
					db::User::update(conn, user);

					tran.commit();
				}

				resp["succeeded"] = true;
			});

			http_handler.register_json_method("/user/set_password_hash", [this](Json::Value&& req, Json::Value& resp)
			{
				auto id = req["id"].asInt64();
				auto hash_base64 = req["password_hash"].asString();

				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					db::User::set_password_hash(conn, id, hash_base64);

					tran.commit();
				}

				resp["succeeded"] = true;
			});

			http_handler.register_json_method("/user/get_password_hash", [this](Json::Value&& req, Json::Value& resp)
			{
				auto id = req["id"].asInt64();
				std::string hash_b64;

				{
					auto conn = impl_->db.open();
					soci::transaction tran{ conn.get_session() };

					hash_b64 = db::User::get_password_hash(conn, id);

					tran.commit();
				}

				resp["succeeded"] = true;
				resp["password_hash"] = hash_b64;
			});

			msg_server.register_typed_handler([this](
				protocol::Body<protocol::MessageType::GetLoginChallengeReq>&& req,
				net::NetSession& session)
			{
				auto& challenge = session.get<std::string>("login_challenge");
				if (challenge.empty())
				{
					protocol::HashValType nonce;
					{
						std::unique_lock<std::mutex> lock(impl_->mutex);
						impl_->random.GenerateBlock(nonce.data(), nonce.size());
					}

					std::ostringstream ss;
					for (const auto& x : nonce)
					{
						ss.fill('0');
						ss.width(2);
						ss << std::uppercase << std::hex << (int)x;
					}

					challenge = ss.str();

					log::write(
						log::Level::Debug,
						"generated challenge %|| for client %||",
						challenge, session.client_id());
				}
				else
				{
					log::write(
						log::Level::Debug,
						"reusing challenge %|| for client %||",
						challenge, session.client_id());
				}

				protocol::Body<protocol::MessageType::GetLoginChallengeResp> resp;
				resp.challenge_nonce = challenge;
				return resp;
			});

			msg_server.register_typed_handler([this](
				protocol::Body<protocol::MessageType::LoginReq>&& req,
				net::NetSession& session)
			{
				protocol::Body<protocol::MessageType::LoginResp> resp;
				auto& challenge = session.get<std::string>("login_challenge");
				if (challenge.size() != protocol::HashValLen * 2)
					throw std::runtime_error("client tried login without first retrieving challenge.");

				bool succeeded = false;
				if (impl_->center_conn_->valid())
				{
					Json::Value center_req;
					center_req["login_name"] = req.login_name.value();
					center_req["challenge"] = session.get<std::string>("login_challenge");
					center_req["response"] = req.login_response.value();
					auto center_res = impl_->center_conn_->call_game_center(center_req, "/services/verify_login");

					if (center_res["succeeded"] == true)
					{
						succeeded = true;
						log::write(log::Level::Info, "client %|| has logged in with id %||",
							session.client_id(),
							req.login_name.value());
					}
					else
					{
						log::write(log::Level::Info, "client %|| was denied by game center from logging in with id %||",
							session.client_id(),
							req.login_name.value());
					}
				}
				else
				{
					std::string password_hash;
					{
						auto db_session = impl_->db.open();
						soci::transaction tran{ db_session.get_session() };

						auto user = db::User::find_by_login_name(db_session, req.login_name);
						if (!user || !user->active)
						{
							resp.result = protocol::LoginVerifyResult::NoSuchUser;
							return resp;
						}

						password_hash = db::User::get_password_hash(db_session, user->id);

						tran.commit();
					}

					//if (succeeded)
					{
						CryptoPP::SHA1 hasher;
						hasher.Update((const byte*)challenge.data(), challenge.size());
						hasher.Update((const byte*)password_hash.data(), password_hash.size());

						CryptoPP::SecBlock<byte> correct_resp(hasher.DigestSize());
						hasher.Final((byte*)correct_resp.data());

						std::ostringstream ss;
						for (auto x : correct_resp)
						{
							ss.fill('0');
							ss.width(2);
							ss << std::hex << std::uppercase << (int)x;
						}

						if (ss.str() == req.login_response.value())
							succeeded = true;
					}

					
				}
				
				if (succeeded)
				{
					session.erase("login_challenge");
					session.get<std::string>("user_id") = req.login_name;
				}
				resp.result = succeeded
					? protocol::LoginVerifyResult::Succeeded
					: protocol::LoginVerifyResult::PasswordIncorrect;
				
				return resp;
			});

			msg_server.register_typed_handler([this](
				protocol::Body<protocol::MessageType::OnNotifyLoginNameReq>&& req,
				net::NetSession& session)
			{
				auto conn = impl_->db.open();
				auto result = mserver::db::Member::find_by_login_name(conn, req.login_name);

				protocol::Body<protocol::MessageType::OnNotifyLoginNameResp> resp;
				if (result == boost::none)
				{
					mserver::db::Member member;
					member.login_name_ = req.login_name;
					resp.user_id = mserver::db::Member::insert(conn, member);

					log::write(log::Level::Info, "created user '%||' in member table",
						req.login_name.value());
				}
				else
				{
					resp.isSucceed = true;
					resp.user_id = result.value();
				}
				return resp;				
			});
		}

		UserManageServices::~UserManageServices() = default;
	}
}
