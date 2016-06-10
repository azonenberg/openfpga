ADD_LIBRARY(xbpar STATIC
	xbpar.cpp

	PAREngine.cpp
	PARGraph.cpp
	PARGraphNode.cpp
)

target_link_libraries(xbpar
	m log
	)
