include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

# We clone the poco project at config time
# We do not use the configure/build steps
# of FetchContent because we want an isolated
# configure/build steps for this project so that
# we do not pollute our own build variables
FetchContent_Declare(
  poco
  GIT_REPOSITORY https://github.com/pocoproject/poco.git
  GIT_TAG poco-1.11.2-release
  GIT_PROGRESS TRUE
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/poco/src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/poco/build
  # we do not want FetchContent to configure the project automatically
  # so we point it to non existing subdir
  SOURCE_SUBDIR null
)
FetchContent_MakeAvailable(poco)

# We manually add poco configure step to our own
# configure phase
execute_process(
    COMMAND cmake ${poco_SOURCE_DIR}
    -DCMAKE_BUILD_TYPE=Release
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/toolstack
    WORKING_DIRECTORY "${poco_BINARY_DIR}")

# Finally we use the isolated build step only of ExternalProject
# in our own build phase
include(ExternalProject)
ExternalProject_Add(poco
  SOURCE_DIR        "${poco_SOURCE_DIR}"
  BINARY_DIR        "${poco_BINARY_DIR}"
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND "")

# create target for install step to add a
# dependency to it in the main project
ExternalProject_Add_StepTargets(poco install)
