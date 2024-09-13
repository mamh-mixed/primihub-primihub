package(default_visibility = ["//visibility:public"])
cc_library(
  name = "http_node_lib",
  srcs = ["http_main.cc"],
  deps = [
    ":node_http_impl",
  ],
)

cc_library(
  name = "node_http_impl",
  hdrs = [
    "request_handler_factory.h",
    "node_http_interface.h",
    "node_http_server.h",
    "node_request_handler.h",
  ],
  srcs = [
    "node_http_interface.cc",
    "node_http_server.cc",
    "node_request_handler.cc",
  ],
  deps = [
    "@poco//:poco",
    "//src/primihub/common:common_defination",
    "//src/primihub/common/config:config_lib",
    "//src/primihub/common/config:server_config",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/node:node_impl_internal",
    "//src/primihub/node:data_register_service_handler",
    "@com_github_glog_glog//:glog",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
  ],
)