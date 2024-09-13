#ifndef SRC_PRIMIHUB_NODE_HTTP_NODE_HTTP_SERVER_H_
#define SRC_PRIMIHUB_NODE_HTTP_NODE_HTTP_SERVER_H_
#include <iostream>

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"
#include "src/primihub/node/http/request_handler_factory.h"
#include "src/primihub/common/config/server_config.h"
#include "src/primihub/node/node_impl.h"
#include "src/primihub/node/http/node_http_interface.h"
#include "src/primihub/service/dataset/service.h"
#include "src/primihub/service/dataset/meta_service/factory.h"
namespace primihub {
using Poco::Net::ServerSocket;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerParams;
using Poco::ThreadPool;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class HttpNodeServer: public Poco::Util::ServerApplication {
	/// The main application class.
	///
	/// This class handles command-line arguments and
	/// configuration files.
	/// Start the HTTPTimeServer executable with the help
	/// option (/help on Windows, --help on Unix) for
	/// the available command line options.
	///
	/// To use the sample configuration file (HTTPTimeServer.properties),
	/// copy the file to the directory where the HTTPTimeServer executable
	/// resides. If you start the debug version of the HTTPTimeServer
	/// (HTTPTimeServerd[.exe]), you must also create a copy of the configuration
	/// file named HTTPTimeServerd.properties. In the configuration file, you
	/// can specify the port on which the server is listening (default
	/// 9980) and the format of the date/time string sent back to the client.
	///
	/// To test the TimeServer you can use any web browser (http://localhost:9980/).
public:
	HttpNodeServer(std::unique_ptr<VMNodeHttpInterface> service_impl):
      _helpRequested(false) {}
  HttpNodeServer(const std::string& config_file):
      _helpRequested(false),
      config_file_(config_file){}

	~HttpNodeServer() = default;

protected:
	void initialize(Application& self) {
		// loadConfiguration(); // load default configuration files, if present
		ServerApplication::initialize(self);
	}

	void uninitialize() {
		ServerApplication::uninitialize();
	}

	void defineOptions(OptionSet& options) {
		ServerApplication::defineOptions(options);
		options.addOption(
			Option("help", "h", "display help information on command line arguments")
				.required(false)
				.repeatable(false));
	}

	void handleOption(const std::string& name, const std::string& value) {
		ServerApplication::handleOption(name, value);
		if (name == "help")
			_helpRequested = true;
	}

	void displayHelp() {
		HelpFormatter helpFormatter(options());
		helpFormatter.setCommand(commandName());
		helpFormatter.setUsage("OPTIONS");
		helpFormatter.setHeader("A web server that serves "
                            "the current date and time.");
		helpFormatter.format(std::cout);
	}

	int main(const std::vector<std::string>& args) override;

private:
	bool _helpRequested;
  std::string config_file_;
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_HTTP_NODE_HTTP_SERVER_H_
