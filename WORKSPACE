load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

BAZEL_TOOLCHAIN_TAG = "0.8.2"
BAZEL_TOOLCHAIN_SHA = "0fc3a2b0c9c929920f4bed8f2b446a8274cad41f5ee823fd3faa0d7641f20db0"

http_archive(
    name = "com_grail_bazel_toolchain",
    sha256 = BAZEL_TOOLCHAIN_SHA,
    strip_prefix = "bazel-toolchain-{tag}".format(tag = BAZEL_TOOLCHAIN_TAG),
    canonical_id = BAZEL_TOOLCHAIN_TAG,
    url = "https://github.com/grailbio/bazel-toolchain/archive/refs/tags/{tag}.tar.gz".format(tag = BAZEL_TOOLCHAIN_TAG),
)

load("@com_grail_bazel_toolchain//toolchain:deps.bzl", "bazel_toolchain_dependencies")

bazel_toolchain_dependencies()

load("@com_grail_bazel_toolchain//toolchain:rules.bzl", "llvm_toolchain")

llvm_toolchain(
    name = "llvm_toolchain",
    llvm_version = "14.0.0",
)

load("@llvm_toolchain//:toolchains.bzl", "llvm_register_toolchains")

llvm_register_toolchains()

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-21.3",
    sha256 = "8b7178046cddb00c200c0b9920af1ffa10e06f9fb46ef66fced4a49d831f6dd9",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v21.3.zip"
    ],
)

load("@com_google_protobuf//:protobuf_deps.bzl", "protobuf_deps")

protobuf_deps()

http_archive(
    name = "agones",
    build_file = "@//third_party:agones.BUILD",
    sha256 = "5f64e233ba02b62aaabfd511b040f3b4ae2e0fdfdf8894c23a76f62b5268f10d",
    urls = [
        "https://github.com/googleforgames/agones/releases/download/v1.33.0/agonessdk-1.33.0-linux-arch_64.tar",
    ],
)

http_archive(
    name = "flatbuffers",
    sha256 = "1cce06b17cddd896b6d73cc047e36a254fb8df4d7ea18a46acf16c4c0cd3f3f3",
    # build_file = "@//third_party:flatbuffers.BUILD",
    strip_prefix = "flatbuffers-23.5.26",
    urls = [
        "https://github.com/google/flatbuffers/archive/refs/tags/v23.5.26.tar.gz",
    ],
)

GRPC_COMMIT_SHA = "b39ffcc425ea990a537f98ec6fe6a1dcb90470d7"

http_archive(
    name = "com_github_grpc_grpc",
    patch_args = ["-p1"],
    patches = ["//third_party:grpc.openssl.patch"],
    urls = [
        "https://github.com/grpc/grpc/archive/{sha}.tar.gz".format(sha = GRPC_COMMIT_SHA),
    ],
    strip_prefix = "grpc-{sha}".format(sha = GRPC_COMMIT_SHA),
)
load("@com_github_grpc_grpc//bazel:grpc_deps.bzl", "grpc_deps")
grpc_deps()
load("@com_github_grpc_grpc//bazel:grpc_extra_deps.bzl", "grpc_extra_deps")
grpc_extra_deps(ignore_version_differences = True)

http_archive(
    name = "openssl",
    build_file = "@//third_party:openssl.BUILD",
    sha256 = "e8f73590815846db286d215950fdef9b882bb6b886d50acb431c0285782fe35b",
    strip_prefix = "openssl-openssl-3.0.7",
    urls = [
        "https://github.com/openssl/openssl/archive/refs/tags/openssl-3.0.7.tar.gz",
    ],
)

# @rules_foreign_cc compile C/C++ code via non-bazel build system
http_archive(
    name = "rules_foreign_cc",
    sha256 = "2a4d07cd64b0719b39a7c12218a3e507672b82a97b98c6a89d38565894cf7c51",
    strip_prefix = "rules_foreign_cc-0.9.0",
    urls = [
        "https://github.com/bazelbuild/rules_foreign_cc/archive/refs/tags/0.9.0.tar.gz",
    ],
)

load("@rules_foreign_cc//foreign_cc:repositories.bzl", "rules_foreign_cc_dependencies")

# This sets up some common toolchains for building targets. For more details, please see
# https://bazelbuild.github.io/rules_foreign_cc/0.9.0/flatten.html#rules_foreign_cc_dependencies
rules_foreign_cc_dependencies()
