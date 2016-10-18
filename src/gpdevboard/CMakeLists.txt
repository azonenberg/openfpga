add_library(gpdevboard STATIC
	usb.cpp
	protocol.cpp)

target_include_directories(gpdevboard
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(gpdevboard
	usb-1.0 log)
