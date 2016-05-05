add_executable(gp4par
	main.cpp

	commit.cpp
	make_graphs.cpp
	par_main.cpp
	par_reporting.cpp

	Greenpak4PAREngine.cpp
)

target_link_libraries(gp4par
	greenpak4 xbpar)

install(TARGETS gp4par
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
