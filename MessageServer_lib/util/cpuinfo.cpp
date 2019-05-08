#include "stdafx.h"
#include "cpuinfo.h"
#include <stdexcept>

#if defined (WIN32)
#  define WIN32_LEAN_AND_MEAN
#  include <windows.h>
#endif

#if LINUX
#  include <fstream>
#  include <string>
#endif

namespace mserver {
	namespace util {

#if defined (WIN32)
		int get_cpu_count()
		{
			SYSTEM_INFO sysinfo;
			::GetSystemInfo(&sysinfo);
			return sysinfo.dwNumberOfProcessors;
		}
#elif LINUX
		int get_cpu_count()
		{
			std::fstream fs;
			fs.open("/proc/cpuinfo", std::ios::in);
			if (fs.fail())
				throw std::runtime_error("failed to open /proc/cpuinfo");

			int count = 0;

			std::string line;
			while (!fs.eof())
			{
				getline(fs, line);
				if (line.size() >= 9 && line.substr(0, 9) == "processor")
					++count;
			}

			if (count == 0)
				throw std::runtime_error("no 'processor' line was found in /proc/cpuinfo");

			return count;
		}
#else
#  error Current platform is not supported.
#endif
	}
}
