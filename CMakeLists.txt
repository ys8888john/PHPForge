cmake_minimum_required(VERSION 3.10.0)
project(PHPForge)

set(CMAKE_CXX_STANDARD 17)

# ============================================================
# LLVM 15
# ============================================================
set(LLVM_DIR "/usr/local/clang-15/lib/cmake/llvm")
find_package(LLVM 15 REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# ============================================================
# Sources
# ============================================================
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

file(GLOB LEXER_SOURCES "src/lexer/*.cpp")
file(GLOB PARSER_SOURCES "src/parser/*.cpp")
file(GLOB AST_SOURCES "src/ast/*.cpp")
file(GLOB SEMANTIC_SOURCES "src/semantic/*.cpp")
file(GLOB CODEGEN_SOURCES "src/codegen/*.cpp")

add_executable(PHPForge main.cpp)

target_sources(PHPForge PRIVATE
    ${LEXER_SOURCES}
    ${PARSER_SOURCES}
    ${AST_SOURCES}
    ${SEMANTIC_SOURCES}
    ${CODEGEN_SOURCES}
)

# ============================================================
# Linking
# ============================================================
# LLVM was built without RTTI — we must match
if (NOT MSVC)
    target_compile_options(PHPForge PRIVATE -fno-rtti)
endif()

# Use llvm_config to resolve components + system libs automatically
llvm_config(PHPForge core native orcjit)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)