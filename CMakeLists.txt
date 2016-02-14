########################################################################################################################
# Root build script

cmake_minimum_required(VERSION 2.8)
project(openfpga)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

set(EXTRA_WARNINGS "-Wcast-align -Winit-self -Wmissing-declarations")
set(EXTRA_WARNINGS "${EXTRA_WARNINGS} -Wswitch -Wwrite-strings ")
set( CMAKE_CXX_FLAGS "-Wall -Wextra ${EXTRA_WARNINGS} -std=c++11 -fPIC -shared-libgcc -g3")

add_subdirectory(src)
