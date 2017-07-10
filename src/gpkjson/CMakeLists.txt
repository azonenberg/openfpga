add_executable(gpkjson
	main.cpp
)

target_link_libraries(gpkjson
	greenpak4 xbpar log)

install(TARGETS gpkjson
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
