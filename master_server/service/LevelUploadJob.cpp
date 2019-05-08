#include "stdafx.h"

#include "LevelUploadJob.h"
#include "db/Level.h"
#include "db/LevelFileBlock.h"
#include "core/Exception.h"

#include "CryptoPP/sha3.h"

#include <boost/filesystem.hpp>

namespace mserver {
	namespace service {
		namespace fs = boost::filesystem;

		LevelUploadJob::LevelUploadJob(db::Database& db, const fs::path& level_file)
			: SimpleJob("LevelLoadJob")
			, db_(db)
			, level_file_(level_file)
		{}

		void LevelUploadJob::run()
		{
			update_status(job::Status{ "Registering level file." + level_file_.string() });
			register_level_file();
		}

		void LevelUploadJob::register_level_file()
		{
			db::Level level;
		
			fs::fstream ifs{ level_file_, std::ios::in | std::ios::binary };
			if (!ifs.good())
				throw core::Exception("failed to load file") << core::errinfo_file_path(level_file_.string());

			protocol::FileBlock fb;
			CryptoPP::SHA3_256 file_hasher;
			CryptoPP::SHA3_256 block_hasher;

			std::vector<protocol::HashValType> block_hashes;
			protocol::HashValType file_hash;

			while (!ifs.eof())
			{
				update_status(job::Status{ "Calculating level file hash." });

				protocol::HashValType block_hash;

				ifs.read((char*)fb.data(), protocol::BlockSize);
				auto read_cnt = ifs.gcount();
				
				file_hasher.Update(fb.data(), (size_t)read_cnt);
				block_hasher.Update(fb.data(), (size_t)read_cnt);

				block_hasher.Final(block_hash.data());
				block_hashes.push_back(block_hash);
			}

			file_hasher.Final(file_hash.data());


			level.name = level_file_.filename().string();
			level.file_length = (std::uint32_t)fs::file_size(level_file_);
			level.hash = file_hash;

			auto conn = db_.open();
			{
				soci::transaction tr{ conn.get_session() };
				auto file_id = db::Level::insert(conn, level);  

				db::Level_File_Block::insert_blocks(conn, file_id, block_hashes);
				tr.commit();
			}
		}
	}
}