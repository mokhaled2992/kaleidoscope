include(FetchContent)

Set(FETCHCONTENT_QUIET FALSE)

# We clone the llvm project at config time
# We do not use the configure/build steps
# of FetchContent because we want an isolated
# configure/build steps for this project so that
# we do not pollute our own build variables
FetchContent_Declare(
  llvm-project
  GIT_REPOSITORY https://github.com/llvm/llvm-project.git
  GIT_TAG llvmorg-14.0.0
  GIT_PROGRESS TRUE
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-project/src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/llvm-project/build
  # we do not want FetchContent to configure the project automatically
  # so we point it to non existing subdir
  SOURCE_SUBDIR null
)
FetchContent_MakeAvailable(llvm-project)

# We manually add llvm configure step to our own
# configure phase
execute_process(
    COMMAND cmake ${llvm-project_SOURCE_DIR}/llvm
    -DCMAKE_BUILD_TYPE=Release
    -DLLVM_ENABLE_PROJECTS=lld
    -DLLVM_ENABLE_DUMP=ON
    -DLLVM_BUILD_LLVM_DYLIB=ON
    -DLLVM_CCACHE_BUILD=ON
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_BINARY_DIR}/toolstack
    WORKING_DIRECTORY "${llvm-project_BINARY_DIR}")

# Finally we use the isolated build step only of ExternalProject
# in our own build phase
include(ExternalProject)
ExternalProject_Add(llvm-project
  SOURCE_DIR        "${llvm-project_SOURCE_DIR}/llvm"
  BINARY_DIR        "${llvm-project_BINARY_DIR}"
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND "")
