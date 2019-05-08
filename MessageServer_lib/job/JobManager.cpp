#include "stdafx.h"
#include "job/Manager.h"
#include "job/Job.h"
#include "core/Exception.h"
#include "http/Handler.h"
#include "logger/Logger.h"
#include "json/value.h"
#include "json/writer.h"
#include "json/reader.h"

#include <mutex>
#include <thread>
#include <unordered_map>

namespace mserver {
	namespace job {

		struct Manager::Impl
		{
			class Worker
			{
			private:
				Manager::Impl& self_;
				std::shared_ptr<Job> job_;
				Id id_;
				std::thread thread_;

			public:
				explicit Worker(Manager::Impl& self, std::shared_ptr<Job>&& job, Id id)
					: self_(self)
					, job_(std::move(job))
					, id_(id)
				{
					thread_ = std::thread([this]()
					{
						try
						{
							job_->run();
						}
						catch (core::Exception& ex)
						{
							log::write(log::Level::Error, "error in job %|| (%||): %||",
								id_, job_->name(), ex.diagnostic_information());
						}
						catch (std::exception& ex)
						{
							log::write(log::Level::Error, "error in job %|| (%||): %||",
								id_, job_->name(), ex.what());
						}

						thread_.detach();
						self_.worker_finished(id_);
					});
				}

				~Worker()
				{
					if (job_)
						job_->cancel();
					if (thread_.joinable())
						thread_.join();
				}

				Job& job() { return *job_; }
			};

			boost::asio::io_service& io_;
			std::mutex mutex_;
			Id next_id_ = 1;
			std::unordered_map<Id, std::unique_ptr<Worker>> workers_;

			Impl(http::Handler& http_handler, boost::asio::io_service& io)
				: io_(io)
			{
				register_http_commands(http_handler);
			}

			~Impl()
			{
				workers_.clear();
			}

			Id start(std::shared_ptr<Job>&& job)
			{
				std::unique_lock<std::mutex> lock(mutex_);
				auto id = next_id_++;
				log::write(log::Level::Info, "starting job %||, id=%||", job->name(), id);
				workers_[id] = std::make_unique<Worker>(*this, std::move(job), id);
				return id;
			}

			void worker_finished(Id id)
			{
				log::write(log::Level::Info, "job %|| finished", id);

				std::unique_lock<std::mutex> lock(mutex_);
				workers_.erase(id);
			}

			void register_http_commands(http::Handler& handler)
			{
				handler.register_json_method("/job/list", [this](Json::Value&& req, Json::Value& resp)
				{
					auto& jobs = resp["jobs"];
					jobs = Json::ValueType::arrayValue;

					{
						std::unique_lock<std::mutex> lock(mutex_);
						for (const auto& entry : workers_)
						{
							auto& job_json = jobs[jobs.size()];
							job_json["id"] = entry.first;
							job_json["name"] = entry.second->job().name();
						}
					}

					resp["succeeded"] = true;
				});

				handler.register_json_method("/job/info", [this](Json::Value&& req, Json::Value& resp)
				{
					Id id = req["id"].asInt();

					bool succeeded = false;
					std::string job_name;
					Status job_status;

					{
						std::unique_lock<std::mutex> lock(mutex_);
						auto iter = workers_.find(id);
						if (iter != workers_.end())
						{
							succeeded = true;
							auto& job = iter->second->job();
							job_name = job.name();
							job_status = job.status();
						}
					}

					resp["id"] = id;
					resp["succeeded"] = succeeded;
					if (succeeded)
					{
						resp["name"] = job_name;

						auto& status_json = resp["status"];
						status_json["message"] = job_status.message;
						status_json["progress"] = job_status.progress;
					}
				});

				handler.register_json_method("/job/cancel", [this](Json::Value&& req, Json::Value& resp)
				{
					Id id = req["id"].asInt();
					bool succeeded = false;

					{
						std::unique_lock<std::mutex> lock(mutex_);
						auto iter = workers_.find(id);
						if (iter != workers_.end())
						{
							iter->second->job().cancel();
							succeeded = true;
						}
					}

					resp["id"] = id;
					resp["succeeded"] = succeeded;
				});
			}
		};

		Manager::Manager() = default;
		Manager::~Manager() = default;
		Manager::Manager(Manager&&) = default;
		Manager::Manager(const Manager&) = default;
		Manager& Manager::operator =(Manager&&) = default;
		Manager& Manager::operator =(const Manager&) = default;

		Manager Manager::create(http::Handler& http_handler, boost::asio::io_service& io)
		{
			Manager res;
			res.impl_ = std::make_shared<Impl>(http_handler, io);
			return res;
		}

		Id Manager::start(std::shared_ptr<Job> job)
		{
			return impl_->start(std::move(job));
		}
	}
}
