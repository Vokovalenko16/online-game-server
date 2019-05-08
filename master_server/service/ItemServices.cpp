#include "stdafx.h"
#include "ItemServices.h"
#include "game/Game.h"
#include "game/GameManager.h"
#include "protocol/Message.h"
#include "protocol/GameCommands.h"
#include "protocol/Types.h"
#include "db/Equipment.h"
#include "CryptoPP/rsa.h"
#include "CryptoPP/osrng.h"
#include "CryptoPP/sha.h"
#include "CryptoPP/filters.h"
#include "CryptoPP/base64.h"
#include "json/value.h"
#include "json/reader.h"
#include "db/Equip_list.h"
#include "db/Equip_history.h"
#include "logger/Logger.h"

#include <boost/asio/basic_waitable_timer.hpp>

#include <fstream>
#include <memory>
#include <utility>

using R = CryptoPP::RSASS<CryptoPP::PKCS1v15, CryptoPP::SHA1>;

namespace mserver {
	namespace service {
		using Timer = boost::asio::basic_waitable_timer<std::chrono::steady_clock>;

		struct ItemServices::Impl
		{
			std::string orienteering_priv_key;
			std::string game_center_pub_key;
			mserver::http::GameCenterConnection* center_conn_;
			bool is_key_file_loaded;
			boost::asio::io_service& io_;
			db::Database& db_;
			Timer retry_timer_;
			
			using M = protocol::MessageType;

			~Impl()
			{
				retry_timer_.cancel();
			}

			Impl(boost::asio::io_service& io, db::Database& db, http::Handler& http_handler, net::MessageServer& server)
				: io_(io)
				, db_(db)
				, retry_timer_(io)
			{
				is_key_file_loaded = load_key_files();

				server.register_typed_handler(
					[db](protocol::Body<protocol::MessageType::GetItemInfoReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::GetItemInfoResp> resp;
					auto dbSession = db.open();
					resp.equips = db::Equipment::find_by_user(dbSession, req.user_id);
					return resp;
				});

				server.register_typed_handler(
					[db](protocol::Body<protocol::MessageType::GetItemInfoByCategoryReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::GetItemInfoByCategoryResp> resp;
					auto dbSession = db.open();
					resp.equips = db::Equipment::find_by_user_and_category(dbSession, req.user_id, req.category_id);
					return resp;
				});

				server.register_typed_handler([db](protocol::Body<protocol::MessageType::DecreaseItemCntReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::DecreaseItemCntResp> resp;
					auto dbSession = db.open();
					
					soci::transaction trans(dbSession.get_session());
					
					resp.isSucceed = db::Equipment::decrease_item_cnt(dbSession, req.user_id, req.itemName);

					trans.commit();

					return resp;
				});

				server.register_typed_handler([db](protocol::Body <protocol::MessageType::GetItemListReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::GetItemListResp> resp;
					auto dbSession = db.open();

					soci::transaction trans(dbSession.get_session());

					resp.equip_kinds = db::Equip_list::get_all_detail(dbSession);

					trans.commit();

					return resp;
				});

				server.register_typed_handler(
					[db, this](protocol::Body<protocol::MessageType::PurchaseItemReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::PurchaseItemResp> resp;
					auto dbSession = db.open();

					if (!db::Equip_list::is_having_stock(dbSession, req.equip_name))
					{
						resp.isSucceed = false;
						resp.message = "o_out_of_stock";
						return resp;
					}
					std::uint32_t price = db::Equipment::get_equip_price(dbSession, req.equip_name);
					std::uint32_t amount = price * req.equip_cnt;
					
					
					Json::Value draft_json;
					db::Equip_history row;

					row.transaction_id= pick_trans_id(dbSession);
					row.title = "orienteering";
					row.user = req.user_id;
					row.item = req.equip_name;
					row.item_change = req.equip_cnt;
					row.star_change = amount;
					row.status = true;
					row.error_desc = "";

					draft_json["id"] = row.transaction_id;
					draft_json["title"] = row.title;
					draft_json["user"] = row.user;
					draft_json["item"] = row.item;
					draft_json["item_change"] = row.item_change;
					draft_json["star_change"] = row.star_change;
					draft_json["status"] = (bool)row.status;
					draft_json["error_desc"] = row.error_desc;

					auto json_trans = draft_json.toStyledString();
						
					row.transaction = json_trans;
					row.signature = sign(json_trans);
					row.pending = 0;

					{
						soci::transaction trans{ dbSession.get_session() };
						db::Equip_history::insert(
							dbSession, 
							row
							);
						trans.commit();
					}
					return send_purchase_item_req_and_dispose_resp(dbSession, row);
				});

				server.register_typed_handler([this](protocol::Body<protocol::MessageType::GetBalanceReq>&& req, net::NetSession& session) 
				{
					protocol::Body<protocol::MessageType::GetBalanceResp> resp;
					Json::Value req_json;

					req_json["user"] = req.user_id.value();
					auto resp_json = center_conn_->call_game_center(req_json, "/services/get_balance");
					resp.balance = resp_json["balance"].asUInt();

					return resp;
				});
				
				http_handler.register_json_method("/settle/total", [this](Json::Value&& req, Json::Value& resp)
				{
					auto conn = db_.open();
					{
						soci::transaction trans{ conn.get_session() };
						auto settles = db::Equip_history::get_settles(conn, req["start_date"].asString(), req["end_date"].asString());
						trans.commit();

						auto& rows_json = resp["rows"];
						rows_json = Json::ValueType::arrayValue;
						std::uint32_t total = 0;
						for (const auto& row : settles)
						{
							auto& row_json = rows_json[rows_json.size()];
							row_json["user"] = row.user;
							row_json["money"] = row.money;
							total += row.money;
						}
						resp["total"] = total;
					}
					resp["succeeded"] = true;
				});

				http_handler.register_json_method("/settle/by_user", [this](Json::Value&& req, Json::Value& resp) {
					auto conn = db_.open();
					{
						soci::transaction trans{ conn.get_session() };
						auto settles = db::Equip_history::get_settles_by_user(
							conn, req["user"].asString(), req["start_date"].asString(), req["end_date"].asString());
						trans.commit();

						auto& rows_json = resp["rows"];
						rows_json = Json::ValueType::arrayValue;
						std::uint32_t total = 0;
						for (const auto& row : settles)
						{
							auto& row_json = rows_json[rows_json.size()];
							row_json["date_time"] = row.date_time;
							row_json["item"] = row.item;
							row_json["item_change"] = row.item_change;
							row_json["star_change"] = row.star_change;
							total += row.star_change;
						}
						resp["total"] = total;
					}
					resp["succeeded"] = true;
				});
			}

			std::string pick_trans_id(db::DbSession& dbSession)
			{
				std::string trans_id;
				do
				{
					CryptoPP::AutoSeededRandomPool random;
					std::mutex mutex;
					std::array<std::uint8_t, 10> nonce;
					{
						random.GenerateBlock(nonce.data(), nonce.size());
					}
					std::ostringstream ss;
					ss << "g";
					for (const auto& x : nonce)
					{
						ss.fill('0');
						ss.width(2);
						ss << std::uppercase << std::hex << (int)x;
					}
					trans_id = ss.str();
					if (db::Equipment::check_duplicated_id(dbSession, trans_id) == false)
						trans_id = "";
				} while (trans_id.empty());
				
				return trans_id;
			}

			protocol::Body<protocol::MessageType::PurchaseItemResp> send_purchase_item_req_and_dispose_resp(
				db::DbSession& dbSession,
				const db::Equip_history& row
			)
			{
				protocol::Body<protocol::MessageType::PurchaseItemResp> resp;
				retry_timer_.expires_after(std::chrono::seconds(5));
				retry_timer_.async_wait([this](const boost::system::error_code& ec) {
					if (ec)
						return;	
					try
					{
						auto dbSession = db_.open();
						auto pending_trans_res = db::Equip_history::not_finished_transaction(dbSession);
						if (pending_trans_res == boost::none)
						{
							return;
						}
						auto pending_trans = pending_trans_res.get();
						send_purchase_item_req_and_dispose_resp(
							dbSession,
							pending_trans
						);
					}
					catch (const std::exception& ex)
					{
						log::write(log::Level::Error, "error occurred while retrying transaction: %s", ex.what());
					}
				});
				try
				{
					Json::Value http_req;
					http_req["transaction"] = row.transaction;
					http_req["signature"] = row.signature;

					Json::Value center_resp = center_conn_->call_game_center(http_req, "/services/purchase_item");

					if (!verify(center_resp["data"].asString(), center_resp["signature"].asString()))
						throw std::runtime_error("Failed to verify.");

					std::string data_str = center_resp["data"].asString();
					Json::Value data_json;

					if (!Json::Reader().parse(data_str.data(), data_str.data() + data_str.size(), data_json, false))
						throw std::runtime_error("invalid JSON data.");
					{
						soci::transaction trans(dbSession.get_session());
						db::Equip_history::insert(
							dbSession,
							data_json["id"].asString(),
							data_json["title"].asString(),
							data_json["user"].asString(),
							data_json["item"].asString(),
							data_json["item_change"].asInt(),
							data_json["star_change"].asInt(),
							data_json["status"].asUInt(),
							data_json["error_desc"].asString(),
							center_resp["data"].asString(),
							center_resp["signature"].asString(),
							1);
						db::Equip_history::update_pending(dbSession, row.transaction_id, 1);

						if (data_json["status"] == true)
						{
							resp.isSucceed = true;
							db::Equipment::insert(dbSession, row.user, row.item, row.item_change);
							db::Equip_list::update_stock(dbSession, row.item, row.item_change);
						}
						else
						{
							resp.isSucceed = false;
							resp.message = data_json["error_desc"].asString();
						}
						trans.commit();
					}
				}
				catch (std::exception& ex)
				{
					std::string err_code = ex.what();
					resp.isSucceed = false;
					resp.message = "###" + err_code;
				}
				return resp;
			}

			bool load_key_files()
			{
				try
				{
					{
						orienteering_priv_key = "";
						std::fstream keyfile;
						keyfile.open("Orienteering_priv.key", std::ios::in);
						if (!keyfile.good())
							throw std::runtime_error("unable to open Orienteering_priv.key file.\n");
						std::string line;
						std::getline(keyfile, line);
						if (line != "-----BEGIN RSA PRIVATE KEY-----")
							return false;

						while (!keyfile.eof())
						{
							std::getline(keyfile, line);
							if (line == "-----END RSA PRIVATE KEY-----")
								break;

							orienteering_priv_key += line + "\n";
						}
					}

					{
						game_center_pub_key = "";
						std::fstream keyfile;
						keyfile.open("Game_Center_pub.key", std::ios::in);
						if (!keyfile.good())
							throw std::runtime_error("unable to open Orienteering_priv.key file.\n");
						std::string line;
						std::getline(keyfile, line);
						if (line != "-----BEGIN PUBLIC KEY-----")
							return false;

						while (!keyfile.eof())
						{
							std::getline(keyfile, line);
							if (line == "-----END PUBLIC KEY-----")
								break;

							game_center_pub_key += line + "\n";
						}
					}
				}
				catch (const std::exception&)
				{
					return false;
				}
				return true;
			}

			std::string sign(const std::string& data_to_sign)
			{
				// Load key material
				R::Signer signer;
				{
					CryptoPP::StringSource b64{ orienteering_priv_key, true, new CryptoPP::Base64Decoder };
					signer.AccessKey().BERDecodePrivateKey(b64, false, 0);
				}

				// Generate signature
				CryptoPP::AutoSeededRandomPool random;
				std::vector<byte> signature(signer.MaxSignatureLength());
				auto sig_len = signer.SignMessage(random, (const byte*)data_to_sign.data(), data_to_sign.size(), signature.data());

				// Encode as base64
				std::string b64_buf;
				CryptoPP::Base64Encoder b64{ new CryptoPP::StringSink{ b64_buf } };
				b64.Put(signature.data(), sig_len);
				b64.MessageEnd();
				return b64_buf;
			}

			bool verify(const std::string& data_to_verify, const std::string& signature)
			{
				// Load key material.
				R::Verifier verifier;
				{
					CryptoPP::StringSource b64{ game_center_pub_key, true, new CryptoPP::Base64Decoder };
					verifier.AccessKey().Load(b64);
				}

				// Verify signature
				bool verify_result;
				{
					CryptoPP::StringSource b64{
						std::string(signature.begin(), signature.end()),
						true, new CryptoPP::Base64Decoder };

					std::vector<byte> signature(verifier.MaxSignatureLength());
					auto sig_len = b64.Get(signature.data(), signature.size());

					verify_result = verifier.VerifyMessage((const byte*)data_to_verify.data(), data_to_verify.size(), signature.data(), sig_len);
				}

				return verify_result;
			}
		};

		ItemServices::ItemServices(boost::asio::io_service& io, db::Database& db, http::Handler& http_handler, net::MessageServer& server, http::GameCenterConnection& center_conn)
		{
			impl_ = std::make_unique<Impl>(io, db, http_handler, server);
			impl_->center_conn_ = std::move(&center_conn);
		}

		ItemServices::~ItemServices() = default;
	}
}