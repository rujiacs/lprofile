# add include path for DyninstAPI
include_directories(${DYNINST_SOURCE_DIR}/dyninstAPI/h
		${DYNINST_SOURCE_DIR}/common/h
		${DYNINST_SOURCE_DIR}/dynC_API/h
		${DYNINST_SOURCE_DIR}/elf/h
		${DYNINST_SOURCE_DIR}/instructionAPI/h
		${DYNINST_SOURCE_DIR}/parseAPI/h
		${DYNINST_SOURCE_DIR}/patchAPI/h
		${DYNINST_SOURCE_DIR}/proccontrol/h
		${DYNINST_SOURCE_DIR}/stackwalk/h
		${DYNINST_SOURCE_DIR}/symtabAPI/h
		)

# add libraries searching path for DyninstAPI
link_directories(${DYNINST_BINARY_DIR}/dyninstAPI
		${DYNINST_BINARY_DIR}/symtabAPI
		${DYNINST_BINARY_DIR}/parseAPI
		)

####################### Boost ###########################
# Set list of Boost libraries
set(BOOST_LIBS boost_atomic
				boost_chrono
				boost_date_time
				boost_filesystem
				boost_system
				boost_thread
				boost_timer)

# set c++11 flags for gcc
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O2 -std=c++11")

######################### tracer ############################
# set source files
set(LPROFILE_SRC
		count.cc
		funcmap.cc
		funcmaptest.cc
		prof.cc
		simple.cc
		replace.cc
		test.cc
		wrap.cc
)

# add build target
add_executable(lprofile ${LPROFILE_SRC})
target_link_libraries(lprofile ${BOOST_LIBS} dyninstAPI parseAPI symtabAPI)

install(TARGETS lprofile
		RUNTIME DESTINATION ${LIB_INSTALL}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
