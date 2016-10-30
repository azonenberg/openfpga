########################################################################################################################
# Bitstreams that are not (yet) part of a HiL test

########################################################################################################################
# HiL tests

add_greenpak4_hiltest(SocketTestLoopback_STQFN14 SLG46140V)

########################################################################################################################
# Cosimulation tests (WIP, not yet functional)

add_greenpak4_netlist(Cosim_LFOSC SLG46140V)
add_sim_netlist(Cosim_LFOSC_TB)
