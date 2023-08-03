load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive", "http_file")

http_archive(
    name = "bazel_skylib",
    sha256 = "66ffd9315665bfaafc96b52278f57c7e2dd09f5ede279ea6d39b2be471e7e3aa",
    urls = [
        "https://mirror.bazel.build/github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
        "https://github.com/bazelbuild/bazel-skylib/releases/download/1.4.2/bazel-skylib-1.4.2.tar.gz",
    ],
)

load("@bazel_skylib//:workspace.bzl", "bazel_skylib_workspace")

bazel_skylib_workspace()

http_archive(
    name = "rules_cc",
    urls = ["https://github.com/bazelbuild/rules_cc/releases/download/0.0.8/rules_cc-0.0.8.tar.gz"],
    sha256 = "ae46b722a8b8e9b62170f83bfb040cbf12adb732144e689985a66b26410a7d6f",
    strip_prefix = "rules_cc-0.0.8",
)

http_archive(
    name = "aspect_gcc_toolchain",
    strip_prefix = "gcc-toolchain-0.4.2",
    sha256 = "dd07660d9a28a6be19eac90a992f5a971a3db6c9d0a52814f111e41aea5afba4",
    urls = [
        "https://github.com/aspect-build/gcc-toolchain/archive/refs/tags/0.4.2.zip"
    ],
)

load(
    "@rules_cc//cc:repositories.bzl",
    "rules_cc_dependencies",
)
load("@aspect_gcc_toolchain//toolchain:repositories.bzl", "gcc_toolchain_dependencies")
load("@aspect_gcc_toolchain//toolchain:defs.bzl", "ARCHS", "gcc_register_toolchain")

rules_cc_dependencies()
gcc_toolchain_dependencies()
gcc_register_toolchain(
    name = "gcc_toolchain_x86_64",
    target_arch = ARCHS.x86_64,
)

http_archive(
    name = "com_google_protobuf",
    strip_prefix = "protobuf-23.4",
    sha256 = "9f47ba30e0db51bf58d4fdfbdedebefa8bf9b27be5a676db9336ebf623c89f9a",
    urls = [
        "https://github.com/protocolbuffers/protobuf/archive/refs/tags/v23.4.zip"
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
    build_file = "@//third_party:flatbuffers.BUILD",
    urls = [
        "https://github.com/google/flatbuffers/archive/refs/tags/v23.5.26.tar.gz",
    ],
)

