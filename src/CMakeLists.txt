cmake_minimum_required (VERSION 3.0)
project(test_utf_conv C CXX)

set(CMAKE_CXX_STANDARD 11)

if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  set(CMAKE_CXX_FLAGS_DEBUG "-g3 -fsanitize=address -fsanitize=leak -fsanitize=undefined")
endif()

set (TEST_UTF_CONV_SOURCES
        charset_conv_iconv.h charset_conv_iconv.cpp utf_conv.h utf_conv_impl.h main_tests.cpp)

add_executable(test_utf_conv ${TEST_UTF_CONV_SOURCES})

