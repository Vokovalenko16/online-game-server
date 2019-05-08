#include "stdafx.h"
#include "Logger.h"
#include <mutex>
#include <fstream>
#include <memory>
#include <thread>
#include <condition_variable>
#include <chrono>

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/date_time.hpp>
#include <boost/format.hpp>
#include <boost/noncopyable.hpp>

namespace mserver {
	namespace log {
		namespace detail {

			namespace fs = boost::filesystem;

			const std::streamsize MaxLogSize = 0x400000;

			const char* level_to_string(Level level)
			{
				switch (level)
				{
				case Level::Debug:		return "DEBUG   ";
				case Level::Info:		return "INFO    ";
				case Level::Warning:	return "WARNING ";
				case Level::Error:		return "ERROR   ";
				case Level::Critical:	return "CRITICAL";
				default:				return "UNKNOWN ";
				}
			}

			class Logger : boost::noncopyable
			{
			private:
				std::mutex mutex_;
				std::condition_variable cond_;
				bool stop_ = false;
				std::thread flush_thread_;
				fs::path dir_;
				Level level_;
				fs::fstream file_;

			public:
				Logger(const std::string& dir, Level level)
				{
					dir_ = dir;
					level_ = level;
					fs::create_directories(dir_);

					flush_thread_ = std::thread([this]() { run_flush_thread(); });
				}

				~Logger()
				{
					{
						std::unique_lock<std::mutex> lock(mutex_);
						stop_ = true;
					}
					cond_.notify_all();
					flush_thread_.join();
				}

				Level level() const { return level_; }

				void write(Level level, const std::string& msg)
				{
					try
					{
						std::unique_lock<std::mutex> lock(mutex_);
						if (file_.is_open() && (std::streamsize)file_.tellp() >= MaxLogSize)
						{
							file_.flush();
							file_.close();
						}

						auto now = boost::posix_time::second_clock::local_time();

						if (!file_.is_open())
						{
							auto path = dir_ / (boost::format("%|04|-%|02|-%|02| %|02|_%|02|_%|02|.txt")
								% now.date().year()
								% now.date().month().as_number()
								% now.date().day()
								% now.time_of_day().hours()
								% now.time_of_day().minutes()
								% now.time_of_day().seconds()
								).str();

							file_.open(path, std::ios::out | std::ios::app);
							file_ << "\n";
						}

						if (file_.is_open())
						{
							file_ << (boost::format("[%|04|-%|02|-%|02| %|02|:%|02|:%|02|] %|| ")
								% now.date().year()
								% now.date().month().as_number()
								% now.date().day()
								% now.time_of_day().hours()
								% now.time_of_day().minutes()
								% now.time_of_day().seconds()
								% level_to_string(level)
								).str()
								<< msg << "\n";
						}
					}
					catch (...)
					{ }
				}

			private:
				void run_flush_thread()
				{
					std::unique_lock<std::mutex> lock(mutex_);
					for (;;)
					{
						if (cond_.wait_for(lock, std::chrono::seconds(30), [this]() { return stop_; }))
							return;

						if (file_.is_open())
							file_.flush();
					}
				}
			};

			std::unique_ptr<Logger> logger_;

			bool should_log(Level level)
			{
				return logger_ ? level >= logger_->level() : false;
			}

			void write_log(Level level, const std::string& msg)
			{
				if (logger_)
					logger_->write(level, msg);
			}
		}

		void init(const std::string& dir, Level level)
		{
			detail::logger_ = std::make_unique<detail::Logger>(dir, level);
		}
	}
}
