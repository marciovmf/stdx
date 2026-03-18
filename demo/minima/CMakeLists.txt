cmake_minimum_required(VERSION 3.13)

project(stdx-tool-minima C)

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}/lib")

set(SOURCES 
  src/mi_parser.c
  src/mi_runtime.c
  src/mi_builtins.c
  src/minima.h
)

add_library(minimalib ${SOURCES})
target_include_directories(minimalib
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src"
  "${GEN_DIR}"
)
  
add_executable(minima src/main.c ${SOURCES})
target_link_libraries(minima PRIVATE minimalib)
target_include_directories(minima
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src"
  "${GEN_DIR}"
)

add_executable(test_minima src/test_minima.c ${SOURCES})
target_link_libraries(test_minima PRIVATE minimalib)
target_include_directories(test_minima
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src"
  "${GEN_DIR}"
)
