#include "stdafx.h"

#include "LevelServices.h"
#include "LevelUploadJob.h"
#include "MasterServerConfig.h"
#include "json/value.h"
#include "net/MessageServer.h"
#include "protocol/StringUtils.h"
#include "protocol/FileTransferCommands.h"
#include "protocol/base16.h"

#include "db/Level.h"
#include "db/LevelFileBlock.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <stdexcept>
#include <sstream>

namespace mserver {
	namespace service {
		namespace fs = boost::filesystem;

		struct LevelService::Impl
		{
			const MasterServerConfig& config_;
			db::Database& db_;
			http::Handler& http_handler_;
			job::Manager& jobManager_;
			
			Impl(
				const MasterServerConfig& config, 
				db::Database& db, 
				http::Handler& handler, 
				job::Manager& jobManager, 
				net::MessageServer& msg_server)
				: config_(config)
				, db_(db)
				, http_handler_(handler)
				, jobManager_(jobManager)
			{
				http_handler_.register_json_method("/level/upload", [this](Json::Value&& req, Json::Value& resp)
				{
					auto level_file = fs::path{ config_.levels_dir } / req["level"].asString();
					if (!fs::exists(level_file))
					{
						resp["succeeded"] = false;
						resp["message"] = "level file was not found.";
						return;
					}

					resp["job_id"] = jobManager_.start(std::make_shared<LevelUploadJob>(db_, level_file));

					resp["succeeded"] = true;
				});

				http_handler_.register_json_method("/level/uploaded_list", [this](Json::Value&& req, Json::Value& resp) 
				{
					auto conn = db_.open();
					{
						soci::transaction trans{ conn.get_session() };
						auto levels = db::Level::get_all_detail(conn);
						trans.commit();

						auto& levels_json = resp["level"];
						levels_json = Json::ValueType::arrayValue;
						for (const auto& level : levels)
						{
							auto& level_json = levels_json[levels_json.size()];
							level_json["id"] = level.id;
							level_json["name"] = level.name;
							level_json["file_length"] = level.file_length;

							std::ostringstream oss;
							for (const auto& one_byte : level.hash)
							{
								oss << std::uppercase << std::hex << one_byte;
							}
							level_json["hash"] = protocol::to_base16(oss.str().data(), oss.str().length());
						}
					}
					resp["succeeded"] = true;
				});

				msg_server.register_handler(
					protocol::MessageType::GetLevelInfoReq,
					[db](protocol::Message& msg, net::NetSession& net_session)
				{

					protocol::NetString<20> level_name;
					protocol::MessageReader reader(msg);
					reader & level_name;

					auto dbSession = db.open();
					auto level = db::Level::find_by_name(dbSession, level_name);

					if (level)
					{
						protocol::MessageWriter writer(msg);
						protocol::LevelInfo li;
						li.id = level.get().id;
						li.file_length = level.get().file_length;
						li.name = level.get().name;
						li.hash = level.get().hash;

						writer & li;
					}
					else
						throw std::runtime_error("level file name does not exist.");


					return protocol::MessageType::GetLevelInfoResp;
				});

				msg_server.register_handler(
					protocol::MessageType::GetLevelBlockInfoReq,
					[db](protocol::Message& msg, net::NetSession& net_session)
				{

					protocol::IDType file_id = 0;
					protocol::IndexType block_index_start = 0;
					protocol::IndexType block_count = 0;

					protocol::MessageReader reader(msg);
					reader & file_id;
					reader & block_index_start;
					reader & block_count;

					auto dbSession = db.open();
					std::vector<db::Level_File_Block> blocks;
					blocks = db::Level_File_Block::get_file_blocks_by_file_id(
						dbSession, file_id, block_index_start, block_count);

					protocol::MessageWriter writer(msg);
					protocol::IndexType size = blocks.size();
					for (protocol::IndexType i = 0; i < size; i++)
					{
						writer & blocks[i].hash;
					}

					return protocol::MessageType::GetLevelBlockInfoResp;
				});

				msg_server.register_handler(
					protocol::MessageType::GetLevelDataReq,
					[config, db](protocol::Message& msg, net::NetSession& net_session)
				{
					protocol::IDType file_id;
					protocol::IndexType block_index;
					protocol::MessageReader reader(msg);
					reader & file_id;
					reader & block_index;

					fs::ifstream *if_cur_file;
					if_cur_file = &net_session.get<fs::ifstream>("cur_file_stream");

					if (net_session.get<protocol::IDType>("cur_file_id") != file_id)
						if_cur_file->close();

					if (net_session.get<protocol::IDType>("cur_file_id") != file_id
						|| !if_cur_file->good())
					{
						//open file stream
						auto dbSession = db.open();

						auto level = db::Level::find_by_id(dbSession, file_id);
						if (level)
						{
							auto filepath = fs::path(config.levels_dir) / level.get().name;
							if_cur_file->close();
							if_cur_file->open(filepath, std::ios_base::in | std::ios_base::binary);
							if (!if_cur_file->good())
								throw std::runtime_error("file reading failed");
						}
						else
							throw std::runtime_error(
								(boost::format{ "file id does not exist. file id: %||" } % file_id).str());
					}

					net_session.get<protocol::IDType>("cur_file_id") = file_id;

					//send response message
					if_cur_file->seekg(block_index * protocol::BlockSize, std::ios::beg);
					protocol::FileBlock block;
					if_cur_file->read((char*)block.data(), protocol::BlockSize);
					std::uint32_t read_cnt = (std::uint32_t)if_cur_file->gcount();


					protocol::MessageWriter writer(msg);
					writer.save_binary((char*)block.data(), read_cnt);

					return protocol::MessageType::GetLevelDataResp;
				});
			}
		};

		LevelService::LevelService(
			MasterServerConfig& cfg, 
			db::Database& db, 
			http::Handler& handler, 
			job::Manager& jobManager,
			net::MessageServer& msg_server)
			: impl_(std::make_unique<LevelService::Impl>(cfg, db, handler, jobManager, msg_server))
		{
		}

		LevelService::~LevelService() = default;
	}
}