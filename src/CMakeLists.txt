add_subdirectory(vendor)
add_subdirectory(gpdevboard)
add_subdirectory(greenpak4)
add_subdirectory(gp4prog)
add_subdirectory(gp4par)
add_subdirectory(gp4tchar)
add_subdirectory(xbpar)
add_subdirectory(log)
add_subdirectory(gpcosim)
add_subdirectory(xptools)

if(BUILD_RUST)
    add_custom_target(xc2bit ALL
        ${CMAKE_COMMAND} -E env CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/xc2bit ${CARGO} build
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/src/xc2bit)
endif()
