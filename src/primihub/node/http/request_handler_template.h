#ifndef SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_TEMPLATE_H_
#define SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_TEMPLATE_H_
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "src/primihub/common/common.h"
namespace primihub {
using Poco::Net::HTTPServerRequest;
using Poco::Net::HTTPServerResponse;
template <typename Func, typename REQ, typename RES>
retcode TaskOperatorHandler(HTTPServerRequest& req, HTTPServerResponse& resp, Func func) {
  REQ rpc_req;
  RES rpc_res;
  func(rpc_req, &rpc_res);
  return retcode::SUCCESS;
}
}  // namespace primihub
#endif  // SRC_PRIMIHUB_NODE_HTTP_REQUEST_HANDLER_TEMPLATE_H_
