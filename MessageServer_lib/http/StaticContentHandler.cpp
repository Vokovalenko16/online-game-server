#include "stdafx.h"
#include "StaticContentHandler.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/beast/http/vector_body.hpp>

namespace mserver {
	namespace http {

		namespace fs = boost::filesystem;
		namespace beast = boost::beast;

		std::string mime_type(const fs::path& path)
		{
			using boost::beast::iequals;
			auto const ext = path.extension().string();
			if (iequals(ext, ".htm"))  return "text/html";
			if (iequals(ext, ".html")) return "text/html";
			if (iequals(ext, ".php"))  return "text/html";
			if (iequals(ext, ".css"))  return "text/css";
			if (iequals(ext, ".txt"))  return "text/plain";
			if (iequals(ext, ".js"))   return "application/javascript";
			if (iequals(ext, ".json")) return "application/json";
			if (iequals(ext, ".xml"))  return "application/xml";
			if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
			if (iequals(ext, ".flv"))  return "video/x-flv";
			if (iequals(ext, ".png"))  return "image/png";
			if (iequals(ext, ".jpe"))  return "image/jpeg";
			if (iequals(ext, ".jpeg")) return "image/jpeg";
			if (iequals(ext, ".jpg"))  return "image/jpeg";
			if (iequals(ext, ".gif"))  return "image/gif";
			if (iequals(ext, ".bmp"))  return "image/bmp";
			if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
			if (iequals(ext, ".tiff")) return "image/tiff";
			if (iequals(ext, ".tif"))  return "image/tiff";
			if (iequals(ext, ".svg"))  return "image/svg+xml";
			if (iequals(ext, ".svgz")) return "image/svg+xml";
			return "application/text";
		}

		StaticContentHandler::StaticContentHandler(Handler& handler, const boost::filesystem::path& root_dir)
		{
			root_dir_ = root_dir;
			scan_directory(handler, root_dir_, "");
		}

		void StaticContentHandler::scan_directory(Handler& handler, const boost::filesystem::path& physical_path, const std::string& relative_path)
		{
			for (auto iter = fs::directory_iterator(physical_path); iter != fs::directory_iterator(); ++iter)
			{
				auto child_relative_path = relative_path + "/" + iter->path().filename().string();

				switch (iter->status().type())
				{
				case fs::file_type::directory_file:
					scan_directory(handler, iter->path(), child_relative_path);
					break;

				case fs::file_type::regular_file:
					add_file(handler, iter->path(), child_relative_path);
					if (iter->path().filename().string() == "index.html")
						add_file(handler, iter->path(), relative_path + "/");
					break;
				}
			}
		}

		void StaticContentHandler::add_file(Handler& handler, const boost::filesystem::path& physical_path, const std::string& relative_path)
		{
			handler.register_resource(
				relative_path,
				[this, content_type = mime_type(physical_path), physical_path](Request&& request, Response& response)
				{
					if (request.method() != Verb::get)
						return Handler::bad_request(request, "Invalid method.", response);

					response.set(Field::content_type, content_type);
					fs::fstream file;
					file.open(physical_path, std::ios::in | std::ios::binary);
					if (!file.good())
						return Handler::server_error(request, "Failed to open resource.", response);

					file.seekg(0, std::ios::end);
					auto len = (size_t)file.tellg();
					file.seekg(0, std::ios::beg);

					auto& body = response.body();
					body.resize(len);
					file.read(body.data(), len);
					if (file.fail())
						return Handler::server_error(request, "Failed to read resource.", response);
				});
		}
	}
}
