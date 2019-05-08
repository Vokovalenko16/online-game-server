#pragma once

#include <string>
#include <utility>
#include <exception>
#include <boost/format.hpp>

namespace mserver {
	namespace log {

		enum class Level
		{
			Debug,
			Info,
			Warning,
			Error,
			Critical,
		};

		namespace detail {
			bool should_log(Level level);
			void write_log(Level level, const std::string& msg);

			inline void feed_args(boost::format& f) {}

			template<typename FirstArg, typename... Args>
			inline void feed_args(boost::format& f, FirstArg&& first_arg, Args&&... args)
			{
				f % std::forward<FirstArg>(first_arg);
				feed_args(f, std::forward<Args>(args) ...);
			}
		}

		template<typename... Args> inline void write(Level level, const char* format, Args&&... args)
		{
			if (detail::should_log(level))
			{
				try
				{
					boost::format f(format);
					detail::feed_args(f, std::forward<Args>(args)...);
					detail::write_log(level, f.str());
				}
				catch (std::exception& ex)
				{
					detail::write_log(Level::Error, ex.what());
				}
			}
		}

		void init(const std::string& dir, Level level);
	}
}
