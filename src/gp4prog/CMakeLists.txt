add_executable(gp4prog
	main.cpp
	boardproto.cpp
	
	usb.cpp
)

target_link_libraries(gp4prog
	usb-1.0 log)

install(TARGETS gp4prog
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

