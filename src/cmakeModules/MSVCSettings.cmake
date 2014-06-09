
# Allow use of folders (this breaks VS Express, so is off by default in CMake, but not a prob for us)
#SET_PROPERTY(GLOBAL PROPERTY USE_FOLDERS TRUE)

# Optimisations from http://www.willus.com/ccomp_benchmark2.shtml?p3

# /MP parallel compilation
# /O2 speed optimisation
# /WX treat warnings as errors
# /arch:SSE2 32bit only optimisation
# /MT multithreaded
# /GS- disable security checks
# /fp:fast "fast" floating point model (less predictable results)
# /Qfast_transcendentals generate inline FP intrinsics
# /arch:SSE2 enable use of SSE2-enabled CPU (not available on 64-bit compiles!)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /openmp /MP /WX /MT")

IF(CMAKE_BUILD_TYPE STREQUAL "Release") # only add optimise flags for the release
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /O2 /GS- /fp:fast /Qfast_transcendentals /arch:SSE2")
ENDIF()

ADD_DEFINITIONS("-D_SCL_SECURE_NO_DEPRECATE")
ADD_DEFINITIONS("-D_CRT_SECURE_NO_DEPRECATE")
ADD_DEFINITIONS("-D_CRT__NONSTDC_NO_DEPRECATE")
#ADD_DEFINITIONS("-D_WIN32_WINNT=0x0500")
ADD_DEFINITIONS("-DWIN32_LEAN_AND_MEAN")
ADD_DEFINITIONS("-DNOMINMAX")
ADD_DEFINITIONS("-DNOMSG")
ADD_DEFINITIONS("-DBOOST_PYTHON_STATIC_LIB")

IF(CMAKE_BUILD_TYPE STREQUAL "Release")
    ADD_DEFINITIONS("-D_SECURE_SCL=0")
    ADD_DEFINITIONS("-D_HAS_ITERATOR_DEBUGGING=0")
ENDIF()

# set the boost location here, any bypass all the FindBoost bollocks which hardly ever works
SET(BOOST_VERSION_S "1.55.0")
SET(BOOST_VERSION_STRING "1_55")
SET(Boost_INCLUDE_DIR "C:/boost/include/boost-1_55")
SET(Boost_LIBRARY_DIRS "C:/boost/lib/")
IF (MSVC11)
    SET(COMPILER_PREFIX "vc110")
ELSE()
    SET(COMPILER_PREFIX "vc100")
ENDIF()

SET(PYTHON_LIB "C:/Miniconda.27/libs/python27.lib")

IF(CMAKE_BUILD_TYPE STREQUAL "Release") 
    SET(BOOST_LIB_TYPE "mt") # "mt-s"
ELSE()
    SET(BOOST_LIB_TYPE "mt-gd") # "mt-s"
ENDIF()

LIST(APPEND Boost_LIBRARIES 
    "${Boost_LIBRARY_DIRS}libboost_chrono-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib"
    "${Boost_LIBRARY_DIRS}libboost_date_time-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib"
    "${Boost_LIBRARY_DIRS}libboost_filesystem-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib"
    "${Boost_LIBRARY_DIRS}libboost_program_options-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib"
    "${Boost_LIBRARY_DIRS}libboost_regex-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib" 
    "${Boost_LIBRARY_DIRS}libboost_system-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib" 
    "${Boost_LIBRARY_DIRS}libboost_thread-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib" 
    "${Boost_LIBRARY_DIRS}libboost_unit_test_framework-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib" 
    "${Boost_LIBRARY_DIRS}libboost_python-${COMPILER_PREFIX}-${BOOST_LIB_TYPE}-${BOOST_VERSION_STRING}.lib"    
)

