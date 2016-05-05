########################################################################################################################
# Root build script

cmake_minimum_required(VERSION 2.8)
project(openfpga)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(EXTRA_WARNINGS "-Wcast-align -Winit-self -Wmissing-declarations")
  set(EXTRA_WARNINGS "${EXTRA_WARNINGS} -Wswitch -Wwrite-strings")
endif()
set(CMAKE_CXX_FLAGS "-Wall -Wextra ${EXTRA_WARNINGS} -std=c++11 -fPIC")
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared-libgcc -ggdb")
  set(CMAKE_LINK_FLAGS "-ggdb")
endif()

add_subdirectory(src)
add_subdirectory(tests)
