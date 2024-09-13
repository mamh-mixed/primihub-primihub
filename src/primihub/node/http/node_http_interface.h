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

#ifndef SRC_PRIMIHUB_NODE_NODE_HTTP_INTERFACE_H_
#define SRC_PRIMIHUB_NODE_NODE_HTTP_INTERFACE_H_

#include <glog/logging.h>

#include <memory>
#include <thread>
#include <string>
#include <vector>

#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Timestamp.h"
#include "Poco/DateTimeFormatter.h"
#include "Poco/DateTimeFormat.h"
#include "Poco/Exception.h"
#include "Poco/ThreadPool.h"
#include "Poco/Util/ServerApplication.h"
#include "Poco/Util/Option.h"
#include "Poco/Util/OptionSet.h"
#include "Poco/Util/HelpFormatter.h"

#include "src/primihub/protos/worker.pb.h"
#include "src/primihub/protos/common.pb.h"
#include "src/primihub/node/node_impl.h"
#include "src/primihub/common/common.h"

namespace primihub {
using Poco::Net::ServerSocket;
using Poco::Net::HTTPRequestHandler;
using Poco::Net::HTTPRequestHandlerFactory;
using Poco::Net::HTTPServer;
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
using Poco::Net::HTTPServerParams;
using Poco::Timestamp;
using Poco::DateTimeFormatter;
using Poco::DateTimeFormat;
using Poco::ThreadPool;
using Poco::Util::ServerApplication;
using Poco::Util::Application;
using Poco::Util::Option;
using Poco::Util::OptionSet;
using Poco::Util::HelpFormatter;

class VMNodeHttpInterface final {
 public:
  explicit VMNodeHttpInterface(std::unique_ptr<VMNodeImpl> node_impl) :
      server_impl_(std::move(node_impl)) {
  }
  ~VMNodeHttpInterface() = default;

  retcode SubmitTask(const rpc::PushTaskRequest pushTaskRequest,
                     rpc::PushTaskReply *pushTaskReply);

  retcode ExecuteTask(const rpc::PushTaskRequest& request,
                     rpc::PushTaskReply* response);

  retcode StopTask(const rpc::TaskContext& request,
                  rpc::Empty* response);
  retcode KillTask(const rpc::KillTaskRequest& request,
                  rpc::KillTaskResponse* response);
  // task status operation
  retcode FetchTaskStatus(const rpc::TaskContext& request,
                         rpc::TaskStatusReply* response);

  retcode UpdateTaskStatus(const rpc::TaskStatus& request,
                          rpc::Empty* response);

  retcode Send(const rpc::TaskRequest& request,
              rpc::TaskResponse* response);

  retcode Recv(const rpc::TaskRequest& request,
               rpc::TaskResponse* response);
  /**
   * @param
   *
  */
  retcode SendRecv(const rpc::TaskResponse& request,
                   rpc::TaskRequest* response){return retcode::SUCCESS;}

  // for communication between different process
  // retcode ForwardSend(const rpc::ForwardTaskRequest& reader,
  //                    rpc::TaskResponse* response);

  // for communication between different process
  retcode ForwardRecv(const rpc::TaskRequest& request,
                      rpc::TaskRequest* response);
  /**
   * wait until complete queue has filled expected number of complete status
  */
  retcode CompleteStatus(const rpc::CompleteStatusRequest* request,
                        rpc::Empty* response){return retcode::SUCCESS;}

  retcode WaitUntilWorkerReady(const std::string& worker_id,
                               int timeout = -1);

  VMNodeImpl* ServerImpl() {return server_impl_.get();}
 protected:
  retcode BuildTaskResponse(const std::string& data,
      rpc::TaskResponse* response);

  retcode BuildTaskRequest(const rpc::TaskContext& task_info,
      const std::string& key,
      const std::string& data,
      rpc::TaskRequest* requests);

 private:
  std::unique_ptr<VMNodeImpl> server_impl_{nullptr};
};
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_NODE_HTTP_INTERFACE_H_
