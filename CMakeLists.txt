cmake_minimum_required(VERSION 3.13.4)
project(sood)

# set(CMAKE_VERBOSE_MAKEFILE on)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# TODO: Get rid of the very specific LLVM-10 option
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -lLLVM-10")

# LLVM
find_package(LLVM REQUIRED CONFIG)
# find_package(spdlog REQUIRED)

find_package(BISON)
message(STATUS "Bison version: ${BISON_VERSION}")

find_package(FLEX)
message(STATUS "Flex version: ${FLEX_VERSION}")

message(STATUS "LLVM version: ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

llvm_map_components_to_libnames(llvm_libs support core irreader)

# BUILD
add_subdirectory(src)

# TEST
# enable_testing()
# add_subdirectory(test)
