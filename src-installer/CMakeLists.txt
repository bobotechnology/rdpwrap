cmake_minimum_required(VERSION 3.20)
project(RDPWInst LANGUAGES CXX)

if(NOT WIN32)
  message(FATAL_ERROR "RDPWInst is a Windows-only application")
endif()
if(NOT MSVC)
  message(FATAL_ERROR "RDPWInst is built and tested with MSVC only")
endif()

# The repository-level INI is the maintained source of truth.  Do not keep a
# second, silently ageing copy inside the Delphi .res container.
set(INSTALLER_CONFIG_INI "${CMAKE_CURRENT_SOURCE_DIR}/../res/rdpwrap.ini"
    CACHE FILEPATH "INI file embedded as the installer's CONFIG resource")
set(INSTALLER_CONFIG_ARM_INI "${CMAKE_CURRENT_SOURCE_DIR}/../res/rdpwrap-arm-kb.ini"
    CACHE FILEPATH "ARM INI file embedded as the installer's CONFIG_ARM resource")
if(NOT EXISTS "${INSTALLER_CONFIG_INI}")
  message(FATAL_ERROR "Installer CONFIG INI not found: ${INSTALLER_CONFIG_INI}")
endif()
if(NOT EXISTS "${INSTALLER_CONFIG_ARM_INI}")
  message(FATAL_ERROR "Installer ARM CONFIG INI not found: ${INSTALLER_CONFIG_ARM_INI}")
endif()

set(INSTALLER_RDPW32 "" CACHE FILEPATH "Optional x86 rdpwrap.dll embedded as RDPW32")
set(INSTALLER_RDPW64 "" CACHE FILEPATH "Optional x64 rdpwrap.dll embedded as RDPW64")
set(INSTALLER_RDPWARM "" CACHE FILEPATH "Optional ARM32 rdpwrap.dll embedded as RDPWARM")
set(INSTALLER_RDPWARM64 "" CACHE FILEPATH "Optional ARM64 rdpwrap.dll embedded as RDPWARM64")
set(INSTALLER_RDP_CNC "" CACHE FILEPATH "Optional RDP_CnC.exe embedded as RDP_CNC")
set(INSTALLER_RESOURCE_PATCHER "" CACHE FILEPATH
    "Optional native-host installer_resource_patcher used for cross-compiling")

# In the repository build, embed the native Wrapper and RDP_CnC produced by
# this build. A Win32 aggregate release supplies the other three Wrappers from
# separate cross-architecture builds.
if(NOT PROJECT_IS_TOP_LEVEL)
  set(INSTALLER_RDP_CNC "$<TARGET_FILE:RDP_CnC>")
  if(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM64")
    set(INSTALLER_RDPWARM64 "$<TARGET_FILE:rdpwrap>")
  elseif(CMAKE_VS_PLATFORM_NAME STREQUAL "ARM")
    set(INSTALLER_RDPWARM "$<TARGET_FILE:rdpwrap>")
  elseif(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(INSTALLER_RDPW64 "$<TARGET_FILE:rdpwrap>")
  else()
    set(INSTALLER_RDPW32 "$<TARGET_FILE:rdpwrap>")
    if(INSTALLER_RDPW64 STREQUAL "" OR INSTALLER_RDPWARM STREQUAL "" OR
       INSTALLER_RDPWARM64 STREQUAL "")
      message(WARNING
        "Win32 RDPWInst is missing one or more cross-architecture Wrappers. "
        "Set INSTALLER_RDPW64, INSTALLER_RDPWARM, and INSTALLER_RDPWARM64 "
        "for a complete aggregate release.")
    endif()
  endif()
endif()

set(RESOURCE_REPLACEMENT_DEPENDS
  "${INSTALLER_CONFIG_INI}" "${INSTALLER_CONFIG_ARM_INI}")
foreach(resource_name IN ITEMS RDPW32 RDPW64 RDPWARM RDPWARM64 RDP_CNC)
  if(NOT "${INSTALLER_${resource_name}}" STREQUAL "")
    list(APPEND RESOURCE_REPLACEMENT_DEPENDS "${INSTALLER_${resource_name}}")
  endif()
endforeach()

add_executable(RDPWInst RDPWInst.cpp)
target_compile_features(RDPWInst PRIVATE cxx_std_17)
target_compile_definitions(RDPWInst PRIVATE UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
target_link_libraries(RDPWInst PRIVATE
  advapi32
  ole32
  shell32
  wininet
  version
)

set(LEGACY_RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/resource.res")
set_source_files_properties("${LEGACY_RESOURCE}" PROPERTIES EXTERNAL_OBJECT TRUE)
target_sources(RDPWInst PRIVATE "${LEGACY_RESOURCE}")
target_compile_options(RDPWInst PRIVATE /W4 /permissive- /utf-8)
target_link_options(RDPWInst PRIVATE /MANIFEST:NO)
set_property(TARGET RDPWInst PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# A cross-compiled ARM patcher cannot execute on the x64 build runner. Allow
# the caller to supply a host-native patcher produced by the Win32 build.
if(INSTALLER_RESOURCE_PATCHER STREQUAL "")
  add_executable(installer_resource_patcher resource_patcher.cpp)
  target_compile_features(installer_resource_patcher PRIVATE cxx_std_17)
  target_compile_definitions(installer_resource_patcher PRIVATE
    UNICODE _UNICODE WIN32_LEAN_AND_MEAN NOMINMAX)
  target_compile_options(installer_resource_patcher PRIVATE /W4 /permissive- /utf-8)
  set_property(TARGET installer_resource_patcher PROPERTY
    MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
  set(INSTALLER_RESOURCE_PATCHER_COMMAND "$<TARGET_FILE:installer_resource_patcher>")
  add_dependencies(RDPWInst installer_resource_patcher)
else()
  if(NOT EXISTS "${INSTALLER_RESOURCE_PATCHER}")
    message(FATAL_ERROR
      "Installer resource patcher not found: ${INSTALLER_RESOURCE_PATCHER}")
  endif()
  set(INSTALLER_RESOURCE_PATCHER_COMMAND "${INSTALLER_RESOURCE_PATCHER}")
endif()

set(APP_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/RDPWInst.manifest")
set_property(TARGET RDPWInst APPEND PROPERTY LINK_DEPENDS
  "${APP_MANIFEST}" "${LEGACY_RESOURCE}" ${RESOURCE_REPLACEMENT_DEPENDS})
if(NOT PROJECT_IS_TOP_LEVEL)
  add_dependencies(RDPWInst rdpwrap RDP_CnC)
endif()
add_custom_command(TARGET RDPWInst POST_BUILD
  COMMAND "${INSTALLER_RESOURCE_PATCHER_COMMAND}"
          "$<TARGET_FILE:RDPWInst>"
          "${APP_MANIFEST}"
          "${INSTALLER_CONFIG_INI}"
          "${INSTALLER_CONFIG_ARM_INI}"
          "${INSTALLER_RDPW32}"
          "${INSTALLER_RDPW64}"
          "${INSTALLER_RDPWARM}"
          "${INSTALLER_RDPWARM64}"
          "${INSTALLER_RDP_CNC}"
  VERBATIM
  COMMENT "Updating installer payloads and asInvoker manifest"
)

if(NOT PROJECT_IS_TOP_LEVEL)
  set_target_properties(RDPWInst PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
  set(INSTALLER_INSTALL_DESTINATION ".")
else()
  set(INSTALLER_INSTALL_DESTINATION "bin")
endif()

install(TARGETS RDPWInst RUNTIME DESTINATION "${INSTALLER_INSTALL_DESTINATION}")
