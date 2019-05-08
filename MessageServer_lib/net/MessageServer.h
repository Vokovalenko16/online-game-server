#pragma once

#include "TlsConnection.h"
#include "NetSession.h"
#include "detail/TypedHandler.h"

#include "protocol/Message.h"
#include "util/unique_shared_ptr.h"

#include <memory>
#include <chrono>
#include <functional>
#include <type_traits>
#include <boost/asio/io_service.hpp>

namespace mserver {
	namespace net {

		using MessageHandler = std::function<protocol::MessageType (protocol::Message&, NetSession&)>;

		class MessageServer
		{
		public:
			struct Impl;

		private:
			util::unique_shared_ptr<Impl> impl_;

		public:
			MessageServer();
			~MessageServer();
			MessageServer(MessageServer&&);
			MessageServer& operator =(MessageServer&&);

			static MessageServer create(boost::asio::io_service& io, std::chrono::steady_clock::duration idle_timeout);
			void service(TlsConnection conn);

			void register_handler(protocol::MessageType type, MessageHandler handler);

			template<typename Handler>
			void register_typed_handler(Handler&& handler)
			{
				using Tr = detail::TypedHandlerTraits<Handler>;
				static_assert(Tr::requirements_met, "Typed handler must be a callable object, whose signature is compatible to 'protocol::Body<RESPONSE> (protocol::Body<REQUEST>&&, NetSession&)'");

				register_handler(Tr::Request, [handler = std::move(handler)](protocol::Message& msg, NetSession& session) -> protocol::MessageType
				{
					protocol::Body<Tr::Request> request;
					protocol::MessageReader{ msg } >> request;

					protocol::Body<Tr::Response> response = handler(std::move(request), session);

					protocol::MessageWriter{ msg } << response;
					return Tr::Response;
				});
			}
		};
	}
}
