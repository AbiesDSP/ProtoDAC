# Include(FetchContent)

# FetchContent_Declare(
#   Catch2
#   GIT_REPOSITORY https://github.com/catchorg/Catch2.git
#   GIT_TAG        v3.4.0 # or a later release
# )

# FetchContent_MakeAvailable(Catch2)
find_package(Catch2 3 REQUIRED)
include(Catch)

# Main test runner.
add_executable(run_all_tests
    test_hello.cpp
)
target_link_libraries(run_all_tests Catch2::Catch2WithMain)

set_target_properties(run_all_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/tests)

catch_discover_tests(run_all_tests)
