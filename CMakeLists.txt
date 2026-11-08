cmake_minimum_required(VERSION 3.13.4)
project(sood VERSION 1.0.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

# LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVMConfig.cmake: ${LLVM_DIR}")

# SPDLOG
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# CXXOpts (header only)
find_path(CXXOPTS_INCLUDE_DIR cxxopts.hpp REQUIRED)

# Bison
find_package(BISON 3.0 REQUIRED)
message(STATUS "Bison version: ${BISON_VERSION}")

# Flex
find_package(FLEX REQUIRED)
message(STATUS "Flex version: ${FLEX_VERSION}")

# Build
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(SYSTEM ${LLVM_INCLUDE_DIRS} ${CXXOPTS_INCLUDE_DIR})
separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})

if(LLVM_LINK_LLVM_DYLIB)
  set(llvm_libs LLVM)
else()
  llvm_map_components_to_libnames(llvm_libs
    support core irreader passes bitreader bitwriter orcjit
    ${LLVM_TARGETS_TO_BUILD})
endif()

add_subdirectory(src)

# TEST
enable_testing()
add_test(NAME sood-tests
  COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/tests/run-tests.sh $<TARGET_FILE:sood>)
