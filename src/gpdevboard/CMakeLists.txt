add_library(gpdevboard STATIC
	usb.cpp
	utils.cpp
	protocol.cpp)

target_include_directories(gpdevboard
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

find_library(HIDAPI_LIBRARY
	NAMES hidapi hidapi-libusb)

target_link_libraries(gpdevboard
	${HIDAPI_LIBRARY} log)
