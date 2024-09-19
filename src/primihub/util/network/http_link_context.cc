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
  // curl_ = curl_easy_init();
}
retcode HttpChannel::ExecuteHttpRequest(const Node& dest,
                                        const std::string& method_name,
                                        std::string_view request_info_sv,
                                        std::string* result) {
  std::lock_guard<std::mutex> lck(mtx_);
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
  auto req_info_sv =
      std::string_view(request_info.c_str(), request_info.length());
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
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::Send, send_data, &result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::Send failed";
    return retcode::FAIL;
  }
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
  auto link_ctx = this->getLinkContext();
  VLOG(5) << "forwardRecv request info: job_id: "
          << link_ctx->job_id() << " "
          << "task_id: " << link_ctx->task_id() << " "
          << "request id: " << link_ctx->request_id() << " "
          << "recv key: " << role << " "
          << "nodeinfo: " << this->dest_node_.to_string();
  std::string req_data;
  send_request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::ForwardRecv, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::ForwardRecv failed";
    return std::string("");
  }
  rpc::TaskRequest recv_response;
  recv_response.ParseFromString(result_buf);
  std::string tmp_buf = recv_response.data();
  return tmp_buf;
}

retcode HttpChannel::submitTask(const rpc::PushTaskRequest& request,
                                rpc::PushTaskReply* reply) {
  int retry_time{0};
  const auto& task_info = request.task().task_info();
  std::string TASK_INFO_STR = pb_util::TaskInfoToString(task_info);
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::SubmitTask, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::SubmitTask failed";
    return retcode::FAIL;
  }
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::executeTask(const rpc::PushTaskRequest& request,
                                 rpc::PushTaskReply* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::ExecuteTask, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::ExecuteTask failed";
    return retcode::FAIL;
  }
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::StopTask(const rpc::TaskContext& request,
                              rpc::Empty* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::StopTask, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::StopTask failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode HttpChannel::killTask(const rpc::KillTaskRequest& request,
                              rpc::KillTaskResponse* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::KillTask, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::KillTask failed";
    return retcode::FAIL;
  }
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}

retcode HttpChannel::updateTaskStatus(const rpc::TaskStatus& request,
                                      rpc::Empty* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::UpdateTaskStatus,
                                req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::UpdateTaskStatus failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode HttpChannel::fetchTaskStatus(const rpc::TaskContext& request,
                                     rpc::TaskStatusReply* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::FetchTaskStatus,
                                req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::FetchTaskStatus failed";
    return retcode::FAIL;
  }
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
  return retcode::SUCCESS;
}

retcode HttpChannel::NewDataset(const rpc::NewDatasetRequest& request,
                                rpc::NewDatasetResponse* reply) {
  std::string req_data;
  request.SerializeToString(&req_data);
  std::string result_buf;
  auto ret = ExecuteHttpRequest(dest_node_,
                                HttpMethod::NewDataset, req_data, &result_buf);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "ExecuteHttpRequest for HttpMethod::NewDataset failed";
    return retcode::FAIL;
  }
  reply->ParseFromString(result_buf);
  return retcode::SUCCESS;
}
}  // namespace primihub::network
