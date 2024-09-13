/*
 Copyright 2022 PrimiHub

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

      https://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 */

#include "src/primihub/node/ds_handler.h"
#include <unistd.h>
#include <sys/stat.h>
#include <algorithm>
#include <string>
#include <utility>
#include <thread>
#include <future>
#include <fstream>
#include <nlohmann/json.hpp>
#include "src/primihub/service/dataset/model.h"
#include "src/primihub/util/util.h"
#include "src/primihub/common/common.h"
#include "src/primihub/util/thread_local_data.h"
#include "src/primihub/util/proto_log_helper.h"
#include "src/primihub/common/config/server_config.h"
#include "src/primihub/util/file_util.h"

using DatasetMeta = primihub::service::DatasetMeta;
namespace pb_util = primihub::proto::util;
namespace primihub {
retcode DataServiceHandler::ProcessNewDataset(
    const rpc::NewDatasetRequest& request,
    rpc::NewDatasetResponse *response) {
//
  DatasetMetaInfo meta_info;
  // convert pb to DatasetMetaInfo
  ConvertToDatasetMetaInfo(request.meta_info(), &meta_info);
  std::set<std::string> dup;
  for (const auto& info : meta_info.schema) {
    auto& field_name = std::get<0>(info);
    if (dup.find(field_name) != dup.end()) {
      std::string err_msg = "Duplicate Field name: ";
      err_msg.append(field_name).append(" in dataset Schema");
      LOG(ERROR) << err_msg;
      SetResponseErrorMsg(std::move(err_msg), response);
      return retcode::FAIL;
    }
    dup.insert(field_name);
  }

  auto op_type = request.op_type();
  switch (op_type) {
    case rpc::NewDatasetRequest::REGISTER:
      RegisterDatasetProcess(meta_info, response);
      break;
    case rpc::NewDatasetRequest::UNREGISTER:
      UnRegisterDatasetProcess(meta_info, response);
      break;
    case rpc::NewDatasetRequest::UPDATE:
      UpdateDatasetProcess(meta_info, response);
      break;
    default: {
      std::string err_msg = "unsupported operation type: ";
      err_msg.append(std::to_string(op_type));
      LOG(ERROR) << err_msg;
      SetResponseErrorMsg(std::move(err_msg), response);
    }
  }
  return retcode::SUCCESS;
}

retcode DataServiceHandler::DownloadDataImpl(
    const rpc::DownloadRequest& request,
    ThreadSafeQueue<DataBlock>* data_queue) {
  const auto& request_id = request.request_id();
  const auto& file_list = request.file_list();
  for (const auto& file_info : file_list) {
    std::string file_name = file_info;
    std::string file_path = CompletePath(file_info);
    LOG(INFO) << pb_util::TaskInfoToString(request_id)
              << "begin to read data for " << file_name;
    if (!FileExists(file_path)) {
      std::string err_msg = "file: ";
      err_msg.append(file_path).append(" is not exist");
      LOG(ERROR) << pb_util::TaskInfoToString(request_id) << err_msg;
      throw std::runtime_error(err_msg);
      return retcode::FAIL;
    }
    struct stat statbuf;
    stat(file_path.c_str(), &statbuf);
    size_t filesize = statbuf.st_size;
    VLOG(5) << pb_util::TaskInfoToString(request_id)
            << "file size: " << filesize;

    std::ifstream fin(file_path, std::ios::binary);
    if (fin) {
      size_t block_size = LIMITED_PACKAGE_SIZE;
      size_t block_bum = filesize / block_size;
      for (size_t i = 0 ; i < block_bum; i++) {
        DataBlock data_block;
        data_block.file_name = file_name;
        data_block.data.resize(block_size);
        auto& buf = data_block.data;
        fin.read(&buf[0], block_size);
        data_queue->push(std::move(data_block));
      }
      size_t last_block_size = filesize % block_size;
      LOG(INFO) << pb_util::TaskInfoToString(request_id)
                << "last_block_size: " << last_block_size;
      if (last_block_size) {
        DataBlock data_block;
        data_block.file_name = file_name;
        data_block.data.resize(last_block_size);
        auto& buf = data_block.data;
        fin.read(&buf[0], last_block_size);
        data_queue->push(std::move(data_block));
      }
      fin.close();
      // flag for end read
      DataBlock data_block;
      data_block.is_last_block = true;
      data_queue->push(std::move(data_block));
    } else {
      std::string err_msg;
      err_msg.append("open file ").append(file_path).append(" failed");
      LOG(ERROR) << pb_util::TaskInfoToString(request_id) << err_msg;
      throw std::runtime_error(err_msg);
    }
  }
  return retcode::SUCCESS;
}

retcode DataServiceHandler::UploadDataImpl(const rpc::DownloadRequest& request,
    ThreadSafeQueue<DataBlock>* data_queue) {
//
  return retcode::SUCCESS;
}

retcode DataServiceHandler::QueryResultImpl(const rpc::QueryResultRequest& request,
    rpc::QueryResultResponse* response) {
//
  return retcode::SUCCESS;
}

retcode DataServiceHandler::RegisterDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  auto& driver_type = meta_info.driver_type;
  VLOG(2) << "start to create dataset."
      << "meta info: " << meta_info.access_info << " "
      << "fid: " << meta_info.id << " "
      << "driver_type: " << meta_info.driver_type;
  std::shared_ptr<DataDriver> driver{nullptr};
  std::string access_meta;
  try {
    auto access_info =
        this->GetDatasetService()->createAccessInfo(driver_type, meta_info);
    if (access_info == nullptr) {
      std::string err_msg = "create access info failed";
      throw std::invalid_argument(err_msg);
    }
    access_meta = access_info->toString();
    driver = DataDirverFactory::getDriver(driver_type,
                                          DatasetLocation(),
                                          std::move(access_info));
    this->GetDatasetService()->registerDriver(meta_info.id, driver);
  } catch (std::exception& e) {
    size_t len = strlen(e.what());
    len = len > 1024 ? 1024 : len;
    auto err_msg = std::string(e.what(), len);
    LOG(ERROR) << "Failed to load dataset from: "
            << meta_info.access_info << " "
            << "driver_type: " << driver_type << " "
            << "fid: " << meta_info.id << " "
            << "exception: " << err_msg;
    SetResponseErrorMsg(err_msg, reply);
    return retcode::FAIL;
  }


  try {
    DatasetMeta mate;
    auto dataset = GetDatasetService()->newDataset(
        driver, meta_info.id, access_meta, &mate);
    if (dataset == nullptr) {
      LOG(ERROR) << "register dataset " << meta_info.id << " failed";
      auto& err_msg = ThreadLocalErrorMsg();
      SetResponseErrorMsg(err_msg, reply);
      ResetThreadLocalErrorMsg();
      this->GetDatasetService()->unRegisterDriver(meta_info.id);
      return retcode::FAIL;
    } else {
      reply->set_ret_code(rpc::NewDatasetResponse::SUCCESS);
      reply->set_dataset_url(mate.getDataURL());
      LOG(INFO) << "end of register dataset, dataurl: " << mate.getDataURL();
    }
  } catch (std::exception& e) {
    size_t len = strlen(e.what());
    len = len > 1024 ? 1024 : len;
    auto err_msg = std::string(e.what(), len);
    LOG(ERROR) << "Failed to register dataset from: "
            << meta_info.access_info << " "
            << "driver_type: " << driver_type << " "
            << "fid: " << meta_info.id << " "
            << "exception: " << err_msg;
    SetResponseErrorMsg(err_msg, reply);
    return retcode::FAIL;
  }


  return retcode::SUCCESS;
}

retcode DataServiceHandler::UnRegisterDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  reply->set_ret_code(rpc::NewDatasetResponse::SUCCESS);
  return retcode::SUCCESS;
}

retcode DataServiceHandler::UpdateDatasetProcess(
    const DatasetMetaInfo& meta_info,
    rpc::NewDatasetResponse* reply) {
  return RegisterDatasetProcess(meta_info, reply);
}

retcode DataServiceHandler::ConvertToDatasetMetaInfo(
    const rpc::MetaInfo& meta_info_pb,
    DatasetMetaInfo* meta_info_ptr) {
  auto& meta_info = *meta_info_ptr;
  meta_info.driver_type = meta_info_pb.driver();
  meta_info.access_info = meta_info_pb.access_info();
  meta_info.id = meta_info_pb.id();
  meta_info.visibility = static_cast<Visibility>(meta_info_pb.visibility());
  auto& schema = meta_info.schema;
  const auto& data_field_type = meta_info_pb.data_type();
  for (const auto& field : data_field_type) {
    auto& name = field.name();
    int type = field.type();
    schema.push_back(std::make_tuple(name, type));
  }
  return retcode::SUCCESS;
}
}  // namespace primihub
