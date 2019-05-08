#pragma once
#include "Types.h"
#include <boost/serialization/vector.hpp>

namespace protocol {

	template<> struct Body<MessageType::GetGameRoomCountReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
		}
	};

	template<> struct Body<MessageType::GetGameRoomCountResp>
	{
		protocol::IndexType room_count;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & room_count;
		}
	};

	template<> struct Body<MessageType::GetGameInfoReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
		}
	};

	template<> struct Body<MessageType::GetGameInfoResp>
	{
		std::vector<GameInfo> gameInfos;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & gameInfos;
		}
	};

	template<> struct Body<MessageType::OnScoreChangedReq>
	{
		std::uint64_t game_id;
		std::uint64_t member_id;
		std::uint32_t pass_point;
		std::uint32_t pass_time;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & member_id;
			ar & pass_point;
			ar & pass_time;
		}
	};

	template<> struct Body<MessageType::OnScoreChangedResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::OnNotifyCehckpointCountReq>
	{
		std::uint64_t game_id;
		std::uint32_t checkpoint_count;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & checkpoint_count;
		}
	};

	template<> struct Body<MessageType::OnNotifyCehckpointCountResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::OnPlayerConnectReq>
	{
		std::uint64_t game_id;
		std::uint64_t member_id;
		std::uint32_t running_player_number;
		std::uint32_t waiting_time;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & member_id;
			ar & running_player_number;
			ar & waiting_time;
		}
	};

	template<> struct Body<MessageType::OnPlayerConnectResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::OnPlayerDropReq>
	{
		std::uint64_t game_id;
		std::uint64_t member_id;
		std::uint32_t running_player_number;
		std::uint32_t waiting_time;

		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & member_id;
			ar & running_player_number;
			ar & waiting_time;
		}
	};

	template<> struct Body<MessageType::OnPlayerDropResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::GetItemInfoReq>
	{
		NetString<30> user_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
		}
	};

	template<> struct Body<MessageType::GetItemInfoResp>
	{
		std::vector<EquipDisplay> equips;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & equips;
		}
	};

	template<> struct Body<MessageType::GetItemInfoByCategoryReq>
	{
		NetString<30> user_id;
		std::uint64_t category_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
			ar & category_id;
		}
	};

	template<> struct Body<MessageType::GetItemInfoByCategoryResp>
	{
		std::vector<EquipDisplay> equips;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & equips;
		}
	};

	template<> struct Body<MessageType::DecreaseItemCntReq>
	{
		NetString<30> user_id;
		NetString<30> itemName;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
			ar & itemName;
		}
	};

	template<> struct Body<MessageType::DecreaseItemCntResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::PurchaseItemReq>
	{
		NetString<30> user_id;
		NetString<30> equip_name;
		std::uint32_t equip_cnt;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
			ar & equip_name;
			ar & equip_cnt;
		}
	};

	template<> struct Body<MessageType::PurchaseItemResp>
	{
		bool isSucceed;
		NetString<50> message;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
			ar & message;
		}
	};

	template<> struct Body<MessageType::GetItemListReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
		}
	};

	template<> struct Body<MessageType::GetItemListResp>
	{
		std::vector<ItemInfo> equip_kinds;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & equip_kinds;
		}
	};

	template<> struct Body<MessageType::GetHistoryReq>
	{
		std::uint64_t member_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & member_id;
		}
	};

	template<> struct Body<MessageType::GetHistoryResp>
	{
		std::vector<GameHistoryInfo> history_records;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & history_records;
		}
	};

	template<> struct Body<MessageType::UpdateGamePassTimeReq>
	{
		std::uint64_t game_id;
		std::uint64_t member_id;
		std::uint32_t passTime;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & member_id;
			ar & passTime;
		}
	};

	template<> struct Body<MessageType::UpdateGamePassTimeResp>
	{
		bool isSucceed;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & isSucceed;
		}
	};

	template<> struct Body<MessageType::GetScoreAndRankReq>
	{
		std::uint64_t member_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & member_id;
		}
	};

	template<> struct Body<MessageType::GetScoreAndRankResp>
	{
		std::uint32_t score;
		std::uint32_t rank;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & score;
			ar & rank;
		}
	};

	template<> struct Body<MessageType::GetRanksReq>
	{
		std::uint32_t start;
		std::uint32_t record_cnt;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & start;
			ar & record_cnt;
		}
	};

	template<> struct Body<MessageType::GetRanksResp>
	{
		std::vector<protocol::NameRank> records;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & records;
		}
	};

	template<> struct Body<MessageType::GetMyGameInfoReq>
	{
		std::uint64_t game_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
		}
	};

	template<> struct Body<MessageType::GetMyGameInfoResp>
	{
		GameInfo game;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game;
		}
	};

	template<> struct Body<MessageType::GetTotalMemberCountReq>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
		}
	};

	template<> struct Body<MessageType::GetTotalMemberCountResp>
	{
		std::uint64_t total_cnt;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & total_cnt;
		}
	};

	template<> struct Body<MessageType::UpdateWaitingTimeReq>
	{
		std::uint64_t game_id;
		std::uint32_t waiting_time;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
			ar & waiting_time;
		}
	};

	template<> struct Body<MessageType::UpdateWaitingTimeResp>
	{
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{}
	};

	template<> struct Body<MessageType::GetBalanceReq>
	{
		NetString<20> user_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & user_id;
		}
	};

	template<> struct Body<MessageType::GetBalanceResp>
	{
		std::uint32_t balance;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & balance;
		}
	};


	template<> struct Body<MessageType::GetLiveGameRankReq>
	{
		std::uint64_t game_id;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & game_id;
		}
	};

	template<> struct Body<MessageType::GetLiveGameRankResp>
	{
		std::vector<LiveGameRank> records;
		template<typename Ar> void serialize(Ar& ar, unsigned int /*version*/)
		{
			ar & records;
		}
	};
}