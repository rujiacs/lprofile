###################### libprofile.so #######################
### library that needed to be inserted into the mutatee ####
set(PROBE_SRC emptyprobe.c)

set(PROBE_HDR probe.h)

add_library(probe SHARED ${PROBE_SRC})

set_target_properties(probe PROPERTIES PUBLIC_HEADER "${PROBE_HDR}")

install(TARGETS probe
		RUNTIME DESTINATION ${LIB_INSTALL}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL}
		PUBLIC_HEADER DESTINATION ${HEADER_INSTALL})
