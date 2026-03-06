# Copyright (c) 2023 dingodb.com, Inc. All Rights Reserved
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.23.1 FATAL_ERROR)
project(dingodb-cli C CXX)

option(LINK_TCMALLOC "Link tcmalloc if possible" ON)
option(DINGO_BUILD_STATIC "Link libraries statically to generate dingodb_cli" OFF)
option(WITH_VECTOR_INDEX_USE_DOCUMENT "Build with vector index use document speed up" ON)
set(THIRD_PARTY_BUILD_TYPE
    "${CMAKE_BUILD_TYPE}"
    CACHE STRING "Third-party build type")

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

if(NOT DINGO_EUREKA_INSTALL_PATH)
  if(DEFINED ENV{DINGO_EUREKA_INSTALL_PATH})
    set(DINGO_EUREKA_INSTALL_PATH $ENV{DINGO_EUREKA_INSTALL_PATH})
  else()
    set(DINGO_EUREKA_INSTALL_PATH "$ENV{HOME}/.local/dingo-eureka")
  endif()
endif()

message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
message(STATUS "THIRD_PARTY_BUILD_TYPE=${THIRD_PARTY_BUILD_TYPE}")
message(STATUS "DINGO_EUREKA_INSTALL_PATH=${DINGO_EUREKA_INSTALL_PATH}")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSE_BTHREAD_MUTEX")

if(WITH_VECTOR_INDEX_USE_DOCUMENT)
  add_definitions(-DWITH_VECTOR_INDEX_USE_DOCUMENT_SPEEDUP=1)
else()
  add_definitions(-DWITH_VECTOR_INDEX_USE_DOCUMENT_SPEEDUP=0)
endif()

if(DINGO_BUILD_STATIC)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
    set(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
  endif()
endif()

set(CMAKE_PREFIX_PATH ${DINGO_EUREKA_INSTALL_PATH} ${CMAKE_PREFIX_PATH})
set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

set(THIRD_PARTY_PATH ${CMAKE_CURRENT_BINARY_DIR}/third-party)
set(EXTERNAL_PROJECT_LOG_ARGS
    LOG_DOWNLOAD
    0
    LOG_UPDATE
    1
    LOG_CONFIGURE
    0
    LOG_BUILD
    0
    LOG_TEST
    1
    LOG_INSTALL
    0)

find_package(Threads REQUIRED)
find_package(gflags REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(Snappy REQUIRED)
find_package(fmt REQUIRED)
find_package(ZLIB REQUIRED)
find_package(glog REQUIRED)
find_package(Protobuf CONFIG REQUIRED)
find_program(PROTOC protoc REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(RapidJSON REQUIRED)
find_package(leveldb REQUIRED)
find_package(brpc REQUIRED)
find_package(braft REQUIRED)
find_package(uring REQUIRED)
find_package(unwind REQUIRED)
find_package(backtrace REQUIRED)

if(LINK_TCMALLOC)
  find_package(gperftools REQUIRED)
endif()

find_package(RocksDB REQUIRED)

include(ftxui)
include(tantivy-search)
add_subdirectory(contrib/cli11)

set(MESSAGE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/dingo-store-proto/proto)
set(MESSAGE_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto)
file(MAKE_DIRECTORY ${MESSAGE_DIR})

file(GLOB_RECURSE MSG_PROTOS ${MESSAGE_SOURCE_DIR}/*.proto)
set(PROTO_SRCS "")
set(PROTO_HDRS "")
foreach(msg ${MSG_PROTOS})
  get_filename_component(FIL_WE ${msg} NAME_WE)
  list(APPEND PROTO_SRCS "${MESSAGE_DIR}/${FIL_WE}.pb.cc")
  list(APPEND PROTO_HDRS "${MESSAGE_DIR}/${FIL_WE}.pb.h")

  add_custom_command(
    OUTPUT "${MESSAGE_DIR}/${FIL_WE}.pb.cc" "${MESSAGE_DIR}/${FIL_WE}.pb.h"
    COMMAND ${PROTOC}
    ARGS --cpp_out ${MESSAGE_DIR} -I ${MESSAGE_SOURCE_DIR} ${msg}
    DEPENDS ${msg}
    COMMENT "Running C++ protocol buffer compiler on ${msg}"
    VERBATIM)
endforeach()
set_source_files_properties(${PROTO_SRCS} ${PROTO_HDRS} PROPERTIES GENERATED TRUE)

add_library(proto_objs OBJECT ${PROTO_SRCS})
target_include_directories(proto_objs PUBLIC ${CMAKE_CURRENT_BINARY_DIR} ${DINGO_EUREKA_INSTALL_PATH}/include)
target_link_libraries(proto_objs PUBLIC protobuf::libprotobuf)
add_custom_target(
  build_proto ALL
  DEPENDS ${PROTO_SRCS} ${PROTO_HDRS}
  COMMENT "generate message target"
  VERBATIM)

file(GLOB_RECURSE SERIAL_SRCS ${PROJECT_SOURCE_DIR}/src/serial/src/*.cc)
add_library(serial STATIC ${SERIAL_SRCS})

set(CLI_SUPPORT_SRCS
    ${PROJECT_SOURCE_DIR}/src/common/cli_options.cc
    ${PROJECT_SOURCE_DIR}/src/common/helper.cc
    ${PROJECT_SOURCE_DIR}/src/common/logging.cc
    ${PROJECT_SOURCE_DIR}/src/common/role.cc
    ${PROJECT_SOURCE_DIR}/src/common/serial_helper.cc
    ${PROJECT_SOURCE_DIR}/src/common/service_access.cc
    ${PROJECT_SOURCE_DIR}/src/common/version.cc
    ${PROJECT_SOURCE_DIR}/src/coordinator/coordinator_interaction.cc
    ${PROJECT_SOURCE_DIR}/src/coprocessor/utils.cc
    ${PROJECT_SOURCE_DIR}/src/mvcc/codec.cc
    ${PROJECT_SOURCE_DIR}/src/vector/codec.cc
    ${PROJECT_SOURCE_DIR}/src/document/codec.cc)

add_library(dingodb_cli_support STATIC ${CLI_SUPPORT_SRCS})

file(GLOB CLIENT_V2_SRCS ${PROJECT_SOURCE_DIR}/src/client_v2/*.cc)
add_executable(dingodb_cli ${CLIENT_V2_SRCS} $<TARGET_OBJECTS:proto_objs>)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

execute_process(
  COMMAND git describe --tags --always --dirty
  OUTPUT_VARIABLE GIT_VERSION
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git describe --abbrev=0 --tags --always
  OUTPUT_VARIABLE GIT_TAG_NAME
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git log --pretty=format:%an -1
  OUTPUT_VARIABLE GIT_COMMIT_USER
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git log --pretty=format:%ae -1
  OUTPUT_VARIABLE GIT_COMMIT_MAIL
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git log --pretty=format:%ai -1
  OUTPUT_VARIABLE GIT_COMMIT_TIME
  OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(
  COMMAND git submodule
  WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
  RESULT_VARIABLE GIT_SUBMODULE_RESULT
  OUTPUT_VARIABLE GIT_SUBMODULE
  ERROR_QUIET
  OUTPUT_STRIP_TRAILING_WHITESPACE)

if(NOT GIT_VERSION)
  set(GIT_VERSION "unknown")
endif()
if(NOT GIT_TAG_NAME)
  set(GIT_TAG_NAME "unknown")
endif()
if(NOT GIT_COMMIT_USER)
  set(GIT_COMMIT_USER "unknown")
endif()
if(NOT GIT_COMMIT_MAIL)
  set(GIT_COMMIT_MAIL "unknown")
endif()
if(NOT GIT_COMMIT_TIME)
  set(GIT_COMMIT_TIME "unknown")
endif()
if(NOT GIT_SUBMODULE)
  set(GIT_SUBMODULE "unknown")
endif()
if(NOT GIT_SUBMODULE_RESULT EQUAL 0)
  set(GIT_SUBMODULE "unknown")
endif()

string(REPLACE "\n" "\t" GIT_SUBMODULE "${GIT_SUBMODULE}")
add_definitions(-DGIT_VERSION="${GIT_VERSION}")
add_definitions(-DGIT_TAG_NAME="${GIT_TAG_NAME}")
add_definitions(-DGIT_COMMIT_USER="${GIT_COMMIT_USER}")
add_definitions(-DGIT_COMMIT_MAIL="${GIT_COMMIT_MAIL}")
add_definitions(-DGIT_COMMIT_TIME="${GIT_COMMIT_TIME}")
add_definitions(-DGIT_SUBMODULE="${GIT_SUBMODULE}")
add_definitions(-DDINGO_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
add_definitions(-DDINGO_CONTRIB_BUILD_TYPE="${THIRD_PARTY_BUILD_TYPE}")

set(CLI_LINK_LIBS
    braft::braft
    brpc::brpc
    protobuf::libprotobuf
    gflags
    leveldb::leveldb
    RocksDB::rocksdb
    Snappy::snappy
    fmt::fmt
    glog::glog
    OpenSSL::SSL
    unwind::unwind
    unwind::unwind-generic
    backtrace::backtrace
    ZLIB::ZLIB
    ${TANTIVY_SEARCH_LIBRARIES}
    ${FTXUI_COMPONENT_LIBRARY}
    ${FTXUI_DOM_LIBRARY}
    ${FTXUI_SCREEN_LIBRARY}
    dl
    Threads::Threads)

if(LINK_TCMALLOC)
  list(APPEND CLI_LINK_LIBS gperftools::gperftools_minimal)
endif()

target_include_directories(
  serial
  PUBLIC ${PROJECT_SOURCE_DIR}/src
         ${PROJECT_SOURCE_DIR}/src/serial/src
         ${CMAKE_CURRENT_BINARY_DIR}
         ${DINGO_EUREKA_INSTALL_PATH}/include
         ${TANTIVY_SEARCH_INCLUDE_DIR}
         ${FTXUI_INCLUDE_DIR})

target_include_directories(
  dingodb_cli_support
  PUBLIC ${PROJECT_SOURCE_DIR}/src
         ${PROJECT_SOURCE_DIR}/src/serial/src
         ${CMAKE_CURRENT_BINARY_DIR}
         ${DINGO_EUREKA_INSTALL_PATH}/include
         ${TANTIVY_SEARCH_INCLUDE_DIR}
         ${FTXUI_INCLUDE_DIR})

target_include_directories(
  dingodb_cli
  PUBLIC ${PROJECT_SOURCE_DIR}/src
         ${PROJECT_SOURCE_DIR}/src/serial/src
         ${CMAKE_CURRENT_BINARY_DIR}
         ${DINGO_EUREKA_INSTALL_PATH}/include
         ${TANTIVY_SEARCH_INCLUDE_DIR}
         ${FTXUI_INCLUDE_DIR})

target_compile_definitions(dingodb_cli_support PUBLIC GLOG_CUSTOM_PREFIX_SUPPORT)
target_compile_definitions(dingodb_cli PUBLIC GLOG_CUSTOM_PREFIX_SUPPORT)

target_link_libraries(dingodb_cli_support PUBLIC serial)

target_link_libraries(dingodb_cli PRIVATE dingodb_cli_support serial CLI11::CLI11 ${CLI_LINK_LIBS})
