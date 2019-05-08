#include "stdafx.h"
#include <iostream>

#include "CryptoPP/sha3.h"

#include "CommonClientServices.h"
#include "logger/Logger.h"
#include "protocol/GlobalFunctions.h"
#include "db/Files_Model.h"
#include "db/Releases_Model.h"
#include "db/File_Blocks_Model.h"
#include "protocol/StringUtils.h"
#include <boost/serialization/vector.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/array.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem.hpp>
#include <memory>

namespace mserver {
	namespace service {
		using namespace soci;
		namespace filesystem = boost::filesystem;
		using boost::format;

		CommonClientServices::CommonClientServices(net::MessageServer& server, db::Database& database)
		{
			server.register_handler(protocol::MessageType::Ping, [](protocol::Message& msg, net::NetSession&)
			{
				protocol::MessageWriter writer(msg);
				return protocol::MessageType::Ping;
			});

			server.register_handler(
				protocol::MessageType::GetLatestReleaseReq, 
				[database](protocol::Message& msg, net::NetSession& net_session) 
			{
				auto dbSession = database.open();
				auto res = db::Releases_Model::get_latest_release(dbSession);

				protocol::MessageWriter writer(msg);
				writer & res.id;
				
				std::array<char, protocol::MaxStringLen> strBuf;
				strBuf = protocol::to_array(res.name);
				writer & strBuf;

				writer & res.file_count;

				return protocol::MessageType::GetLatestReleaseResponse;
			});

			server.register_handler(
				protocol::MessageType::GetFileIDsInReleaseReq,
				[database](protocol::Message& msg, net::NetSession& net_session)
			{
				protocol::IDType release_id = 0;
				protocol::IndexType file_index_start = 0;
				protocol::IndexType file_count = 0;

				protocol::MessageReader reader(msg);
				reader & release_id;
				reader & file_index_start;
				reader & file_count;

				auto dbSession = database.open();
				std::vector<protocol::IDType> fileIDs = db::Files_Model::get_file_ids_by_release_id(
					dbSession, release_id, file_index_start, file_count);

				protocol::MessageWriter writer(msg);
				writer & fileIDs;


				return protocol::MessageType::GetFileIDsInReleaseResponse;
			});

			server.register_handler(
				protocol::MessageType::GetFileInfoReq,
				[database](protocol::Message& msg, net::NetSession& net_session)
			{

				protocol::IDType file_id = 0;
				protocol::MessageReader reader(msg);
				reader & file_id;

				auto dbSession = database.open();
				auto file_model = db::Files_Model::find_by_id(dbSession, file_id);

				protocol::MessageWriter writer(msg);
				
				writer & file_model.file_length;
				writer & file_model.hash;
				
				std::array<char, protocol::MaxStringLen> str_buf;
				str_buf = protocol::to_array(file_model.logical_path);
				writer & str_buf;

				return protocol::MessageType::GetFileInfoResponse;
			});

			server.register_handler(
				protocol::MessageType::GetFileBlockInfoReq,
				[database](protocol::Message& msg, net::NetSession& net_session)
			{

				protocol::IDType file_id = 0;
				protocol::IndexType block_index_start = 0;
				protocol::IndexType block_count = 0;

				protocol::MessageReader reader(msg);
				reader & file_id;
				reader & block_index_start;
				reader & block_count;

				auto dbSession = database.open();
				std::vector<db::File_Blocks_Model> blocks;
				blocks = db::File_Blocks_Model::get_file_blocks_by_file_id(
					dbSession, file_id, block_index_start, block_count);

				protocol::MessageWriter writer(msg);
				protocol::IndexType size = blocks.size();
				for (protocol::IndexType i = 0; i < size; i++)
				{
					writer & blocks[i].hash;
				}

				return protocol::MessageType::GetFileBlockInfoResponse;
			});

			server.register_handler(
				protocol::MessageType::GetFileDataReq,
				[database](protocol::Message& msg, net::NetSession& net_session)
			{
				protocol::IDType file_id;
				protocol::IndexType block_index;
				protocol::MessageReader reader(msg);
				reader & file_id;
				reader & block_index;

				filesystem::ifstream *if_cur_file;
				if_cur_file = &net_session.get<filesystem::ifstream>("cur_file_stream");

				if (net_session.get<protocol::IDType>("cur_file_id") != file_id)
					if_cur_file->close();
				
				if (net_session.get<protocol::IDType>("cur_file_id") != file_id
					|| !if_cur_file->good())
				{
					//open file stream
					auto dbSession = database.open();
					std::string filepath = db::Files_Model::find_by_id(dbSession, file_id).physical_path;
					if_cur_file->open(filepath, std::ios_base::in | std::ios_base::binary);
					if (!if_cur_file->good())
						throw std::runtime_error("file reading failed");
				}

				net_session.get<protocol::IDType>("cur_file_id") = file_id;

				//send response message
				if_cur_file->seekg(block_index * protocol::BlockSize, std::ios::beg);
				protocol::FileBlock block;
				if_cur_file->read((char*)block.data(), protocol::BlockSize);
				std::uint32_t read_cnt = (std::uint32_t)if_cur_file->gcount();


				protocol::MessageWriter writer(msg);
				writer.save_binary((char*)block.data(), read_cnt);

				return protocol::MessageType::GetFileDataResponse;
			});
		}

	}
}
