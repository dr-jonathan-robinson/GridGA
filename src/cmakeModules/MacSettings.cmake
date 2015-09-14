#SET(CMAKE_C_COMPILER "/usr/local/Cellar/gcc48/4.8.4/bin/gcc-4.8")
#SET(CMAKE_CXX_COMPILER "/usr/local/Cellar/gcc48/4.8.4/bin/g++-4.8")

# optimisations from http://www.willus.com/ccomp_benchmark2.shtml?p3

LIST(APPEND CLANG_FLAGS
    #-Wall
    -Werror
    -Wno-error=deprecated-declarations
    -Wno-long-long # needed for TA-lib
    -Wno-overloaded-virtual
    # http://lists.boost.org/Archives/boost/2009/05/151305.php
    -Wno-strict-aliasing 
    -Wno-error=unused-function
    -O3
    -ffast-math
    -fomit-frame-pointer 
    -momit-leaf-frame-pointer
    -fopenmp  
    -msse2      

    # C++11 settings
    -Wno-error=narrowing
    -std=c++11
    )


INCLUDE_DIRECTORIES("/usr/local/include/libiomp")
SET(PYTHON_LIB "$ENV{HOME}/anaconda/lib/libpython2.7.dylib")
INCLUDE_DIRECTORIES("$ENV{HOME}/anaconda/include/python2.7")
SET(ZeroMQLib "/usr/local/lib/libzmq.dylib")


# Turn the list into a string
FOREACH(_FLAG ${CLANG_FLAGS})
    SET(CMAKE_CXX_FLAGS "${_FLAG} ${CMAKE_CXX_FLAGS}")
ENDFOREACH()


