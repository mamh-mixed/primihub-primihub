package(default_visibility = ["//visibility:public"])

cc_library(
  name = "node_proxy_impl",
  hdrs = [
    "node_interface.h",
  ],
  srcs = [
    "main.cc",
    "node_interface.cc",
  ],
  deps = [
    ":data_register_service",
    "//src/primihub/common:common_defination",
    "//src/primihub/node/worker:worker_lib_impl",
    "//src/primihub/protos:worker_proto",
    "//src/primihub/common/config:config_lib",
    "//src/primihub/util:util_lib",
    "//src/primihub/util:log_util",
    "//src/primihub/util:pb_log_helper",
    "@com_github_glog_glog//:glog",
    "@com_github_grpc_grpc//:grpc++",
    "@com_google_absl//absl/flags:flag",
    "@com_google_absl//absl/flags:parse",
    "//src/primihub/util/network:communication_lib",
    "@arrow"
  ],
)

cc_library(
  name = "data_register_service",
  hdrs = ["data_service_proxy.h"],
  srcs = ["data_service_proxy.cc"],
  deps = [
    "//src/primihub/protos:service_proto",
    "//src/primihub/util:threadsafe_queue",
    "//src/primihub/util:pb_log_helper",
    "@com_github_grpc_grpc//:grpc++",
    "//src/primihub/common:common_defination",
    "//src/primihub/util/network:communication_lib"
  ],
)


