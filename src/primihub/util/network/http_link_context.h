// Copyright [2022] <primihub.com>
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_HTTP_LINK_CONTEXT_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_HTTP_LINK_CONTEXT_H_

#include <curl/curl.h>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "src/primihub/util/network/link_context.h"
#include "src/primihub/common/common.h"
#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/protos/service.pb.h"

namespace primihub::network {
namespace rpc = primihub::rpc;
struct HttpMethod {
  static constexpr const char* SubmitTask       = "SubmitTask";
  static constexpr const char* ExecuteTask      = "ExecuteTask";
  static constexpr const char* StopTask         = "StopTask";
  static constexpr const char* KillTask         = "KillTask";
  static constexpr const char* FetchTaskStatus  = "FetchTaskStatus";
  static constexpr const char* UpdateTaskStatus = "UpdateTaskStatus";
  static constexpr const char* Send             = "Send";
  static constexpr const char* Recv             = "Recv";
  static constexpr const char* ForwardRecv      = "ForwardRecv";
  static constexpr const char* NewDataset       = "NewDataset";
};

class HttpChannel : public IChannel {
 public:
  HttpChannel(const primihub::Node& node, LinkContext* link_ctx);
  virtual ~HttpChannel() {
    curl_global_cleanup();
  };
  retcode send(const std::string& role, const std::string& data) override;
  retcode send(const std::string& role, std::string_view sv_data) override;
  bool send_wrapper(const std::string& role, const std::string& data) override;
  bool send_wrapper(const std::string& role, std::string_view sv_data) override;
  retcode sendRecv(const std::string& role,
                   const std::string& send_data,
                   std::string* recv_data) override;
  retcode sendRecv(const std::string& role,
                   std::string_view send_data, std::string* recv_data) override;
  retcode submitTask(const rpc::PushTaskRequest& request,
                     rpc::PushTaskReply* reply) override;
  retcode executeTask(const rpc::PushTaskRequest& request,
                      rpc::PushTaskReply* reply) override;
  retcode StopTask(const rpc::TaskContext& request,
                   rpc::Empty* reply) override;
  retcode killTask(const rpc::KillTaskRequest& request,
                   rpc::KillTaskResponse* reply) override;

  retcode updateTaskStatus(const rpc::TaskStatus& request,
                           rpc::Empty* reply) override;
  retcode fetchTaskStatus(const rpc::TaskContext& request,
                          rpc::TaskStatusReply* reply) override;
  std::string forwardRecv(const std::string& role) override;
  retcode buildTaskRequest(const std::string& role,
                           const std::string& data,
                           rpc::TaskRequest* send_pb_data);
  retcode buildTaskRequest(const std::string& role,
                           std::string_view sv_data,
                           rpc::TaskRequest* send_pb_data);
  // data set related operation
  retcode DownloadData(const rpc::DownloadRequest& request,
                       std::vector<std::string>* data) override;
  retcode CheckSendCompleteStatus(
      const std::string& key, uint64_t expected_complete_num) override;
  retcode NewDataset(const rpc::NewDatasetRequest& request,
                     rpc::NewDatasetResponse* reply) override;

 protected:
  retcode BuildTaskInfo(rpc::TaskContext* task_info);
  retcode ExecuteHttpRequest(const Node& dest,
                            const std::string& method_name,
                            const std::string& request_info,
                            std::string* result);
  retcode ExecuteHttpRequest(const Node& dest,
                            const std::string& method_name,
                            std::string_view request_info_sv,
                            std::string* result);

 private:
  CURL* curl_{nullptr};
  primihub::Node dest_node_;
  int retry_max_times_{3};
};

class HttpLinkContext : public LinkContext {
 public:
  HttpLinkContext() = default;
  virtual ~HttpLinkContext() = default;
  std::shared_ptr<IChannel> buildChannel(const primihub::Node& node,
                                         LinkContext* link_ctx);
  std::shared_ptr<IChannel> getChannel(const primihub::Node& node) override;
};

}  // namespace primihub::network

#endif  // SRC_PRIMIHUB_UTIL_NETWORK_HTTP_LINK_CONTEXT_H_
