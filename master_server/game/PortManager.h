#pragma once

#include <memory>

namespace mserver {
	namespace game {

		class PortManager
		{
		private:
			struct Impl;
			std::shared_ptr<Impl> impl_;

		public:
			class Reservation
			{
			private:
				std::shared_ptr<Impl> self_;
				int port_;

			public:
				Reservation() : port_(-1) {}

				explicit Reservation(const PortManager&);
				~Reservation();

				Reservation(const Reservation&) = delete;
				Reservation(Reservation&&);
				void swap(Reservation&);
				Reservation& operator =(Reservation o) { swap(o); return *this; }

				int get() const { return port_; }
			};

			PortManager();
			~PortManager();

			static PortManager create(int port_min, int port_max);
		};
	}
}
