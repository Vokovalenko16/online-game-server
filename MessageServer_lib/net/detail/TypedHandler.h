#pragma once

#include "protocol/Message.h"
#include "../NetSession.h"

namespace mserver {
	namespace net {

		namespace detail {

			template<typename> struct FunctionType
			{
				using Type = void;
			};

			template<typename Res, typename... Args>
			struct FunctionType<Res(*)(Args...)>
			{
				using Type = Res(*)(Args...);
			};
			template<typename Res, typename... Args>
			struct FunctionType<Res(&)(Args...)>
			{
				using Type = Res(*)(Args...);
			};
			template<typename Res, typename... Args>
			struct FunctionType<Res(Args...)>
			{
				using Type = Res(*)(Args...);
			};
			template<typename Cls, typename Res, typename... Args>
			struct FunctionType<Res(Cls::*)(Args...)>
			{
				using Type = Res(*)(Args...);
			};
			template<typename Cls, typename Res, typename... Args>
			struct FunctionType<Res(Cls::*)(Args...) const>
			{
				using Type = Res(*)(Args...);
			};

			template<typename T, typename F = typename FunctionType<decltype(&T::operator())>::Type> F get_function_type(T&&, int);
			template<typename T> typename FunctionType<T>::Type get_function_type(T&&, ...);

			template<typename T> struct HandlerArgResultType
			{
				using ArgType = void;
				using ResType = void;
			};

			template<typename Res, typename Arg> struct HandlerArgResultType<Res(*)(Arg, NetSession&)>
			{
				using ArgType = Arg;
				using ResType = Res;
			};

			template<typename T> typename std::decay<T>::type&& rvalue();

			template<typename> struct RequestTraits
			{
				static const bool requirements_met = false;
			};
			template<protocol::MessageType R> struct RequestTraits<protocol::Body<R>>
			{
				static const bool requirements_met = true;
				static const protocol::MessageType Request = R;
			};
			template<protocol::MessageType R> struct RequestTraits<protocol::Body<R>&&>
			{
				static const bool requirements_met = true;
				static const protocol::MessageType Request = R;
			};
			template<protocol::MessageType R> struct RequestTraits<const protocol::Body<R>&>
			{
				static const bool requirements_met = true;
				static const protocol::MessageType Request = R;
			};

			template<typename> struct ResponseTraits
			{
				static const bool requirements_met = false;
			};
			template<protocol::MessageType R> struct ResponseTraits<protocol::Body<R>>
			{
				static const bool requirements_met = true;
				static const protocol::MessageType Response = R;
			};

			template<typename Types,
				bool RequirementsMet = RequestTraits<typename Types::ArgType>::requirements_met
					&& ResponseTraits<typename Types::ResType>::requirements_met
			> struct TypedHandlerTraitsBase
			{
				static const bool requirements_met = false;
			};
			template<typename Types> struct TypedHandlerTraitsBase<Types, true>
			{
				static const bool requirements_met = true;

				static const protocol::MessageType Request = RequestTraits<typename Types::ArgType>::Request;
				static const protocol::MessageType Response = ResponseTraits<typename Types::ResType>::Response;
			};

			template<typename T> struct TypedHandlerTraits
				: TypedHandlerTraitsBase< HandlerArgResultType<decltype(get_function_type(rvalue<T>(), 0))> >
			{
			};
		}
	}
}
