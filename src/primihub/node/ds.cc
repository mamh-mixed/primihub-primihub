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

#include "src/primihub/node/ds.h"
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
grpc::Status DataServiceImpl::NewDataset(grpc::ServerContext *context,
                                          const rpc::NewDatasetRequest *request,
                                          rpc::NewDatasetResponse *response) {
  VLOG(5) << "DataServiceImpl begin to ProcessNewDataset";
  ds_handler_->ProcessNewDataset(*request, response);
  return grpc::Status::OK;
}

grpc::Status DataServiceImpl::QueryResult(grpc::ServerContext* context,
    const rpc::QueryResultRequest* request,
    rpc::QueryResultResponse* response) {
  response->set_code(rpc::Status::FAIL);
  response->set_info("Unimplement");
  return grpc::Status::OK;
}

grpc::Status DataServiceImpl::DownloadData(grpc::ServerContext* context,
    const rpc::DownloadRequest* request,
    grpc::ServerWriter<rpc::DownloadRespone>* writer) {
  const auto& file_list = request->file_list();
  const auto& request_id = request->request_id();
  if (file_list.empty()) {
    std::string err_msg = "download list is empty, no data file for download";
    LOG(ERROR) << pb_util::TaskInfoToString(request_id) << err_msg;
    rpc::DownloadRespone resp;
    resp.set_info(err_msg);
    resp.set_code(rpc::Status::FAIL);
    writer->Write(resp);
    return grpc::Status::OK;
  }
  // using pipeline mode to read and send data, make sure data sequence: fifo
  ThreadSafeQueue<DataBlock> resp_queue;
  std::atomic<bool> error{false};
  std::string error_msg;
  auto read_fut = std::async(
    std::launch::async,
    [&]() -> retcode {
      try {
        auto ret = ds_handler_->DownloadDataImpl(*request, &resp_queue);
        if (ret != retcode::SUCCESS) {
          LOG(ERROR) << "DownloadDataImpl encountes error";
          error.store(true);
          resp_queue.shutdown();
          return retcode::FAIL;
        } else {
          return retcode::SUCCESS;
        }
      } catch (std::exception& e) {
        error_msg = e.what();
        error.store(true);
        resp_queue.shutdown();
      }
    });
  // read data from
  do {
    DataBlock data_block;
    resp_queue.wait_and_pop(data_block);
    if (data_block.is_last_block) {  // read end flag
      break;
    }
    rpc::DownloadRespone resp;
    if (error.load(std::memory_order::memory_order_relaxed)) {
      resp.set_info(error_msg);
      resp.set_code(rpc::Status::FAIL);
      LOG(ERROR) << pb_util::TaskInfoToString(request_id) << error_msg;
    } else {
      resp.set_info("SUCCESS");
      resp.set_code(rpc::Status::SUCCESS);
      resp.set_file_name(data_block.file_name);
      resp.set_data(data_block.data);
    }
    writer->Write(resp);
    if (error.load(std::memory_order::memory_order_relaxed)) {
      break;
    }
  } while (true);
  read_fut.get();
  return grpc::Status::OK;
}

grpc::Status DataServiceImpl::UploadData(grpc::ServerContext* context,
    grpc::ServerReader<rpc::UploadFileRequest>* reader,
    rpc::UploadFileResponse* response) {
  response->set_code(rpc::Status::FAIL);
  response->set_info("Unimplement");
  return grpc::Status::OK;
}

}  // namespace primihub
