ADD_LIBRARY(greenpak4 SHARED

	# Post-PAR netlist
	Greenpak4BitstreamEntity.cpp
	Greenpak4CrossConnection.cpp
	Greenpak4Device.cpp
	Greenpak4Flipflop.cpp
	Greenpak4IOB.cpp
	Greenpak4IOBTypeA.cpp
	Greenpak4IOBTypeB.cpp
	Greenpak4LFOscillator.cpp
	Greenpak4LUT.cpp
	Greenpak4PowerRail.cpp
	
	# Unplaced (but techmapped) netlist
	Greenpak4Netlist.cpp
	Greenpak4NetlistCell.cpp
	Greenpak4NetlistModule.cpp
	Greenpak4NetlistPort.cpp
)

target_link_libraries(greenpak4
	json-c xbpar
	)
