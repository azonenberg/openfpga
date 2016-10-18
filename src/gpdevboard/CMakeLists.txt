add_library(gpdevboard STATIC
	usb.cpp
	protocol.cpp)

target_link_libraries(gpdevboard
	usb-1.0 log)
