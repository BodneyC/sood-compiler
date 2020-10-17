FLEX_TARGET(lexer lexer.l  ${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp)
BISON_TARGET(parser parser.y ${CMAKE_CURRENT_BINARY_DIR}/parser.cpp)

ADD_FLEX_BISON_DEPENDENCY(lexer parser)

set(SOURCE_FILES
  ${PROJECT_SOURCE_DIR}/src/ast.cpp
  ${PROJECT_SOURCE_DIR}/src/lexer.cpp
  ${PROJECT_SOURCE_DIR}/src/core-functions.cpp
  ${PROJECT_SOURCE_DIR}/src/parser.cpp
  ${PROJECT_SOURCE_DIR}/src/codegen.cpp
)

set(SOURCE_TEST_FILES ${SOURCE_FILES} PARENT_SCOPE)

add_executable(sood main.cpp ${SOURCE_FILES})
target_link_libraries(sood ${llvm_libs}) # spdlog::spdlog_header_only)
