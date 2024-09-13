
#include <glog/logging.h>
#include <iostream>
#include <memory>
#include <csignal>

#include "absl/flags/flag.h"
#include "absl/flags/parse.h"
#include "absl/memory/memory.h"
#include "absl/strings/str_cat.h"
#include "src/primihub/common/common.h"
#include "src/primihub/common/config/config.h"
#include "src/primihub/common/config/server_config.h"
#include "src/primihub/node/http/node_http_server.h"


ABSL_FLAG(std::string, config, "./config/node.yaml", "config file");

int main(int argc, char** argv) {
  signal(SIGINT, [](int sig) {
      LOG(INFO) << "Node received SIGINT signal, shutting down...";
      exit(0);
  });

  google::InitGoogleLogging(argv[0]);
  FLAGS_colorlogtostderr = true;
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = "./log";
  FLAGS_max_log_size = 10;
  FLAGS_stop_logging_if_full_disk = true;

  absl::ParseCommandLine(argc, argv);
  // int service_port = absl::GetFlag(FLAGS_service_port);
  std::string config_file = absl::GetFlag(FLAGS_config);
  LOG(INFO) << "config_file: " << config_file;
  auto& server_config = primihub::ServerConfig::getInstance();
  auto ret = server_config.initServerConfig(config_file);
  if (ret != primihub::retcode::SUCCESS) {
    LOG(ERROR) << "init server config failed";
    return -1;
  }
  auto& host_config = server_config.getServiceConfig();
  int32_t service_port = host_config.port();
  std::string node_id = host_config.id();
	primihub::HttpNodeServer app(config_file);
  argc = 1;
	return app.run(argc, argv);
}