#include <catch2/catch_test_macros.hpp>

#include "tt/version.hpp"

TEST_CASE("core library links and runs", "[smoke]") {
    REQUIRE(tt::add(2, 3) == 5);
}
