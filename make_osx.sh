mkdir build
cd build

export ZMQ_ROOT=/usr/local

rm CMakeCache.txt

cmake ../src

rm -fr ../../build_ga
mkdir ../../build_ga
cd ../../build_ga
cmake -G"Eclipse CDT4 - Unix Makefiles" -DCMAKE_ECLIPSE_GENERATE_SOURCE_PROJECT=TRUE ../GridGA/src/
