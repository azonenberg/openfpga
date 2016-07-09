########################################################################################################################
# Root build script

cmake_minimum_required(VERSION 2.8)
project(openfpga CXX)
include(GNUInstallDirs)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set( LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(EXTRA_WARNINGS "-Wcast-align -Winit-self -Wmissing-declarations")
  set(EXTRA_WARNINGS "${EXTRA_WARNINGS} -Wswitch -Wwrite-strings")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ${EXTRA_WARNINGS} -std=c++11 -fPIC")
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -shared-libgcc -ggdb")
  set(CMAKE_LINK_FLAGS "-ggdb")
endif()

# Sanitizers
if(SANITIZE)
  if(CMAKE_CXX_COMPILER_ID STREQUAL Clang)
    if(SANITIZE STREQUAL memory)
      message(STATUS "Please note that you need to build instrumented libc++ and libjson-c.")
      message(STATUS "Try: cmake -DCMAKE_CXX_FLAGS=\"-I\${MSAN_PREFIX}/include/ \\")
      message(STATUS "                              -stdlib=libc++ -I\${MSAN_PREFIX}/include/c++/v1/\" \\")
      message(STATUS "           -DCMAKE_EXE_LINKER_FLAGS=\"-lc++abi -L\${MSAN_PREFIX}/lib\" ...")
      message(STATUS "See https://github.com/google/sanitizers/wiki/MemorySanitizerLibcxxHowTo")
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=memory -fsanitize-memory-track-origins")
    elseif(SANITIZE STREQUAL undefined)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
    else()
      message(FATAL_ERROR "Unsupported sanitizer ${SANITIZE}")
    endif()
  else()
    message(FATAL_ERROR "Sanitizers not supported on compiler ${CMAKE_CXX_COMPILER_ID}")
  endif()
endif()

add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(doc)
