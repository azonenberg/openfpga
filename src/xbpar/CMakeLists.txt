ADD_LIBRARY(xbpar STATIC
	xbpar.cpp

	PAREngine.cpp
	PARGraph.cpp
	PARGraphNode.cpp
)

target_include_directories(xbpar
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(xbpar
	m log)
