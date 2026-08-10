cmake_minimum_required(VERSION 3.20)

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
configure_file(rdpwrap.rc.in
  "${CMAKE_CURRENT_BINARY_DIR}/generated/rdpwrap.rc" @ONLY)

add_library(rdpwrap SHARED
  dllmain.cpp
  cpp_configparser/src/parser.cpp
  rdpwrap_globals.cpp
  rdpwrap_utils.cpp
  rdpwrap_policy.cpp
  rdpwrap_hook.cpp
  rdpwrap_entry.cpp
  stdafx.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/generated/rdpwrap.rc"
  Export.def
)

target_compile_features(rdpwrap PRIVATE cxx_std_17)
target_compile_definitions(rdpwrap PRIVATE
  UNICODE _UNICODE _WINDOWS _USRDLL RDPWRAP_EXPORTS
  WINVER=0x0600 _WIN32_WINNT=0x0600)
target_include_directories(rdpwrap PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp_configparser/include")
target_link_libraries(rdpwrap PRIVATE shlwapi version)

target_compile_options(rdpwrap PRIVATE /W4 /permissive- /utf-8 /EHsc)
set_property(TARGET rdpwrap PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

set_target_properties(rdpwrap PROPERTIES
  PREFIX ""
  OUTPUT_NAME "rdpwrap"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS rdpwrap
  RUNTIME DESTINATION .
  LIBRARY DESTINATION .)
