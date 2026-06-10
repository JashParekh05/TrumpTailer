#pragma once

#include <cstdint>
#include <unordered_set>

namespace std::chrono {
class time_zone;  // fwd-declare; full <chrono> stays in the .cpp
}

namespace tt {

enum class SessionType : std::int8_t {
    RegularHours = 0,
    PreMarket = 1,
    AfterHours = 2,
    Closed = 3,  // weekend or full-closure holiday
};

// NYSE session calendar over the research period (2021-2026), embedded
// closure/early-close tables, DST-correct via the IANA tzdb.
//
// This class is the single look-ahead gate: an out-of-session timestamp is
// only ever actionable at the NEXT regular open, never an earlier bar.
class TradingCalendar {
public:
    TradingCalendar();

    SessionType classify(std::int64_t ts_utc_ns) const;

    // Earliest UTC ns at which a new order could interact with the regular
    // session: ts itself if RegularHours, otherwise the next regular open.
    // Invariant: actionable_time(ts) >= ts.
    std::int64_t actionable_time(std::int64_t ts_utc_ns) const;

    // First regular-session open strictly after ts.
    std::int64_t next_open_after(std::int64_t ts_utc_ns) const;

    // Regular-session close (16:00 ET, or 13:00 on early-close days) of the
    // trading day containing ts; only valid when classify(ts) != Closed.
    std::int64_t session_close(std::int64_t ts_utc_ns) const;

private:
    bool is_trading_day(std::int32_t yyyymmdd) const;  // weekday + not holiday
    bool is_early_close(std::int32_t yyyymmdd) const;

    const std::chrono::time_zone* ny_;
    std::unordered_set<std::int32_t> holidays_;      // full closures, yyyymmdd
    std::unordered_set<std::int32_t> early_closes_;  // 13:00 ET closes
};

}  // namespace tt
