load("@rules_foreign_cc//foreign_cc:defs.bzl", "cmake")

filegroup(
  name = "src",
  srcs = glob(["**"]),
  visibility = ["//visibility:public"]
)

cmake(
  name = "poco",
  cache_entries = {
    "CMAKE_INSTALL_LIBDIR": "lib",
    "BUILD_SHARED_LIBS": "NO",
    "ENABLE_ENCODINGS": "OFF",
    "ENABLE_CRYPTO": "OFF",
    "ENABLE_NETSSL": "OFF",
    "ENABLE_DATA": "OFF",
    "ENABLE_DATA_MYSQL": "OFF",
    "ENABLE_DATA_POSTGRESQL": "OFF",
    "ENABLE_DATA_SQLITE": "OFF",
    "ENABLE_DATA_ODBC": "OFF",
    "ENABLE_JWT": "OFF",
    "ENABLE_XML": "ON",
    "ENABLE_JSON": "ON",
    "ENABLE_MONGODB": "OFF",
    "ENABLE_PDF": "OFF",
    "ENABLE_UTIL": "ON",
    "ENABLE_NET": "ON",
    "ENABLE_SEVENZIP": "OFF",
    "ENABLE_ZIP": "OFF",
    "ENABLE_CPPPARSER": "OFF",
    "ENABLE_POCODOC": "OFF",
    "ENABLE_PAGECOMPILER": "OFF",
    "ENABLE_PAGECOMPILER_FILE2PAGE": "OFF",
    "ENABLE_ACTIVERECORD": "OFF",
    "ENABLE_ACTIVERECORD_COMPILER": "OFF",
    "ENABLE_REDIS": "OFF",
    "ENABLE_PROMETHEUS": "OFF",
  },
  build_args = [
    "-j10",
  ],
  lib_source = ":src",
  out_static_libs = [
    "libPocoUtil.a",
    "libPocoJSON.a",
    "libPocoXML.a",
    "libPocoNet.a",
    "libPocoFoundation.a",
  ],
  visibility = ["//visibility:public"],
)