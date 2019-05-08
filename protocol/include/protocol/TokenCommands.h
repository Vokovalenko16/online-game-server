#pragma once

#include "Types.h"
#include <array>
#include <cstdint>
#include <boost/serialization/array.hpp>

namespace protocol {

	template<> struct Body<MessageType::TokenReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{}
	};

	template<> struct Body<MessageType::TokenResp>
	{
		HashValType token;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & token;
		}
	};

	template<> struct Body<MessageType::VerifyTokenReq>
	{
		NetString<40> user_id;
		HashValType nonce;
		HashValType response;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id & nonce & response;
		}
	};

	template<> struct Body<MessageType::VerifyTokenResp>
	{
		HashValType token;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & token;
		}
	};
}
