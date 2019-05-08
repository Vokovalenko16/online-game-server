#include "stdafx.h"
#include "GameService.h"
#include "game/Game.h"
#include "game/GameManager.h"
#include "protocol/Message.h"
#include "protocol/GameCommands.h"
#include "protocol/Types.h"

namespace mserver {
	namespace service {

		struct GameServices::Impl
		{
			using M = protocol::MessageType;

			Impl(net::MessageServer& server, game::GameManager& gameManager)
			{
				server.register_typed_handler(
					[&gameManager](protocol::Body<protocol::MessageType::OnScoreChangedReq>&& req, net::NetSession& session)
				{
					gameManager.update_player_score(req.game_id, req.member_id, req.pass_time, req.pass_point);
					protocol::Body<protocol::MessageType::OnScoreChangedResp> resp;
					resp.isSucceed = true;
					return resp;
				});

				server.register_typed_handler(
					[&gameManager](protocol::Body<protocol::MessageType::OnNotifyCehckpointCountReq>&& req, net::NetSession& session)
				{
					gameManager.set_game_checkpointcount(req.game_id, req.checkpoint_count);
					protocol::Body<protocol::MessageType::OnNotifyCehckpointCountResp> resp;
					resp.isSucceed = true;
					return resp;
				});

				server.register_typed_handler(
					[&gameManager](protocol::Body<protocol::MessageType::OnPlayerConnectReq>&& req, net::NetSession& session)
				{
					bool res = gameManager.process_connect_player(req.game_id, req.member_id, req.running_player_number, req.waiting_time);
					protocol::Body<protocol::MessageType::OnPlayerConnectResp> resp;
					resp.isSucceed = res;
					return resp;
				});

				server.register_typed_handler(
					[&gameManager](protocol::Body<protocol::MessageType::OnPlayerDropReq>&& req, net::NetSession& session)
				{
					bool res = gameManager.process_drop_player(req.game_id, req.member_id, req.running_player_number, req.waiting_time);
					protocol::Body<protocol::MessageType::OnPlayerDropResp> resp;
					resp.isSucceed = res;
					return resp;
				});

				server.register_typed_handler([&gameManager](protocol::Body<protocol::MessageType::UpdateWaitingTimeReq>&& req, net::NetSession&) 
				{
					protocol::Body<protocol::MessageType::UpdateWaitingTimeResp> resp;
					gameManager.update_waiting_time(req.game_id, req.waiting_time);
					return resp;
				});
			}
		};

		GameServices::GameServices(net::MessageServer& server, game::GameManager& gameManager)
		{
			impl_ = std::make_unique<Impl>(server, gameManager);
		}

		GameServices::~GameServices() = default;
	}
}