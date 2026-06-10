#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <vector>

#include "tt/backtester.hpp"
#include "tt/bar_store.hpp"

using namespace tt;
using Catch::Matchers::WithinAbs;

namespace {
constexpr long long kDay = 86400000000000LL;

BarStore make_store() {
    BarStore s;
    std::vector<Bar> bars(20);
    for (int i = 0; i < 20; ++i) {
        bars[i] = Bar{i * kDay, 0, 0, 0, 100.0 + i, 0};  // steadily rising
    }
    s.load_symbol("UP", bars);
    return s;
}
}  // namespace

TEST_CASE("long trade on a rising series is profitable; costs reduce it", "[backtester]") {
    BarStore store = make_store();
    BacktestConfig cfg;
    cfg.holding_bars = 3;
    cfg.latency_bars = 0;
    cfg.costs = LinearCostModel{5.0, 5.0, 0.0, 1e9};  // 10 bps/side

    BacktestResult r = EventDrivenBacktester::run({"UP"}, {5}, {+1.0}, store, cfg);
    REQUIRE(r.n == 1);
    const Trade& t = r.trades[0];
    // entry bar 5 (close 105) -> exit bar 8 (close 108): +2.857% gross.
    REQUIRE_THAT(t.gross_ret, WithinAbs(108.0 / 105.0 - 1.0, 1e-12));
    REQUIRE(t.net_ret < t.gross_ret);                  // costs bite
    REQUIRE_THAT(t.cost, WithinAbs(2.0 * 10.0 / 1e4, 1e-12));  // 20 bps round trip
}

TEST_CASE("short direction flips the sign", "[backtester]") {
    BarStore store = make_store();
    BacktestConfig cfg;
    cfg.holding_bars = 2;
    cfg.costs = LinearCostModel{0, 0, 0, 1e9};  // zero cost to isolate sign

    BacktestResult r = EventDrivenBacktester::run({"UP"}, {5}, {-1.0}, store, cfg);
    REQUIRE(r.n == 1);
    REQUIRE(r.trades[0].gross_ret < 0.0);  // shorting a rising series loses
}

TEST_CASE("no look-ahead: events without enough future bars are dropped", "[backtester][pit]") {
    BarStore store = make_store();  // 20 bars
    BacktestConfig cfg;
    cfg.holding_bars = 3;

    // event at bar 19 cannot hold 3 bars forward -> skipped.
    BacktestResult r = EventDrivenBacktester::run({"UP"}, {19}, {1.0}, store, cfg);
    REQUIRE(r.n == 0);
}

TEST_CASE("zero score is not traded", "[backtester]") {
    BarStore store = make_store();
    BacktestResult r = EventDrivenBacktester::run({"UP"}, {5}, {0.0}, store, BacktestConfig{});
    REQUIRE(r.n == 0);
}
