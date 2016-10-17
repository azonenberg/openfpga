########################################################################################################################
# Root build script

cmake_minimum_required(VERSION 2.8)
project(openfpga CXX)
enable_testing()
include(GNUInstallDirs)

# Project configuration
set(BUILD_DOC     ON      CACHE BOOL   "Build documentation")
set(YOSYS_COMMAND "yosys" CACHE STRING "Command used to run yosys")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Compiler configuration
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(EXTRA_WARNINGS "-Wcast-align -Winit-self -Wmissing-declarations")
  set(EXTRA_WARNINGS "${EXTRA_WARNINGS} -Wswitch -Wwrite-strings")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ${EXTRA_WARNINGS} -std=c++11 -fPIC")
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared-libgcc -ggdb")
  set(CMAKE_LINK_FLAGS "-ggdb")
endif()
if(MSYS)
  string(REPLACE "-fPIC" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
  add_definitions(-D__USE_MINGW_ANSI_STDIO=1)
endif()

# Compilation database for Clang static analyzer
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Clang sanitizers
if(SANITIZE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    if(SANITIZE STREQUAL memory)
      message(STATUS "Please note that you need to build instrumented libc++ and libjson-c.")
      message(STATUS "Try: cmake -DCMAKE_CXX_FLAGS=\"-I\${MSAN_PREFIX}/include/ \\")
      message(STATUS "                              -stdlib=libc++ -I\${MSAN_PREFIX}/include/c++/v1/\" \\")
      message(STATUS "           -DCMAKE_EXE_LINKER_FLAGS=\"-lc++abi -L\${MSAN_PREFIX}/lib\" ...")
      message(STATUS "See https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins")
    elseif(SANITIZE STREQUAL address)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
    elseif(SANITIZE STREQUAL undefined)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined,integer")
    else()
      message(FATAL_ERROR "Unsupported sanitizer ${SANITIZE}")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  else()
    message(FATAL_ERROR "Sanitizers not supported on compiler ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

# Subdirectories
add_subdirectory(src)
add_subdirectory(tests)
if(BUILD_DOC)
  add_subdirectory(doc)
endif()
