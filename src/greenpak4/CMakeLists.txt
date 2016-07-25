ADD_LIBRARY(greenpak4 STATIC

	# Post-PAR netlist
	Greenpak4Abuf.cpp
	Greenpak4Bandgap.cpp
	Greenpak4BitstreamEntity.cpp
	Greenpak4Comparator.cpp
	Greenpak4Counter.cpp
	Greenpak4CrossConnection.cpp
	Greenpak4DAC.cpp
	Greenpak4DualEntity.cpp
	Greenpak4Device.cpp
	Greenpak4EntityOutput.cpp
	Greenpak4Flipflop.cpp
	Greenpak4Inverter.cpp
	Greenpak4IOB.cpp
	Greenpak4IOBTypeA.cpp
	Greenpak4IOBTypeB.cpp
	Greenpak4LFOscillator.cpp
	Greenpak4LUT.cpp
	Greenpak4LUTPgen.cpp
	Greenpak4PGA.cpp
	Greenpak4PowerOnReset.cpp
	Greenpak4PowerRail.cpp
	Greenpak4RCOscillator.cpp
	Greenpak4RingOscillator.cpp
	Greenpak4ShiftRegister.cpp
	Greenpak4SystemReset.cpp
	Greenpak4VoltageReference.cpp

	# Unplaced (but techmapped) netlist
	Greenpak4Netlist.cpp
	Greenpak4NetlistCell.cpp
	Greenpak4NetlistModule.cpp
	Greenpak4NetlistPort.cpp
)

target_link_libraries(greenpak4
	json-c xbpar log
	)
