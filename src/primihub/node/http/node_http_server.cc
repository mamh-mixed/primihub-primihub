#include "src/primihub/node/http/node_http_server.h"
namespace primihub {
int HttpNodeServer::main(const std::vector<std::string>& args) {
  // get parameters from configuration file
  auto& server_config = primihub::ServerConfig::getInstance();
  auto& host_config = server_config.getServiceConfig();
  int32_t service_port = host_config.port();
  int maxQueued  = config().getInt("HTTPTimeServer.maxQueued", 100);
  int maxThreads = config().getInt("HTTPTimeServer.maxThreads", 16);
  ThreadPool::defaultPool().addCapacity(maxThreads);

  // HTTPServerParams* pParams = new HTTPServerParams;
  auto pParams = new HTTPServerParams;
  pParams->setMaxQueued(maxQueued);
  pParams->setMaxThreads(maxThreads);
  LOG(INFO) << "maxQueued: " << maxQueued << " maxThreads: " << maxThreads;
  auto config_file = server_config.getConfigFile();
  auto& node_cfg = server_config.getNodeConfig();
  auto& meta_service_cfg = node_cfg.meta_service_config;
  auto meta_service =
      primihub::service::MetaServiceFactory::Create(
          meta_service_cfg.mode, meta_service_cfg.host_info);
  // service for dataset manager
  using DatasetService = primihub::service::DatasetService;
  auto dataset_manager = std::make_shared<DatasetService>(
      std::move(meta_service));
  auto node_impl = std::make_unique<VMNodeImpl>(config_file, dataset_manager);
  auto http_interface =
      std::make_unique<VMNodeHttpInterface>(std::move(node_impl));
  // set-up a server socket
  ServerSocket svs(service_port);
  auto interface = reinterpret_cast<void*>(http_interface.get());
  auto factory = std::make_unique<NodeRequestHandlerFactory>(interface);
  // set-up a HTTPServer instance
  HTTPServer srv(factory.release(), svs, pParams);
  // start the HTTPServer
  srv.start();
  LOG(INFO) << "server is listen on http://0.0.0.0:" << std::to_string(service_port);
  // wait for CTRL-C or kill
  waitForTerminationRequest();
  // Stop the HTTPServer
  srv.stop();
  return Application::EXIT_OK;
}
}  // namespace primihub