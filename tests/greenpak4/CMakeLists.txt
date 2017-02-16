########################################################################################################################
# Synthesize an HDL file for simulation

function(add_sim_netlist name)

	add_custom_command(
		OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${name}.vvp"
		COMMAND "${IVERILOG_COMMAND}" "${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
				-o "${CMAKE_CURRENT_BINARY_DIR}/${name}.vvp"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
		COMMENT "Synthesizing Verilog file ${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		VERBATIM)

	add_custom_target(sim-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.vvp")

endfunction()

########################################################################################################################
# Synthesize an HDL file for GP4

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

	add_custom_target(netlist-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")

endfunction()

########################################################################################################################
# Add a HiL test and run it

function(add_greenpak4_hiltest name part)

	add_greenpak4_bitstream(${name} ${part})

	add_executable(hiltest-${part}-${name}
		${name}.cpp)
	target_link_libraries(hiltest-${part}-${name}
		gpdevboard)

	add_test(
		NAME "${part}-${name}"
		COMMAND hiltest-${part}-${name}
			"${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
			)

endfunction()

########################################################################################################################
# PAR an HDL file

function(add_greenpak4_bitstream name part)

	add_greenpak4_netlist(${name} ${part})

	# Add the PCF file if we have one
	set(pcfargs1 "")
	set(pcfargs2 "")
	if(${ARGC} EQUAL 3)
		set(pcfargs1 "--constraints")
		set(pcfargs2 "${ARGV2}")
	endif()

	add_custom_command(
		OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
		COMMAND gp4par "--stdout-only"
					   --usercode 41
					   # --quiet
					   --debug
					   --part ${part}
					   ${pcfargs1}
					   ${pcfargs2}
					   --output  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
					   --logfile "${CMAKE_CURRENT_BINARY_DIR}/${name}-par.log"
					   "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		DEPENDS "${pcfargs2}"
		DEPENDS gp4par
		COMMENT "Place and route netlist ${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		VERBATIM)

	add_custom_target(bitstream-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt")

endfunction()

########################################################################################################################
# Add our subdirectories

add_subdirectory(slg46140v)
add_subdirectory(slg46620v)
add_subdirectory(slg46621v)
