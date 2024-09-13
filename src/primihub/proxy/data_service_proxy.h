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

#ifndef SRC_PRIMIHUB_PROXY_DATA_SERVICE_PROXY_H_
#define SRC_PRIMIHUB_PROXY_DATA_SERVICE_PROXY_H_

#include <glog/logging.h>
#include <grpc/grpc.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <memory>
#include <string>

#include "src/primihub/protos/service.grpc.pb.h"
#include "src/primihub/protos/service.pb.h"
#include "src/primihub/common/common.h"
#include "src/primihub/util/network/link_context.h"

namespace primihub {
using LinkContext = primihub::network::LinkContext;
class DataServiceProxyImpl final: public rpc::DataSetService::Service {
 public:
  explicit DataServiceProxyImpl(LinkContext* link_ctx, const Node& dest_node) :
      link_ctx_(link_ctx), dest_node_(dest_node) {}

  grpc::Status NewDataset(grpc::ServerContext *context,
                          const rpc::NewDatasetRequest *request,
                          rpc::NewDatasetResponse *response) override;
  // grpc::Status QueryResult(grpc::ServerContext* context,
  //                          const rpc::QueryResultRequest* request,
  //                          rpc::QueryResultResponse* response);
  // grpc::Status DownloadData(grpc::ServerContext* context,
  //                           const rpc::DownloadRequest* request,
  //                           grpc::ServerWriter<rpc::DownloadRespone>* writer);
  // grpc::Status UploadData(grpc::ServerContext* context,
  //                         grpc::ServerReader<rpc::UploadFileRequest>* reader,
  //                         rpc::UploadFileResponse* response);

 private:
  LinkContext* link_ctx_{nullptr};
  Node dest_node_;
};

}  // namespace primihub
#endif  // SRC_PRIMIHUB_PROXY_DATA_SERVICE_PROXY_H_
