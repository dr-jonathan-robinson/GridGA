# ------------------------------------------------------------------
#
# Find the zeromq includes and library
#
# This module defines the following variables
#
# ZMQ_FOUND             Do not attempt to use Pcap if "no" or undefined.
# ZMQ_LIBRARY           The full path to the library
# ZMQ_INCLUDE_DIR       Where to find header files
# ZMQ_LIBRARY_DIR       Fully qualified library directory to link against
# ZMQ_LIB               Name of ZMQ library file
# ------------------------------------------------------------------

SET(ZMQ_ROOT $ENV{ZMQ_ROOT})

IF(ZMQ_ROOT STREQUAL "")
	MESSAGE(STATUS "ERROR:: ZMQ_ROOT environment variable does not appear to be set.")
ENDIF()

message(STATUS "FindZmq ZMQ_ROOT = ${ZMQ_ROOT}")

if (WIN32)
    set(ADDITIONAL_ZMQ_INCLUDE_PATHS
        ${ZMQ_ROOT}/include)
    set(ADDITIONAL_ZMQ_LIBRARY_PATHS
        ${ZMQ_ROOT}/builds/msvc/Release)
	IF(CMAKE_SIZEOF_VOID_P EQUAL 4)
		set(ZMQ_LIB libzmq32.lib)
	ELSE()
		SET(ZMQ_LIB libzmq64.lib)
	ENDIF()
else()
    
    set(ADDITIONAL_ZMQ_INCLUDE_PATHS
        ${ZMQ_ROOT}/include
        /usr/local/include)
    set(ADDITIONAL_ZMQ_LIBRARY_PATHS
        ${ZMQ_ROOT}/lib
        /usr/local/lib)
    set(ZMQ_LIB zmq)
endif()

find_path(ZMQ_INCLUDE_DIR NAMES zmq.hpp PATHS ${ADDITIONAL_ZMQ_INCLUDE_PATHS})
mark_as_advanced(ZMQ_INCLUDE_DIR)

find_library(ZMQ_LIBRARY ${ZMQ_LIB} PATHS ${ADDITIONAL_ZMQ_LIBRARY_PATHS})
mark_as_advanced(ZMQ_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ZMQ DEFAULT_MSG ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

if(ZMQ_FOUND)
    get_filename_component(ZMQ_LIBRARY_DIR ${ZMQ_LIBRARY} PATH)
else()
    set(ZMQ_LIB) 
	MESSAGE(STATUS "ERROR:: Could not find ZeroMQ. Did you remember to set the ZMT_ROOT environment varaible?")
endif()
mark_as_advanced(ZMQ_LIBRARY_DIR)


message(STATUS "FindZmq ZMQ_LIBRARY_DIR = ${ZMQ_LIBRARY_DIR}")
message(STATUS "FindZmq ZMQ_LIBRARY = ${ZMQ_LIBRARY}")
