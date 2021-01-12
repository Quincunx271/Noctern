# Set up warnings / similar flags
set(werr ${NOCTERN_WARNINGS_AS_ERRORS})
set(compile_options)
set(link_options)

if(CMAKE_CXX_COMPILER_ID STREQUAL MSVC)
  list(APPEND compile_options
    /permissive- /W4
    $<$<BOOL:${werr}>:/WX>
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
    -fsanitize=undefined
  )
elseif(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  list(APPEND compile_options
    -Wall -Wextra -Wpedantic
    -fsanitize=address
    -fsanitize=undefined
    $<$<BOOL:${werr}>:-Werror>
  )
  list(APPEND link_options
    -fsanitize=address
    -fsanitize=undefined
  )
endif()

# Set up targets
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "*.cpp")
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "*.test.cpp")
list(REMOVE_ITEM sources ${test_sources})

add_library(Noctern ${sources})
add_library(Noctern::Noctern ALIAS Noctern)
target_include_directories(Noctern
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(Noctern
  PUBLIC
    cxx_std_20
)

add_executable(nocternc nocternc.main.cpp)
target_link_libraries(nocternc PRIVATE Noctern::Noctern)

# Set up tests
if(NOT EXISTS ${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp)
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp [[
    #define CATCH_CONFIG_MAIN
    #include <catch2/catch.hpp>
  ]])
endif()

add_executable(test.Noctern ${test_sources} ${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp)
target_link_libraries(test.Noctern
  PRIVATE
    Noctern::Noctern
    CONAN_PKG::catch2
)
target_compile_options(test.Noctern
  PRIVATE
    ${compile_options}
)
target_link_options(test.Noctern
  PRIVATE
    ${link_options}
)

include(Catch)

catch_discover_tests(test.Noctern
  EXTRA_ARGS $<$<BOOL:${NOCTERN_TEST_COLOR}>:--use-colour=yes>
)
