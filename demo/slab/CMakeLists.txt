cmake_minimum_required(VERSION 3.13)

project(stdx-tool-slab C)

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")

set(SOURCES 
  src/main.c
  src/slab.c
  src/slab.h
  src/template.c
  src/commands.c
)

add_executable(slab ${SOURCES})

target_link_libraries(slab PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../minima/lib/minimalib.lib")
target_include_directories(slab
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src"
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../minima/src"
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../md2html/src"
)
