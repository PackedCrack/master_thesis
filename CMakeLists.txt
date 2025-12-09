cmake_minimum_required(VERSION 3.16)

SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(EXECUTABLE mal_techniques)

project("MalTechniques" LANGUAGES C)


include(${PROJECT_SOURCE_DIR}/cmake/compiler_warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/target_settings.cmake)
add_executable(${EXECUTABLE})

add_subdirectory("src")
get_target_property(TARGET_SOURCES ${EXECUTABLE} SOURCES)
source_group(TREE "${PROJECT_SOURCE_DIR}/.." FILES ${TARGET_SOURCES})

apply_compiler_warnings(${EXECUTABLE})
update_output_directory(${EXECUTABLE})
enable_enum_switch_error(${EXECUTABLE})


