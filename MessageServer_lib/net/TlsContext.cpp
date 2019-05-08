#include "stdafx.h"
#include "TlsContext.h"
#include "TlsContextImpl.h"

namespace mserver {
	namespace net {

		namespace asio = boost::asio;
		namespace ssl = boost::asio::ssl;

		detail::TlsContextImpl::TlsContextImpl()
			: ctx_(ssl::context::tlsv12)
		{
			ctx_.set_options(ssl::context::no_sslv2 | ssl::context::no_sslv3 | ssl::context::no_tlsv1);
		}

		TlsContext::TlsContext() = default;
		TlsContext::~TlsContext() = default;
		TlsContext::TlsContext(const TlsContext&) = default;
		TlsContext::TlsContext(TlsContext&&) = default;
		TlsContext& TlsContext::operator =(const TlsContext&) = default;
		TlsContext& TlsContext::operator =(TlsContext&&) = default;

		TlsContext TlsContext::create()
		{
			TlsContext r;
			r.impl_ = std::make_shared<detail::TlsContextImpl>();
			return r;
		}

		void TlsContext::use_certificate_chain(const void* data, size_t len)
		{
			impl_->ctx_.use_certificate_chain(
				asio::buffer(data, len));
		}

		void TlsContext::use_private_key(const void* data, size_t len)
		{
			impl_->ctx_.use_private_key(asio::buffer(data, len), ssl::context_base::pem);
		}
	}
}
