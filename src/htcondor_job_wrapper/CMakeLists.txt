SET (HT_CONDOR_JOB_WRAPPER_SRC_FILES 
    Main.cpp
)

SET (HT_CONDOR_JOB_WRAPPER_HDR_FILES 
    # Third Party
    ../run_ga/Zmq.hpp
)

MSVC_PCH_SUPPORT("${HT_CONDOR_JOB_WRAPPER_SRC_FILES}") 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# Need this definition, otherwise VS2012 will bollocks up it's min/max macros and numeric_limits won't compile 
ADD_DEFINITIONS("-DNOMINMAX")

ADD_EXECUTABLE(htcondor_job_wrapper ${HT_CONDOR_JOB_WRAPPER_SRC_FILES} ${HT_CONDOR_JOB_WRAPPER_HDR_FILES} stdafx.cpp stdafx.hpp) 
    
IF (WIN32)
    IF (MINGW)
        TARGET_LINK_LIBRARIES(htcondor_job_wrapper  ${Boost_LIBRARIES} ${ZMQ_LIBRARIES} ws2_32 gomp)
    ELSE()
        TARGET_LINK_LIBRARIES(htcondor_job_wrapper ${Boost_LIBRARIES} ${ZMQ_LIBRARIES} ws2_32)
        ADD_DEFINITIONS(-DZMQ_STATIC)
    ENDIF()
ELSE()
    TARGET_LINK_LIBRARIES(htcondor_job_wrapper ${Boost_LIBRARIES} ${ZMQ_LIBRARY} ${CMAKE_DL_LIBS})
ENDIF()
