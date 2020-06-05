# Copyright 2017 gRPC authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

licenses(["notice"])  # 3-clause BSD

package(default_visibility = ["//visibility:public"])

load("@rules_proto//proto:defs.bzl", "proto_library")
load("@rules_cc//cc:defs.bzl", "cc_binary", "cc_proto_library", "cc_library")
load("@com_github_grpc_grpc//bazel:cc_grpc_library.bzl", "cc_grpc_library")

# The following three rules demonstrate the usage of the cc_grpc_library rule in
# in a mode compatible with the native proto_library and cc_proto_library rules.
proto_library(
    name = "foodsystem_proto",
    srcs = ["foodsystem.proto"],
)

cc_proto_library(
    name = "foodsystem_cc_proto",
    deps = [":foodsystem_proto"],
)

cc_grpc_library(
    name = "foodsystem_cc_grpc",
    srcs = [":foodsystem_proto"],
    grpc_only = True,
    deps = [":foodsystem_cc_proto"],
)

cc_library(
    name = "exporters",
    srcs = ["exporters.cc"],
    hdrs = ["exporters.h"],
    deps = [
        "@io_opencensus_cpp//opencensus/exporters/stats/stackdriver:stackdriver_exporter",
        "@io_opencensus_cpp//opencensus/exporters/stats/stdout:stdout_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/ocagent:ocagent_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/stackdriver:stackdriver_exporter",
        "@io_opencensus_cpp//opencensus/exporters/trace/stdout:stdout_exporter",
        "@com_google_absl//absl/strings",
    ],
)

cc_binary(
    name = "foodfinder",
    srcs = ["foodfinder.cc"],
    deps = [
        ":foodsystem_cc_grpc",
        ":exporters",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",

        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/tags:with_tag_map",
        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@io_opencensus_cpp//opencensus/trace:with_span",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

cc_binary(
    name = "foodvendor",
    srcs = ["foodvendor.cc"],
    deps = [
        ":foodsystem_cc_grpc",
        ":exporters",
        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",
        "@com_google_absl//absl/strings",
    ],
)



cc_binary(
    name = "foodsupplier",
    srcs = ["foodsupplier.cc"],
    deps = [
        ":foodsystem_cc_grpc",
        ":exporters",
        "@com_github_grpc_grpc//:grpc++",
        "@com_github_grpc_grpc//:grpc_opencensus_plugin",

        "@io_opencensus_cpp//opencensus/tags",
        "@io_opencensus_cpp//opencensus/tags:context_util",
        "@io_opencensus_cpp//opencensus/tags:with_tag_map",


        "@io_opencensus_cpp//opencensus/trace",
        "@io_opencensus_cpp//opencensus/trace:context_util",
        "@io_opencensus_cpp//opencensus/trace:with_span",
        "@com_google_absl//absl/strings",
        "@com_google_absl//absl/time",
    ],
)

# build docker images
load("@io_bazel_rules_docker//cc:image.bzl", "cc_image")


cc_image(
    name = "foodfinder_image",
    binary = ":foodfinder",
)

cc_image(
    name = "foodsupplier_image",
    binary = ":foodsupplier",
)

cc_image(
    name = "foodvendor_image",
    binary = ":foodvendor",
)

platform(
    name = "linux_x86",
    constraint_values = [
        "@platforms//os:linux",
        "@platforms//cpu:x86_64",
        ":glibc_2_29",
    ],
)