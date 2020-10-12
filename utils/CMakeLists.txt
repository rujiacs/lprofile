# add include path for DyninstAPI
include_directories(${DYNINST_HEADER_DIR})

# add libraries searching path for DyninstAPI
link_directories(${DYNINST_LIB_DIR})

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
		funcmap.cc
		funcmaptest.cc
		test.cc
)

# add build target
add_executable(lprofile ${LPROFILE_SRC})
target_link_libraries(lprofile ${BOOST_LIBS} dyninstAPI)

install(TARGETS lprofile
		RUNTIME DESTINATION ${LIB_INSTALL}
		LIBRARY DESTINATION ${LIB_INSTALL}
		ARCHIVE DESTINATION ${LIB_INSTALL})
