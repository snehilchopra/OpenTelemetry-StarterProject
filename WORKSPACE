load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")


http_archive(
		name = "io_opencensus_cpp",
		strip_prefix = "opencensus-cpp-master",
		urls = ["https://github.com/census-instrumentation/opencensus-cpp/archive/master.zip"],
		)



# We depend on Abseil.
http_archive(
    name = "com_google_absl",
    strip_prefix = "abseil-cpp-master",
    urls = ["https://github.com/abseil/abseil-cpp/archive/master.zip"],
)

# gRPC
http_archive(
    name = "com_github_grpc_grpc",
    strip_prefix = "grpc-master",
    urls = ["https://github.com/grpc/grpc/archive/master.tar.gz"],
)

# Google APIs - used by Stackdriver exporter.
http_archive(
    name = "com_google_googleapis",
    strip_prefix = "googleapis-master",
    urls = ["https://github.com/googleapis/googleapis/archive/master.zip"],
)

# Needed by opencensus-proto.
http_archive(
    name = "io_bazel_rules_go",
    sha256 = "9fb16af4d4836c8222142e54c9efa0bb5fc562ffc893ce2abeac3e25daead144",
    urls = [
        "https://storage.googleapis.com/bazel-mirror/github.com/bazelbuild/rules_go/releases/download/0.19.0/rules_go-0.19.0.tar.gz",
        "https://github.com/bazelbuild/rules_go/releases/download/0.19.0/rules_go-0.19.0.tar.gz",
    ],
)

# OpenCensus proto - used by OcAgent exporter.
http_archive(
    name = "opencensus_proto",
    strip_prefix = "opencensus-proto-master/src",
    urls = ["https://github.com/census-instrumentation/opencensus-proto/archive/master.zip"],
)

load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")

grpc_deps()

# grpc_deps() cannot load() its deps, this WORKSPACE has to do it.
# See also: https://github.com/bazelbuild/bazel/issues/1943
load(
    "@build_bazel_rules_apple//apple:repositories.bzl",
    "apple_rules_dependencies",
)

apple_rules_dependencies()

load(
    "@build_bazel_apple_support//lib:repositories.bzl",
    "apple_support_dependencies",
)

apple_support_dependencies()

load("@upb//bazel:repository_defs.bzl", "bazel_version_repository")

bazel_version_repository(name = "upb_bazel_version")


# Google APIs - used by Stackdriver exporter.
load("@com_google_googleapis//:repository_rules.bzl", "switched_rules_by_language")

switched_rules_by_language(
    name = "com_google_googleapis_imports",
    cc = True,
    grpc = True,
)

# Needed by @opencensus_proto.
load("@io_bazel_rules_go//go:deps.bzl", "go_register_toolchains", "go_rules_dependencies")

go_rules_dependencies()

go_register_toolchains()
