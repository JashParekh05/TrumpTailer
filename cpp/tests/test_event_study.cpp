#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <cmath>
#include <string>
#include <vector>

#include "tt/bar_store.hpp"
#include "tt/event_study.hpp"

using namespace tt;
using Catch::Matchers::WithinAbs;

namespace {
constexpr long long kBase = 1700000000000000000LL;
constexpr long long kDay = 86400000000000LL;

// Market return at bar t (>=1): small repeating, nonzero-variance pattern.
double rm(int t) { return 0.004 * ((t % 5) - 2); }
}  // namespace

// A market that the instruments track with beta=0.5, alpha=0 — except a known
// abnormal shock injected on the event day. The study must recover that shock.
TEST_CASE("event study recovers a known abnormal return", "[event_study]") {
    const int N = 200;
    const int kEvent = 150;
    const std::vector<double> shocks = {-0.020, -0.025, -0.030, -0.035, -0.040};

    BarStore store;
    std::vector<Bar> mkt(N);
    mkt[0] = Bar{kBase, 0, 0, 0, 100.0, 0};
    for (int t = 1; t < N; ++t) {
        mkt[t] = Bar{kBase + t * kDay, 0, 0, 0, mkt[t - 1].close * std::exp(rm(t)), 0};
    }
    store.load_symbol("MKT", mkt);

    std::vector<std::string> symbols;
    std::vector<long long> idx;
    for (std::size_t e = 0; e < shocks.size(); ++e) {
        std::vector<Bar> inst(N);
        inst[0] = Bar{kBase, 0, 0, 0, 50.0, 0};
        for (int t = 1; t < N; ++t) {
            double ri = 0.5 * rm(t) + (t == kEvent ? shocks[e] : 0.0);
            inst[t] = Bar{kBase + t * kDay, 0, 0, 0, inst[t - 1].close * std::exp(ri), 0};
        }
        std::string sym = "AAA" + std::to_string(e);
        store.load_symbol(sym, inst);
        symbols.push_back(sym);
        idx.push_back(kEvent);
    }

    EventStudyConfig cfg{-120, -20, -1, 1};  // event window [-1, +1]
    EventStudyResult r = EventStudy::run(symbols, idx, store, "MKT", cfg);

    REQUIRE(r.n == 5);
    REQUIRE(r.offsets == std::vector<int>{-1, 0, 1});

    // Only tau=0 carries the shock; neighbours are clean.
    REQUIRE_THAT(r.aar[0], WithinAbs(0.0, 1e-9));
    REQUIRE_THAT(r.aar[1], WithinAbs(-0.030, 1e-9));   // mean of shocks
    REQUIRE_THAT(r.aar[2], WithinAbs(0.0, 1e-9));

    // Mean CAR over the window equals the mean shock.
    REQUIRE_THAT(r.caar_total, WithinAbs(-0.030, 1e-9));

    // t-stat: mean / (sample_std / sqrt(n)). std of shocks = 0.0079057...
    REQUIRE_THAT(r.t_caar, WithinAbs(-8.4853, 1e-3));
    REQUIRE_THAT(r.t_aar[1], WithinAbs(-8.4853, 1e-3));  // same as caar (single shocked offset)
}

TEST_CASE("event study skips events lacking history", "[event_study]") {
    BarStore store;
    std::vector<Bar> mkt(50);
    for (int t = 0; t < 50; ++t) {
        mkt[t] = Bar{kBase + t * kDay, 0, 0, 0, 100.0 + t, 0};
    }
    store.load_symbol("MKT", mkt);
    store.load_symbol("AAA", mkt);

    // Event at index 5 cannot fill a [-120,-20) estimation window.
    EventStudyResult r = EventStudy::run({"AAA"}, {5}, store, "MKT", EventStudyConfig{});
    REQUIRE(r.n == 0);
}
