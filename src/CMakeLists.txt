find_package(FLEX 2.6.4 REQUIRED)

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
file(GLOB_RECURSE sources CONFIGURE_DEPENDS "noctern/*.cpp" "noctern/*.hpp")
file(GLOB_RECURSE test_sources CONFIGURE_DEPENDS "noctern/*.test.cpp")
list(REMOVE_ITEM sources ${test_sources})

flex_target(noctern.lex noctern/parsing/lexer.l "${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp")

add_library(Noctern
  ${sources}
  ${FLEX_noctern.lex_OUTPUTS}
)
add_library(Noctern::Noctern ALIAS Noctern)
target_include_directories(Noctern
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_compile_features(Noctern
  PUBLIC
    cxx_std_20
)
target_link_libraries(Noctern
  PUBLIC
    CONAN_PKG::frozen
    CONAN_PKG::fmt
)

add_executable(nocternc nocternc.main.cpp)
target_link_libraries(nocternc PRIVATE Noctern::Noctern)

add_executable(noctern.lexer lexer.main.cpp)
target_link_libraries(noctern.lexer PRIVATE Noctern::Noctern)

add_executable(noctern.parser parser.main.cpp)
target_link_libraries(noctern.parser PRIVATE Noctern::Noctern)

# Set up tests
include(write_if_diff)
write_if_diff(${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp [[
  #define CATCH_CONFIG_MAIN
  #include <catch2/catch.hpp>
]])
add_library(catch_main OBJECT ${CMAKE_CURRENT_BINARY_DIR}/catch_main.test.cpp)
target_link_libraries(catch_main PUBLIC CONAN_PKG::catch2)

add_executable(test.Noctern ${test_sources})
target_link_libraries(test.Noctern
  PRIVATE
    Noctern::Noctern
    catch_main
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
