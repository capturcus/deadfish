filegroup(
    name = "server-hdrs",
    srcs = [
        "agones.hpp",
        "deadfish.hpp",
        "game_thread.hpp",
        "level_loader.hpp",
        "skills.hpp",
        "websocket.hpp",
    ]
)

cc_binary(
    name = "server",
    srcs = [
        "agones.cpp",
        # "deadfish.cpp",
        # "game_thread.cpp",
        # "level_loader.cpp",
        # "main.cpp",
        # "mobs.cpp",
        # "skills.cpp",
        # "websocket.cpp",
        ":server-hdrs",
    ],
    deps = [
        "@com_google_protobuf//:protobuf",
        "@agones",
        # "@flatbuffers",
    ]
)

cc_binary(
  name = "gcc_version",
  deps = ["@com_google_absl//absl/strings"],
  srcs = ["gcc_version.cpp"],
)