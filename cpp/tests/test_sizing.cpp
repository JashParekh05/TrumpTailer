#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include "tt/metrics.hpp"
#include "tt/sizing.hpp"

using namespace tt;
using Catch::Matchers::WithinAbs;

TEST_CASE("kelly fraction textbook values", "[sizing][kelly]") {
    REQUIRE_THAT(kelly_fraction(0.6, 1.0), WithinAbs(0.2, 1e-12));   // (0.6-0.4)/1
    REQUIRE_THAT(kelly_fraction(0.5, 1.0), WithinAbs(0.0, 1e-12));   // no edge
    REQUIRE_THAT(kelly_gaussian(0.001, 0.04), WithinAbs(0.025, 1e-12));
    REQUIRE(kelly_gaussian(0.01, 0.0) == 0.0);                       // guard
}

TEST_CASE("fractional kelly scales and clamps", "[sizing]") {
    REQUIRE_THAT(fractional_kelly(0.2, 0.5, 1.0), WithinAbs(0.1, 1e-12));
    REQUIRE_THAT(fractional_kelly(4.0, 0.5, 0.5), WithinAbs(0.5, 1e-12));   // capped
    REQUIRE_THAT(fractional_kelly(-4.0, 0.5, 0.5), WithinAbs(-0.5, 1e-12)); // capped short
    REQUIRE_THAT(vol_target_weight(0.10, 0.20, 1.0), WithinAbs(0.5, 1e-12));
}

TEST_CASE("portfolio volatility uses correlation", "[sizing][corr]") {
    // Two equal-weight assets, var 0.04 each, correlation 1.0 -> vol 0.4.
    std::vector<double> w = {1.0, 1.0};
    std::vector<double> cov = {0.04, 0.04, 0.04, 0.04};
    REQUIRE_THAT(portfolio_volatility(w, cov), WithinAbs(0.4, 1e-12));
    // Uncorrelated -> lower vol than correlated.
    std::vector<double> cov0 = {0.04, 0.0, 0.0, 0.04};
    REQUIRE(portfolio_volatility(w, cov0) < portfolio_volatility(w, cov));
}

TEST_CASE("fractional Kelly equity is less volatile than full", "[sizing]") {
    std::vector<double> r = {0.02, -0.015, 0.03, -0.01, 0.025, -0.02};
    std::vector<double> full(r.size(), 1.0), frac(r.size(), 0.5);
    auto eq_full = sized_equity(r, full);
    auto eq_frac = sized_equity(r, frac);

    std::vector<double> rf, rr;
    for (std::size_t i = 1; i < eq_full.size(); ++i) {
        rf.push_back(eq_full[i] / eq_full[i - 1] - 1.0);
        rr.push_back(eq_frac[i] / eq_frac[i - 1] - 1.0);
    }
    REQUIRE(stddev(rr, mean(rr), 1) < stddev(rf, mean(rf), 1));
}

TEST_CASE("drawdown stop caps losses", "[sizing][risk]") {
    std::vector<double> r = {-0.05, -0.05, -0.05, -0.05, -0.05};  // relentless losses
    std::vector<double> sizes(r.size(), 1.0);
    auto unstopped = sized_equity(r, sizes, 0.0);
    auto stopped = sized_equity(r, sizes, 0.10);  // halt after 10% drawdown
    REQUIRE(max_drawdown(stopped) < max_drawdown(unstopped));
    REQUIRE(stopped.back() > unstopped.back());    // stopping preserved capital
}
