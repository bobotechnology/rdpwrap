cmake_minimum_required(VERSION 3.16)
project(ini_configparser VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(ini_configparser STATIC
    src/parser.cpp
)

add_library(ini::configparser ALIAS ini_configparser)

target_include_directories(ini_configparser
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(ini_configparser PUBLIC cxx_std_17)

include(GNUInstallDirs)
install(TARGETS ini_configparser
    EXPORT ini_configparserTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(EXPORT ini_configparserTargets
    FILE ini_configparserTargets.cmake
    NAMESPACE ini::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ini_configparser
)

include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ini_configparserConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ini_configparserConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ini_configparserConfig.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ini_configparser
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ini_configparserConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ini_configparserConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ini_configparser
)

enable_testing()
add_executable(ini_configparser_basic_test tests/basic_test.cpp)
target_link_libraries(ini_configparser_basic_test PRIVATE ini_configparser)
add_test(NAME ini_configparser_basic_test COMMAND ini_configparser_basic_test)
