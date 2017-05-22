add_library(hidapi STATIC
    hid.c)

target_include_directories(hidapi
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
