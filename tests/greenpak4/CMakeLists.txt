function(add_greenpak4_test name)
	add_custom_command(
		OUTPUT  "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		COMMAND "${YOSYS_COMMAND}" -q
				-p "read_verilog ${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
				-p "synth_greenpak4 -top ${name} -part SLG46620V"
				-p "write_json   ${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${name}.v"
		COMMENT "Synthesizing Verilog file ${CMAKE_CURRENT_SOURCE_DIR}/${name}"
		VERBATIM)

	add_custom_target(testcase-gp4-${name}
		ALL
		DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/${name}.json")

	add_test(
		NAME    "${name}"
		COMMAND gp4par --output  "${CMAKE_CURRENT_BINARY_DIR}/${name}.txt"
		               --logfile "${CMAKE_CURRENT_BINARY_DIR}/${name}.log"
		               "${CMAKE_CURRENT_BINARY_DIR}/${name}.json"
		               "--debug")
endfunction()

add_greenpak4_test(Analog)
add_greenpak4_test(Bargraph)
add_greenpak4_test(Blinky)
#add_greenpak4_test(Dac)
add_greenpak4_test(Location)
add_greenpak4_test(Tristate)
add_greenpak4_test(Loop)
