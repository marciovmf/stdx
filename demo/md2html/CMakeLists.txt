cmake_minimum_required(VERSION 3.13)
  project(stdx-tool-md2html C)

set(STDX_INCLUDE_DIR  "${CMAKE_CURRENT_LIST_DIR}/../../src")
set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")

add_executable(md2html src/main.c)
target_include_directories(md2html PRIVATE ${STDX_INCLUDE_DIR})
