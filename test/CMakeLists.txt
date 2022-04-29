include(FetchContent)

FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(tests tests.cpp)

target_include_directories(tests
                           PRIVATE
                           ${kaleidoscope_SOURCE_DIR}/src
                           ${googletest_SOURCE_DIR}/googletest/include)

target_link_libraries(tests
                      PRIVATE
                      gtest_main
                      lexer
                      parser
                      codegen
                      driver)

target_link_options(tests
                    PUBLIC
                    "-rdynamic")

################ extern ################

add_library(mylib
            STATIC
            mylib.cpp)

set_target_properties(mylib
                      PROPERTIES
                      ARCHIVE_OUTPUT_DIRECTORY lib)

include(GoogleTest)

gtest_discover_tests(tests)