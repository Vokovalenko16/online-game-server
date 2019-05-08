#include "stdafx.h"
#include "PortManager.h"
#include <mutex>
#include <vector>
#include <utility>
#include <exception>

namespace mserver {
	namespace game {

		struct PortManager::Impl
		{
			std::mutex mutex_;
			std::vector<int> ports_;
		};

		PortManager::PortManager() = default;
		PortManager::~PortManager() = default;

		PortManager PortManager::create(int port_min, int port_max)
		{
			PortManager res;
			res.impl_ = std::make_shared<Impl>();
			for (auto i = port_min; i < port_max; ++i)
				res.impl_->ports_.push_back(i);

			return res;
		}

		PortManager::Reservation::Reservation(const PortManager& mgr)
		{
			self_ = mgr.impl_;

			std::unique_lock<std::mutex> lock(self_->mutex_);
			if (self_->ports_.empty())
				throw std::runtime_error("no free port");

			port_ = self_->ports_.back();
			self_->ports_.pop_back();
		}

		PortManager::Reservation::~Reservation()
		{
			if (self_)
			{
				std::unique_lock<std::mutex> lock(self_->mutex_);
				self_->ports_.push_back(port_);
			}
		}

		PortManager::Reservation::Reservation(Reservation&& o)
		{
			port_ = o.port_;
			self_ = std::move(o.self_);

			o.port_ = -1;
			o.self_ = nullptr;
		}

		void PortManager::Reservation::swap(Reservation& o)
		{
			std::swap(port_, o.port_);
			std::swap(self_, o.self_);
		}
	}
}
