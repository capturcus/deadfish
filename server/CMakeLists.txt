# set clang

SET (CMAKE_C_COMPILER             "/usr/bin/clang")
SET (CMAKE_C_FLAGS                "-Wall -Wpedantic -Wextra -std=c99 -g -O0")
SET (CMAKE_C_FLAGS_DEBUG          "-g -O0")
SET (CMAKE_C_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_C_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_CXX_COMPILER             "/usr/bin/clang++")
SET (CMAKE_CXX_FLAGS                "-Wall -Wpedantic -Wextra -g -O0")
SET (CMAKE_CXX_FLAGS_DEBUG          "-g -O0")
SET (CMAKE_CXX_FLAGS_MINSIZEREL     "-Os -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELEASE        "-O4 -DNDEBUG")
SET (CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")

SET (CMAKE_AR      "/usr/bin/llvm-ar")
SET (CMAKE_LINKER  "/usr/bin/llvm-ld")
SET (CMAKE_NM      "/usr/bin/llvm-nm")
SET (CMAKE_OBJDUMP "/usr/bin/llvm-objdump")
SET (CMAKE_RANLIB  "/usr/bin/llvm-ranlib")

cmake_minimum_required(VERSION 3.0.0)
project(deadfishserver VERSION 0.1.0)
include_directories("/snap/flatbuffers/current/include")

find_package(Boost COMPONENTS system program_options REQUIRED)
find_package(Threads REQUIRED)
find_package(Box2D REQUIRED)
find_package(agones CONFIG REQUIRED)

include_directories(${agones_INCLUDE_DIRS})

set(CMAKE_CXX_STANDARD 17)

file(GLOB server_SRC
  "*.cpp"
)

add_executable(deadfishserver
  ${server_SRC}
)

target_link_libraries(deadfishserver
  ${Boost_SYSTEM_LIBRARY}
  ${Boost_PROGRAM_OPTIONS_LIBRARY}
  Threads::Threads
  ${BOX2D_LIBRARY}
  agones
)
