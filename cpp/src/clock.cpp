#include "tt/clock.hpp"

#include <chrono>
#include <stdexcept>

namespace tt {

using namespace std::chrono;

namespace {

constexpr int kOpenMin = 9 * 60 + 30;   // 09:30 ET
constexpr int kCloseMin = 16 * 60;      // 16:00 ET
constexpr int kEarlyCloseMin = 13 * 60; // 13:00 ET

int32_t to_int(const year_month_day& ymd) {
    return int(ymd.year()) * 10000 + int(unsigned(ymd.month())) * 100 +
           int(unsigned(ymd.day()));
}

year_month_day from_int(int32_t v) {
    return year_month_day{year{v / 10000}, month{unsigned((v / 100) % 100)},
                          day{unsigned(v % 100)}};
}

}  // namespace

TradingCalendar::TradingCalendar() : ny_(get_tzdb().locate_zone("America/New_York")) {
    // NYSE full-closure holidays, 2021–2026 (observed dates).
    holidays_ = {
        20210101, 20210118, 20210215, 20210402, 20210531, 20210705, 20210906,
        20211125, 20211224, 20211231,
        20220117, 20220221, 20220415, 20220530, 20220620, 20220704, 20220905,
        20221124, 20221226,
        20230102, 20230116, 20230220, 20230407, 20230529, 20230619, 20230704,
        20230904, 20231123, 20231225,
        20240101, 20240115, 20240219, 20240329, 20240527, 20240619, 20240704,
        20240902, 20241128, 20241225,
        20250101, 20250109, 20250120, 20250217, 20250418, 20250526, 20250619,
        20250704, 20250901, 20251127, 20251225,
        20260101, 20260119, 20260216, 20260403, 20260525, 20260619, 20260703,
        20260907, 20261126, 20261225,
    };
    // Early closes (13:00 ET): day after Thanksgiving, some July 3 / Dec 24.
    early_closes_ = {
        20211126,
        20221125,
        20230703, 20231124,
        20240703, 20241129, 20241224,
        20250703, 20251128, 20251224,
        20261127, 20261224,
    };
}

bool TradingCalendar::is_trading_day(int32_t ymd) const {
    weekday wd{sys_days{from_int(ymd)}};
    if (wd == Saturday || wd == Sunday) return false;
    return holidays_.find(ymd) == holidays_.end();
}

bool TradingCalendar::is_early_close(int32_t ymd) const {
    return early_closes_.find(ymd) != early_closes_.end();
}

SessionType TradingCalendar::classify(int64_t ts) const {
    sys_time<nanoseconds> tp{nanoseconds{ts}};
    local_time<nanoseconds> lt = zoned_time{ny_, tp}.get_local_time();
    auto ld = floor<days>(lt);
    int32_t ymd = to_int(year_month_day{ld});
    if (!is_trading_day(ymd)) return SessionType::Closed;

    int minute = duration_cast<minutes>(lt - ld).count();
    int close_min = is_early_close(ymd) ? kEarlyCloseMin : kCloseMin;
    if (minute < kOpenMin) return SessionType::PreMarket;
    if (minute < close_min) return SessionType::RegularHours;
    return SessionType::AfterHours;
}

// Convert a local wall-clock time on a given date to UTC nanoseconds.
static int64_t local_to_utc_ns(const std::chrono::time_zone* tz,
                               const year_month_day& ymd, int hour, int min) {
    local_time<nanoseconds> lt =
        local_days{ymd} + hours{hour} + minutes{min};
    return zoned_time{tz, lt}.get_sys_time().time_since_epoch().count();
}

int64_t TradingCalendar::session_close(int64_t ts) const {
    sys_time<nanoseconds> tp{nanoseconds{ts}};
    auto ld = floor<days>(zoned_time{ny_, tp}.get_local_time());
    year_month_day ymd{ld};
    int close_min = is_early_close(to_int(ymd)) ? kEarlyCloseMin : kCloseMin;
    return local_to_utc_ns(ny_, ymd, close_min / 60, close_min % 60);
}

int64_t TradingCalendar::next_open_after(int64_t ts) const {
    sys_time<nanoseconds> tp{nanoseconds{ts}};
    auto ld = floor<days>(zoned_time{ny_, tp}.get_local_time());
    for (int i = 0; i < 12; ++i) {
        year_month_day ymd{ld + days{i}};
        if (is_trading_day(to_int(ymd))) {
            int64_t open = local_to_utc_ns(ny_, ymd, 9, 30);
            if (open > ts) return open;
        }
    }
    throw std::runtime_error("no trading day within 12 days of timestamp");
}

int64_t TradingCalendar::actionable_time(int64_t ts) const {
    SessionType st = classify(ts);
    if (st == SessionType::RegularHours) return ts;
    if (st == SessionType::PreMarket) {
        sys_time<nanoseconds> tp{nanoseconds{ts}};
        auto ld = floor<days>(zoned_time{ny_, tp}.get_local_time());
        return local_to_utc_ns(ny_, year_month_day{ld}, 9, 30);
    }
    return next_open_after(ts);  // AfterHours or Closed
}

}  // namespace tt
