#include "stdafx.h"
#include "Database.h"

namespace mserver {
	namespace db {

		using soci::details::execute;

		DbSession::~DbSession()
		{

		}

		DbSession::DbSession(std::string conn)
		{
			session_ = std::make_unique<soci::session>();
			session_->open(soci::sqlite3, conn);
		}

		soci::session& DbSession::get_session()
		{
			return *session_;
		}

		protocol::IDType DbSession::get_last_insert_rowid()
		{
			int res;
			execute((
				*session_ << ("select last_insert_rowid()"), soci::into(res)
				));

			return (protocol::IDType)res;
		}

		struct Database::Impl
		{
			std::string db_path;
		};

		Database::Database() = default;
		Database::~Database() = default;

		Database Database::create(const std::string& path)
		{
			Database res;
			res.impl_ = std::make_shared<Impl>();
			res.impl_->db_path = path;
			return res;
		}

		DbSession Database::open() const
		{
			return DbSession(impl_->db_path);
		}
	}
}
