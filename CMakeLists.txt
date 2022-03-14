cmake_minimum_required(VERSION 3.20)

SET(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(kaleidoscope LANGUAGES CXX)

add_compile_options(-std=c++20
                    -ggdb3
                    -Wall
                    -Wno-maybe-uninitialized
                    -O3)

################ LEXER ################

add_library(lexer
            SHARED
            ${kaleidoscope_SOURCE_DIR}/src/compiler/lexer/lexer.cpp)

set_target_properties(lexer
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY lib)


################ PARSER ################

add_library(parser
            SHARED
            ${kaleidoscope_SOURCE_DIR}/src/compiler/parser/parser.cpp)

target_include_directories(parser
                           PRIVATE
                           ${kaleidoscope_SOURCE_DIR}/src/compiler/lexer
                           ${kaleidoscope_SOURCE_DIR}/src)

target_link_libraries(parser
                      PRIVATE
                      lexer)

set_target_properties(parser
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY lib)




################ CODEGEN ################

add_library(codegen
            SHARED
            ${kaleidoscope_SOURCE_DIR}/src/compiler/codegen/codegen.cpp)

target_include_directories(codegen
                           PRIVATE
                           ${kaleidoscope_SOURCE_DIR}/src/compiler/parser
                           ${kaleidoscope_SOURCE_DIR}/src
                           /lib/llvm-13/include)

target_link_directories(codegen
                        PRIVATE
                        /lib/llvm-13/lib/)

target_link_libraries(codegen
                      PRIVATE
                      parser
                      LLVM-13)

target_link_options(codegen
                    PRIVATE
                    "-Wl,-rpath=/lib/llvm-13/lib/")

set_target_properties(codegen
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY lib)

add_subdirectory(test)
