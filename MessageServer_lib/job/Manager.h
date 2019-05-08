#pragma once

#include <memory>
#include <boost/asio/io_service.hpp>

namespace mserver {
	namespace http {
		class Handler;
	}

	namespace job {

		struct Job;
		using Id = int;

		class Manager
		{
		private:
			struct Impl;
			std::shared_ptr<Impl> impl_;

		public:
			Manager();
			~Manager();
			Manager(Manager&&);
			Manager(const Manager&);
			Manager& operator =(Manager&&);
			Manager& operator =(const Manager&);

			static Manager create(http::Handler&, boost::asio::io_service&);

			Id start(std::shared_ptr<Job>);
		};
	}
}
