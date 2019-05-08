#pragma once

#include <memory>

namespace mserver {
	namespace net {

		namespace detail {
			struct TlsContextImpl;
		}

		class TlsContext
		{
		private:
			std::shared_ptr<detail::TlsContextImpl> impl_;

		public:
			TlsContext();
			~TlsContext();
			TlsContext(const TlsContext&);
			TlsContext(TlsContext&&);
			TlsContext& operator =(const TlsContext&);
			TlsContext& operator =(TlsContext&&);

			static TlsContext create();
			detail::TlsContextImpl* impl() const { return impl_.get(); }

			void use_certificate_chain(const void* data, size_t len);
			void use_private_key(const void* data, size_t len);
		};
	}
}
