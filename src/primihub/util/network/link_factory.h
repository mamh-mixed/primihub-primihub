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
#ifndef SRC_PRIMIHUB_UTIL_NETWORK_LINK_FACTORY_H_
#define SRC_PRIMIHUB_UTIL_NETWORK_LINK_FACTORY_H_
#include <glog/logging.h>
#include <memory>
#include "src/primihub/util/network/link_context.h"
#include "src/primihub/util/network/grpc_link_context.h"
#include "src/primihub/util/network/http_link_context.h"
#include "src/primihub/util/util.h"

namespace primihub::network {
enum class LinkMode {
    GRPC = 0,
    RAW_SOCKET,
    HTTP,
};
struct LinkModeName {
  static constexpr const char* GRPC       = "GRPC";
  static constexpr const char* RAW_SOCKET = "RAW_SOCKET";
  static constexpr const char* HTTP       = "HTTP";
};

class LinkFactory {
 public:
  static std::unique_ptr<LinkContext> createLinkContext(
      LinkMode mode = LinkMode::GRPC) {
    if (mode == LinkMode::GRPC) {
      return std::make_unique<GrpcLinkContext>();
    } else if (mode == LinkMode::HTTP) {
      return std::make_unique<HttpLinkContext>();
    } else {
      LOG(ERROR) << "Unimplement Mode: " << static_cast<int>(mode);
    }
    return nullptr;
  }
  static LinkMode GetLinkMode(const std::string& mode_name) {
    std::string mode = strToUpper(mode_name);
    auto link_mode = primihub::network::LinkMode::GRPC;
    if (mode == LinkModeName::HTTP) {
      link_mode = primihub::network::LinkMode::HTTP;
    } else if (mode == LinkModeName::GRPC) {
      link_mode = primihub::network::LinkMode::GRPC;
    } else {
      LOG(WARNING) << "unknow mode: " << mode << " using default: GRPC";
    }
    return link_mode;
  }
};
}  // namespace primihub::network
#endif  // SRC_PRIMIHUB_UTIL_NETWORK_LINK_FACTORY_H_
