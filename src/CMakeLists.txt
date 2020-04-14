cmake_minimum_required (VERSION 3.0)
project(test_utf_conv CXX)

set(CMAKE_CXX_STANDARD 11)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  # additional warnings
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  # default release flags : remove -DNDEBUG (its a test project, want the asserts...) 
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  # default debug flags : add sanitizer and up the debug symbols
  set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined")
endif()

set (TEST_UTF_CONV_SOURCES
        charset_conv_iconv.h charset_conv_iconv.cpp utf_conv.h utf_conv_impl.h main_tests.cpp)

add_executable(test_utf_conv ${TEST_UTF_CONV_SOURCES})

