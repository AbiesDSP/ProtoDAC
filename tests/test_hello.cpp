// #include <iostream>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("Hello World", "[h]")
{
    // std::cout << "Hmm:" << std::endl;
    // FAIL("Hello, World!");
    REQUIRE(0 == 0);
}
