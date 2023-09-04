cmake_minimum_required(VERSION 3.22)
project(ProtoDAC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE On)

add_subdirectory(ProtoDAC.cydsn)
add_subdirectory(tests)
