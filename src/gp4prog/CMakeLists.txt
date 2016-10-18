add_executable(gp4prog
	main.cpp)

target_link_libraries(gp4prog
	gpdevboard)

install(TARGETS gp4prog
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

