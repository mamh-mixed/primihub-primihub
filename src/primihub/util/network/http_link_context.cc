// Copyright [2022] <primihub.com>
#include "src/primihub/util/network/http_link_context.h"
#include <glog/logging.h>
#include <vector>
#include <algorithm>
#include <utility>
#include <memory>

#include "src/primihub/util/util.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/proto_log_helper.h"

namespace pb_util = primihub::proto::util;
namespace primihub::network {
static size_t my_fwrite(void *buffer, size_t size, size_t nmemb, void *stream) {
  auto buf = reinterpret_cast<std::string *>(stream);
  size_t total_len = size * nmemb;
  VLOG(5) << "total_len: " << total_len << " "
      << " size: " << size << " nmemb: " << nmemb;
  if (total_len > 0) {
    buf->reserve(total_len);
    buf->append(reinterpret_cast<char*>(buffer), total_len);
  }
  return total_len;
}

HttpChannel::HttpChannel(const primihub::Node& node, LinkContext* link_ctx) :
    IChannel(link_ctx) {
  dest_node_ = node;
  std::string address_ = node.ip_ + ":" + std::to_string(node.port_);
  VLOG(5) << "address_: " << address_;
  curl_global_init(CURL_GLOBAL_ALL);
  /* get a curl handle */
  curl_ = curl_easy_init();
}
retcode HttpChannel::ExecuteHttpRequest(const Node& dest,
                                        const std::string& method_name,
                                        std::string_view request_info_sv,
                                        std::string* result) {
//
  this->curl_ = curl_easy_init();
  std::string url = "http://";
  url.append(dest.ip()).append(":").append(std::to_string(dest.port()))
      .append("/primihub/").append(method_name);
  VLOG(5) << "curl: " << url;
  std::string result_buf;
  if(curl_) {
    /* First set the URL that is about to receive our POST. This URL can
       just as well be an https:// URL if that is what should receive the
       data. */
    curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
    /* Now specify the POST data */
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, request_info_sv.data());
    curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, request_info_sv.length());
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, my_fwrite);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &result_buf);

    /* Perform the request, res gets the return code */
    CURLcode res = curl_easy_perform(curl_);
     /* always cleanup */
    curl_easy_cleanup(this->curl_);
    /* Check for errors */
    if(res != CURLE_OK) {
      LOG(ERROR) << "curl_easy_perform() failed: " << curl_easy_strerror(res);
      return retcode::FAIL;
    }
  }
  *result = std::move(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::ExecuteHttpRequest(const Node& dest,
                                        const std::string& method_name,
                                        const std::string& request_info,
                                        std::string* result) {
//
  auto req_info_sv = std::string_view(request_info.c_str(), request_info.length());
  return ExecuteHttpRequest(dest, method_name, req_info_sv, result);
}

retcode HttpChannel::BuildTaskInfo(rpc::TaskContext* task_info) {
  auto link_ctx = this->getLinkContext();
  if (link_ctx == nullptr) {
    return retcode::FAIL;
  }
  LOG(ERROR) << "HttpChannel::BuildTaskInfo job id: " << link_ctx->job_id() << " "
      << "task_id: " << link_ctx->task_id() << " "
      << " request_id: " << link_ctx->request_id();
  task_info->set_job_id(link_ctx->job_id());
  task_info->set_task_id(link_ctx->task_id());
  task_info->set_sub_task_id(link_ctx->sub_task_id());
  task_info->set_request_id(link_ctx->request_id());
  return retcode::SUCCESS;
}

// std::shared_ptr<grpc::Channel> HttpChannel::buildChannel(
//     std::string& server_address,
//     bool use_tls) {
//   std::shared_ptr<grpc::ChannelCredentials> creds{nullptr};
//   grpc::ChannelArguments channel_args;
//   // channel_args.SetMaxReceiveMessageSize(128*1024*1024);
//   if (use_tls) {
//     auto link_context = this->getLinkContext();
//     auto& cert_config = link_context->getCertificateConfig();
//     grpc::SslCredentialsOptions ssl_opts;
//     ssl_opts.pem_root_certs = cert_config.rootCAContent();
//     ssl_opts.pem_private_key = cert_config.keyContent();
//     ssl_opts.pem_cert_chain = cert_config.certContent();
//     creds = grpc::SslCredentials(ssl_opts);
//   } else {
//     creds = grpc::InsecureChannelCredentials();
//   }
//   grpc_channel_ = grpc::CreateCustomChannel(server_address,
//                                             creds, channel_args);
//   return grpc_channel_;
// }

retcode HttpChannel::sendRecv(const std::string& role,
    std::string_view send_data, std::string* recv_data) {
  return retcode::SUCCESS;
}

retcode HttpChannel::sendRecv(const std::string& role,
    const std::string& send_data, std::string* recv_data) {
  std::string_view data_sv{send_data.c_str(), send_data.length()};
  return sendRecv(role, data_sv, recv_data);
}

bool HttpChannel::send_wrapper(const std::string& role,
                               const std::string& data) {
  auto ret = this->send(role, data);
  if (ret != retcode::SUCCESS) {
    rpc::TaskContext task_info;
    BuildTaskInfo(&task_info);
    std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kTask) << "send data encountes error";
    return false;
  }
  return true;
}

bool HttpChannel::send_wrapper(const std::string& role,
                               std::string_view sv_data) {
  auto ret = this->send(role, sv_data);
  if (ret != retcode::SUCCESS) {
    rpc::TaskContext task_info;
    BuildTaskInfo(&task_info);
    PH_LOG(ERROR, LogType::kTask)
        << pb_util::TaskInfoToString(task_info)
        << "send data encountes error";
    return false;
  }
  return true;
}
retcode HttpChannel::send(const std::string& role, const std::string& data) {
  std::string_view data_sv(data.c_str(), data.length());
  return send(role, data_sv);
}

retcode HttpChannel::send(const std::string& role, std::string_view data_sv) {
  // VLOG(5) << "GrpcChannel::send begin to send, use key: " << role;
  rpc::TaskRequest send_requests;
  buildTaskRequest(role, data_sv, &send_requests);
  auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  int retry_time{0};
  std::string TASK_INFO_STR;
  const auto& task_info = send_requests.task_info();
  TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  std::string send_data;
  send_requests.SerializeToString(&send_data);
  std::string result;
  ExecuteHttpRequest(dest_node_, HttpMethod::Send, send_data, &result);
  // do {
  //   grpc::ClientContext context;
  //   if (send_tiemout_ms > 0) {
  //     auto deadline = std::chrono::system_clock::now() +
  //       std::chrono::milliseconds(send_tiemout_ms);
  //     context.set_deadline(deadline);
  //   }
  //   rpc::TaskResponse task_response;
  //   using writer_t = grpc::ClientWriter<rpc::TaskRequest>;
  //   std::unique_ptr<writer_t> writer(stub_->Send(&context, &task_response));
  //   for (const auto& request : send_requests) {
  //     writer->Write(request);
  //   }
  //   writer->WritesDone();
  //   grpc::Status status = writer->Finish();
  //   if (status.ok()) {
  //     auto ret_code = task_response.ret_code();
  //     if (ret_code) {
  //         PH_LOG(ERROR, LogType::kTask)
  //             << "send data to [" << dest_node_.to_string()
  //             << "] return failed error code: " << ret_code;
  //         return retcode::FAIL;
  //     }
  //     break;
  //   } else {
  //     PH_LOG(WARNING, LogType::kTask)
  //         << "send data to [" << dest_node_.to_string()
  //         << "] failed. error_code: " << status.error_code() << " "
  //         << "error message: " << status.error_message() << " "
  //         << "retry: " << retry_time;
  //     retry_time++;
  //     if (retry_time < retry_max_times_) {
  //       continue;
  //     } else {
  //       PH_LOG(ERROR, LogType::kTask)
  //           << TASK_INFO_STR
  //           << "send data to [" << dest_node_.to_string()
  //           << "] failed. error_code: " << status.error_code() << " "
  //           << "error message: " << status.error_message();
  //       return retcode::FAIL;
  //     }
  //   }
  // } while (true);
  return retcode::SUCCESS;
}

retcode HttpChannel::buildTaskRequest(const std::string& role,
    const std::string& data, rpc::TaskRequest* send_pb_data) {
  std::string_view data_sv(data.c_str(), data.length());
  return buildTaskRequest(role, data_sv, send_pb_data);
}

retcode HttpChannel::buildTaskRequest(const std::string& role,
    std::string_view data_sv, rpc::TaskRequest* task_request) {
  size_t total_length = data_sv.size();
  char* send_buf = const_cast<char*>(data_sv.data());
  auto task_info = task_request->mutable_task_info();
  BuildTaskInfo(task_info);
  task_request->set_role(role);
  task_request->set_data_len(total_length);
  auto data_ptr = task_request->mutable_data();
  data_ptr->append(send_buf, total_length);
  return retcode::SUCCESS;
}

std::string HttpChannel::forwardRecv(const std::string& role) {
  int retry_time{0};
  rpc::TaskRequest send_request;
  auto task_info = send_request.mutable_task_info();
  BuildTaskInfo(task_info);
  send_request.set_role(role);
  VLOG(5) << "forwardRecv request info: job_id: "
          << this->getLinkContext()->job_id()
          << " task_id: " << this->getLinkContext()->task_id()
          << " request id: " << this->getLinkContext()->request_id()
          << " recv key: " << role
          << " nodeinfo: " << this->dest_node_.to_string();
  std::string req_data;
  send_request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::ForwardRecv, req_data, &result_buf);
  rpc::TaskRequest recv_response;
  recv_response.ParseFromString(result_buf);
  std::string tmp_buf = recv_response.data();
  return tmp_buf;
  // SCopedTimer timer;
  // grpc::ClientContext context;
  // auto send_tiemout_ms = this->getLinkContext()->sendTimeout();
  // if (send_tiemout_ms > 0) {
  //   auto deadline = std::chrono::system_clock::now() +
  //     std::chrono::milliseconds(send_tiemout_ms);
  //   context.set_deadline(deadline);
  // }
  // rpc::TaskRequest send_request;
  // auto task_info = send_request.mutable_task_info();
  // BuildTaskInfo(task_info);
  // send_request.set_role(role);
  // // VLOG(5) << "forwardRecv request info: job_id: "
  // //         << this->getLinkContext()->job_id()
  // //         << " task_id: " << this->getLinkContext()->task_id()
  // //         << " request id: " << this->getLinkContext()->request_id()
  // //         << " recv key: " << role
  // //         << " nodeinfo: " << this->dest_node_.to_string();
  // // using reader_t = grpc::ClientReader<rpc::TaskRequest>;
  // auto client_reader = this->stub_->ForwardRecv(&context, send_request);

  // // waiting for response
  // std::string tmp_buff;
  // rpc::TaskRequest recv_response;
  // std::string TASK_INFO_STR = pb_util::TaskInfoToString(*task_info);
  // bool init_flag{false};
  // while (client_reader->Read(&recv_response)) {
  //   auto data = recv_response.data();
  //   if (!init_flag) {
  //     size_t data_len = recv_response.data_len();
  //     tmp_buff.reserve(data_len);
  //     init_flag = true;
  //   }
  //   PH_VLOG(5, LogType::kTask)
  //       << "data length: " << data.size();
  //   tmp_buff.append(data);
  // }

  // grpc::Status status = client_reader->Finish();
  // if (!status.ok()) {
  //   PH_LOG(ERROR, LogType::kTask)
  //       << "recv data encountes error, detail: "
  //       << status.error_code() << ": " << status.error_message();
  //   return std::string("");
  // }
  // // VLOG(5) << "recv data success, data size: " << tmp_buff.size();
  // auto time_cost = timer.timeElapse();
  // PH_VLOG(5, LogType::kTask)
  //     << "forwardRecv time cost(ms): " << time_cost;
  // return tmp_buff;
}

retcode HttpChannel::submitTask(const rpc::PushTaskRequest& request,
                                rpc::PushTaskReply* reply) {
  int retry_time{0};
  const auto& task_info = request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::SubmitTask, req_data, &result_buf);
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::executeTask(const rpc::PushTaskRequest& request,
                                 rpc::PushTaskReply* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::ExecuteTask, req_data, &result_buf);
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::StopTask(const rpc::TaskContext& request,
                              rpc::Empty* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::StopTask, req_data, &result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::killTask(const rpc::KillTaskRequest& request,
                              rpc::KillTaskResponse* reply) {
//
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::KillTask, req_data, &result_buf);
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::updateTaskStatus(const rpc::TaskStatus& request,
                                      rpc::Empty* reply) {
  //
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::UpdateTaskStatus, req_data, &result_buf);
  // reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::fetchTaskStatus(const rpc::TaskContext& request,
                                     rpc::TaskStatusReply* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::FetchTaskStatus, req_data, &result_buf);
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

std::shared_ptr<IChannel> HttpLinkContext::buildChannel(
    const primihub::Node& node,
    LinkContext* link_ctx) {
  return std::make_shared<HttpChannel>(node, link_ctx);
}

std::shared_ptr<IChannel> HttpLinkContext::getChannel(
    const primihub::Node& node) {
  std::string node_info = node.to_string();
  {
    std::shared_lock<std::shared_mutex> lck(this->connection_mgr_mtx);
    auto it = connection_mgr.find(node_info);
    if (it != connection_mgr.end()) {
      return it->second;
    }
  }
  // create channel
  auto channel = buildChannel(node, this);
  {
    std::lock_guard<std::shared_mutex> lck(this->connection_mgr_mtx);
    connection_mgr[node_info] = channel;
  }
  return channel;
}

// dataset related operation
retcode HttpChannel::DownloadData(const rpc::DownloadRequest& request,
                                  std::vector<std::string>* data) {
  return retcode::SUCCESS;
}

retcode HttpChannel::CheckSendCompleteStatus(
    const std::string& key, uint64_t expected_complete_num) {
  int retry_time{0};
  rpc::CompleteStatusRequest request;
  auto task_info_ptr = request.mutable_task_info();
  BuildTaskInfo(task_info_ptr);
  request.set_key(key);
  request.set_complete_count(expected_complete_num);
  const auto& task_info = request.task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  // do {
  //   grpc::ClientContext context;
  //   auto deadline = std::chrono::system_clock::now() +
  //       std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
  //   context.set_deadline(deadline);
  //   rpc::Empty reply;
  //   grpc::Status status = stub_->CompleteStatus(&context, request, &reply);
  //   if (status.ok()) {
  //     PH_VLOG(5, LogType::kTask)
  //         << TASK_INFO_STR
  //         << "send CompleteStatus to node: ["
  //         << dest_node_.to_string() << "] rpc succeeded.";
  //     break;
  //   } else {
  //     PH_LOG(WARNING, LogType::kTask)
  //         << TASK_INFO_STR
  //         << "send CompleteStatus to Node ["
  //         << dest_node_.to_string() << "] rpc failed. "
  //         << status.error_code() << ": " << status.error_message() << " "
  //         << "retry times: " << retry_time;
  //     retry_time++;
  //     if (retry_time < this->retry_max_times_) {
  //       continue;
  //     } else {
  //       PH_LOG(ERROR, LogType::kTask)
  //           << TASK_INFO_STR
  //           << "send CompleteStatus to Node ["
  //           << dest_node_.to_string() << "] rpc failed. "
  //           << status.error_code() << ": " << status.error_message();
  //       return retcode::FAIL;
  //     }
  //   }
  // } while (true);
  return retcode::SUCCESS;
}

retcode HttpChannel::NewDataset(const rpc::NewDatasetRequest& request,
                                rpc::NewDatasetResponse* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  ExecuteHttpRequest(dest_node_, HttpMethod::NewDataset, req_data, &result_buf);
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
  // int retry_time{0};
  // // const auto& task_info = request.task().task_info();
  // std::string TASK_INFO_STR = "";
  // do {
  //   grpc::ClientContext context;
  //   auto deadline = std::chrono::system_clock::now() +
  //       std::chrono::seconds(CONTROL_CMD_TIMEOUT_S);
  //   context.set_deadline(deadline);
  //   grpc::Status status = dataset_stub_->NewDataset(&context, request, reply);
  //   if (status.ok()) {
  //     PH_VLOG(5, LogType::kTask)
  //         << TASK_INFO_STR
  //         << "send NewDataset to node: ["
  //         << dest_node_.to_string() << "] rpc succeeded.";
  //     break;
  //   } else {
  //     PH_LOG(WARNING, LogType::kTask)
  //         << TASK_INFO_STR
  //         << "send NewDataset to Node ["
  //         << dest_node_.to_string() << "] rpc failed. "
  //         << status.error_code() << ": " << status.error_message() << " "
  //         << "retry times: " << retry_time;
  //     retry_time++;
  //     if (retry_time < this->retry_max_times_) {
  //       continue;
  //     } else {
  //       PH_LOG(ERROR, LogType::kTask)
  //           << TASK_INFO_STR
  //           << "send NewDataset to Node ["
  //           << dest_node_.to_string() << "] rpc failed. "
  //           << status.error_code() << ": " << status.error_message();
  //       return retcode::FAIL;
  //     }
  //   }
  // } while (true);
  return retcode::SUCCESS;
}
}  // namespace primihub::network
