cmake_minimum_required(VERSION 3.13.4)
project(sood VERSION 1.0.1)

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -rdynamic -std=c++14 -lLLVM")

# LLVM
find_package(LLVM REQUIRED CONFIG)
message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVMConfig.cmake: ${LLVM_DIR}")

# SPDLOG
find_package(spdlog REQUIRED)
message(STATUS "spdlog version: ${spdlog_VERSION}")

# Bison
find_package(BISON)
message(STATUS "Bison version: ${BISON_VERSION}")

# Flex
find_package(FLEX)
message(STATUS "Flex version: ${FLEX_VERSION}")

# Build
include_directories(
  ${LLVM_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/include)
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

add_subdirectory(src)

# TEST
# enable_testing()
# add_subdirectory(test)
