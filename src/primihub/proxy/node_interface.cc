/*
 * Copyright (c) 2023 by PrimiHub
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      https://www.apache.org/licenses/
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "src/primihub/proxy/node_interface.h"
#include <glog/logging.h>
#include <algorithm>
#include <utility>

#include "src/primihub/util/util.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/util/log.h"
#include "src/primihub/util/network/link_context.h"

namespace primihub {

Status VMNodeProxyInterface::SubmitTask(ServerContext *context,
                                   const rpc::PushTaskRequest *request,
                                   rpc::PushTaskReply *reply) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->submitTask(*request, reply);
  return Status::OK;
}

Status VMNodeProxyInterface::ExecuteTask(ServerContext* context,
                                    const rpc::PushTaskRequest* request,
                                    rpc::PushTaskReply* reply) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->executeTask(*request, reply);
  return Status::OK;
}

Status VMNodeProxyInterface::StopTask(ServerContext* context,
                                 const rpc::TaskContext* request,
                                 rpc::Empty* reply) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->StopTask(*request, reply);
  return Status::OK;
}

Status VMNodeProxyInterface::KillTask(ServerContext* context,
                                 const rpc::KillTaskRequest* request,
                                 rpc::KillTaskResponse* response) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->killTask(*request, response);
  return Status::OK;
}

// task status operation
Status VMNodeProxyInterface::FetchTaskStatus(ServerContext* context,
                                        const rpc::TaskContext* request,
                                        rpc::TaskStatusReply* response) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->fetchTaskStatus(*request, response);
  return Status::OK;
}

Status VMNodeProxyInterface::UpdateTaskStatus(ServerContext* context,
                                         const rpc::TaskStatus* request,
                                         rpc::Empty* response) {
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->updateTaskStatus(*request, response);
  return Status::OK;
}

// communication interface
Status VMNodeProxyInterface::Send(ServerContext* context,
                             ServerReader<rpc::TaskRequest>* reader,
                             rpc::TaskResponse* response) {
  return Status::OK;
}

Status VMNodeProxyInterface::Recv(ServerContext* context,
                             const rpc::TaskRequest* request,
                             ServerWriter<rpc::TaskResponse>* writer) {
  return grpc::Status::OK;
}

Status VMNodeProxyInterface::SendRecv(ServerContext* context,
    ServerReaderWriter<rpc::TaskResponse, rpc::TaskRequest>* stream) {
  return Status::OK;
}

// for communication between different process
Status VMNodeProxyInterface::ForwardSend(ServerContext* context,
    ServerReader<rpc::ForwardTaskRequest>* reader,
    rpc::TaskResponse* response) {
  return Status::OK;
}

// for communication between different process
Status VMNodeProxyInterface::ForwardRecv(ServerContext* context,
                                    const rpc::TaskRequest* request,
                                    ServerWriter<rpc::TaskRequest>* writer) {
  return Status::OK;
}

Status VMNodeProxyInterface::CompleteStatus(ServerContext* context,
                                       const rpc::CompleteStatusRequest* request,
                                       rpc::Empty* response) {
  // waiting for peer node fetch data complete
  return Status::OK;
}
}  // namespace primihub
