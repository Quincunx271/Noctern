find_package(Boost 1.74.0 REQUIRED)
find_package(fmt 7.1.2 REQUIRED)
find_package(Catch2 2.13.3 REQUIRED)

# Set up warnings / similar flags
set(werr ${NOCTERN_WARNINGS_AS_ERRORS})
set(compile_options)
set(link_options)

set(fmtlib fmt::fmt)

if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  list(APPEND compile_options
    /permissive- /W4
    $<$<BOOL:${werr}>:/WX>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
  )
  if(NOCTERN_DEVELOPER_DEFAULTS)
    # We get undefined references in CI otherwise.
    # Use this workaround.
    set(fmtlib fmt::fmt-header-only)
  endif()
endif()

if(NOCTERN_DEVELOPER_DEFAULTS)
  add_compile_options(${compile_options})
  add_link_options(${link_options})
endif()

# Set up targets
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "noctern/*.cpp" "noctern/*.hpp")
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "noctern/*.test.cpp")
list(REMOVE_ITEM sources ${test_sources})

add_library(Noctern
  ${sources}
)
add_library(Noctern::Noctern ALIAS Noctern)
target_include_directories(Noctern
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(Noctern
  PUBLIC
    cxx_std_23
)
target_link_libraries(Noctern
  PUBLIC
    "${fmtlib}"
)

file(GLOB_RECURSE main_sources CONFIGURE_DEPENDS "*.main.cpp")

foreach(main IN LISTS main_sources)
  get_filename_component(main_name ${main} NAME_WE)
  if(NOT main_name MATCHES "^noctern")
    set(main_name noctern.${main_name})
  endif()

  add_executable(${main_name} ${main})
  target_link_libraries(${main_name} PRIVATE Noctern::Noctern)
endforeach()

# Set up tests
include(write_if_diff)
write_if_diff(${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp [[
  #define CATCH_CONFIG_MAIN
  #include <catch2/catch.hpp>
]])
add_library(catch_main OBJECT ${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp)
target_link_libraries(catch_main PUBLIC Catch2::Catch2)

include(Catch)

foreach(test_source IN LISTS test_sources)
  get_filename_component(test_name ${test_source} NAME_WE)

  add_executable(test.${test_name} ${test_source})
  target_link_libraries(test.${test_name}
    PRIVATE
      Noctern::Noctern
      catch_main
  )

  catch_discover_tests(test.${test_name}
    EXTRA_ARGS $<$<BOOL:${NOCTERN_TEST_COLOR}>:--use-colour=yes>
  )
endforeach()

