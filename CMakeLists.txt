#
# Copyright (c) 2023,2024 SICK AG, Waldkirch
#
# SPDX-License-Identifier: Unlicense

cmake_minimum_required(VERSION 3.24)

message(STATUS sick_visionary_cpp_base)
project(sick_visionary_cpp_base
        VERSION 1.1.0
        DESCRIPTION "Visionary access via network"
        HOMEPAGE_URL "https://github.com/SICKAG/sick_visionary_cpp_base"
        LANGUAGES CXX)


### Options
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(VISIONARY_BASE_ENABLE_AUTOIP "Enables the SOPAS Auto-IP device scan code (needs boost's ptree)" ON)
option(VISIONARY_BASE_USE_BUNDLED_BOOST "Uses the bundled Boost implementation" ON)
option(VISIONARY_BASE_ENABLE_UNITTESTS "Enables google-test based unit tests" OFF)

### Configuration
if(WIN32)
  set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON) # If building as shared lib on Windows
endif()

### COMPILER FLAGS ###
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
set(CMAKE_COMPILE_WARNING_AS_ERROR TRUE)


### create the target
set(TARGET_NAME ${PROJECT_NAME})
add_library(${TARGET_NAME})
add_library(${TARGET_NAME}::${TARGET_NAME} ALIAS ${TARGET_NAME})


### Dependencies
set(VISIONARY_BASE_PRIVATE_INCLUDE_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/3pp/md5
  ${CMAKE_CURRENT_SOURCE_DIR}/3pp/sha256)

set(VISIONARY_BASE_LINK_LIBRARIES)

if (VISIONARY_BASE_ENABLE_AUTOIP)
  if (VISIONARY_BASE_USE_BUNDLED_BOOST)
    message(STATUS "Using bundled Boost")
    # include path must must be set explicitely, otherwise when using find_package with  3pp a previous outside find_package(Boost) would take precendence
    list(APPEND
      VISIONARY_BASE_PRIVATE_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/3pp)
  else()
    # require minimum version when property_tree was introduced - 1.41 (foreach is even older)
    find_package(Boost 1.41 REQUIRED)
    target_link_libraries(${TARGET_NAME} Boost::boost)
  endif()
endif()

find_package(Threads)
list(APPEND VISIONARY_BASE_LINK_LIBRARIES ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  list(APPEND VISIONARY_BASE_LINK_LIBRARIES wsock32 ws2_32)
endif()


### BUILD ###
if (BUILD_SHARED_LIBS)
  message(STATUS "A shared library will be generated")
endif()

set_target_properties(${TARGET_NAME} PROPERTIES
  CXX_STANDARD 11
  CXX_STANDARD_REQUIRED YES
  CXX_EXTENSIONS OFF)

set (VISIONARY_BASE_SRCS
  src/UdpSocket.cpp src/TcpSocket.cpp
  src/CoLaBProtocolHandler.cpp src/CoLa2ProtocolHandler.cpp
  src/AuthenticationLegacy.cpp src/AuthenticationSecure.cpp
  src/CoLaParameterReader.cpp src/CoLaParameterWriter.cpp
  src/CoLaCommand.cpp src/CoLaParameterReader.cpp src/CoLaParameterWriter.cpp
  src/CoLaError.cpp
  3pp/md5/MD5.cpp 3pp/sha256/SHA256.cpp
  src/VisionaryType.cpp
  src/VisionaryControl.cpp src/ControlSession.cpp
  src/VisionaryDataStream.cpp src/FrameGrabberBase.cpp
  src/VisionaryData.cpp src/VisionarySData.cpp src/VisionaryTMiniData.cpp
  src/PointCloudPlyWriter.cpp src/NetLink.cpp)

set(VISIONARY_BASE_PUBLIC_HEADERS
  include/sick_visionary_cpp_base/UdpSocket.h
  include/sick_visionary_cpp_base/TcpSocket.h
  include/sick_visionary_cpp_base/ITransport.h
  include/sick_visionary_cpp_base/CoLaBProtocolHandler.h
  include/sick_visionary_cpp_base/CoLa2ProtocolHandler.h
  include/sick_visionary_cpp_base/IProtocolHandler.h
  include/sick_visionary_cpp_base/AuthenticationLegacy.h
  include/sick_visionary_cpp_base/AuthenticationSecure.h
  include/sick_visionary_cpp_base/IAuthentication.h
  include/sick_visionary_cpp_base/CoLaParameterReader.h
  include/sick_visionary_cpp_base/CoLaParameterWriter.h
  include/sick_visionary_cpp_base/CoLaCommand.h
  include/sick_visionary_cpp_base/CoLaCommandType.h
  include/sick_visionary_cpp_base/CoLaError.h
  include/sick_visionary_cpp_base/VisionaryType.h
  include/sick_visionary_cpp_base/ControlSession.h
  include/sick_visionary_cpp_base/VisionaryControl.h
  include/sick_visionary_cpp_base/FrameGrabberBase.h
  include/sick_visionary_cpp_base/FrameGrabber.h
  include/sick_visionary_cpp_base/VisionaryDataStream.h
  include/sick_visionary_cpp_base/VisionaryData.h
  include/sick_visionary_cpp_base/VisionarySData.h
  include/sick_visionary_cpp_base/VisionaryTMiniData.h
  include/sick_visionary_cpp_base/PointCloudPlyWriter.h
  include/sick_visionary_cpp_base/PointXYZ.h
  include/sick_visionary_cpp_base/NetLink.h
  include/sick_visionary_cpp_base/VisionaryEndian.h)

if(VISIONARY_BASE_ENABLE_AUTOIP)
  message(STATUS "SOPAS AutoIP support is built")
  list(APPEND VISIONARY_BASE_SRCS src/VisionaryAutoIP.cpp)
  list(APPEND VISIONARY_BASE_PUBLIC_HEADERS include/sick_visionary_cpp_base/VisionaryAutoIP.h)
endif()

target_sources(${TARGET_NAME}
  PRIVATE
    ${VISIONARY_BASE_SRCS}
  PUBLIC FILE_SET publicHeaders
    TYPE HEADERS
    BASE_DIRS include
    FILES ${VISIONARY_BASE_PUBLIC_HEADERS}
)

target_include_directories(${TARGET_NAME}
  PUBLIC
    "$<INSTALL_INTERFACE:include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/sick_visionary_cpp_base>"
  PRIVATE
    ${VISIONARY_BASE_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME} ${VISIONARY_BASE_LINK_LIBRARIES})

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_source_files_properties(src/CoLaBProtocolHandler.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
  set_source_files_properties(src/CoLa2ProtocolHandler.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
endif() # compilers

# coverage
if(VISIONARY_BASE_ENABLE_CODE_COVERAGE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    message(STATUS "Enabling coverage instrumentation")
    list(APPEND VISIONARY_BASE_CFLAGS -fprofile-arcs -ftest-coverage)
    target_link_libraries(${TARGET_NAME} gcov)
  else()
    message(ERROR "Coverage can be only enabled for gcc")
  endif()
endif()

# apply options
target_compile_options(${TARGET_NAME} PRIVATE ${VISIONARY_BASE_CFLAGS})

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)
configure_package_config_file(cmake/${PROJECT_NAME}Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NO_SET_AND_CHECK_MACRO
  NO_CHECK_REQUIRED_COMPONENTS_MACRO
)
write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

install(TARGETS ${TARGET_NAME}
  EXPORT ${PROJECT_NAME}Targets
  FILE_SET publicHeaders
)

install(
  EXPORT ${PROJECT_NAME}Targets
  FILE ${PROJECT_NAME}Targets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  NAMESPACE ${PROJECT_NAME}::
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Unit tests
# find GoogleTest, using 'non-standard' form of e.g. CONAN packages
if(VISIONARY_BASE_ENABLE_UNITTESTS)
  message(STATUS "Enabling unit tests")
  find_package(GTest)
  if(GTest_FOUND)
    message(STATUS "Building unit tests")
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
  else()
    message(STATUS "GTest not found. Tests are not built")
  endif()
endif()
