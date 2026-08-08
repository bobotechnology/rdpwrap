cmake_minimum_required(VERSION 3.20)
project(RDPWrap LANGUAGES CXX RC)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file(src-RDP_CnC/app.rc.in "${CMAKE_BINARY_DIR}/generated/rdp_cnc.rc" @ONLY)

add_executable(manifest_embedder src-RDP_CnC/manifest_embedder.cpp)
target_compile_features(manifest_embedder PRIVATE cxx_std_17)
add_executable(RDP_CnC WIN32
  src-RDP_CnC/main.cpp
  src-RDP_CnC/admin.cpp
  "${CMAKE_BINARY_DIR}/generated/rdp_cnc.rc"
  src-RDP_CnC/app.manifest
)
target_compile_features(RDP_CnC PRIVATE cxx_std_17)
target_compile_definitions(RDP_CnC PRIVATE UNICODE _UNICODE _WIN32_WINNT=0x0A00)
target_link_libraries(RDP_CnC PRIVATE advapi32 version user32 shell32 ws2_32 wtsapi32 comctl32 uxtheme)
set_target_properties(RDP_CnC PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_custom_command(TARGET RDP_CnC POST_BUILD
  COMMAND manifest_embedder "$<TARGET_FILE:RDP_CnC>" "${CMAKE_CURRENT_SOURCE_DIR}/src-RDP_CnC/app.manifest"
  VERBATIM
)

install(TARGETS RDP_CnC RUNTIME DESTINATION .)

if(MSVC)
  target_compile_options(RDP_CnC PRIVATE /W4 /permissive-)
else()
  target_compile_options(RDP_CnC PRIVATE -Wall -Wextra -Wpedantic)
  target_link_options(RDP_CnC PRIVATE -municode)
  target_link_options(manifest_embedder PRIVATE -municode)
endif()
