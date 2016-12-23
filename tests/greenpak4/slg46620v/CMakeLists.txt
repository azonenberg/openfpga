########################################################################################################################
# Bitstreams that are not (yet) part of a HiL test

add_greenpak4_bitstream(Blinky SLG46620V)
add_greenpak4_bitstream(Dac SLG46620V)
add_greenpak4_bitstream(DCMP SLG46620V)
add_greenpak4_bitstream(Delay SLG46620V)
add_greenpak4_bitstream(Ethernet SLG46620V)
add_greenpak4_bitstream(Inverters SLG46620V)
add_greenpak4_bitstream(Location SLG46620V)
add_greenpak4_bitstream(Loop SLG46620V)
add_greenpak4_bitstream(POR SLG46620V)
add_greenpak4_bitstream(SPIToDCMP SLG46620V)
add_greenpak4_bitstream(Tristate SLG46620V)
add_greenpak4_bitstream(Vector SLG46620V)
add_greenpak4_bitstream(VrefEXT SLG46620V)
add_greenpak4_bitstream(VrefVDD SLG46620V)

# Not strictly a HiL test, but we use the (cached) version of this for socket loopback testing
# TODO: have a HiL test that runs this
add_greenpak4_bitstream(SocketTestLoopback_STQFN20 SLG46620V)

########################################################################################################################
# HiL tests

add_greenpak4_hiltest(Bargraph SLG46620V)
add_greenpak4_hiltest(Latch SLG46620V)
add_greenpak4_hiltest(PGA SLG46620V)

########################################################################################################################
# Cosimulation tests

# none yet
