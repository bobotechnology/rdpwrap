cmake_minimum_required(VERSION 3.20)

add_library(rdpwrap SHARED
  dllmain.cpp
  cpp_configparser/src/parser.cpp
  rdpwrap_globals.cpp
  rdpwrap_utils.cpp
  rdpwrap_policy.cpp
  rdpwrap_hook.cpp
  rdpwrap_entry.cpp
  stdafx.cpp
  Export.def
)

target_compile_features(rdpwrap PRIVATE cxx_std_17)
target_compile_definitions(rdpwrap PRIVATE
  UNICODE _UNICODE _WINDOWS _USRDLL RDPWRAP_EXPORTS)
target_include_directories(rdpwrap PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${CMAKE_CURRENT_SOURCE_DIR}/cpp_configparser/include")
target_link_libraries(rdpwrap PRIVATE shlwapi version)

# The original Visual Studio project keys its patch structures on MSVC's
# architecture macros.  Define the equivalent for MinGW builds.
if(MINGW)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    target_compile_definitions(rdpwrap PRIVATE _M_X64)
  else()
    target_compile_definitions(rdpwrap PRIVATE _M_IX86)
  endif()
  target_compile_options(rdpwrap PRIVATE -Wall -Wextra -Wpedantic)
  target_link_options(rdpwrap PRIVATE -static -static-libgcc -static-libstdc++)
elseif(MSVC)
  target_compile_options(rdpwrap PRIVATE /W4 /permissive- /utf-8)
  set_property(TARGET rdpwrap PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set_target_properties(rdpwrap PROPERTIES
  PREFIX ""
  OUTPUT_NAME "rdpwrap"
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

install(TARGETS rdpwrap
  RUNTIME DESTINATION .
  LIBRARY DESTINATION .)
