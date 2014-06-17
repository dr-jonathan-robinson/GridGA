SET (GRID_GA_SRC_FILES 
    GeneticAlgo.cpp
    GenerateXMLConfig.cpp
    Genome.cpp
    Log.cpp
    Main.cpp
    HTCondor.cpp
    Utils.cpp
)

SET (GRID_GA_HDR_FILES 
    FileUtils.hpp
    GeneticAlgo.hpp 
    GenerateXMLConfig.hpp
    Genome.hpp
    HTCondor.hpp
    Log.hpp
    Utils.hpp

    # Third Party
    Zmq.hpp
)

MSVC_PCH_SUPPORT("${GRID_GA_SRC_FILES}") 

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

# Need this definition, otherwise VS2012 will bollocks up it's min/max macros and numeric_limits won't compile 
ADD_DEFINITIONS("-DNOMINMAX")

ADD_EXECUTABLE(run_ga ${GRID_GA_SRC_FILES} ${GRID_GA_HDR_FILES} stdafx.cpp stdafx.hpp) 
    
IF (WIN32)
    IF (MINGW)
        TARGET_LINK_LIBRARIES(run_ga  ${Boost_LIBRARIES} ${ZMQ_LIBRARIES} ws2_32 gomp)
    ELSE()
        TARGET_LINK_LIBRARIES(run_ga ${Boost_LIBRARIES} ${ZMQ_LIBRARIES} ws2_32)
        ADD_DEFINITIONS(-DZMQ_STATIC)
    ENDIF()
ELSE()
    TARGET_LINK_LIBRARIES(run_ga ${Boost_LIBRARIES} ${ZMQ_LIBRARY} ${CMAKE_DL_LIBS})
ENDIF()
