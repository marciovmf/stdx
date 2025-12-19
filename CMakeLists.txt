cmake_minimum_required(VERSION 3.13)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_STANDARD 99)
project(stdx C)

#--------------------------------------------------------------------------------
# create_test(TARGET <name> SOURCES <src1> [src2 ...] [LIBRARIES <lib1> [lib2 ...]] [NORUN])
# Creates a test executable and a custom target to run it.
# - TARGET:     Name of the test executable (required)
# - SOURCES:    Source files for the test (required)
# - LIBRARIES:  Libraries to link with the test executable (optional)
# - NORUN:      Build but do not RUN
#-------------------------------------------------------------------------------
function(create_test)
  set(options NORUN)
  set(oneValueArgs TARGET)
  set(multiValueArgs SOURCES LIBRARIES)
  cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

  if(NOT ARG_TARGET)
    message(FATAL_ERROR "create_test: TARGET is required")
  endif()
  if(NOT ARG_SOURCES)
    message(FATAL_ERROR "create_test: SOURCES is required")
  endif()

  add_executable(${ARG_TARGET} ${ARG_SOURCES})

  # Optional tweaks (safe no-ops if vars unset)
  if(DEFINED OUTPUT_NAME_SUFFIX AND NOT OUTPUT_NAME_SUFFIX STREQUAL "")
    set_target_properties(${ARG_TARGET} PROPERTIES OUTPUT_NAME "${ARG_TARGET}${OUTPUT_NAME_SUFFIX}")
  endif()
  if(DEFINED STDX_INCLUDE_DIR AND NOT STDX_INCLUDE_DIR STREQUAL "")
    target_include_directories(${ARG_TARGET} PRIVATE ${STDX_INCLUDE_DIR})
  endif()
  if(ARG_LIBRARIES)
    target_link_libraries(${ARG_TARGET} PRIVATE ${ARG_LIBRARIES})
  endif()

  # Always remember the target so we can depend on building all tests
  get_property(_all_targets GLOBAL PROPERTY STDX_ALL_TEST_TARGETS)
  list(APPEND _all_targets ${ARG_TARGET})
  set_property(GLOBAL PROPERTY STDX_ALL_TEST_TARGETS "${_all_targets}")

  # Remember its binary path; split into “all” vs “runnable” (NORUN skipped)
  get_property(_all_bins GLOBAL PROPERTY STDX_ALL_TEST_BINS)
  list(APPEND _all_bins $<TARGET_FILE:${ARG_TARGET}>)
  set_property(GLOBAL PROPERTY STDX_ALL_TEST_BINS "${_all_bins}")

  if(NOT ARG_NORUN)
    get_property(_run_bins GLOBAL PROPERTY STDX_RUN_TEST_BINS)
    list(APPEND _run_bins $<TARGET_FILE:${ARG_TARGET}>)
    set_property(GLOBAL PROPERTY STDX_RUN_TEST_BINS "${_run_bins}")
  endif()
endfunction()

function(build_and_run_tests)
# Create a custom target that depends on all individual test targets
get_property(_all_test_bins GLOBAL PROPERTY STDX_RUN_TEST_BINS)
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
endfunction()

#---------------------------------------------------------------------------
# Globals
#----------------------------------------------------------------------------

if (MSVC)
  add_compile_options(/utf-8 /WX)
elseif(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-finput-charset=UTF-8 -fexec-charset=UTF-8 -Wall -Wextra -pedantic -Werror)
endif()

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}/bin/${CMAKE_SYSTEM_NAME}_${CMAKE_HOST_SYSTEM_PROCESSOR}/$<CONFIG>")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(STDX_INCLUDE_DIR                "${CMAKE_CURRENT_LIST_DIR}/src/")

#---------------------------------------------------------------------------
# Demos
#----------------------------------------------------------------------------

# Webserver
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_CURRENT_LIST_DIR}/demo/webserver")
add_executable(webserver demo/webserver/src/webserver.c)
target_include_directories(webserver PUBLIC ${STDX_INCLUDE_DIR})

# Doxter
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_CURRENT_LIST_DIR}/demo/doxter")
add_executable(doxter demo/doxter/src/doxter.c demo/doxter/src/doxter_parser.c)
target_include_directories(doxter PUBLIC ${STDX_INCLUDE_DIR})

#-----------------------------------------------------------------------------
# Tests
#------------------------------------------------------------------------------

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
create_test(TARGET test_arena SOURCES tests/test_arena.c)
create_test(TARGET test_cpuid SOURCES tests/test_cpuid.c)
create_test(TARGET test_array SOURCES tests/test_array.c)
create_test(TARGET test_hashtable SOURCES tests/test_hashtable.c)
create_test(TARGET test_string SOURCES tests/test_string.c)
create_test(TARGET test_stringbuilder SOURCES tests/test_stringbuilder.c)
create_test(TARGET test_filesystem SOURCES tests/test_filesystem.c)
create_test(TARGET test_threading SOURCES tests/test_threading.c)
create_test(TARGET test_threadpool SOURCES tests/test_threadpool.c)
create_test(TARGET test_network SOURCES tests/test_network.c NORUN)
create_test(TARGET test_io SOURCES tests/test_io.c)
create_test(TARGET test_time SOURCES tests/test_time.c)
create_test(TARGET test_ini SOURCES tests/test_ini.c)
build_and_run_tests()
