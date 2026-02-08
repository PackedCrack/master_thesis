cmake_minimum_required(VERSION 3.16)

SET(OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
SET(EXECUTABLE mal_techniques)

project("MalTechniques" LANGUAGES C ASM_MASM)
enable_language(ASM_MASM)

include(${PROJECT_SOURCE_DIR}/cmake/compiler_warnings.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/target_settings.cmake)
add_executable(${EXECUTABLE})

add_subdirectory("src")
get_target_property(TARGET_SOURCES ${EXECUTABLE} SOURCES)
source_group(TREE "${PROJECT_SOURCE_DIR}/.." FILES ${TARGET_SOURCES})

apply_compiler_warnings(${EXECUTABLE})
update_output_directory(${EXECUTABLE})
enable_enum_switch_error(${EXECUTABLE})

if(T1082)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1082)
endif()
if(T1083)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1083)
endif()
if(T1057)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1057)
endif()
if(T1070)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1070)
endif()
if(T1547)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1547)
endif()
if(T1059)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1059)
endif()
if(T1005)
  target_compile_definitions(${EXECUTABLE} PRIVATE T1005)
endif()


target_sources(${EXECUTABLE} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/penter.asm
)

set_source_files_properties(
    ${CMAKE_CURRENT_SOURCE_DIR}/src/penter.asm
    PROPERTIES
        LANGUAGE ASM_MASM
)