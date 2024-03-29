cmake_minimum_required(VERSION 3.20)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(llvm)
include(poco)
include(fmt)

project(kaleidoscope LANGUAGES CXX)

include_directories(/usr/include/c++/11)
link_directories(/usr/lib32)
link_libraries(stdc++)
add_compile_options(-std=c++2a
                    -ggdb3
                    -Wall
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
                           PUBLIC
                           ${kaleidoscope_SOURCE_DIR}/src
                           ${CMAKE_CURRENT_BINARY_DIR}/toolstack/include)

target_link_directories(codegen
                        PUBLIC
                        ${CMAKE_CURRENT_BINARY_DIR}/toolstack/lib/)

add_dependencies(codegen
                 llvm-project-install)

target_link_libraries(codegen
                      PUBLIC
                      parser
                      LLVM)


target_link_options(codegen
                    PUBLIC
                    "-Wl,-rpath=${CMAKE_CURRENT_BINARY_DIR}/toolstack/lib")

set_target_properties(codegen
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY lib)


################ DRIVER ################

add_library(driver
            SHARED
            ${kaleidoscope_SOURCE_DIR}/src/compiler/driver/driver.cpp)

target_include_directories(driver
                           PUBLIC
                           ${kaleidoscope_SOURCE_DIR}/src
                           ${CMAKE_CURRENT_BINARY_DIR}/toolstack/include)

target_link_directories(driver
                        PUBLIC
                        ${CMAKE_CURRENT_BINARY_DIR}/toolstack/lib)

add_dependencies(driver
                 poco-install
                 fmt-project-install)

target_link_libraries(driver
                      PUBLIC
                      fmt
                      lexer
                      parser
                      codegen
                      LLVM
                      lldELF
                      lldCommon
                      PocoFoundation
                      z)

target_link_options(driver
                    PUBLIC
                    "-Wl,-rpath=${CMAKE_CURRENT_BINARY_DIR}/toolstack/lib")

set_target_properties(driver
                      PROPERTIES
                      LIBRARY_OUTPUT_DIRECTORY lib)

add_subdirectory(test)
