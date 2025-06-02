cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)

project(stdx C)

if (MSVC)
  add_compile_options(/utf-8)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(STDX_INCLUDE_DIR                "${CMAKE_CURRENT_LIST_DIR}/src/")

#add_executable(test_build tests/test_build.c)
#target_include_directories(test_build PUBLIC ${STDX_INCLUDE_DIR})

#------------------------------------------------------------------------------------
# create_test(TARGET <name> SOURCES <src1> [src2 ...] [LIBRARIES <lib1> [lib2 ...]])
# Creates a test executable and a custom target to run it.
# - TARGET:     Name of the test executable (required)
# - SOURCES:    Source files for the test (required)
# - LIBRARIES:  Libraries to link with the test executable (optional)
#------------------------------------------------------------------------------------
function(create_test)
  set(options "")
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES LIBRARIES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "TARGET parameter is required for create_test function.")
    return()
  endif()

  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "SOURCES parameter is required for create_test function.")
    return()
  endif()

  add_executable(${ARG_TARGET} ${ARG_SOURCES})
  set_target_properties(${ARG_TARGET} PROPERTIES OUTPUT_NAME ${ARG_TARGET}${OUTPUT_NAME_SUFFIX})
  target_include_directories(${ARG_TARGET} PUBLIC ${STDX_INCLUDE_DIR})

  add_custom_target(test_${ARG_TARGET}
    COMMAND $<TARGET_FILE:${ARG_TARGET}>
    COMMENT "Testing $<TARGET_FILE:${ARG_TARGET}>"
  )

  # Accumulate test targets in a global property
  get_property(_all_test_bins GLOBAL PROPERTY STDX_ALL_TEST_BINS)
  list(APPEND _all_test_bins $<TARGET_FILE:${ARG_TARGET}>)
  set_property(GLOBAL PROPERTY STDX_ALL_TEST_BINS "${_all_test_bins}")

  get_property(_all_tests GLOBAL PROPERTY STDX_ALL_TESTS)
  list(APPEND _all_tests ${test_target})
  set_property(GLOBAL PROPERTY STDX_ALL_TESTS "${_all_tests}")
endfunction()

#------------------------------------------------------------------------------------
# Doxter
#------------------------------------------------------------------------------------

add_executable(doxter src/doxter/main.c)
target_include_directories(doxter PUBLIC ${STDX_INCLUDE_DIR})

#------------------------------------------------------------------------------------
# Tests
#------------------------------------------------------------------------------------

create_test(TARGET test_arena SOURCES tests/test_arena.c)
create_test(TARGET test_array SOURCES tests/test_array.c)
create_test(TARGET test_hashtable SOURCES tests/test_hashtable.c)
create_test(TARGET test_string SOURCES tests/test_string.c)
create_test(TARGET test_stringbuilder SOURCES tests/test_stringbuilder.c)
create_test(TARGET test_filesystem SOURCES tests/test_filesystem.c)
create_test(TARGET test_threading SOURCES tests/test_threading.c)
create_test(TARGET test_threadpool SOURCES tests/test_threadpool.c)
create_test(TARGET test_network SOURCES tests/test_network.c)
create_test(TARGET test_io SOURCES tests/test_io.c)

# Create a custom target that depends on all individual test targets
get_property(_all_test_bins GLOBAL PROPERTY STDX_ALL_TEST_BINS)
set(_all_test_commands "")

foreach(test_bin ${_all_test_bins})
  list(APPEND _all_test_commands
    COMMAND ${CMAKE_COMMAND} -E echo "Running test: ${test_bin}"
    COMMAND ${test_bin}
  )
endforeach()

# --- Define the final target that runs all tests
add_custom_target(all_tests ALL
  ${_all_test_commands}
  COMMENT "Running all unit tests"
)
