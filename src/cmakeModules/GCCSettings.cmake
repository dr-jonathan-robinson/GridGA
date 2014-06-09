
# optimisations from http://www.willus.com/ccomp_benchmark2.shtml?p3

IF(MINGW)

    LIST(APPEND GCC_FLAGS
        -Wall
        -Werror
        -Wno-error=deprecated-declarations
        -Wno-long-long # needed for TA-lib
        # http://lists.boost.org/Archives/boost/2009/05/151305.php
        #-Wno-strict-aliasing 
        #-pedantic
        -Ofast
        -ffast-math
        -march=native
        -fomit-frame-pointer 
        -momit-leaf-frame-pointer
        #-fopenmp  
     
        -std=gnu++0x
        -msse2
        #-mfpmath=sse
        #-fpermissive
        #-enable-stdcall-fixup
    )
    
    SET(CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -enable-stdcall-fixup")

ELSE()

    LIST(APPEND GCC_FLAGS
        -Wall
        -Werror
        -Wno-error=deprecated-declarations
        -Wno-error=narrowing
        -Wno-long-long # needed for TA-lib
        # http://lists.boost.org/Archives/boost/2009/05/151305.php
        -Wno-strict-aliasing 
        -Wno-error=unused-function
        #-pedantic
        -O3
        -ffast-math
        #-march=native
        -fomit-frame-pointer 
        -momit-leaf-frame-pointer
        -fopenmp  
        #-std=gnu++0x
        -std=c++11
        -msse2
        #-mfpmath=sse
        #-fprofile-generate/use
        -lrt        
        
        # the following optimise multi-threaded bits
        #-fgraphite-identity
        #-floop-interchange
        #-floop-block
        #-floop-parallelize-all
        #-ftree-loop-distribution  
        #-ftree-parallelize-loops
    )
    

ENDIF()


SET(PYTHON_LIB "/home/dev/anaconda/lib/libpython2.7.so")

#IF(FNOPIC)
#    LIST(APPEND GCC_FLAGS -fno-PIC)
#ELSE()
#    LIST(APPEND GCC_FLAGS -fPIC)
#ENDIF()


#ADD_DEFINITIONS("-Doverride=")

# Turn the list into a string
FOREACH(_FLAG ${GCC_FLAGS})
    SET(CMAKE_CXX_FLAGS "${_FLAG} ${CMAKE_CXX_FLAGS}")
ENDFOREACH()


