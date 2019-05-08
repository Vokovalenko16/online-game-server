#include "stdafx.h"
#include "io_worker.h"
#include "cpuinfo.h"
#include "logger/Logger.h"

#include <thread>
#include <vector>
#include <exception>

namespace mserver {
	namespace util {

		void run_io_workers(boost::asio::io_service& io, int threads_per_core)
		{
			int num_threads;

			try {
				num_threads = get_cpu_count() * threads_per_core;
			}
			catch (std::exception& ex) {
				log::write(log::Level::Error, "error retrieving cpu count (%||), assuming 1.", ex.what());
				num_threads = threads_per_core;
			}

			log::write(log::Level::Info, "spawning %|| worker threads.", num_threads);
			std::vector<std::thread> workers(num_threads);
			for (auto& t : workers)
				t = std::thread([&io]() { io.run(); });

			for (auto& t : workers)
				t.join();
		}
	}
}
