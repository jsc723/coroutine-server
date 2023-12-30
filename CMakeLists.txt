cmake_minimum_required(VERSION 3.20)
project(scheduler)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(main main.cpp)
add_executable(server echo-server.cpp)