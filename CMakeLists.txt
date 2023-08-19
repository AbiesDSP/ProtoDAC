cmake_minimum_required(VERSION 3.22)
project(HeadphoneDAC)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE On)

add_subdirectory(HeadphoneDAC.cydsn)
add_subdirectory(tests)
