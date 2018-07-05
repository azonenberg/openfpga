if(NOT WIN32)
	include_directories("/usr/local/include/iverilog")
	include_directories("/usr/include/iverilog")
endif()

# If we got here, WIN32 users will have VPI_ROOT defined.
if(DEFINED VPI_ROOT)
	include_directories("${VPI_ROOT}/include/iverilog")
	link_directories("${VPI_ROOT}/lib")
endif()

add_library(gpcosim SHARED
	gpcosim.cpp)
set_target_properties(gpcosim PROPERTIES PREFIX "")
set_target_properties(gpcosim PROPERTIES SUFFIX ".vpi")

if(NOT WIN32)
	target_link_libraries(gpcosim
		gpdevboard log)
else()
	target_link_libraries(gpcosim
		gpdevboard log veriuser vpi)
endif()
