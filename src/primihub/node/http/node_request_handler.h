#ifndef SRC_PRIMIHUB_NODE_HTTP_NODE_REQUEST_HANDLER_H_
#define SRC_PRIMIHUB_NODE_HTTP_NODE_REQUEST_HANDLER_H_
#include <glog/logging.h>
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "src/primihub/node/http/node_http_interface.h"
#include "src/primihub/node/ds_handler.h"
namespace primihub {
using Poco::Timestamp;
using Poco::DateTimeFormatter;
using Poco::DateTimeFormat;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
class TimeRequestHandler: public HTTPRequestHandler {
/// Return a HTML document with the current date and time.
 public:
  TimeRequestHandler() = default;

  void handleRequest(HTTPServerRequest& request, HTTPServerResponse& response) {
    LOG(INFO) << "Request from " + request.clientAddress().toString();

    Timestamp now;
    std::string dt(DateTimeFormatter::format(now, _format));

    response.setChunkedTransferEncoding(true);
    response.setContentType("text/html");

    std::ostream& ostr = response.send();
    ostr << "<html><head><title>"
    "HTTPTimeServer powered by POCO C++ Libraries</title>";
    ostr << "<meta http-equiv=\"refresh\" content=\"5\"></head>";
    ostr << "<body><p style=\"text-align: center; font-size: 48px;\">";
    ostr << dt;
    ostr << "</p></body></html>";
  }

private:
  std::string _format{DateTimeFormat::SORTABLE_FORMAT};
};

class TaskRequestHandler: public HTTPRequestHandler {
 public:
  TaskRequestHandler(void* service_impl);
  virtual ~TaskRequestHandler() = default;

 protected:
  VMNodeHttpInterface* Interface() {return interface_;}
  retcode ExtractRequestData(HTTPServerRequest& request,
                             std::string* payload);

 private:
  VMNodeHttpInterface* interface_{nullptr};
};

// -----------------------------SubmitTask------------------------------------
class SubmitTaskRequestHandler: public TaskRequestHandler {
 public:
  SubmitTaskRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                     HTTPServerResponse& response) override;
};

// -----------------------------ExecuteTask------------------------------------
class ExecuteTaskRequestHandler: public TaskRequestHandler {
 public:
  ExecuteTaskRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------StopTask------------------------------------
class StopTaskRequestHandler: public TaskRequestHandler {
 public:
  StopTaskRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------KillTask------------------------------------
class KillTaskRequestHandler: public TaskRequestHandler {
 public:
  KillTaskRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------FetchTaskStatus-----------------------------
class FetchTaskStatusRequestHandler: public TaskRequestHandler {
 public:
  FetchTaskStatusRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------UpdateTaskStatus-----------------------------
class UpdateTaskStatusRequestHandler: public TaskRequestHandler {
 public:
  UpdateTaskStatusRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------SendData------------------------------------
class SendDataRequestHandler: public TaskRequestHandler {
 public:
  SendDataRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------RecvData------------------------------------
class RecvDataRequestHandler: public TaskRequestHandler {
 public:
  RecvDataRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// -----------------------------ForwardRecvData-------------------------------
class ForwardRecvRequestHandler: public TaskRequestHandler {
 public:
  ForwardRecvRequestHandler(void* service_impl);
  void handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) override;
};

// // -----------------------------NewDataset-------------------------------
class NewDatasetRequestHandler: public TaskRequestHandler {
 public:
  NewDatasetRequestHandler(void* service_impl, DataServiceHandler* ds_handler);
  void handleRequest(HTTPServerRequest& request,
                     HTTPServerResponse& response) override;
 private:
  DataServiceHandler* ref_ds_handler_;
};
} // namespace primihub


#endif  // SRC_PRIMIHUB_NODE_HTTP_NODE_REQUEST_HANDLER_H_
