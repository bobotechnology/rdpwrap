cmake_minimum_required(VERSION 3.20)
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" RDPWRAP_VERSION)
string(STRIP "${RDPWRAP_VERSION}" RDPWRAP_VERSION)
if(NOT RDPWRAP_VERSION MATCHES
   "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(alpha|beta|rc)\\.[1-9][0-9]*)?$")
  message(FATAL_ERROR
    "VERSION must be x.y.z or x.y.z-(alpha|beta|rc).N: ${RDPWRAP_VERSION}")
endif()
set(RDPWRAP_NUMERIC_VERSION
  "${CMAKE_MATCH_1}.${CMAKE_MATCH_2}.${CMAKE_MATCH_3}")
project(RDPWrap VERSION "${RDPWRAP_NUMERIC_VERSION}" LANGUAGES CXX RC)
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/VERSION")

if(NOT MSVC)
  message(FATAL_ERROR "RDPWrap is built and tested with MSVC only")
endif()

add_subdirectory(src-multiarch)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/generated")
configure_file(src-RDP_CnC/app.rc.in "${CMAKE_BINARY_DIR}/generated/rdp_cnc.rc" @ONLY)

add_executable(manifest_embedder src-RDP_CnC/manifest_embedder.cpp)
target_compile_features(manifest_embedder PRIVATE cxx_std_17)
add_executable(RDP_CnC WIN32
  src-RDP_CnC/main.cpp
  "${CMAKE_BINARY_DIR}/generated/rdp_cnc.rc"
  src-RDP_CnC/app.manifest
)
target_compile_features(RDP_CnC PRIVATE cxx_std_17)
target_compile_definitions(RDP_CnC PRIVATE
  UNICODE _UNICODE WINVER=0x0600 _WIN32_WINNT=0x0600)
target_link_libraries(RDP_CnC PRIVATE
  advapi32 ole32 oleaut32 shell32 uuid version user32 wtsapi32 comctl32 uxtheme)
set_target_properties(RDP_CnC PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
add_custom_command(TARGET RDP_CnC POST_BUILD
  COMMAND manifest_embedder "$<TARGET_FILE:RDP_CnC>" "${CMAKE_CURRENT_SOURCE_DIR}/src-RDP_CnC/app.manifest"
  VERBATIM
)

# Build the installer as part of the repository-level project so the primary
# release tools are produced by one configure/build invocation.
add_subdirectory(src-installer)

install(TARGETS RDP_CnC RUNTIME DESTINATION .)

target_compile_options(RDP_CnC PRIVATE /W4 /permissive- /utf-8)
target_link_options(RDP_CnC PRIVATE /MANIFEST:NO)
set_property(TARGET RDP_CnC manifest_embedder PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
