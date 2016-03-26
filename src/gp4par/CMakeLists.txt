add_executable(gp4par
	main.cpp
	par_main.cpp
	par_reporting.cpp
	
	Greenpak4PAREngine.cpp
)

target_link_libraries(gp4par
	greenpak4 xbpar)
