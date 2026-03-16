cmake_minimum_required(VERSION 3.10.0)
project(PHPForge)

set(CMAKE_CXX_STANDARD 17)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

# Collect source files
file(GLOB LEXER_SOURCES "src/lexer/*.cpp")
file(GLOB PARSER_SOURCES "src/parser/*.cpp")
file(GLOB AST_SOURCES "src/ast/*.cpp")

add_executable(PHPForge main.cpp)

target_sources(PHPForge PRIVATE
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${AST_SOURCES}
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)