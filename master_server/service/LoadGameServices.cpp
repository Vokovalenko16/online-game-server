#include "stdafx.h"
#include "LoadGameServices.h"
#include "MasterServerConfig.h"
#include "game/Game.h"
#include "game/GameManager.h"
#include "protocol/Message.h"
#include "protocol/GameCommands.h"
#include "protocol/Types.h"

#include <stdexcept>
#include <boost/format.hpp>

namespace mserver {
	namespace service {
		
		struct LoadGameServices::Impl
		{
			using M = protocol::MessageType;

			Impl(const MasterServerConfig& config, net::MessageServer& server, game::GameManager& gameManager)
			{
				server.register_typed_handler(
					[&gameManager](protocol::Body<protocol::MessageType::GetGameRoomCountReq>&& req, net::NetSession& session)
				{
					protocol::Body<protocol::MessageType::GetGameRoomCountResp> resp;
					resp.room_count = gameManager.get_running_games_count();
					return resp;
				});

				server.register_typed_handler([&gameManager, pub_addr = config.public_ip_addr](protocol::Body<M::GetGameInfoReq>&& req, net::NetSession& session)
				{
					auto& games = gameManager.get_running_games();
					protocol::Body<M::GetGameInfoResp> resp;

					for (const auto& game : games)
					{
						protocol::GameInfo gameInfo;
						gameInfo.ip_addr = pub_addr;
						gameInfo.port = game.second.port();
						gameInfo.level_name = game.second.level_name();
						gameInfo.game_name = game.second.game_name();
						gameInfo.game_mode = game.second.game_mode();
						gameInfo.start_time = game.second.start_time();
						gameInfo.end_time = game.second.end_time();
						gameInfo.running_player_number = game.second.running_player_number();
						gameInfo.waiting_time = game.second.waiting_time();
						gameInfo.game_id = (std::uint32_t)game.second.game_id();
						resp.gameInfos.push_back(gameInfo);
					}

					return resp;
				});

				server.register_typed_handler([&gameManager, pub_addr = config.public_ip_addr](protocol::Body<M::GetMyGameInfoReq>&& req, net::NetSession& session)
				{
					auto& games = gameManager.get_running_games();
					protocol::Body<M::GetMyGameInfoResp> resp;
					
					auto my_game = games.find(req.game_id);

					if (my_game == games.end())
						throw std::runtime_error((boost::format{ "Your game is not registered to master server. game_id: %||" } % req.game_id).str());

					resp.game.ip_addr = pub_addr;
					resp.game.port = my_game->second.port();
					resp.game.level_name = my_game->second.level_name();
					resp.game.game_name = my_game->second.game_name();
					resp.game.game_mode = my_game->second.game_mode();
					resp.game.start_time = my_game->second.start_time();
					resp.game.end_time = my_game->second.end_time();

					return resp;
				});
			}
		};

		LoadGameServices::LoadGameServices(const MasterServerConfig& config, net::MessageServer& server, game::GameManager& gameManager)
		{
			impl_ = std::make_unique<Impl>(config, server, gameManager);
		}

		LoadGameServices::~LoadGameServices() = default;
	}
}