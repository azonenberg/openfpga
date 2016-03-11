ADD_LIBRARY(xbpar SHARED
	xbpar.cpp
	
	PAREngine.cpp
	PARGraph.cpp
	PARGraphNode.cpp
)

target_link_libraries(xbpar
	m
	)
