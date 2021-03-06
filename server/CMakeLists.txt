SET (CMAKE_CXX_FLAGS                "-Wall -Wpedantic -Wextra -g")

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
