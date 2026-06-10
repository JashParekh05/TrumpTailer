#pragma once

#include <cstdint>
#include <string>

namespace tt {

enum class EventCategory : std::int8_t {
    TariffTrade = 0,
    FedRates = 1,
    SingleName = 2,
    Geopolitical = 3,
    Other = 4,
};

enum class Direction : std::int8_t {
    Bearish = -1,
    Neutral = 0,
    Bullish = 1,
};

// One classified, instrument-resolved event. Timestamps are UTC nanoseconds
// since epoch everywhere in the core; conversion to market sessions happens
// only in TradingCalendar.
struct Event {
    std::int64_t ts_utc_ns = 0;
    EventCategory category = EventCategory::Other;
    Direction direction = Direction::Neutral;
    double magnitude = 0.0;   // [0, 1]
    double novelty = 0.0;     // [0, 1]
    double confidence = 0.0;  // [0, 1]
    std::string instrument;   // resolved ticker, e.g. "SPY"
    std::string source_id;    // traces back to the originating post/article
};

}  // namespace tt
