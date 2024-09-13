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

#include "src/primihub/node/http/node_http_interface.h"
#include <glog/logging.h>
#include <algorithm>
#include <utility>

#include "src/primihub/util/util.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/util/log.h"

namespace primihub {
retcode VMNodeHttpInterface::SubmitTask(const rpc::PushTaskRequest request,
                                        rpc::PushTaskReply* reply) {
  if (VLOG_IS_ON(5)) {
    auto reqeust_str = proto::util::TaskRequestToString(request);
    LOG(INFO) << reqeust_str;
  }
  ServerImpl()->DispatchTask(request, reply);

  auto task_info = reply->mutable_task_info();
  task_info->CopyFrom(request.task().task_info());
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::ExecuteTask(const rpc::PushTaskRequest& request,
                                          rpc::PushTaskReply* response) {
  const auto& task_info = request.task().task_info();
  std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "enter VMNodeImpl::ExecuteTask";
  auto ret = ServerImpl()->ExecuteTask(request, response);
  if (ret != retcode::SUCCESS) {
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "ExecuteTask encountes error";
  }
  PH_VLOG(5, LogType::kScheduler)
      << TASK_INFO_STR
      << "exit VMNodeImpl::ExecuteTask";
  response->mutable_task_info()->CopyFrom(request.task().task_info());
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::StopTask(const rpc::TaskContext& request,
                  rpc::Empty* response) {

  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::KillTask(const rpc::KillTaskRequest& request,
                rpc::KillTaskResponse* response) {
  auto ret = ServerImpl()->KillTask(request, response);
  if (ret != retcode::SUCCESS) {
    const auto& task_info = request.task_info();
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "KillTask encountes error";
  }
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::FetchTaskStatus(const rpc::TaskContext& request,
                                             rpc::TaskStatusReply* response) {
  auto ret = ServerImpl()->FetchTaskStatus(request, response);
  if (ret != retcode::SUCCESS) {
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(request);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "FetchTaskStatus encountes error";
  }
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::UpdateTaskStatus(const rpc::TaskStatus& request,
                        rpc::Empty* response){
  auto ret = this->ServerImpl()->UpdateTaskStatus(request);
  if (ret != retcode::SUCCESS) {
    const auto& task_info = request.task_info();
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kScheduler)
        << TASK_INFO_STR
        << "UpdateTaskStatus encountes error";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::Send(const rpc::TaskRequest& request,
            rpc::TaskResponse* response){
  std::string received_data = request.data();
  size_t data_size = received_data.size();
  const auto & task_info = request.task_info();
  auto TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
  std::string key = request.role();
  VLOG(5) << "VMNodeHttpInterface::Send TASK_INFO_STR: " << TASK_INFO_STR;
  auto ret = this->ServerImpl()->ProcessReceivedData(task_info, key,
                                                     std::move(received_data));
  if (ret != retcode::SUCCESS) {
    response->set_ret_code(rpc::retcode::FAIL);
  }
  PH_VLOG(5, LogType::kTask)
      << TASK_INFO_STR
      << "end of VMNodeImpl::Send, data total received size:" << data_size;
  response->set_ret_code(rpc::retcode::SUCCESS);
}

retcode VMNodeHttpInterface::Recv(const rpc::TaskRequest& request,
              rpc::TaskResponse* response){
  std::string send_data;
  const auto& task_info = request.task_info();
  std::string key = request.role();
  auto ret = this->ServerImpl()->ProcessSendData(task_info, key, &send_data);
  if (ret != retcode::SUCCESS) {
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << "no data is available for key: " << key;
    std::string err_msg = "no data is available for key:" + key;
    response->set_ret_code(rpc::retcode::FAIL);
    response->set_msg_info(std::move(err_msg));
    return retcode::FAIL;
  }
  BuildTaskResponse(send_data, response);
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::BuildTaskResponse(const std::string& data,
      rpc::TaskResponse* response) {
  response->set_ret_code(rpc::retcode::SUCCESS);
  response->set_data_len(data.size());
  auto data_ptr = response->mutable_data();
  data_ptr->append(data.data(), data.size());
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::ForwardRecv(const rpc::TaskRequest& request,
                                          rpc::TaskRequest* response) {
  // waiting for peer node send data
  const auto& task_info = request.task_info();
  std::string key = request.role();
  std::string recv_data;
  auto ret = this->ServerImpl()->ProcessForwardData(task_info, key, &recv_data);
  if (ret != retcode::SUCCESS) {
    std::string TASK_INFO_STR = proto::util::TaskInfoToString(task_info);
    std::string err_msg = "no data is available for key:" + key;
    PH_LOG(ERROR, LogType::kTask)
        << TASK_INFO_STR << err_msg;
    return retcode::FAIL;
  }
  this->BuildTaskRequest(task_info, key, recv_data, response);

  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::BuildTaskRequest(const rpc::TaskContext& task_info,
      const std::string& key,
      const std::string& data,
      rpc::TaskRequest* request) {
  auto task_info_ptr = request->mutable_task_info();
  task_info_ptr->CopyFrom(task_info);
  request->set_role(key);
  request->set_data_len(data.size());
  auto data_ptr = request->mutable_data();
  data_ptr->append(data.data(), data.size());
  return retcode::SUCCESS;
}

retcode VMNodeHttpInterface::WaitUntilWorkerReady(const std::string& worker_id,
                                                  int timeout) {
  LOG(ERROR) << "VMNodeHttpInterface::WaitUntilWorkerReady: worker_id: " << worker_id;
  std::string TASK_INFO_STR = proto::util::TaskInfoToString(worker_id);
  auto _start = std::chrono::high_resolution_clock::now();
  SCopedTimer timer;
  do {
    // try to get lock
    if (ServerImpl()->IsTaskWorkerReady(worker_id)) {
      break;
    }
    PH_VLOG(5, LogType::kTask)
        << TASK_INFO_STR
        << "sleep and wait for worker ready ......";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (timeout == -1) {
      continue;
    }
    auto time_elapse_ms = timer.timeElapse();
    if (time_elapse_ms > timeout) {
      PH_LOG(ERROR, LogType::kTask)
          << TASK_INFO_STR
          << "wait for worker ready is timeout";
      return retcode::FAIL;
    }
  } while (true);
  return retcode::SUCCESS;
}

}  // namespace primihub
