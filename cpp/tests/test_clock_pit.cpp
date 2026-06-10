#include <catch2/catch_test_macros.hpp>

#include <chrono>

#include "tt/bar_store.hpp"
#include "tt/clock.hpp"

using namespace tt;
using namespace std::chrono;

// Build a UTC-nanosecond timestamp from an explicit UTC wall clock.
static int64_t utc_ns(int y, int mo, int d, int h, int mi) {
    sys_time<nanoseconds> tp = sys_days{year{y} / mo / d} + hours{h} + minutes{mi};
    return tp.time_since_epoch().count();
}

TEST_CASE("session classification", "[clock]") {
    TradingCalendar cal;
    // 2025-04-02 (Wed) 14:00 ET == 18:00 UTC (EDT) -> regular hours.
    REQUIRE(cal.classify(utc_ns(2025, 4, 2, 18, 0)) == SessionType::RegularHours);
    // 2025-04-02 02:00 ET == 06:00 UTC -> pre-market.
    REQUIRE(cal.classify(utc_ns(2025, 4, 2, 6, 0)) == SessionType::PreMarket);
    // 2025-04-02 20:00 ET == 00:00 UTC next day -> after hours.
    REQUIRE(cal.classify(utc_ns(2025, 4, 3, 0, 0)) == SessionType::AfterHours);
    // Saturday 2025-04-05 -> closed.
    REQUIRE(cal.classify(utc_ns(2025, 4, 5, 18, 0)) == SessionType::Closed);
    // Good Friday 2025-04-18 (holiday) -> closed even at 14:00 ET.
    REQUIRE(cal.classify(utc_ns(2025, 4, 18, 18, 0)) == SessionType::Closed);
}

TEST_CASE("no look-ahead: actionable time never precedes the event", "[clock][pit]") {
    TradingCalendar cal;

    // Regular-hours post is actionable immediately.
    int64_t reg = utc_ns(2025, 4, 2, 18, 0);
    REQUIRE(cal.actionable_time(reg) == reg);

    // After-hours post acts at the NEXT open, strictly after that day's close.
    int64_t after = utc_ns(2025, 4, 3, 0, 0);  // 2025-04-02 20:00 ET
    int64_t act_after = cal.actionable_time(after);
    REQUIRE(act_after > after);
    REQUIRE(act_after > cal.session_close(after));  // not the contemporaneous close
    REQUIRE(cal.classify(act_after) == SessionType::RegularHours);

    // Weekend post acts at Monday's open.
    int64_t sat = utc_ns(2025, 4, 5, 18, 0);
    int64_t act_sat = cal.actionable_time(sat);
    REQUIRE(act_sat > sat);
    REQUIRE(cal.classify(act_sat) == SessionType::RegularHours);
    REQUIRE(act_sat == cal.next_open_after(sat));

    // Pre-market post acts at the SAME day's open.
    int64_t pre = utc_ns(2025, 4, 2, 6, 0);  // 02:00 ET
    int64_t act_pre = cal.actionable_time(pre);
    REQUIRE(act_pre > pre);
    REQUIRE(cal.classify(act_pre) == SessionType::RegularHours);
}

TEST_CASE("BarStore as-of access never returns a future bar", "[bar_store][pit]") {
    BarStore store;
    std::vector<Bar> bars;
    for (int i = 0; i < 5; ++i) {
        bars.push_back(Bar{utc_ns(2025, 4, 1 + i, 20, 0), 0, 0, 0, double(i), 0});
    }
    store.load_symbol("SPY", bars);

    // Query strictly before the first bar -> nothing.
    REQUIRE_FALSE(store.bar_asof("SPY", utc_ns(2025, 3, 31, 0, 0)).has_value());

    // Query between bar 1 and bar 2 -> returns bar 1 (the latest <= query).
    auto b = store.bar_asof("SPY", utc_ns(2025, 4, 2, 23, 0));
    REQUIRE(b.has_value());
    REQUIRE(b->close == 1.0);
    REQUIRE(b->ts_utc_ns <= utc_ns(2025, 4, 2, 23, 0));
}

TEST_CASE("BarStore rejects non-monotonic timestamps", "[bar_store]") {
    BarStore store;
    std::vector<Bar> bad = {Bar{200, 0, 0, 0, 0, 0}, Bar{100, 0, 0, 0, 0, 0}};
    REQUIRE_THROWS_AS(store.load_symbol("X", bad), std::invalid_argument);
}
