add_library(hidapi STATIC
	hid.c)

target_include_directories(hidapi
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)

# FIXME: Why do we need udev?
target_link_libraries(hidapi
	udev)
