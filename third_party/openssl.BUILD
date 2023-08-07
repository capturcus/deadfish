load("@rules_foreign_cc//foreign_cc:defs.bzl", "configure_make")

filegroup(
    name = "srcs",
    srcs = glob(
        include = ["**"],
        exclude = ["**/* *"],
    ),
    visibility = ["//visibility:private"],
)

configure_make(
    name = "openssl",
    args = ["-j"],
    configure_command = "Configure",
    configure_options = select({
        "@platforms//cpu:x86_64": [
            "no-comp",
            "no-idea",
            "no-weak-ssl-ciphers",
            "no-zlib",
        ],
        "@platforms//cpu:arm64":  [
            "no-comp",
            "no-idea",
            "no-weak-ssl-ciphers",
            "no-zlib",
            "linux-aarch64",
        ],
    }),
    lib_source = "@openssl//:srcs",
    out_include_dir = "include",
    out_lib_dir = select({
        "@platforms//cpu:x86_64": "lib64",
        "@platforms//cpu:arm64":  "lib",
    }),
    out_static_libs = [
        "libssl.a",
        "libcrypto.a",
    ],
    out_shared_libs = [
        "libssl.so",
        "libcrypto.so",
    ],
    targets = ["", "install"],
    visibility = ["//visibility:public"],
)

# grpc needs separate target for libssl
cc_library(
    name = "ssl",
    visibility = ["//visibility:public"],
    deps = [":openssl"],
)

# grpc needs separate target for libcrypto
cc_library(
    name = "crypto",
    visibility = ["//visibility:public"],
    deps = [":openssl"],
)
