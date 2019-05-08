#pragma once

#include <memory>
#include <utility>

namespace mserver {
	namespace util {

		template<typename T> class unique_shared_ptr
		{
		private:
			std::shared_ptr<T> base_;

		public:
			unique_shared_ptr() {}
			unique_shared_ptr(std::nullptr_t) {}

			~unique_shared_ptr()
			{
				if (base_)
					base_->dispose();
			}

			explicit unique_shared_ptr(std::shared_ptr<T> base)
				: base_(std::move(base))
			{}

			unique_shared_ptr(unique_shared_ptr<T>&& o)
				: base_(std::move(o.base_))
			{
				o.base_ = nullptr;
			}
			unique_shared_ptr(const unique_shared_ptr<T>&) = delete;
			void swap(unique_shared_ptr<T>& o)
			{
				base_.swap(o.base_);
			}
			unique_shared_ptr<T>& operator =(unique_shared_ptr<T> o)
			{
				swap(o);
				return *this;
			}

			T* get() const { return base_.get(); }
			T* operator ->() const { return base_.get(); }
			T& operator *() const { return *base_.get(); }
			explicit operator bool() const { return base_ != nullptr; }

			const std::shared_ptr<T>& shared() const { return base_; }
			std::weak_ptr<T> weak() const { return base_; }
		};

		template<typename T, typename... Args> inline unique_shared_ptr<T> make_unique_shared(Args&& ...args)
		{
			return unique_shared_ptr<T>(std::make_shared<T>(std::forward<Args>(args) ...));
		}
	}
}
