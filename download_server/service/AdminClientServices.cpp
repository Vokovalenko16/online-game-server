#include "stdafx.h"
#include "AdminClientServices.h"
#include "protocol/GlobalFunctions.h"
#include "core/Exception.h"
#include "logger/Logger.h"
#include "DirectoryLoadJob.h"

#include "db/Files_Model.h"
#include "db/File_Blocks_Model.h"
#include "db/Releases_Model.h"

#include "json/value.h"
#include "json/writer.h"
#include "json/reader.h"

#include <vector>
#include <string>
#include <memory>

#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/format.hpp>
#include <boost/beast/http/field.hpp>


namespace mserver {
	namespace service {
		
		using boost::format;
		namespace fs = boost::filesystem;

		AdminClientService::AdminClientService(http::Handler& server, db::Database& database, job::Manager& job_manager)
			: db_(database)
		{
			server.register_json_method("/release/add", [this, &job_manager](Json::Value&& req, Json::Value& resp)
			{
				auto id = req["id"].asInt();
				auto name = req["name"].asString();
				auto physical_path = req["physical_path"].asString();
				auto logical_path = req["logical_path"].asString();

				auto dbSession = db_.open();
				resp["succeed"] = true;
				if (db::Releases_Model::is_id_exist(dbSession, id))
				{
					resp["succeed"] = false;
					resp["message"] = boost::str(format("release id(%||) is already exists") % id);
					return;
				}

				auto ptr_dir_loader = std::make_shared<DirectoryLoadJob>(db_, id, name, physical_path, logical_path);
				resp["job_id"] = job_manager.start(ptr_dir_loader);
			});

			server.register_json_method("/release/set_active", [this, &job_manager](Json::Value&& req, Json::Value& resp) 
			{
				auto conn = db_.open();
				soci::transaction trans{ conn.get_session() };
				resp["succeeded"] = true;
				db::Releases_Model::set_active(conn, req["id"].asInt(), req["active"].asInt());
				trans.commit();
			});

			server.register_json_method("/release/list", [this, &job_manager](Json::Value&& req, Json::Value& resp) 
			{
				auto conn = db_.open();
				{
					soci::transaction trans{ conn.get_session() };
					auto releases = db::Releases_Model::get_all_detail(conn);
					trans.commit();

					resp["succeeded"] = true;
					auto& releases_json = resp["releases"];
					releases_json = Json::ValueType::arrayValue;

					for (const auto& release : releases)
					{
						auto& release_json = releases_json[releases_json.size()];
						release_json["id"] = release.id;
						release_json["name"] = release.name;
						release_json["active"] = release.active;
						release_json["file_count"] = release.file_count;
					}
				}
			});
		}
	}
}
