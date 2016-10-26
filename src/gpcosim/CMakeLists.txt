include_directories("/usr/local/include/iverilog")
include_directories("/usr/include/iverilog")

add_library(gpcosim SHARED
	gpcosim.cpp)
set_target_properties(gpcosim PROPERTIES PREFIX "")
set_target_properties(gpcosim PROPERTIES SUFFIX ".vpi")

target_link_libraries(gpcosim
	gpdevboard log)

