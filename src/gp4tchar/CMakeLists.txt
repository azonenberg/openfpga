add_executable(gp4tchar
	main.cpp)

target_link_libraries(gp4tchar
	gpdevboard greenpak4)

#Don't install, this is a development tool only
#install(TARGETS gp4tchar
#    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})


