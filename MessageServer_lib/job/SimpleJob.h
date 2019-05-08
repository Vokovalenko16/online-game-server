#pragma once

#include "job/Job.h"
#include <mutex>

namespace mserver {
	namespace job {

		class SimpleJob : public Job
		{
		private:
			std::mutex mutex_;
			std::string name_;
			Status status_;
			bool canceled_;

		protected:
			SimpleJob(std::string name)
				: name_(std::move(name))
			{
				status_.message = "";
				status_.progress = 0.0f;
				canceled_ = false;
			}

			void update_status(Status new_status, bool check_cancel = true)
			{
				std::unique_lock<std::mutex> lock(mutex_);
				status_ = std::move(new_status);
				if (check_cancel && canceled_)
					throw JobCanceledException();
			}

		public:
			virtual std::string name() override { return name_; }
			
			virtual void cancel() override
			{
				std::unique_lock<std::mutex> lock(mutex_);
				canceled_ = true;
			}

			virtual Status status() override
			{
				std::unique_lock<std::mutex> lock(mutex_);
				return status_;
			}
		};
	}
}
