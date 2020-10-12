cmake_minimum_required(VERSION 3.5)

set(CMAKE_INSTALL_PREFIX ${PROJECT_BINARY_DIR}/lprofile)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

# Build Dyninst libraries as a seperate project
message(STATUS "Configuring DyninstAPI")
set(DYNINST_SOURCE_DIR ${PROJECT_SOURCE_DIR}/Dyninst-8.2.1)
set(DYNINST_BINARY_DIR ${PROJECT_BINARY_DIR}/dyninst)
set(DYNINST_BUILD_DIR ${CMAKE_INSTALL_PREFIX}/dyninst)
set(DYNINST_LIB_DIR ${DYNINST_BUILD_DIR}/lib)
set(DYNINST_HEADER_DIR ${DYNINST_BUILD_DIR}/include)

file(REMOVE_RECURSE ${DYNINST_BINARY_DIR}/CMakeCache.txt ${DYNINST_BINARY_DIR}/CMakeFiles ${DYNINST_BINARY_DIR}/Makefile ${DYNINST_BINARY_DIR}/Makefile)
file(MAKE_DIRECTORY ${DYNINST_BINARY_DIR})

execute_process(WORKING_DIRECTORY ${DYNINST_BINARY_DIR}
		COMMAND ${CMAKE_COMMAND}
		-DCMAKE_INSTALL_PREFIX=${DYNINST_BUILD_DIR}
		-DINSTALL_LIB_DIR=${DYNINST_LIB_DIR}
		-DINSTALL_INCLUDE_DIR=${DYNINST_HEADER_DIR}
		-DPLATFORM=${PLATFORM}
		-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}
		-G ${CMAKE_GENERATOR}
		${DYNINST_SOURCE_DIR})

add_custom_target(Dyninst
		ALL
		${CMAKE_BUILD_TOOL}
		WORKING_DIRECTORY ${DYNINST_BINARY_DIR}
		COMMENT "Building Dyninst")

#install(EXPORT Dyninst
#		DESTINATION "${DYNINST_BUILD_DIR}")
install(SCRIPT "${DYNINST_BINARY_DIR}/cmake_install.cmake")

set(LIB_INSTALL ${CMAKE_INSTALL_PREFIX}/lib)
set(HEADER_INSTALL ${CMAKE_INSTALL_PREFIX}/include)

# set c flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -g -Werror")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Waddress -Warray-bounds -Wchar-subscripts -Wimplicit-int -Wimplicit-function-declaration -Wcomment")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat -Wmain -Wmissing-braces -Wnonnull -Wpointer-sign -Wreturn-type -Wsign-compare")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wsequence-point -Wstrict-aliasing -Wstrict-overflow=1 -Wswitch -Wtrigraphs")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wuninitialized -Wunknown-pragmas -Wunused-function -Wunused-label -Wvolatile-register-var")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")

add_subdirectory(probe)
add_subdirectory(utils)

message("Please set environment variables as follows:")
message("DYNINSTAPI_RT_LIB=${DYNINST_LIB_DIR}/libdyninstAPI_RT.so")
message("LD_LIBRARY_PATH=$LD_LIBRARY_PATH:${LIB_INSTALL}:${DYNINST_LIB_DIR}/lib")
