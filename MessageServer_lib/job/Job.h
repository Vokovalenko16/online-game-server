#pragma once

#include <string>
#include <exception>

namespace mserver {
	namespace job {

		struct Status
		{
			std::string message;
			float progress;

			Status() = default;
			explicit Status(std::string message, float progress = 0.0f)
				: message(std::move(message))
				, progress(progress)
			{}
		};

		struct Job
		{
			virtual ~Job() {}

			virtual void run() = 0;
			virtual std::string name() = 0;
			virtual void cancel() = 0;
			virtual Status status() = 0;
		};

		struct JobCanceledException : std::exception
		{
			JobCanceledException()
			{}

			virtual const char* what() const noexcept
			{
				return "job was canceled by user.";
			}
		};
	}
}
