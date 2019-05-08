#pragma once

#include <cstdint>
#include <array>
#include "detail/NetString.h"

namespace protocol {

	const size_t MaxMessageLen = 16384;
	const size_t MessageHeaderLen = 12;

	const std::uint32_t HashValLen = 32;
	const std::uint32_t MaxFilePathLen = 256;
	const std::size_t MaxStringLen = 256;
	const std::uint32_t BlockSize = 4096;

	using FileBlock = std::array<uint8_t, BlockSize>;
	using HashValType = std::array<uint8_t, HashValLen>;
	using IDType = std::uint64_t;
	using IndexType = std::uint32_t;
	using FileLengthType = std::uint32_t;

	enum class MessageType : std::uint32_t
	{
		Ping = 1,
		ErrorResponse,	// [std::string error_desc]

		GetLoginChallengeReq,
		GetLoginChallengeResp,
		LoginReq,
		LoginResp,
		TokenReq,
		TokenResp,
		VerifyTokenReq,
		VerifyTokenResp,

		// download server messages
		GetLatestReleaseReq = 20001,
		GetLatestReleaseResponse,
		GetFileIDsInReleaseReq,
		GetFileIDsInReleaseResponse,
		GetFileInfoReq,
		GetFileInfoResponse,
		GetFileBlockInfoReq,
		GetFileBlockInfoResponse,
		GetFileDataReq,
		GetFileDataResponse,
		GetLevelInfoReq,
		GetLevelInfoResp,
		GetLevelBlockInfoReq,
		GetLevelBlockInfoResp,
		GetLevelDataReq,
		GetLevelDataResp,

		// game client messages
		GetGameListRequest = 10001,
		GetGameListResponse,
		GetGameInfoReq,
		GetGameInfoResp,
		GetGameRoomCountReq,
		GetGameRoomCountResp,
		OnScoreChangedReq,
		OnScoreChangedResp,
		OnUserInfoChangedReq,
		OnUserInfoChangedResp,
		OnNotifyCehckpointCountReq,
		OnNotifyCehckpointCountResp,
		OnNotifyLoginNameReq,
		OnNotifyLoginNameResp,
		OnPlayerConnectReq,
		OnPlayerConnectResp,
		OnPlayerDropReq,
		OnPlayerDropResp,
		GetItemInfoReq,
		GetItemInfoResp,
		GetItemInfoByCategoryReq,
		GetItemInfoByCategoryResp,
		DecreaseItemCntReq,
		DecreaseItemCntResp,
		PurchaseItemReq,
		PurchaseItemResp,
		GetItemListReq,
		GetItemListResp,
		GetHistoryReq,
		GetHistoryResp,
		UpdateGamePassTimeReq,
		UpdateGamePassTimeResp,
		GetScoreAndRankReq,
		GetScoreAndRankResp,
		GetRanksReq,
		GetRanksResp,
		GetMyGameInfoReq,
		GetMyGameInfoResp,
		GetTotalMemberCountReq,
		GetTotalMemberCountResp,
		UpdateWaitingTimeReq,
		UpdateWaitingTimeResp,
		GetBalanceReq,
		GetBalanceResp,
		GetLiveGameRankReq,
		GetLiveGameRankResp
	};

	enum class GameMode : std::uint32_t
	{
		RealTime = 1,
		Expert,
		Treasure
	};

	template<MessageType> struct Body;

	struct GameInfo
	{
		NetString<20> ip_addr;
		std::uint32_t port;
		NetString<10> level_name;
		NetString<40> game_name;
		GameMode game_mode = GameMode::RealTime;
		NetString<16> start_time;
		NetString<16> end_time;
		std::uint32_t running_player_number;
		std::uint32_t waiting_time;
		std::uint32_t game_id;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & ip_addr;
			ar & port;
			ar & level_name;
			ar & game_name;
			ar & game_mode;
			ar & start_time;
			ar & end_time;
			ar & running_player_number;
			ar & waiting_time;
			ar & game_id;
		}
	};

	struct ItemInfo
	{
		std::uint64_t id;
		NetString<30>  name;
		std::uint16_t price;
		std::uint64_t category_id;
		NetString<200> comment;
		std::uint32_t duration;
		std::uint32_t cooldown;
		NetString<50> img_path;
		std::uint16_t indexInCategory;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & id;
			ar & name;
			ar & price;
			ar & category_id;
			ar & comment;
			ar & duration;
			ar & cooldown;
			ar & img_path;
			ar & indexInCategory;
		}
	};

	struct EquipDisplay
	{
		NetString<30> login_name;
		NetString<30> equip_name;
		std::uint32_t equip_cnt;
		NetString<30> name;
		std::uint16_t price;
		std::uint64_t category_id;
		NetString<200> comment;
		std::uint32_t duration;
		std::uint32_t cooldown;
		NetString<50> img_path;
		std::uint32_t indexInCategory;
		NetString<30> category_name;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & login_name;
			ar & equip_name;
			ar & equip_cnt;
			ar & name;
			ar & price;
			ar & category_id;
			ar & comment;
			ar & duration;
			ar & cooldown;
			ar & img_path;
			ar & indexInCategory;
			ar & category_name;
		}
	};

	struct GameHistoryInfo
	{
		std::uint64_t game_id;
		std::uint64_t memeber_id;
		NetString<30> pass_time;
		std::uint32_t rank;
		NetString<30> level_name;
		NetString<30> game_name;
		GameMode game_mode;
		bool active;
		NetString<30> start_time;
		NetString<30> end_time;
		std::uint32_t checkpoint_count;
		std::uint32_t game_difficulty;
		NetString<30> login_name;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & memeber_id;
			ar & pass_time;
			ar & rank;
			ar & level_name;
			ar & game_name;
			ar & game_mode;
			ar & active;
			ar & start_time;
			ar & end_time;
			ar & checkpoint_count;
			ar & game_difficulty;
			ar & login_name;
		}
	};

	struct NameRank
	{
		NetString<30> login_name;
		std::uint64_t rank;
		std::uint32_t score;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & login_name;
			ar & rank;
			ar & score;
		}
	};

	struct LiveGameRank
	{
		std::uint32_t rank;
		NetString<30> login_id;
		std::uint16_t grade;
		std::uint32_t passtime;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & rank;
			ar & login_id;
			ar & grade;
			ar & passtime;
		}
	};
}
