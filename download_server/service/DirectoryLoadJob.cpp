#include "stdafx.h"
#include "DirectoryLoadJob.h"
#include "core/Exception.h"
#include "CryptoPP/sha3.h"
#include "logger/Logger.h"
#include "db/File_Blocks_Model.h"
#include "db/Files_Model.h"
#include "db/Releases_Model.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/format.hpp>

namespace mserver {
	namespace service {

		namespace fs = boost::filesystem;
		using boost::format;

		DirectoryLoadJob::DirectoryLoadJob(
			db::Database& db, 
			protocol::IDType release_id, 
			std::string release_name,
			std::string physical_path, 
			std::string logical_path)
			: SimpleJob("DirectoryLoadJob")
			, db_(db)
			, release_id_(release_id)
			, release_name_(release_name)
			, file_index_(0)
			, physical_path_(physical_path)
			, logical_path_(logical_path)
		{
		}

		void DirectoryLoadJob::run()
		{
			try
			{
				add_release();
				load_directory(physical_path_, logical_path_);
				update_release_file_count();
			}
			catch (const job::JobCanceledException& ex)
			{
				log::write(log::Level::Error, "(DirectoryLoad)%||", ex.what());
				roll_back();
			}
			catch (const std::exception& ex)
			{
				log::write(log::Level::Error, "failed to load directories...(%||)", ex.what());
			}
		}

		void DirectoryLoadJob::add_release()
		{
			auto dbSession = db_.open();

			{
				soci::transaction trans(dbSession.get_session());
				db::Releases_Model release;
				release.id = release_id_;
				release.name = release_name_;
				db::Releases_Model::insert_row(dbSession, release);
				trans.commit();
			}

		}

		void DirectoryLoadJob::update_release_file_count()
		{
			auto dbSession = db_.open();
			{
				soci::transaction trans(dbSession.get_session());
				db::Releases_Model::update_row(dbSession, (int)release_id_, (int)file_index_);
				trans.commit();
			}
		}

		void DirectoryLoadJob::load_directory(
			const boost::filesystem::path& physical_path,
			const std::string& logical_path
		)
		{
			auto dbSession = db_.open();

			soci::transaction tran(dbSession.get_session());
			tran.commit();

			for (auto iter = fs::directory_iterator(physical_path); iter != fs::directory_iterator(); ++iter)
			{
				switch (iter->status().type())
				{
				case fs::file_type::directory_file:
					load_directory(iter->path(), logical_path + "/" + iter->path().filename().string());
					break;

				case fs::file_type::regular_file:
					load_file(iter->path(), logical_path + "/" + iter->path().filename().string());
					break;
				}
			}
		}

		void DirectoryLoadJob::load_file(
			const boost::filesystem::path& physical_path,
			const std::string& logical_path
		)
		{
			// Open and calculate file & block hashes
			fs::fstream ifs;
			ifs.open(physical_path, std::ios::in | std::ios::binary);

			if (!ifs.good())
				throw core::Exception("failed to load file") << core::errinfo_file_path(physical_path.string());

			protocol::FileBlock fb;
			CryptoPP::SHA3_256 file_hash;
			std::vector<protocol::HashValType> block_hashes;
			while (!ifs.eof())
			{
				job::Status newStatus;
				newStatus.message = boost::str(
					format("calculating file(%||) & file block(%||) hash...") % physical_path.string() % block_hashes.size());
				newStatus.progress = 0.0f;
				update_status(newStatus);

				ifs.read((char*)fb.data(), protocol::BlockSize);
				auto read_cnt = ifs.gcount();

				file_hash.Update(fb.data(), (size_t)read_cnt);

				CryptoPP::SHA3_256 block_hash;
				block_hash.Update(fb.data(), (size_t)read_cnt);
				protocol::HashValType block_hash_val;
				block_hash.Final(block_hash_val.data());
				block_hashes.push_back(block_hash_val);
			}

			protocol::HashValType file_hash_val;
			file_hash.Final(file_hash_val.data());

			// Load into database
			auto this_file_index = file_index_++;

			{
				job::Status newStatus;
				newStatus.message = "inserting int database ...";
				newStatus.progress = 0;
				update_status(newStatus);

				db::DbSession session = db_.open();
				
				db::Files_Model file;

				file.release_id = release_id_;
				file.file_index = this_file_index;
				file.physical_path = physical_path.string();
				file.logical_path = logical_path;
				file.hash = file_hash_val;
				file.file_length = (protocol::FileLengthType)fs::file_size(physical_path);
				{
					soci::transaction trans(session.get_session());
					db::Files_Model::insert_row(session, file);
					auto file_id = session.get_last_insert_rowid();
					db::File_Blocks_Model::insert_rows(session, file_id, block_hashes);
					trans.commit();
				}
			}

		}

		void DirectoryLoadJob::roll_back()
		{
			db::DbSession session = db_.open();
			soci::transaction tran(session.get_session());
			db::Files_Model::delete_by_release_id(session, release_id_);
			tran.commit();
		}
	}
}