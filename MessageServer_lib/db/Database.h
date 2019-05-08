#pragma once

#include <string>
#include <memory>
#include <boost/noncopyable.hpp>

#include "soci/soci.h"
#include "soci/backends/sqlite/soci-sqlite3.h"

#include "protocol/Types.h"
#include "protocol/ArrayUtils.h"

namespace mserver {
	namespace db {

		class DbSession
		{
			std::unique_ptr<soci::session> session_;

		public:
			DbSession(std::string conn);
			~DbSession();

			DbSession(const DbSession& o) = delete;
			DbSession& operator =(const DbSession& o) = delete;

			DbSession(DbSession&& o) = default;
			DbSession& operator =(DbSession&& o) = default;

			protocol::IDType get_last_insert_rowid();
			soci::session& get_session();
		};

		struct Database
		{
		public:
			Database();
			~Database();

			static Database create(const std::string& path);
			DbSession open() const;

		private:
			struct Impl;
			std::shared_ptr<Impl> impl_;
		};
	}
}
