cmake_minimum_required(VERSION 3.30)

list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(Noctern
  VERSION 0.0.1
  LANGUAGES CXX
  DESCRIPTION "A programming language"
)

include(is_root)
is_root(is_root_project)

set(NOCTERN_DEVELOPER_DEFAULTS "${is_root_project}" CACHE BOOL "Default all options to developer-friendly values")

option(BUILD_TESTING "Enable testing" ${NOCTERN_DEVELOPER_DEFAULTS})
option(NOCTERN_BUILD_DOCS "Build the documentation" OFF)
option(NOCTERN_TEST_COLOR "Force test color" OFF)
option(NOCTERN_WARNINGS_AS_ERRORS "Turn on -Werror or equivalent" OFF)

if(BUILD_TESTING)
  enable_testing()
endif()

if(NOCTERN_DEVELOPER_DEFAULTS)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/bin")
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/lib")
  set(CMAKE_CXX_EXTENSIONS OFF)
endif()

add_subdirectory(src)
