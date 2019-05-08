// master_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "db/Database.h"
#include "net/TlsListener.h"
#include "net/TlsConnection.h"
#include "net/TlsContext.h"
#include "net/TlsHandshaker.h"
#include "net/MessageServer.h"
#include "http/Handler.h"
#include "http/Server.h"
#include "http/StaticContentHandler.h"
#include "http/GameCenterConnection.h"
#include "job/Manager.h"
#include "game/GameManager.h"
#include "util/FileUtil.h"
#include "util/io_worker.h"
#include "logger/Logger.h"
#include "http/Authorizer.h"

#include "service/CommonClientServices.h"
#include "service/GameAdminServices.h"
#include "service/MasterServerConfig.h"
#include "service/UserManageServices.h"
#include "service/LoadGameServices.h"
#include "service/GameService.h"
#include "service/LevelServices.h"
#include "service/TokenServices.h"
#include "service/ItemServices.h"
#include "service/ScoreServices.h"
#include "service/ChildServerAdminServices.h"

#include "protocol/base16.h"

#include <fstream>
#include <vector>
#include <memory>
#include <chrono>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/program_options.hpp>
#include <boost/process.hpp>
#include <boost/filesystem.hpp>
#include <boost/noncopyable.hpp>
#include <CryptoPP/osrng.h>

namespace mserver {
	namespace master_server {

		namespace po = boost::program_options;
		namespace bp = boost::process;
		namespace fs = boost::filesystem;
		using boost::asio::ip::tcp;

		const char DlServerExecutableFileName[] =
#if defined (WIN32)
			"download_server.exe"
#elif defined (LINUX)
			"download_server"
#endif
			;

		service::MasterServerConfig load_config()
		{
			service::MasterServerConfig cfg;

			po::options_description desc("Allowed options");
			desc.add_options()
				("public_ip_addr", po::value(&cfg.public_ip_addr))
				("server_port", po::value(&cfg.server_port)->default_value(4801))
				("download_server_http_port", po::value(&cfg.download_server_http_port)->default_value(4446))
				("http_port", po::value(&cfg.http_port)->default_value(4802))
				("idle_timeout", po::value(&cfg.idle_timeout_seconds)->default_value(120))
				("web_root", po::value(&cfg.web_root_dir)->default_value("web"))
				("game_dir", po::value(&cfg.game_dir)->default_value("game"))
				("temp_dir", po::value(&cfg.temp_dir)->default_value("temp"))
				("levels_dir", po::value(&cfg.levels_dir)->default_value("levels"))
				("game_port_min", po::value(&cfg.game_port_min)->default_value(28000))
				("game_port_max", po::value(&cfg.game_port_max)->default_value(28100))
				("db_path", po::value(&cfg.db_path)->default_value("master_server.db"))
				("dl_server_path", po::value(&cfg.dl_server_path))
				("game_center_addr", po::value(&cfg.game_center_addr))
				("game_center_port", po::value(&cfg.game_center_port))
				("game_center_target", po::value(&cfg.game_center_target))
				("copy_game", po::value(&cfg.b_copy_game)->default_value(true))
				("admin_password", po::value(&cfg.admin_password));

			po::variables_map vm;
			po::store(po::parse_config_file<char>("master_server.config", desc), vm);
			po::notify(vm);

			return cfg;
		}

		net::TlsContext load_certificates()
		{
			auto ctx = net::TlsContext::create();

			std::vector<std::uint8_t> cert_chain;
			util::load_file("game_server.crt", cert_chain);
			util::load_file("CA.crt", cert_chain);
			ctx.use_certificate_chain(cert_chain.data(), cert_chain.size());

			std::vector<std::uint8_t> priv_key;
			util::load_file("game_server.key", priv_key);
			ctx.use_private_key(priv_key.data(), priv_key.size());

			return ctx;
		}

		class DownloadServerLauncher : boost::noncopyable
		{
		public:
			DownloadServerLauncher(const service::MasterServerConfig& cfg, const std::string& security_token)
				: cfg_(cfg)
				, security_token_(security_token)
			{
				monitor_thread_ = std::thread([this]() { run(); });
			}

			~DownloadServerLauncher()
			{
				{
					std::unique_lock<std::mutex> lock(mutex_);
					stop_ = true;

					if (child_process_.valid())
					{
						std::error_code ec;
						child_process_.terminate(ec);
					}
				}
				cond_.notify_all();

				monitor_thread_.join();
			}

		private:
			const service::MasterServerConfig& cfg_;
			std::string security_token_;
			std::mutex mutex_;
			std::condition_variable cond_;
			std::thread monitor_thread_;
			bool stop_ = false;
			bp::child child_process_;

			void run();
			bp::child launch_child();
		};

		void DownloadServerLauncher::run()
		{
			bool was_child_running = false;
			bool delay = false;

			for (;;)
			{
				{
					std::unique_lock<std::mutex> lock(mutex_);
					if (stop_)
						return;

					if (was_child_running)
						log::write(log::Level::Warning, "download server exited with code %||.", child_process_.exit_code());
					child_process_ = {};
					was_child_running = false;

					if (delay)
					{
						if (cond_.wait_until(lock, std::chrono::steady_clock::now() + std::chrono::seconds(30), [this]() { return stop_; }))
							return;
					}

					try
					{
						log::write(log::Level::Info, "starting download server.");

						child_process_ = launch_child();

						was_child_running = true;
						log::write(log::Level::Info, "download server started.");
					}
					catch (std::exception& err)
					{
						log::write(log::Level::Error, "failed to launch download server: %||",
							err.what());
						delay = true;
						continue;
					}
				}

				try {
					child_process_.wait();
				}
				catch (std::exception& err) {
						log::write(log::Level::Warning, "error while monitoring download server: %||.", err.what());
				}

				delay = true;
			}
		}

		bp::child DownloadServerLauncher::launch_child()
		{
			std::string download_server_admin_port;

			{
				std::ostringstream port_ss;
				port_ss << cfg_.download_server_http_port;
				download_server_admin_port = port_ss.str();
			}

			return bp::child{
				(fs::path(cfg_.dl_server_path) / DlServerExecutableFileName).string(),
				"--work_path",
				fs::canonical(fs::path(cfg_.dl_server_path)).string(),
				"--http_port",
				download_server_admin_port,
				"--master_token",
				security_token_
			};
		}

		void run()
		{
			log::init("logs", log::Level::Info);

			auto cfg = load_config();

			boost::asio::io_service io;

			std::string download_server_security_token;

			{
				CryptoPP::AutoSeededRandomPool random;
				std::array<std::uint8_t, 32> raw_data;
				random.GenerateBlock((byte*)raw_data.data(), raw_data.size());
				download_server_security_token = protocol::to_base16(raw_data);
			}

			DownloadServerLauncher download_server{ cfg, download_server_security_token };
			
			// 
			auto http_handler = http::Handler::create(
				{},
				cfg.admin_password.empty() ? nullptr : http::create_basic_authorizer("admin", cfg.admin_password));
			auto msg_server = net::MessageServer::create(
				io,
				std::chrono::seconds(cfg.idle_timeout_seconds));

			auto database = db::Database::create(cfg.db_path);
			auto job_manager = job::Manager::create(http_handler, io);
			auto game_manager = game::GameManager::create(http_handler, database, io);

			// Create Game Center Connection
			http::GameCenterConnection game_center_connection;
			if (!cfg.game_center_addr.empty())
				game_center_connection = http::GameCenterConnection {
					io,
					boost::asio::ip::address::from_string(cfg.game_center_addr),
					cfg.game_center_port,
					cfg.game_center_target };

			// Register services
			service::CommonClientServices common_services{ msg_server };
			http::StaticContentHandler static_contents{ http_handler, cfg.web_root_dir };
			service::GameAdminServices game_admin_services{ cfg, http_handler, job_manager, game_manager };
			service::UserManageServices user_manage_services{ database, http_handler, msg_server, game_center_connection };
			service::LoadGameServices load_game_services{ cfg, msg_server, game_manager };
			//service::GameServices game_services{msg_server, game_manager, game_center_connection};
			service::GameServices game_services{ msg_server, game_manager };
			service::LevelService level_service{ cfg, database, http_handler, job_manager, msg_server };
			service::TokenServices token_service{ database, msg_server };
			service::ItemServices item_service{ io, database, http_handler, msg_server, game_center_connection };
			service::ScoreServices score_service{ database, msg_server, game_manager };

			service::ChildServerAdminServices child_admin{ io, http_handler };
			child_admin.add_child_server("download",
				download_server_security_token,
				boost::asio::ip::address_v4::from_string("127.0.0.1"),
				cfg.download_server_http_port);

			auto tls_ctx = load_certificates();

			// Create HTTP server
			auto http_server = http::Server::create(
				io,
				tcp::endpoint(tcp::v4(), cfg.http_port),
				tls_ctx,
				http_handler);

			// Create message server
			auto handshaker = mserver::net::TlsHandshaker::create(
				io,
				std::chrono::seconds(30),
				[&](mserver::net::TlsConnection conn)
			{
				msg_server.service(std::move(conn));
			});

			auto listener = mserver::net::TlsListener::create(
				io,
				cfg.server_port,
				tls_ctx,
				[&](mserver::net::TlsConnection conn)
			{
				handshaker.begin_handshake(std::move(conn));
			});

			util::run_io_workers(io, 4);
		}
	}
}

int main(int argc, const char* argv[])
{
	if (argc >= 2)
		boost::filesystem::current_path(argv[1]);

	mserver::master_server::run();

    return 0;
}

