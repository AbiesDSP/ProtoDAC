# Include(FetchContent)

# FetchContent_Declare(
#   Catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG        v3.4.0 # or a later release
# )

# FetchContent_MakeAvailable(Catch2)
find_package(Catch2 3 REQUIRED)
include(Catch)

include_directories(${CMAKE_SOURCE_DIR}/HeadphoneDAC.cydsn)
include_directories(${CMAKE_SOURCE_DIR}/HeadphoneDAC.cydsn/Generated_Source/PSoC5)
include_directories(${CMAKE_SOURCE_DIR}/FreeRTOS/Source/portable/MSVC-MingW)
include_directories(${CMAKE_SOURCE_DIR}/FreeRTOS/Source/include)

# Main test runner.
add_executable(run_all_tests
    # test_logging.cpp
    test_filter.cpp
)

target_include_directories(run_all_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/HeadphoneDAC.cydsn
)
target_link_libraries(run_all_tests
    # logging
    filter
    Catch2::Catch2WithMain)

# target_compile_definitions(run_all_tests PRIVATE "LOG_ENABLE_TRACE=$<IF:$<CONFIG:Debug>,1,0>")
# set_target_properties(run_all_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

catch_discover_tests(run_all_tests)
