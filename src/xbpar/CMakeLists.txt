ADD_LIBRARY(xbpar STATIC
	xbpar.cpp

	PAREngine.cpp
	PARGraph.cpp
	PARGraphNode.cpp

	PTVCorner.cpp

	xbpar_ffi.cpp
)

target_include_directories(xbpar
	PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(xbpar
	m log)
