###################### libprofile.so #######################
### library that needed to be inserted into the mutatee ####
set(PROBEEMPTY_SRC emptyprobe.c)

set(PROBEEMPTY_HDR probe.h)

add_library(probeempty SHARED ${PROBEEMPTY_SRC})

set_target_properties(probeempty PROPERTIES PUBLIC_HEADER "${PROBEEMPTY_HDR}")

install(TARGETS probeempty
		RUNTIME DESTINATION ${LIB_INSTALL}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL}
		PUBLIC_HEADER DESTINATION ${HEADER_INSTALL})

set(PROBE_SRC array.c
				evlist.c
				evsel.c
				pmu.c
				profile.c
				threadmap.c
				util.c
)

set(PROBE_HDR profile.h)

add_library(probe SHARED ${PROBE_SRC})

set_target_properties(probe PROPERTIES PUBLIC_HEADER "${PROBE_HDR}")

install(TARGETS probe
		RUNTIME DESTINATION ${LIB_INSTALL}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL}
		PUBLIC_HEADER DESTINATION ${HEADER_INSTALL})
