#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include "tt/metrics.hpp"

using namespace tt;
using Catch::Matchers::WithinAbs;

TEST_CASE("normal cdf / ppf anchors", "[metrics]") {
    REQUIRE_THAT(normal_cdf(0.0), WithinAbs(0.5, 1e-12));
    REQUIRE_THAT(normal_cdf(1.959963985), WithinAbs(0.975, 1e-6));
    REQUIRE_THAT(normal_ppf(0.5), WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(normal_ppf(0.975), WithinAbs(1.959963985, 1e-4));
}

TEST_CASE("sharpe and drawdown", "[metrics]") {
    std::vector<double> r = {0.01, 0.02, -0.01, 0.015, 0.005};
    double m = mean(r);
    REQUIRE_THAT(sharpe(r), WithinAbs(m / stddev(r, m, 1), 1e-12));

    std::vector<double> eq = {1.0, 1.2, 0.9, 1.1};  // peak 1.2 -> trough 0.9
    REQUIRE_THAT(max_drawdown(eq), WithinAbs(0.25, 1e-12));
}

TEST_CASE("probabilistic sharpe rises with the sample Sharpe", "[metrics]") {
    std::vector<double> weak(100, 0.0), strong(100, 0.0);
    for (int i = 0; i < 100; ++i) {
        weak[i] = (i % 2 ? 0.01 : -0.008);   // small positive drift
        strong[i] = (i % 2 ? 0.02 : -0.002); // larger positive drift
    }
    double psr_weak = probabilistic_sharpe(weak, 0.0);
    double psr_strong = probabilistic_sharpe(strong, 0.0);
    REQUIRE(psr_weak > 0.0);
    REQUIRE(psr_weak < 1.0);
    REQUIRE(psr_strong > psr_weak);
}

TEST_CASE("deflation penalizes multiple testing", "[metrics][deflated]") {
    std::vector<double> r(200, 0.0);
    for (int i = 0; i < 200; ++i) r[i] = (i % 2 ? 0.012 : -0.004);

    double psr = probabilistic_sharpe(r, 0.0);
    double dsr_few = deflated_sharpe(r, 5, 0.25);
    double dsr_many = deflated_sharpe(r, 500, 0.25);

    REQUIRE(dsr_few <= psr);          // any deflation can only lower confidence
    REQUIRE(dsr_many < dsr_few);      // more trials -> stronger deflation
    REQUIRE(expected_max_sharpe(1, 0.25) == 0.0);  // a single trial isn't deflated
}
