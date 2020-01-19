cmake_minimum_required(VERSION 3.0.0)
project(deadfishserver VERSION 0.1.0)

find_package(Boost COMPONENTS system REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_executable(deadfishserver main.cpp)

target_link_libraries(deadfishserver
  ${Boost_SYSTEM_LIBRARY}
)