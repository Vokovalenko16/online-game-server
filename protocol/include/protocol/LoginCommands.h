#pragma once

#include <array>
#include "Types.h"

namespace protocol {

	template<> struct Body<MessageType::GetLoginChallengeReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{}
	};

	template<> struct Body<MessageType::GetLoginChallengeResp>
	{
		NetString<HashValLen * 2> challenge_nonce;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & challenge_nonce;
		}
	};

	template<> struct Body<MessageType::LoginReq>
	{
		NetString<256> login_name;
		NetString<HashValLen * 2> login_response;

		template<class Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & login_name;
			ar & login_response;
		}
	};

	enum class LoginVerifyResult : std::uint32_t
	{
		Succeeded,
		NoSuchUser,
		PasswordIncorrect,
	};

	template<> struct Body<MessageType::LoginResp>
	{
		LoginVerifyResult result;

		template<class Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & result;
		}
	};

	template<> struct Body<MessageType::OnNotifyLoginNameReq>
	{
		NetString<20> login_name;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & login_name;
		}
	};

	template<> struct Body<MessageType::OnNotifyLoginNameResp>
	{
		std::uint64_t user_id;
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
			ar & isSucceed;
		}
	};
}
