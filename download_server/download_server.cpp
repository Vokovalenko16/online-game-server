// download_server.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "net/TlsListener.h"
#include "net/TlsConnection.h"
#include "net/TlsContext.h"
#include "net/TlsHandshaker.h"
#include "net/MessageServer.h"
#include "http/Handler.h"
#include "http/Server.h"
#include "http/StaticContentHandler.h"
#include "service/CommonClientServices.h"
#include "service/AdminClientServices.h"
#include "db/Database.h"
#include "util/FileUtil.h"
#include "util/io_worker.h"
#include "logger/Logger.h"
#include "job/Manager.h"

#include <memory>
#include <chrono>
#include <fstream>
#include <vector>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

namespace mserver {
	namespace download_server {

		namespace po = boost::program_options;
		using boost::asio::ip::tcp;

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

		struct CommandLineConfig
		{
			std::string work_path;
			int http_port;
			std::string master_token;
		};

		CommandLineConfig load_cmdline_config(int argc, const char* argv[])
		{
			CommandLineConfig cfg;

			po::options_description desc("Allowed options");
			desc.add_options()
				("work_path", po::value(&cfg.work_path)->default_value(""))
				("http_port", po::value(&cfg.http_port)->default_value(-1))
				("master_token", po::value(&cfg.master_token)->default_value(""));

			po::variables_map vm;
			po::store(po::parse_command_line(argc, argv, desc), vm);
			po::notify(vm);

			return cfg;
		}

		struct ServerConfig
		{
			int server_port;
			int http_port;
			int idle_timeout_seconds;
			std::string db_path;
		};

		ServerConfig load_config()
		{
			ServerConfig cfg;

			po::options_description desc("Allowed options");
			desc.add_options()
				("server_port", po::value(&cfg.server_port)->default_value(4445))
				("http_port", po::value(&cfg.http_port)->default_value(4446))
				("idle_timeout", po::value(&cfg.idle_timeout_seconds)->default_value(120))
				("db_path", po::value(&cfg.db_path)->default_value("download_server.db"));

			po::variables_map vm;
			po::store(po::parse_config_file<char>("download_server.config", desc), vm);
			po::notify(vm);

			return cfg;
		}

		void run(int argc, const char* argv[])
		{
			auto cmdline_cfg = load_cmdline_config(argc, argv);
			if (!cmdline_cfg.work_path.empty())
				boost::filesystem::current_path(cmdline_cfg.work_path);

			log::init("logs", log::Level::Info);

			auto cfg = load_config();
			boost::asio::io_service io;

			auto msg_server = net::MessageServer::create(io, std::chrono::seconds(cfg.idle_timeout_seconds));
			auto http_handler = http::Handler::create(cmdline_cfg.master_token);

			// Initialize services
			auto database = db::Database::create(cfg.db_path);
			job::Manager job_manager = job::Manager::create(http_handler, io);
			service::CommonClientServices common_service(msg_server, database);
			service::AdminClientService admin_service(http_handler, database, job_manager);

			// Create message server
			auto handshaker = net::TlsHandshaker::create(io, std::chrono::seconds(60), [&](net::TlsConnection conn)
			{
				msg_server.service(std::move(conn));
			});

			auto tls_ctx = load_certificates();

			auto listener = net::TlsListener::create(io, cfg.server_port, tls_ctx, [&](net::TlsConnection conn)
			{
				handshaker.begin_handshake(std::move(conn));
			});

			// Create HTTP server
			auto http_server = http::Server::create(
				io,
				tcp::endpoint(tcp::v4(), cmdline_cfg.http_port > 0 ? cmdline_cfg.http_port : cfg.http_port),
				tls_ctx,
				http_handler);

			util::run_io_workers(io, 4);
		}
	}
}

int main(int argc, const char* argv[])
{
	mserver::download_server::run(argc, argv);
    return 0;
}

