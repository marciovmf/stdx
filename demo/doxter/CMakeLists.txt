cmake_minimum_required(VERSION 3.13)

project(stdx-tool-doxter C)

set(OUTPUT_DIR "${CMAKE_CURRENT_LIST_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY  "${OUTPUT_DIR}")

# Directory where generated headers go
set(GEN_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/generated")
file(MAKE_DIRECTORY "${GEN_DIR}")

# We use the demo/bake tool to bake templates, fonts and strings 
set(BAKE_EXE       ${CMAKE_CURRENT_SOURCE_DIR}/../bake/bake.exe)
set(BAKE_TEMPLATE_INPUT     ${CMAKE_CURRENT_SOURCE_DIR}/src/doxter_template.bake)
set(BAKE_TEMPLATE_OUTPUT    ${GEN_DIR}/doxter_template.h)

add_custom_command(
  OUTPUT ${BAKE_TEMPLATE_OUTPUT}
  COMMAND ${BAKE_EXE} ${BAKE_TEMPLATE_INPUT} -o ${BAKE_TEMPLATE_OUTPUT}
  DEPENDS ${BAKE_TEMPLATE_INPUT} ${BAKE_EXE}
  COMMENT "Baking template..."
  VERBATIM
)

set(BAKE_FONTS_INPUT     ${CMAKE_CURRENT_SOURCE_DIR}/src/doxter_fonts.bake)
set(BAKE_FONTS_OUTPUT    ${GEN_DIR}/doxter_fonts.h)
add_custom_command(
  OUTPUT ${BAKE_FONTS_OUTPUT}
  COMMAND ${BAKE_EXE} ${BAKE_FONTS_INPUT} -o ${BAKE_FONTS_OUTPUT}
  DEPENDS ${BAKE_FONTS_INPUT} ${BAKE_EXE}
  COMMENT "Baking fonts..."
  VERBATIM
)

add_custom_target(baking-step
  DEPENDS ${BAKE_TEMPLATE_OUTPUT} ${BAKE_FONTS_OUTPUT}
)

add_executable(doxter
  src/doxter.c
  src/doxter_parser.c
  ${BAKE_TEMPLATE_OUTPUT}
  ${BAKE_FONTS_OUTPUT}
)

target_include_directories(doxter
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src"
  PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../md2html/src"
  "${GEN_DIR}"
)
