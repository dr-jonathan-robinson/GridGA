@echo off
rem Creates a VS2012 solution 
rem Remember to set the build type to 'Release' in the Visual Studio IDE!

mkdir build
cd build
del *.sln
del build.log

del CMakeCache.txt
cmake -G "Visual Studio 11"  ../src

rem for 64 bit use
rem cmake -G "Visual Studio 11 Win64" ../src

