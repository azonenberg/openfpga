########################################################################################################################
# Root build script

cmake_minimum_required(VERSION 2.8)
project(openfpga C CXX)
enable_testing()
include(GNUInstallDirs)

# Project configuration
set(YOSYS_COMMAND    "yosys"    CACHE STRING "Command used to run yosys")
set(IVERILOG_COMMAND "iverilog" CACHE STRING "Command used to run Icarus Verilog")
set(CARGO_COMMAND    "cargo"    CACHE STRING "Command used to run cargo")

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR}/bin)

# Compiler configuration
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU OR CMAKE_CXX_COMPILER_ID STREQUAL Clang)
  set(EXTRA_WARNINGS "-Wcast-align -Winit-self -Wmissing-declarations")
  set(EXTRA_WARNINGS "${EXTRA_WARNINGS} -Wswitch -Wwrite-strings")
endif()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra ${EXTRA_WARNINGS} -std=c++11")
if(CMAKE_CXX_COMPILER_ID STREQUAL GNU)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ggdb")
  set(CMAKE_LINK_FLAGS "-ggdb")
endif()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra ${EXTRA_WARNINGS} -std=c11")
if(CMAKE_C_COMPILER_ID STREQUAL GNU)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ggdb")
  set(CMAKE_LINK_FLAGS "-ggdb")
endif()
if(MSYS)
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

# Optional features
find_program(PDFLATEX pdflatex)
if(DEFINED BUILD_DOC AND NOT BUILD_DOC)
	message(STATUS "BUILD_DOC turned off, documentation will not be built")
elseif(DEFINED BUILD_DOC AND BUILD_DOC AND NOT PDFLATEX)
	message(WARNING "BUILD_DOC turned on but no pdflatex found, documentation will not be built")
	set(BUILD_DOC OFF)
elseif(NOT DEFINED BUILD_DOC AND NOT PDFLATEX)
  message(STATUS "pdflatex not found, documentation will not be built")
  set(BUILD_DOC OFF)
else()
  message(STATUS "pdflatex found, documentation will be built")
  set(BUILD_DOC ON)
endif()

# FORCE is required for the "asked for but missing tool" case
set(BUILD_DOC ${BUILD_DOC} CACHE BOOL "Build documentation" FORCE)

find_program(YOSYS ${YOSYS_COMMAND})
if(DEFINED BUILD_TESTS AND NOT BUILD_TESTS)
  message(STATUS "BUILD_TESTS turned off, tests will not be built nor run")
elseif(DEFINED BUILD_TESTS AND BUILD_TESTS AND NOT YOSYS)
  message(WARNING "BUILD_TESTS turned on but no yosys found, tests will not be built nor run")
  set(BUILD_TESTS OFF)
elseif(NOT DEFINED BUILD_TESTS AND NOT YOSYS)
  message(WARNING "yosys not found, tests will not be built nor run")
  set(BUILD_TESTS OFF)
else()
  set(BUILD_TESTS ON)
endif()

# FORCE is required for the "asked for but missing tool" case
set(BUILD_TESTS ${BUILD_TESTS} CACHE BOOL "Build and run tests" FORCE)

find_program(CARGO ${CARGO_COMMAND})
if(DEFINED BUILD_RUST AND NOT BUILD_RUST)
  message(STATUS "BUILD_RUST turned off, Rust code will not be built nor run")
elseif(DEFINED BUILD_RUST AND BUILD_RUST AND NOT CARGO)
  message(WARNING "BUILD_RUST turned on but no cargo found, Rust code will not be built nor run")
  set(BUILD_RUST OFF)
elseif(NOT DEFINED BUILD_RUST AND NOT CARGO)
  message(WARNING "cargo not found, Rust code will not be built nor run")
  set(BUILD_RUST OFF)
else()
  message(STATUS "Rust enabled")
  set(BUILD_RUST ON)
endif()

# FORCE is required for the "asked for but missing tool" case
set(BUILD_RUST ${BUILD_RUST} CACHE BOOL "Build and run Rust code" FORCE)

# Subdirectories
add_subdirectory(src)
if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
if(BUILD_DOC)
  add_subdirectory(doc)
endif()
