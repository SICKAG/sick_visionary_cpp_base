#
# Copyright (c) 2023,2024 SICK AG, Waldkirch
#
# SPDX-License-Identifier: Unlicense

cmake_minimum_required(VERSION 3.16)

message(STATUS sick_visionary_cpp_base)
project(sick_visionary_cpp_base
        VERSION 1.0.0
        DESCRIPTION "Visionary access via network"
        HOMEPAGE_URL "https://github.com/SICKAG/cpp_base"
        LANGUAGES CXX)


### Options
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
option(VISIONARY_BASE_ENABLE_AUTOIP "Enables the SOPAS Auto-IP device scan code (needs boost's ptree)" ON)
option(VISIONARY_BASE_ENABLE_UNITTESTS "Enables google-test based unit tests" OFF)
option(VISIONARY_BASE_ENABLE_AUTOIP "Enables the SOPAS Auto-IP device scan code (needs boost's ptree)" ON)
option(VISIONARY_BASE_INTERNAL_USE_CONAN "(internal) Build using Conan package manager" OFF)
option(VISIONARY_BASE_USE_BUNDLED_BOOST "Uses the bundled Boost implementation" ON)


### COMPILER FLAGS ###
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()


### create the target
set(TARGET_NAME ${PROJECT_NAME})

add_library(${TARGET_NAME})


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
list(APPEND VISIONARY_BASE_LINK_LIBRARIES
  ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  list(APPEND VISIONARY_BASE_LINK_LIBRARIES
    wsock32 ws2_32)
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
  src/UdpSocket.h src/TcpSocket.h src/ITransport.h
  src/CoLaBProtocolHandler.h src/CoLa2ProtocolHandler.h src/IProtocolHandler.h
  src/AuthenticationLegacy.h src/AuthenticationSecure.h src/IAuthentication.h
  src/CoLaParameterReader.h src/CoLaParameterWriter.h
  src/CoLaCommand.h src/CoLaCommandType.h src/CoLaError.h
  src/VisionaryType.h
  src/ControlSession.h src/VisionaryControl.h
  src/FrameGrabberBase.h src/FrameGrabber.h src/VisionaryDataStream.h
  src/VisionaryData.h src/VisionarySData.h src/VisionaryTMiniData.h
  src/PointCloudPlyWriter.h src/PointXYZ.h src/NetLink.h)

if(VISIONARY_BASE_ENABLE_AUTOIP)
  message(STATUS "SOPAS AutoIP support is built")
  list(APPEND VISIONARY_BASE_SRCS src/VisionaryAutoIP.cpp)
  list(APPEND VISIONARY_BASE_PUBLIC_HEADERS src/VisionaryAutoIP.h)
endif()

target_sources(${TARGET_NAME}
  PUBLIC
  PRIVATE
    ${VISIONARY_BASE_SRCS} )

target_include_directories(${TARGET_NAME}
  PUBLIC
    "$<INSTALL_INTERFACE:include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>"
  PRIVATE
    ${VISIONARY_BASE_PRIVATE_INCLUDE_DIRS}
)

target_link_libraries(${TARGET_NAME} ${VISIONARY_BASE_LINK_LIBRARIES})

# configure warnings
if (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU") AND (NOT DEFINED VISIONARY_BASE_CFLAGS))
  set(VISIONARY_BASE_CFLAGS
    -Werror
    -Wall
    -Wextra
    -Wsign-conversion
    -Wsign-compare
    -Wfloat-equal
    -Wunused-value
    -Wunused-but-set-variable
    -Wtype-limits
    -Wshadow
    -Wconversion-null
    -pedantic
    -Wcast-align
    -Wcast-qual
    -Wformat
    -Wformat-nonliteral
    -Wformat-security
    -Winit-self
    -Wuninitialized
    -Wmissing-declarations
    -Wlogical-op
    -Wmissing-include-dirs
    -Wold-style-cast
    -Wpointer-arith
    -Wunreachable-code
    -Wnormalized=nfc
    -Wwrite-strings
    -Wvla
    -fstrict-aliasing
    -Wstrict-overflow=2
    -Woverloaded-virtual
    -Wsign-promo
    -ffunction-sections
    -fdata-sections
  )
elseif (("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC") AND (NOT DEFINED VISIONARY_BASE_CFLAGS))
  set(VISIONARY_BASE_CFLAGS
    /W4
    /WX
    /w44060
    /w44063
    /w44065
    /w45204
    /w45205
    /w44181
    /w44203
    /w44301
    /w44303
    /w44318
    /w44327
    /w44328
    /w44413
    /w44482
    /w44854
    /w45054
    /w45055
    /w45056
    /w45062
    /w45219
  )
endif() # compilers

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
  set_source_files_properties(3pp/md5/MD5.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-sign-conversion")
  set_source_files_properties(3pp/sha256/SHA256.cpp PROPERTIES COMPILE_FLAGS "-Wno-old-style-cast -Wno-cast-qual")
  set_source_files_properties(src/CoLaBProtocolHandler.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
  set_source_files_properties(src/CoLa2ProtocolHandler.cpp PROPERTIES COMPILE_FLAGS "-Wno-stringop-overflow")
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
  set_source_files_properties(3pp/md5/MD5.cpp PROPERTIES COMPILE_FLAGS "-D _CRT_SECURE_NO_WARNINGS /wd4267")
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

install(TARGETS ${TARGET_NAME}
    LIBRARY
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE
      DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME
      DESTINATION ${CMAKE_INSTALL_BINDIR}
    PUBLIC_HEADER
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    INCLUDES
      DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT
      Development
)

install(
  FILES
    ${VISIONARY_BASE_PUBLIC_HEADERS}
    TYPE
      INCLUDE
)

# Unit tests
# find GoogleTest, using 'non-standard' form of e.g. CONAN packages
if(VISIONARY_BASE_ENABLE_UNITTESTS)
  message(STATUS "Enabling unit tests")
  if(VISIONARY_BASE_INTERNAL_USE_CONAN)
    find_package(GoogleTest)
    if (GoogleTest_FOUND)
      message(STATUS "CONAN GoogleTest package found")
      set(GTest_FOUND ON)
      set(GTest_target GoogleTest::GoogleTest)
    endif()
  else()
    # now try the standard form  we unify it to "GTest"
    find_package(GTest)
    if(GTest_FOUND)
      message(STATUS "GTest found")
      set(GTest_target GTest::gtest)
    endif()
  endif()

  if(GTest_FOUND)
    enable_testing()
    add_subdirectory(${PROJECT_SOURCE_DIR}/tests)
    message(STATUS "Building unit tests")
  else()
    message(STATUS "GTest not found. Tests are not built")
  endif()
endif()
