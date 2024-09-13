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

#include "src/primihub/proxy/data_service_proxy.h"
#include "src/primihub/util/proto_log_helper.h"

namespace pb_util = primihub::proto::util;
namespace primihub {
grpc::Status DataServiceProxyImpl::NewDataset(grpc::ServerContext *context,
    const rpc::NewDatasetRequest *request,
    rpc::NewDatasetResponse *response) {
  VLOG(5) << "DataServiceImpl begin to ProcessNewDataset";
  auto channel = this->link_ctx_->getChannel(dest_node_);
  channel->NewDataset(*request, response);
  return grpc::Status::OK;
}

// grpc::Status DataServiceImpl::QueryResult(grpc::ServerContext* context,
//     const rpc::QueryResultRequest* request,
//     rpc::QueryResultResponse* response) {
//   response->set_code(rpc::Status::FAIL);
//   response->set_info("Unimplement");
//   return grpc::Status::OK;
// }

// grpc::Status DataServiceImpl::DownloadData(grpc::ServerContext* context,
//     const rpc::DownloadRequest* request,
//     grpc::ServerWriter<rpc::DownloadRespone>* writer) {
//   return grpc::Status::OK;
// }

// grpc::Status DataServiceImpl::UploadData(grpc::ServerContext* context,
//     grpc::ServerReader<rpc::UploadFileRequest>* reader,
//     rpc::UploadFileResponse* response) {
//   return grpc::Status::OK;
// }

}  // namespace primihub
