cc_library(
    name = "agones",
    hdrs = glob([
        "agones/include/**",
        "gRPC/include/**",
    ]),
    includes = [
        "agones/include",
        "gRPC/include"
    ],
    srcs = ["agones/lib/libagones.a"],
    alwayslink = True,
    visibility = ["//visibility:public"],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@com_github_grpc_grpc//:grpc++",
    ]
)
