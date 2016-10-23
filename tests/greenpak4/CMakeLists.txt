########################################################################################################################
# Synthesize an HDL file for GP4 (TODO specify device family as arg?)
function(add_greenpak4_netlist name part)

	add_custom_command(
		OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		COMMAND "${YOSYS_COMMAND}" -q
				-p "read_verilog ${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
				-p "synth_greenpak4 -top ${name} -part ${part}"
				-p "write_json   ${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
		COMMENT "Synthesizing Verilog file ${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		VERBATIM)

endfunction()

########################################################################################################################
# PAR an HDL file

function(add_greenpak4_bitstream name part)

	add_greenpak4_netlist(${name} ${part})

	add_custom_command(
		OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
		COMMAND gp4par "--stdout-only"
					   --usercode 41
					   --part ${part}
					   --output  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
					   --logfile "${CMAKE_CURRENT_BINARY_DIR}/${name}-par.log"
					   "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
					   "--quiet"
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		DEPENDS gp4par
		COMMENT "Place and route netlist ${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		VERBATIM)

	add_custom_target(bitstream-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt")

endfunction()

########################################################################################################################
# Run PAR as a test

function(add_greenpak4_test name part)
	add_greenpak4_netlist(${name} ${part})

	add_custom_target(testcase-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")

	add_test(
		NAME    "${name}"
		COMMAND gp4par "--stdout-only"
		               --usercode a3
		               --part ${part}
					   --output  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
		               --logfile "${CMAKE_CURRENT_BINARY_DIR}/${name}.log"
		               "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		               "--debug")
endfunction()

########################################################################################################################
# Run all of the PAR-only tests

add_greenpak4_test(Bargraph SLG46620V)
add_greenpak4_test(Blinky SLG46620V)
add_greenpak4_test(Dac SLG46620V)
add_greenpak4_test(Delay SLG46620V)
add_greenpak4_test(Ethernet SLG46620V)
add_greenpak4_test(Inverters SLG46620V)
add_greenpak4_test(Location SLG46620V)
add_greenpak4_test(Loop SLG46620V)
add_greenpak4_test(POR SLG46620V)
add_greenpak4_test(Tristate SLG46620V)
add_greenpak4_test(Vector SLG46620V)

########################################################################################################################
# Compile bitstreams for HiL tests

add_greenpak4_bitstream(PGA SLG46620V)

########################################################################################################################
# Compile binaries for HiL tests (no install required here)

add_executable(HIL_PGA
	HIL_PGA.cpp)

target_link_libraries(HIL_PGA
	gpdevboard)
