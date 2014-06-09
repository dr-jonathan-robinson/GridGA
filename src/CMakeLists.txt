CMAKE_MINIMUM_REQUIRED(VERSION 2.8)

SET(MSVC_INCREMENTAL_DEFAULT ON)

PROJECT(GridGA)

macro( MSVC_PCH_SUPPORT src_files )
if (MSVC)
	if (USE_MSVC_PCH)
	
		set_source_files_properties(stdafx.cpp
			PROPERTIES
			COMPILE_FLAGS "/Ycstdafx.hpp"
			)
		foreach( src_file ${src_files} )
            MESSAGE(STATUS "Setting /Yustdafx on '${src_file}'")
			set_source_files_properties(
				${src_file}
				PROPERTIES
				COMPILE_FLAGS "/Yustdafx.hpp"
				)
		endforeach()

	endif(USE_MSVC_PCH)
endif (MSVC)
endmacro (MSVC_PCH_SUPPORT)

option (USE_MSVC_PCH "Use precompiled headers in MSVC." ON)

#SET(CMAKE_INCLUDE_PATH "${CMAKE_INCLUDE_PATH}/src")
SET(CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmakeModules")

SET(CMAKE_ECLIPSE_VERSION 3.6)
SET(GRIDGA_VERSION 1.3.1)

SET(CMAKE_DEBUG_SUFFIX "D")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "Release")

SET(USE_INCREMENTAL_LINKING ON)
SET(STATIC_LINKED_RUNTIME ON)

IF(MSVC)
  SET(HOSTNAME "$ENV{COMPUTERNAME}")
ELSE()
  SET(HOSTNAME "$ENV{HOSTNAME}")
ENDIF()

SET(COMPILE_HOST "${HOSTNAME}")

MACRO (TODAY RESULT)
    IF (WIN32)
        EXECUTE_PROCESS(COMMAND "cmd" " /C date /T" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(..)/(..)/..(..).*" "\\1/\\2/\\3" ${RESULT} ${${RESULT}})
    ELSEIF(UNIX)
        EXECUTE_PROCESS(COMMAND "date" "+%d/%m/%Y" OUTPUT_VARIABLE ${RESULT})
        string(REGEX REPLACE "(..)/(..)/..(..).*" "\\1/\\2/\\3" ${RESULT} ${${RESULT}})
    ELSE (WIN32)
        MESSAGE(SEND_ERROR "date not implemented")
        SET(${RESULT} 000000)
    ENDIF (WIN32)
ENDMACRO (TODAY)

TODAY(BUILD_DATE)

IF(MSVC)
    INCLUDE(MSVCSettings)
    IF(CMAKE_BUILD_TYPE MATCHES Debug)
        SET(ZMQ_LIBRARIES "${PROJECT_SOURCE_DIR}/../libs/win32/libzmq_d.lib")
    ELSE()
        SET(ZMQ_LIBRARIES "${PROJECT_SOURCE_DIR}/../libs/win32/libzmq_r.lib")
    ENDIF()
    
    #SET(ZMQ_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/src")
    
ELSE()

    INCLUDE(GCCSettings)

    SET(Boost_USE_STATIC_LIBS ON)
    SET(Boost_USE_MULTITHREAD ON)
    SET(Boost_USE_STATIC_RUNTIME ON)

    SET(BOOST_VERSION_S "1.55.0")
    FIND_PACKAGE(Boost 1.55.0 COMPONENTS  
        date_time
        filesystem
        iostreams
        program_options
        python
        regex
        serialization
        system
        thread
        unit_test_framework
        REQUIRED)
    FIND_PACKAGE(zmq)
    SET(ZMQ_INCLUDE_DIR /usr/local/include)    

    ADD_DEFINITIONS( "-DHAS_BOOST" )
    ADD_DEFINITIONS( "-DBOOST_THREAD_USE_LIB") # required by MinGW
    SET (ZMQ_FIND_QUIETLY TRUE)
    SET (ZMQ_FIND_REQUIRED FALSE)
        
      # Search user environment for headers, then default paths
      FIND_PATH (ZMQ_INCLUDE_DIR zmq.hpp
        PATHS $ENV{ZMQROOT}/include
        NO_DEFAULT_PATH)
      FIND_PATH (ZMQ_INCLUDE_DIR zmq.hpp)
      GET_FILENAME_COMPONENT (ZMQROOT ${ZMQ_INCLUDE_DIR} PATH)
    
      # Search user environment for libraries, then default paths
      FIND_LIBRARY (ZMQ_LIBRARIES 
         NAMES libzmq libzmq-v100-mt
         PATHS $ENV{ZMQROOT}/lib
         NO_DEFAULT_PATH)
      FIND_LIBRARY (ZMQ_LIBRARIES zmq)
    
      # Set ZMQ_FOUND and error out if zmq is not found
      INCLUDE (FindPackageHandleStandardArgs)
      FIND_PACKAGE_HANDLE_STANDARD_ARGS (ZMQ
        DEFAULT_MSG ZMQ_LIBRARIES ZMQ_INCLUDE_DIR)
    
      IF (ZMQ_FOUND)
        MESSAGE (STATUS "INFO:  ZMQ found")
    
        # Set flags and print a status message
        SET (ZMQ_CPPFLAGS "-I${ZMQ_INCLUDE_DIR}")
        SET (ZMQ_LDFLAGS "${ZMQ_LIBRARIES}")
        
        MESSAGE (STATUS "INFO:  ZMQ_INCLUDE_DIR=${ZMQ_INCLUDE_DIR}")
        MESSAGE (STATUS "INFO:  ZMQ_LIBRARIES=${ZMQ_LIBRARIES}")
      ELSE (ZMQ_FOUND)
        MESSAGE (STATUS "INFO:  ZMQ not found. ZMQROOT=${ZMQROOT}")
      ENDIF (ZMQ_FOUND)
       
#    INCLUDE_DIRECTORIES("/opt/centos/devtoolset-1.1/root/usr/include/c++/4.7.2")

    INCLUDE_DIRECTORIES(${ZMQ_INCLUDE_DIR})

ENDIF()


CONFIGURE_FILE( run_ga/VersionConfig.in.hpp ${CMAKE_BINARY_DIR}/generated/VersionConfig.hpp )
INCLUDE_DIRECTORIES( ${CMAKE_BINARY_DIR}/generated/ ) # Make sure it can be included...

INCLUDE_DIRECTORIES(SYSTEM 
  ${PROJECT_SOURCE_DIR} 
  ${Boost_INCLUDE_DIR})

ADD_SUBDIRECTORY(run_ga)
ADD_SUBDIRECTORY(htcondor_job_wrapper)

MESSAGE(STATUS "\n")
MESSAGE(STATUS "***************************************************************************************")
MESSAGE(STATUS "C++ Compiler name: '${CMAKE_CXX_COMPILER}'")
MESSAGE(STATUS "C Compiler name: '${CMAKE_C_COMPILER}'")
MESSAGE(STATUS "Compiler Version: '${CMAKE_CXX_COMPILER_VERSION}'")
MESSAGE(STATUS "Compiler ID: '${CMAKE_CXX_COMPILER_ID}'")
MESSAGE(STATUS "System version: '${CMAKE_SYSTEM_VERSION}'")
MESSAGE(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
MESSAGE(STATUS "Boost Version ${BOOST_VERSION_S}")
MESSAGE(STATUS "C++ flags: ${CMAKE_CXX_FLAGS}")
MESSAGE(STATUS "Source directory: '${PROJECT_SOURCE_DIR}'")
MESSAGE(STATUS "Build directory: '${PROJECT_BINARY_DIR}'")
MESSAGE(STATUS "Boost Library Dir: ${Boost_LIBRARY_DIRS}")
MESSAGE(STATUS "Boost Include Dir: ${Boost_INCLUDE_DIR}")
#MESSAGE(STATUS "ZMQ Include Dir: ${ZMQ_INCLUDE_DIR}")
MESSAGE(STATUS "ZMQ Library File: ${ZMQ_LIBRARIES}")
MESSAGE(STATUS "Boost libs: ${Boost_LIBRARIES}")
