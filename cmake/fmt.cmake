include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

# We clone the fmt project at config time
# We do not use the configure/build steps
# of FetchContent because we want an isolated
# configure/build steps for this project so that
# we do not pollute our own build variables
FetchContent_Declare(
  fmt-project
  GIT_REPOSITORY https://github.com/fmtlib/fmt.git
  GIT_TAG 8.1.1
  GIT_PROGRESS TRUE
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/fmt-project/src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/fmt-project/build
  # we do not want FetchContent to configure the project automatically
  # so we point it to non existing subdir
  SOURCE_SUBDIR null
)
FetchContent_MakeAvailable(fmt-project)

# We manually add fmt configure step to our own
# configure phase
execute_process(
    COMMAND cmake ${fmt-project_SOURCE_DIR}
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/toolstack
    -DBUILD_SHARED_LIBS=TRUE
    WORKING_DIRECTORY "${fmt-project_BINARY_DIR}")

# Finally we use the isolated build step only of ExternalProject
# in our own build phase
include(ExternalProject)
ExternalProject_Add(fmt-project
  SOURCE_DIR        "${fmt-project_SOURCE_DIR}"
  BINARY_DIR        "${fmt-project_BINARY_DIR}"
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND "")