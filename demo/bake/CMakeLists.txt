cmake_minimum_required(VERSION 3.13)
set(PROJECT_NAME stdx-tool-bake)
project(${PROJECT_NAME} C)

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")

add_executable(bake src/bake.c)
target_include_directories(bake
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src" "${GEN_DIR}")
