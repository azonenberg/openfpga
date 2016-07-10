set(FIGURES
    figures/flow.pdf)

set(BIN_FIGURES)
foreach(figure ${FIGURES})
    list(APPEND BIN_FIGURES ${CMAKE_CURRENT_BINARY_DIR}/${figure})
    add_custom_command(
        OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/${figure}
        COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_CURRENT_SOURCE_DIR}/${figure}
                    ${CMAKE_CURRENT_BINARY_DIR}/${figure}
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/${figure}
        VERBATIM)
endforeach()

add_custom_command(
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/gp4-hdl.pdf
    COMMAND pdflatex -halt-on-error ${CMAKE_CURRENT_SOURCE_DIR}/gp4-hdl.tex
    COMMAND pdflatex -halt-on-error ${CMAKE_CURRENT_SOURCE_DIR}/gp4-hdl.tex
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/gp4-hdl.tex ${BIN_FIGURES}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    VERBATIM)
add_custom_target(doc ALL
    DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/gp4-hdl.pdf)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/gp4-hdl.pdf
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/gp4par/)
