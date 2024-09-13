#include "src/primihub/node/http/node_request_handler.h"
#include <glog/logging.h>
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/util/proto_log_helper.h"

namespace primihub {
namespace rpc = primihub::rpc;


TaskRequestHandler::TaskRequestHandler(void* service_impl) {
  interface_ = reinterpret_cast<VMNodeHttpInterface*>(service_impl);
}
retcode TaskRequestHandler::ExtractRequestData(HTTPServerRequest& request,
    std::string* payload) {
  std::istream& istr = request.stream();
  auto len = request.getContentLength();
  if (len <= 0) {
    LOG(WARNING) << "no payload found for request";
    return retcode::SUCCESS;
  }
  payload->resize(len);
  istr.read(payload->data(), len);
  return retcode::SUCCESS;
}
// -----------------------------SubmitTask------------------------------------
SubmitTaskRequestHandler::SubmitTaskRequestHandler(void* service_impl) :
    TaskRequestHandler(service_impl) {

}
void SubmitTaskRequestHandler::handleRequest(HTTPServerRequest& request,
                                             HTTPServerResponse& response) {
//
  LOG(INFO) << "SubmitTaskRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::PushTaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::PushTaskReply rpc_resp;
  Interface()->SubmitTask(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);

  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------ExecuteTask------------------------------------
ExecuteTaskRequestHandler::ExecuteTaskRequestHandler(void* service_impl) :
    TaskRequestHandler(service_impl) {
//
}

void ExecuteTaskRequestHandler::handleRequest(HTTPServerRequest& request,
                                              HTTPServerResponse& response) {
//
  LOG(INFO) << "ExecuteTaskRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::PushTaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::PushTaskReply rpc_resp;
  Interface()->ExecuteTask(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);

  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------StopTask------------------------------------
StopTaskRequestHandler::StopTaskRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}
void StopTaskRequestHandler::handleRequest(HTTPServerRequest& request,
                                           HTTPServerResponse& response) {
//
  LOG(INFO) << "StopTaskRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskContext rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::Empty rpc_resp;
  Interface()->StopTask(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------KillTask------------------------------------
KillTaskRequestHandler::KillTaskRequestHandler(void* service_impl) :
    TaskRequestHandler(service_impl) {

}
void KillTaskRequestHandler::handleRequest(HTTPServerRequest& request,
                                            HTTPServerResponse& response) {
//
  LOG(INFO) << "KillTaskRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::KillTaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::KillTaskResponse rpc_resp;
  Interface()->KillTask(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------FetchTaskStatus-----------------------------
FetchTaskStatusRequestHandler::FetchTaskStatusRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}
void FetchTaskStatusRequestHandler::handleRequest(HTTPServerRequest& request,
    HTTPServerResponse& response) {
//
  VLOG(8) << "FetchTaskStatusRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskContext rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::TaskStatusReply rpc_resp;
  Interface()->FetchTaskStatus(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------UpdateTaskStatus-----------------------------
UpdateTaskStatusRequestHandler::UpdateTaskStatusRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}

void UpdateTaskStatusRequestHandler::handleRequest(HTTPServerRequest& request,
                    HTTPServerResponse& response) {
//
  VLOG(5) << "UpdateTaskStatusRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskStatus rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::Empty rpc_resp;
  Interface()->UpdateTaskStatus(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}

// -----------------------------SendData------------------------------------
SendDataRequestHandler::SendDataRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}
void SendDataRequestHandler::handleRequest(HTTPServerRequest& request,
                  HTTPServerResponse& response) {
//
  VLOG(5) << "SendDataRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::TaskResponse rpc_resp;
  Interface()->Send(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}
// -----------------------------RecvData------------------------------------
RecvDataRequestHandler::RecvDataRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}
void RecvDataRequestHandler::handleRequest(HTTPServerRequest& request,
                  HTTPServerResponse& response) {
//
  VLOG(5) << "RecvDataRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::TaskResponse rpc_resp;
  Interface()->Recv(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);

  std::ostream& ostr = response.send();
  ostr << resp_str;
}
// ------------------------ForwardRecvRequestHan---------------------------
ForwardRecvRequestHandler::ForwardRecvRequestHandler(void* service_impl):
    TaskRequestHandler(service_impl) {

}
void ForwardRecvRequestHandler::handleRequest(HTTPServerRequest& request,
                  HTTPServerResponse& response) {
  VLOG(5) << "ForwardRecvRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::TaskRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::TaskRequest rpc_resp;
  Interface()->ForwardRecv(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}
// -----------------------------NewDataset-------------------------------
NewDatasetRequestHandler::NewDatasetRequestHandler(
    void* service_impl,
    DataServiceHandler* ds_handler):
    TaskRequestHandler(service_impl), ref_ds_handler_(ds_handler) {

}
void NewDatasetRequestHandler::handleRequest(HTTPServerRequest& request,
                                             HTTPServerResponse& response) {
//
  VLOG(5) << "ForwardRecvRequestHandler Request from "
      << request.clientAddress().toString();
  std::string data_str;
  ExtractRequestData(request, &data_str);
  rpc::NewDatasetRequest rpc_req;
  rpc_req.ParseFromString(data_str);
  rpc::NewDatasetResponse rpc_resp;
  ref_ds_handler_->ProcessNewDataset(rpc_req, &rpc_resp);
  std::string resp_str;
  rpc_resp.SerializeToString(&resp_str);
  response.setChunkedTransferEncoding(true);
  std::ostream& ostr = response.send();
  ostr << resp_str;
}

}  // namespace primihub

