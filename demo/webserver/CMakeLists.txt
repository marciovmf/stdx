cmake_minimum_required(VERSION 3.13)
project(webserver)
add_executable(webserver src/webserver.c)
target_include_directories(webserver PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src")
