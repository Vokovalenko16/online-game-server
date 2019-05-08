#include "stdafx.h"
#include "ScoreServices.h"
#include "game/Game.h"
#include "game/GameManager.h"
#include "protocol/Message.h"
#include "protocol/GameCommands.h"
#include "protocol/Types.h"

#include "db/GameHistoryDB.h"
#include "db/Member.h"

#include <stdexcept>
#include <boost/format.hpp>

namespace mserver {
	namespace service {

		struct ScoreServices::Impl
		{
			using M = protocol::MessageType;

			Impl(db::Database& db, net::MessageServer& server, game::GameManager& gameManager)
			{
				server.register_typed_handler([db](protocol::Body<M::GetHistoryReq>&& req, net::NetSession& session)
				{
					protocol::Body<M::GetHistoryResp> resp;
					auto dbSession = db.open();

					soci::transaction trans(dbSession.get_session());

					resp.history_records = db::GameHistoryDB::find_by_memeber_id(dbSession, req.member_id);

					trans.commit();

					return resp;
				});

				server.register_typed_handler([db](protocol::Body<M::UpdateGamePassTimeReq>&& req, net::NetSession& session)
				{
					protocol::Body<M::UpdateGamePassTimeResp> resp;
					auto dbSession = db.open();

					soci::transaction trans(dbSession.get_session());

					db::GameHistoryDB::update_player_pass_time(dbSession, req.game_id, req.member_id, req.passTime);

					trans.commit();

					resp.isSucceed = true;
					return resp;
				});

				server.register_typed_handler([db](protocol::Body<M::GetScoreAndRankReq>&& req, net::NetSession& session) {

					auto dbSession = db.open();

					soci::transaction trans(dbSession.get_session());

					auto resp = db::Member::find_score_and_rank(dbSession, req.member_id);

					trans.commit();
					
					if (resp == boost::none)
						throw std::invalid_argument((boost::format("Not registered member_id(%||) is used.") % req.member_id).str());

					return resp.value();
				});

				server.register_typed_handler([db](protocol::Body<M::GetRanksReq>&& req, net::NetSession& session) {
					auto dbSession = db.open();

					soci::transaction trans(dbSession.get_session());

					auto resp = db::Member::find_ranks(dbSession, req.start, req.record_cnt);

					trans.commit();

					return resp;
				});

				server.register_typed_handler([db](protocol::Body<M::GetTotalMemberCountReq>&& req, net::NetSession& session) {
					auto dbSession = db.open();

					soci::transaction trans{ dbSession.get_session() };

					auto resp = db::Member::find_total_count(dbSession);

					trans.commit();

					return resp;
				});

				server.register_typed_handler([db](protocol::Body<M::GetLiveGameRankReq>&& req, net::NetSession& session) {
					auto dbSession = db.open();
					protocol::Body<M::GetLiveGameRankResp> resp;

					soci::transaction trans{ dbSession.get_session() };

					resp.records = db::GameHistoryDB::find_by_game_id(dbSession, req.game_id);

					trans.commit();

					return resp;
				});
			}
		};

		ScoreServices::ScoreServices(db::Database& db, net::MessageServer& server, game::GameManager& gameManager)
		{
			impl_ = std::make_unique<Impl>(db, server, gameManager);
		}

		ScoreServices::~ScoreServices() = default;
	}
}