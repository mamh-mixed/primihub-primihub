#ifndef SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_FACTORY_H_
#define SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_FACTORY_H_
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "src/primihub/node/http/node_request_handler.h"
#include "src/primihub/node/ds_handler.h"
namespace primihub {
using Poco::Net::HTTPRequestHandlerFactory;

class NodeRequestHandlerFactory: public HTTPRequestHandlerFactory {
public:
	NodeRequestHandlerFactory(void* service_impl) :
      service_impl_(service_impl) {
    auto interface_ = reinterpret_cast<VMNodeHttpInterface*>(service_impl);
    auto ptr = interface_->ServerImpl()->GetDatasetService();
    ds_handler_ = std::make_unique<DataServiceHandler>(ptr.get());
  }

	HTTPRequestHandler* createRequestHandler(const HTTPServerRequest& request) {
		if (request.getURI() == "/") {
      return new TimeRequestHandler();
    } else if (request.getURI() == "/primihub/SubmitTask") {
      LOG(INFO) << "/primihub/SubmitTask";
      return new SubmitTaskRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/ExecuteTask") {
      return new ExecuteTaskRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/StopTask") {
      return new StopTaskRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/KillTask") {
      return new KillTaskRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/FetchTaskStatus") {
      return new FetchTaskStatusRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/UpdateTaskStatus") {
      return new UpdateTaskStatusRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/Send") {
      return new SendDataRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/Recv") {
      return new RecvDataRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/ForwardRecv") {
      return new ForwardRecvRequestHandler(service_impl_);
    } else if (request.getURI() == "/primihub/NewDataset") {
      return new NewDatasetRequestHandler(service_impl_, ds_handler_.get());
    } else {
      LOG(ERROR) << "invalid uri: " << request.getURI();
      return nullptr;
    }
	}
 private:
  std::string format_;
  void* service_impl_{nullptr};
  std::unique_ptr<DataServiceHandler> ds_handler_{nullptr};
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_FACTORY_H_
