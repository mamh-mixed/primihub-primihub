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
#include "src/primihub/task/pybind_wrapper/mpc_task_wrapper.h"
#include <glog/logging.h>
#include <random>
#include "src/primihub/common/common.h"
#include <google/protobuf/text_format.h>
#include "uuid.h"

namespace primihub::task {
MPCExecutor::MPCExecutor(const std::string& task_req_str,
                         const std::string& protocol) {
  task_req_ptr_ = std::make_unique<primihub::rpc::PushTaskRequest>();
  bool succ_flag = task_req_ptr_->ParseFromString(task_req_str);
  if (!succ_flag) {
    throw std::runtime_error("parser task request encountes error");
  }
  auto task_config = task_req_ptr_->mutable_task();
  // add auxiliary compute node to party access info
  auto ret = AddAuxiliaryComputeServer(task_config);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "AddAuxiliaryComputeServer failed";
    return;
  }
  std::string func_name{"mpc_statistics"};
  this->task_req_ptr_->mutable_task()->set_code("mpc_statistics");
  this->task_req_ptr_->set_manual_launch(true);
  task_ptr_ = std::make_unique<MPCTask>(func_name, &(task_req_ptr_->task()));
}

MPCExecutor::~MPCExecutor() {
  this->StopTask();
}

retcode MPCExecutor::StopTask() {
  std::string party_name = this->task_req_ptr_->task().party_name();
  const auto& party_access_info = task_req_ptr_->task().party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "invalid party: " << party_name;
    return retcode::FAIL;
  }
  auto& party_node = it->second;
  int32_t party_id = party_node.party_id();
  if (party_id != 0) {
    return retcode::SUCCESS;
  }
  it = party_access_info.find(AUX_COMPUTE_NODE);
  if (it == party_access_info.end()) {
    LOG(ERROR) << AUX_COMPUTE_NODE << " access info is not found";
    return retcode::FAIL;
  }
  auto pb_aux_node = it->second;
  Node aux_node;
  pbNode2Node(pb_aux_node, &aux_node);
  LOG(ERROR) << "auxiliary compute server info: " << aux_node.to_string();
  auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
  auto channel = link_ctx->getChannel(aux_node);
  rpc::Empty reply;
  rpc::TaskContext task_info;
  task_info.CopyFrom(task_req_ptr_->task().task_info());
  return channel->StopTask(task_info, &reply);
}

retcode MPCExecutor::AddAuxiliaryComputeServer(rpc::Task* task) {
  auto party_access_info = task->mutable_party_access_info();
  size_t party_size = party_access_info->size();
  auto auxiliary_server = task->auxiliary_server();
  auto it = auxiliary_server.find(AUX_COMPUTE_NODE);
  if (it != auxiliary_server.end()) {
    auto& server_node = it->second;
    rpc::Node aux_compute_node;
    aux_compute_node.CopyFrom(server_node);
    aux_compute_node.set_party_id(party_size);
    (*party_access_info)[AUX_COMPUTE_NODE] = std::move(aux_compute_node);
  } else {
    LOG(WARNING) << "no AUXILIARY COMPUTE NODE found";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode MPCExecutor::Max(const std::vector<double>& input,
                         std::vector<double>* result) {
  std::string sub_task_id;
  NegotiateSubTaskId(&sub_task_id);
  auto task_config = this->task_req_ptr_->mutable_task();
  auto task_info = task_config->mutable_task_info();
  LOG(ERROR) << "MPCExecutor::Max: " << task_req_ptr_->manual_launch();
  task_info->set_sub_task_id(sub_task_id);
  SetStatisticsOperation(rpc::Algorithm::MAX, task_config);
  auto ret = InviteAuxiliaryServerToTask();
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "InviteAuxiliaryServerToTask failed";
    return retcode::FAIL;
  }
  std::vector<int64_t> input_shape;
  input_shape.push_back(1);
  input_shape.push_back(input.size());
  BroadcastShape(input_shape);
  task_ptr_->setTaskParam(task_req_ptr_->task());
  if (VLOG_IS_ON(0)) {
    std::string str;
    google::protobuf::TextFormat::PrintToString(*task_req_ptr_, &str);
    VLOG(0) << "MPCExecutor: " << str;
  }
  std::vector<int64_t> col_rows;
  col_rows.reserve(input.size());
  col_rows.assign(input.size(), 0);
  ret = task_ptr_->ExecuteTask(input, col_rows, result);
  if (ret != retcode::SUCCESS) {
    LOG(ERROR) << "run max failed";
    return retcode::FAIL;
  }
  return retcode::SUCCESS;
}

retcode MPCExecutor::Min(const std::vector<double>& input,
                         std::vector<double>* result) {
  *result = input;
  return retcode::SUCCESS;
}

retcode MPCExecutor::Avg(const std::vector<double>& input,
                         std::vector<double>* result) {
  *result = input;
  return retcode::SUCCESS;
}

retcode MPCExecutor::Sum(const std::vector<double>& input,
                         std::vector<double>* result) {
  *result = input;
  return retcode::SUCCESS;
}

retcode MPCExecutor::InviteAuxiliaryServerToTask() {
  std::string party_name = this->task_req_ptr_->task().party_name();
  const auto& party_access_info = task_req_ptr_->task().party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "invalid party: " << party_name;
    return retcode::FAIL;
  }
  auto& party_node = it->second;
  int32_t party_id = party_node.party_id();
  if (party_id != 0) {
    auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
    std::string sync_info;
    Node proxy_node;
    const auto& auxiliary_server = task_req_ptr_->task().auxiliary_server();
    it = auxiliary_server.find(PROXY_NODE);
    if (it == auxiliary_server.end()) {
      LOG(ERROR) << "no proxy node found";
      return retcode::FAIL;
    }
    const auto& pb_proxy_node = it->second;
    pbNode2Node(pb_proxy_node, &proxy_node);
    link_ctx->Recv("SyncFlag", proxy_node, &sync_info);
    if (sync_info != std::string("SyncFlag")) {
      LOG(ERROR) << "recv sync info failed";
      return retcode::FAIL;
    }
    return retcode::SUCCESS;
  }
  // only party 0 send request to auxiliary node
  rpc::PushTaskRequest task_request;
  task_request.CopyFrom(*this->task_req_ptr_);
  auto task_config = task_request.mutable_task();
  task_config->set_party_name(AUX_COMPUTE_NODE);
  task_config->set_language(rpc::Language::PROTO);
  task_config->set_type(rpc::TaskType::ACTOR_TASK);

  it = party_access_info.find(AUX_COMPUTE_NODE);
  if (it == party_access_info.end()) {
    LOG(ERROR) << AUX_COMPUTE_NODE << " access info is not found";
    return retcode::FAIL;
  }
  auto pb_aux_node = it->second;
  Node aux_node;
  pbNode2Node(pb_aux_node, &aux_node);
  LOG(ERROR) << "auxiliary compute server info: " << aux_node.to_string();
  auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
  auto channel = link_ctx->getChannel(aux_node);
  rpc::PushTaskReply reply;
  auto ret = channel->executeTask(task_request, &reply);
  std::vector<Node> receiver_list;
  for (const auto& [party_name, pb_node] : party_access_info) {
    if (party_name == AUX_COMPUTE_NODE ||
        pb_node.party_id() == 0) {
      continue;
    }
    Node tmp_node;
    pbNode2Node(pb_node, &tmp_node);
    receiver_list.emplace_back(std::move(tmp_node));
  }
  std::string sync_info{"SyncFlag"};
  for (const auto& receiver : receiver_list) {
    link_ctx->Send("SyncFlag", receiver, sync_info);
  }
  return retcode::SUCCESS;
}
retcode MPCExecutor::BroadcastShape(const std::vector<int64_t>& shape) {
  std::string party_name = this->task_req_ptr_->task().party_name();
  const auto& party_access_info = task_req_ptr_->task().party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "invalid party: " << party_name;
    return retcode::FAIL;
  }
  auto& party_node = it->second;
  int32_t party_id = party_node.party_id();
  if (party_id != 0) {
    return retcode::SUCCESS;
  }
  // only party 0 send request to auxiliary node
  rpc::PushTaskRequest task_request;
  task_request.CopyFrom(*this->task_req_ptr_);
  auto task_config = task_request.mutable_task();
  task_config->set_party_name(AUX_COMPUTE_NODE);
  task_config->set_language(rpc::Language::PROTO);
  task_config->set_type(rpc::TaskType::ACTOR_TASK);

  it = party_access_info.find(AUX_COMPUTE_NODE);
  if (it == party_access_info.end()) {
    LOG(ERROR) << AUX_COMPUTE_NODE << " access info is not found";
    return retcode::FAIL;
  }
  auto pb_aux_node = it->second;
  Node aux_node;
  pbNode2Node(pb_aux_node, &aux_node);
  LOG(ERROR) << "auxiliary compute server info: " << aux_node.to_string();
  rpc::ParamValue pv;
  pv.set_var_type(rpc::INT64);
  pv.set_is_array(true);
  auto arr = pv.mutable_value_int64_array();
  for (const auto dim : shape) {
    arr->add_value_int64_array(dim);
  }
  std::string shape_info_str;
  pv.SerializeToString(&shape_info_str);
  auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
  link_ctx->Send("mpc_shape", aux_node, shape_info_str);
  return retcode::SUCCESS;
}

retcode MPCExecutor::GenerateSubtaskId(std::string* subtask_id) {
  std::random_device rd;
  auto seed_data = std::array<int, std::mt19937::state_size> {};
  std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
  std::seed_seq seq(std::begin(seed_data), std::end(seed_data));
  std::mt19937 generator(seq);
  uuids::uuid_random_generator gen{generator};
  const uuids::uuid id = gen();
  *subtask_id = uuids::to_string(id);
  return retcode::SUCCESS;
}

retcode MPCExecutor::BroadcastSubtaskId(const std::string& sub_task_id) {
  std::string party_name = this->task_req_ptr_->task().party_name();
  const auto& party_access_info = task_req_ptr_->task().party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "invalid party: " << party_name;
    return retcode::FAIL;
  }
  auto& party_node = it->second;
  int32_t party_id = party_node.party_id();
  if (party_id != 0) {
    return retcode::SUCCESS;
  }
  std::vector<Node> receiver_list;
  for (const auto& [party_name, pb_node] : party_access_info) {
    if (party_name == AUX_COMPUTE_NODE ||
        pb_node.party_id() == 0) {
      continue;
    }
    Node tmp_node;
    pbNode2Node(pb_node, &tmp_node);
    receiver_list.emplace_back(std::move(tmp_node));
  }
  auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
  for (const auto& receiver : receiver_list) {
    link_ctx->Send("subtask_id", receiver, sub_task_id);
  }

}

retcode MPCExecutor::RecvSubTaskId(std::string* subtask_id) {
  std::string party_name = this->task_req_ptr_->task().party_name();
  const auto& party_access_info = task_req_ptr_->task().party_access_info();
  auto it = party_access_info.find(party_name);
  if (it == party_access_info.end()) {
    LOG(ERROR) << "invalid party: " << party_name;
    return retcode::FAIL;
  }
  auto& party_node = it->second;
  int32_t party_id = party_node.party_id();
  if (party_id == 0) {
    return retcode::SUCCESS;
  }
  std::string recv_buf;
  auto& link_ctx = this->task_ptr_->getTaskContext().getLinkContext();
  Node proxy_node;
  const auto& auxiliary_server = task_req_ptr_->task().auxiliary_server();
  it = auxiliary_server.find(PROXY_NODE);
  if (it == auxiliary_server.end()) {
    LOG(ERROR) << "no proxy node found";
    return retcode::FAIL;
  }
  const auto& pb_proxy_node = it->second;
  pbNode2Node(pb_proxy_node, &proxy_node);
  link_ctx->Recv("subtask_id", proxy_node, &recv_buf);
  *subtask_id = std::move(recv_buf);
  LOG(ERROR) << "subtask_id: " << *subtask_id;
}

retcode MPCExecutor::NegotiateSubTaskId(std::string* sub_task_id) {
  GenerateSubtaskId(sub_task_id);
  BroadcastSubtaskId(*sub_task_id);
  RecvSubTaskId(sub_task_id);
  return retcode::SUCCESS;
}

retcode MPCExecutor::SetStatisticsOperation(
    rpc::Algorithm::StatisticsOpType op_type,
    rpc::Task* task) {
  auto algorithm = task->mutable_algorithm();
  algorithm->set_function_type(rpc::Algorithm::Statistics);
  algorithm->set_statistics_op_type(op_type);
  return retcode::SUCCESS;
}

retcode MPCExecutor::SetArithmeticOperation(
    rpc::Algorithm::ArithmeticOpType op_type,
    rpc::Task* task) {
  auto algorithm = task->mutable_algorithm();
  algorithm->set_function_type(rpc::Algorithm::Arithmetic);
  algorithm->set_arithmetic_op_type(op_type);
  return retcode::SUCCESS;
}

}  // namespace primihub::task
