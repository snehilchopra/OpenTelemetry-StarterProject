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
    sha256 = "a8d6b1b354d371a646d2f7927319974e0f9e52f73a2452d2b3877118169eb6bb",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/rules_go/releases/download/v0.23.3/rules_go-v0.23.3.tar.gz",
        "https://github.com/bazelbuild/rules_go/releases/download/v0.23.3/rules_go-v0.23.3.tar.gz",
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


# DOCKER

# Download the rules_docker repository at release v0.14.2
http_archive(
    name = "io_bazel_rules_docker",
    sha256 = "3efbd23e195727a67f87b2a04fb4388cc7a11a0c0c2cf33eec225fb8ffbb27ea",
    strip_prefix = "rules_docker-0.14.2",
    urls = ["https://github.com/bazelbuild/rules_docker/releases/download/v0.14.2/rules_docker-v0.14.2.tar.gz"],
)

load(
    "@io_bazel_rules_docker//repositories:repositories.bzl",
    container_repositories = "repositories",
)
container_repositories()

load(
    "@io_bazel_rules_docker//cc:image.bzl",
    _cc_image_repos = "repositories",
)

_cc_image_repos()

load("@io_bazel_rules_docker//container:container.bzl", "container_pull")

container_pull(
            name = "official_debian_bullseye_slim",
            registry = "index.docker.io",
            repository = "library/debian",
            tag = "bullseye-20200607-slim",
)
