#pragma once

#include <exception>
#include <stdexcept>
#include <type_traits>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <memory>
#include <atomic>
#include <boost/asio/io_service.hpp>
#include <boost/optional.hpp>

namespace mserver {
	namespace net {

		template<typename T> class Future;
		template<typename T> class Promise;
		struct Void {};

#pragma region Implementation details
		namespace detail {
			extern const std::exception_ptr work_canceled_error;

			struct DeferredWorkBase
			{
				std::mutex mutex_;
				std::condition_variable cond_;
				std::exception_ptr error_;
				std::shared_ptr<DeferredWorkBase> continuation_;

				virtual ~DeferredWorkBase() {}
				virtual void precond_ready(const std::shared_ptr<DeferredWorkBase>& self) noexcept {}

				void set_exception(std::exception_ptr error) noexcept
				{
					if (!error)
						error = detail::work_canceled_error;

					std::shared_ptr<detail::DeferredWorkBase> linked_work;
					{
						std::unique_lock<std::mutex> lock(mutex_);
						error_ = std::move(error);

						linked_work = std::move(continuation_);
						continuation_ = nullptr;
					}
					cond_.notify_all();

					if (linked_work)
						linked_work->precond_ready(linked_work);
				}
			};

			template<typename T> struct DeferredWork : DeferredWorkBase
			{
				boost::optional<T> value_;

				void set_value(T&& value)
				{
					std::shared_ptr<detail::DeferredWorkBase> linked_work;
					{
						std::unique_lock<std::mutex> lock(mutex_);
						value_ = std::move(value);

						linked_work = std::move(continuation_);
						continuation_ = nullptr;
					}
					cond_.notify_all();

					if (linked_work)
						linked_work->precond_ready(linked_work);
				}
			};

			template<typename T> struct Continuation : DeferredWork<T>
			{
				std::atomic_uint_least32_t num_pending_results_;
				
				Continuation()
					: num_pending_results_(1)
				{}
			};

			template<typename T, typename Handler> struct ErrorHandler : Continuation<Void>
			{
				Future<T> future_;
				Handler handler_;

				ErrorHandler(Future<T>&& future, Handler&& handler)
					: future_(std::move(future))
					, handler_(std::move(handler))
				{}

				virtual void precond_ready(const std::shared_ptr<DeferredWorkBase>& self) noexcept override;
			};

			template<typename T, typename Handler> struct ImmediateContinuation : Continuation<T>
			{
				Handler handler_;

				template<typename... Args> ImmediateContinuation(Args&&... args)
					: handler_(std::forward<Args>(args)...)
				{}

				virtual void precond_ready(const std::shared_ptr<DeferredWorkBase>& self) noexcept override
				{
					if (--Continuation<T>::num_pending_results_ == 0)
					{
						try
						{
							handler_.invoke(Promise<T>{
								std::static_pointer_cast<DeferredWork<T>>(self)
							});
						}
						catch (...)
						{
							DeferredWork<T>::set_exception(std::current_exception());
						}
					}
				}
			};

			template<typename T, typename Handler> struct DeferredContinuation : Continuation<T>
			{
				boost::asio::io_service& io_;
				Handler handler_;

				template<typename... Args> DeferredContinuation(boost::asio::io_service& io, Args&&... args)
					: io_(io)
					, handler_(std::forward<Args>(args)...)
				{}

				virtual void precond_ready(const std::shared_ptr<DeferredWorkBase>& self) noexcept override
				{
					if (--Continuation<T>::num_pending_results_ == 0)
					{
						try
						{
							io_.post([self = std::static_pointer_cast<DeferredContinuation<T, Handler>>(self)]()
							{
								try
								{
									self->handler_.template invoke<T>(Promise<T>{
										std::static_pointer_cast<DeferredWork<T>>(self)
									});
								}
								catch (...)
								{
									self->DeferredWork<T>::set_exception(std::current_exception());
								}
							});
						}
						catch (...)
						{
							DeferredWork<T>::set_exception(std::current_exception());
						}
					}
				}
			};

			template<typename T> struct FutureResult
			{
				using Type = T;

				template<typename U> static void set_continuation(T& pre, const std::shared_ptr<Continuation<U>>& post)
				{
				}

				static T&& get(T&& v) { return std::move(v); }
				static std::exception_ptr get_error(T&&) { return nullptr; }
			};

			template<typename T> struct FutureResult< Future<T> >
			{
				using Type = T;

				template<typename U> static void set_continuation(Future<T>& pre, const std::shared_ptr<Continuation<U>>& post);
				static T get(Future<T>&& f);
				static std::exception_ptr get_error(Future<T>&& f);
			};

			template<typename Func, typename...> struct PackedHandler;
			template<typename Func, typename First, typename... Remainder> struct PackedHandler<Func, First, Remainder...>
			{
				typename std::decay<First>::type first_;
				PackedHandler<Func, Remainder...> remainder_;

				PackedHandler(Func&& func, First&& first, Remainder&& ... remainder)
					: first_(std::forward<First>(first))
					, remainder_(std::forward<Func>(func), std::forward<Remainder>(remainder)...)
				{
				}

				template<typename Res, typename... Args> void invoke(Promise<Res>&& promise, Args&& ... args)
				{
					return remainder_.invoke<Res>(
						std::move(promise),
						std::forward<Args>(args)...,
						FutureResult<typename std::decay<First>::type>::get(std::move(first_)));
				}

				template<typename U> void set_continuation(const std::shared_ptr<Continuation<U>>& post)
				{
					FutureResult<First>::set_continuation(first_, post);
					remainder_.set_continuation(post);
				}
			};
			template<typename Func> struct PackedHandler<Func>
			{
				Func func_;
				PackedHandler(Func&& func) : func_(std::forward<Func>(func)) {}

				template<typename Res, typename... Args> void invoke(Promise<Res>&& promise, Args&& ... args)
				{
					return func_(std::move(promise), std::forward<Args>(args)...);
				}

				template<typename U> void set_continuation(const std::shared_ptr<Continuation<U>>& post)
				{}
			};
		}
#pragma endregion

		enum class FutureStatus
		{
			Deferred,
			Ready,
			Timeout,
		};

		template<typename T> class Future
		{
			friend struct detail::FutureResult<Future<T>>;

		private:
			std::shared_ptr<detail::DeferredWork<T>> work_;

		public:
			Future() = default;
			~Future() = default;
			explicit Future(std::shared_ptr<detail::DeferredWork<T>> work) : work_(std::move(work)) {}

			Future(const Future<T>&) = delete;
			Future(Future<T>&& o) : work_(std::move(o.work_)) { o.work_ = nullptr; }
			void swap(Future<T>& o) { work_.swap(o.work_); }
			Future<T>& operator =(Future<T> o) { swap(o); return *this; }

			void wait();
			template<typename Rep, typename Period> FutureStatus wait_for(std::chrono::duration<Rep, Period> timeout);
			template<typename Clock, typename Duration> FutureStatus wait_until(std::chrono::time_point<Clock, Duration> until);
			T get();

			template<typename Continuation>
			inline auto then(Continuation&&)
				-> Future<typename std::result_of<Continuation(T&&)>::type>;

			template<typename Continuation>
			inline auto then(boost::asio::io_service&, Continuation&&)
				-> Future<typename std::result_of<Continuation(T&&)>::type>;

			template<typename Res, typename Continuation>
			inline auto then(Continuation&&)
				-> typename std::enable_if< std::is_void<typename std::result_of<Continuation(Promise<Res>&&, T&&)>::type>::value, Future<Res> >::type;

			template<typename Res, typename Continuation>
			inline auto then(boost::asio::io_service&, Continuation&&)
				-> typename std::enable_if< std::is_void<typename std::result_of<Continuation(Promise<Res>&&, T&&)>::type>::value, Future<Res> >::type;

			template<typename Handler>
			void error(Handler&&);
		};

		template<typename T> class Promise
		{
		private:
			std::shared_ptr<detail::DeferredWork<T>> work_;

			void internal_set_exception(std::exception_ptr);

		public:
			Promise() = default;
			~Promise();
			explicit Promise(std::shared_ptr<detail::DeferredWork<T>> work) : work_(std::move(work)) {}

			Promise(const Promise<T>&) = delete;
			Promise(Promise<T>&& o) : work_(std::move(o.work_)) { o.work_ = nullptr; }
			void swap(Promise<T>& o) { work_.swap(o.work_); }
			Promise<T>& operator =(Promise<T> o) { swap(o); return *this; }

			Future<T> get_future();
			void set_value(T&&);
			void set_exception(std::exception_ptr);
		};

		template<typename T, typename Handler, typename... Args>
		typename std::enable_if< std::is_void<typename std::result_of<
			Handler(
				Promise<T>&&,
				typename detail::FutureResult<typename std::decay<Args>::type>::Type&&...
			)>::type>::value, Future<T> >::type
		then(Handler&& handler, Args&&... args)
		{
			using H = detail::PackedHandler<Handler, Args...>;

			auto work = std::make_shared<detail::ImmediateContinuation<T, H>>(
				std::forward<Handler>(handler),
				std::forward<Args>(args)...);
			work->handler_.set_continuation(std::static_pointer_cast<detail::Continuation<T>>(work));
			work->precond_ready(work);

			return Future<T>{ std::move(work) };
		}

		template<typename T, typename Handler, typename... Args>
		typename std::enable_if< std::is_void<typename std::result_of<
			Handler(
				Promise<T>&&,
				typename detail::FutureResult<typename std::decay<Args>::type>::Type&&...
			)>::type>::value, Future<T> >::type
		then(
			boost::asio::io_service& io,
			Handler&& handler,
			Args&&... args)
		{
			using H = detail::PackedHandler<Handler, Args...>;

			auto work = std::make_shared<detail::DeferredContinuation<T, H>>(
				io,
				std::forward<Handler>(handler),
				std::forward<Args>(args)...);
			work->handler_.set_continuation(std::static_pointer_cast<detail::Continuation<T>>(work));
			work->precond_ready(work);

			return Future<T>{ std::move(work) };
		}

		template<typename Handler, typename... Args>
		Future<typename std::result_of<Handler(typename detail::FutureResult<typename std::decay<Args>::type>::Type&& ...)>::type> then(
			Handler&& handler,
			Args&&... args)
		{
			using ResultType = typename std::result_of<Handler(typename detail::FutureResult<typename std::decay<Args>::type>::Type&& ...)>::type;

			return then<ResultType>([h = std::move(handler)](Promise<ResultType>&& promise, typename detail::FutureResult<typename std::decay<Args>::type>::Type&&... args) mutable
			{
				promise.set_value(h(std::forward<typename detail::FutureResult<typename std::decay<Args>::type>::Type>(args)...));
			}, std::forward<Args>(args)...);
		}

		template<typename Handler, typename... Args>
		Future<typename std::result_of<Handler(typename detail::FutureResult<typename std::decay<Args>::type>::Type&& ...)>::type> then(
			boost::asio::io_service& io,
			Handler&& handler,
			Args&&... args)
		{
			using ResultType = typename std::result_of<Handler(typename detail::FutureResult<typename std::decay<Args>::type>::Type&& ...)>::type;

			return then<ResultType>(io, [h = std::move(handler)](Promise<ResultType>&& promise, typename detail::FutureResult<typename std::decay<Args>::type>::Type&&... args) mutable
			{
				promise.set_value(h(std::forward<typename detail::FutureResult<typename std::decay<Args>::type>::Type>(args)...));
			}, std::forward<Args>(args)...);
		}

#pragma region Implementation
		template<typename T, typename Handler>
		void detail::ErrorHandler<T, Handler>::precond_ready(const std::shared_ptr<detail::DeferredWorkBase>& self) noexcept
		{
			if (--num_pending_results_ == 0)
			{
				try
				{
					auto err = detail::FutureResult<Future<T>>::get_error(std::move(future_));
					if (err)
						handler_(std::move(err));
				}
				catch (...)
				{ }
			}
		}

		template<typename T>
		template<typename U>
		void detail::FutureResult<Future<T>>::set_continuation(Future<T>& pre, const std::shared_ptr<detail::Continuation<U>>& post)
		{
			if (!pre.work_)
				throw std::invalid_argument("attempted to set continuation on empty Future<T>");
			auto& work = *pre.work_;
			
			std::unique_lock<std::mutex> lock(work.mutex_);
			if (work.error_ || work.value_)
				return;

			++post->num_pending_results_;
			work.continuation_ = post;
		}

		template<typename T> inline T detail::FutureResult<Future<T>>::get(Future<T>&& f)
		{
			auto& work = *f.work_;
			std::unique_lock<std::mutex> lock(work.mutex_);

			if (work.value_)
				return std::move(*work.value_);
			if (work.error_)
				std::rethrow_exception(work.error_);

			throw std::runtime_error("internal error: continuation routine was invoked when not all results are ready.");
		}

		template<typename T> inline std::exception_ptr detail::FutureResult<Future<T>>::get_error(Future<T>&& f)
		{
			auto& work = *f.work_;
			std::unique_lock<std::mutex> lock(work.mutex_);

			if (work.value_)
				return nullptr;
			if (work.error_)
				return std::move(work.error_);

			return std::make_exception_ptr(
				std::runtime_error("internal error: continuation routine was invoked when not all results are ready."));
		}

		template<typename T> inline Promise<T>::~Promise()
		{
			if (work_)
				internal_set_exception(detail::work_canceled_error);
		}

		template<typename T> inline Future<T> Promise<T>::get_future()
		{
			auto work = std::make_shared<detail::DeferredWork<T>>();
			*this = Promise<T>{ work };
			return Future<T>{ std::move(work) };
		}

		template<typename T> void Promise<T>::set_value(T&& value)
		{
			if (!work_)
				throw std::invalid_argument("attempted to use Promise<T> without first calling get_future().");

			work_->set_value(std::move(value));
			work_ = nullptr;
		}

		template<typename T> inline void Promise<T>::set_exception(std::exception_ptr error)
		{
			if (!work_)
				throw std::invalid_argument("attempted to use Promise<T> without first calling get_future().");

			return internal_set_exception(std::move(error));
		}

		template<typename T> void Promise<T>::internal_set_exception(std::exception_ptr error)
		{
			work_->set_exception(std::move(error));
			work_ = nullptr;
		}

		template<typename T> void Future<T>::wait()
		{
			if (!work_)
				throw std::invalid_argument("wait() called on empty Future<T> object.");

			std::unique_lock<std::mutex> lock(work_->mutex_);
			work_->cond_.wait(lock, [&]() { return work_->value_ || work_->error_; });
		}

		template<typename T> template<typename Rep, typename Period>
		FutureStatus Future<T>::wait_for(std::chrono::duration<Rep, Period> timeout)
		{
			if (!work_)
				throw std::invalid_argument("wait_for() called on empty Future<T> object.");

			std::unique_lock<std::mutex> lock(work_->mutex_);
			if (work_->cond_.wait_for(lock, timeout,
				[&]() { return work_->value_ || work_->error_; }))
				return FutureStatus::Ready;
			else
				return FutureStatus::Timeout;
		}

		template<typename T> template<typename Clock, typename Duration>
		FutureStatus Future<T>::wait_until(std::chrono::time_point<Clock, Duration> until)
		{
			if (!work_)
				throw std::invalid_argument("wait_until() called on empty Future<T> object.");

			std::unique_lock<std::mutex> lock(work_->mutex_);
			if (work_->cond_.wait_until(lock, until, [&]() { return work_->value_ || work_->error_; }))
				return FutureStatus::Ready;
			else
				return FutureStatus::Timeout;
		}

		template<typename T> T Future<T>::get()
		{
			if (!work_)
				throw std::invalid_argument("get() called on empty Future<T> object.");

			Future<T> local = std::move(*this);

			std::unique_lock<std::mutex> lock(local.work_->mutex_);
			for (;;)
			{
				if (local.work_->value_)
					return std::move(*local.work_->value_);
				if (local.work_->error_)
					std::rethrow_exception(std::move(local.work_->error_));

				local.work_->cond_.wait(lock);
			}
		}

		template<typename T> template<typename Continuation>
		inline Future<typename std::result_of<Continuation(T&&)>::type>
			Future<T>::then(Continuation&& continuation)
		{
			return net::then(std::move(continuation), std::move(*this));
		}

		template<typename T> template<typename Continuation>
		inline Future<typename std::result_of<Continuation(T&&)>::type>
			Future<T>::then(boost::asio::io_service& io, Continuation&& continuation)
		{
			return net::then(io, std::move(continuation), std::move(*this));
		}

		template<typename T> template<typename Res, typename Continuation>
		inline auto Future<T>::then(Continuation&& continuation)
			-> typename std::enable_if< std::is_void<typename std::result_of<Continuation(Promise<Res>&&, T&&)>::type>::value, Future<Res> >::type
		{
			return net::then<Res>(std::move(continuation), std::move(*this));
		}

		template<typename T> template<typename Res, typename Continuation>
		inline auto Future<T>::then(boost::asio::io_service& io, Continuation&& continuation)
			-> typename std::enable_if< std::is_void<typename std::result_of<Continuation(Promise<Res>&&, T&&)>::type>::value, Future<Res> >::type
		{
			return net::then<Res>(io, std::move(continuation), std::move(*this));
		}

		template<typename T> template<typename Handler>
		void Future<T>::error(Handler&& handler)
		{
			auto work = std::make_shared<detail::ErrorHandler<T, typename std::decay<Handler>::type>>(
				std::move(*this),
				std::forward<Handler>(handler));
			detail::FutureResult<Future<T>>::set_continuation(
				work->future_,
				std::static_pointer_cast<detail::Continuation<Void>>(work));
			work->precond_ready(work);
		}
#pragma endregion
	}
}
