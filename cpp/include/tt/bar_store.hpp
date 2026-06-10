#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace tt {

// One OHLCV bar. ts_utc_ns is the bar's COMPLETION time (e.g. a daily bar
// carries that session's close time), so "bar with ts <= t" == "information
// available at t".
struct Bar {
    std::int64_t ts_utc_ns = 0;
    double open = 0, high = 0, low = 0, close = 0, volume = 0;
};

// Point-in-time bar container. The only read accessors are as-of style:
// they can never return a bar completed after the query timestamp.
class BarStore {
public:
    // Arrays must be equal length and strictly increasing in ts; throws
    // std::invalid_argument otherwise.
    void load_symbol(const std::string& symbol, std::vector<Bar> bars);

    // Most recent bar with ts_utc_ns <= ts, if any.
    std::optional<Bar> bar_asof(const std::string& symbol, std::int64_t ts) const;

    // Index of that bar in bars(symbol), or -1.
    std::ptrdiff_t index_asof(const std::string& symbol, std::int64_t ts) const;

    const std::vector<Bar>& bars(const std::string& symbol) const;
    bool has_symbol(const std::string& symbol) const;
    std::vector<std::string> symbols() const;

private:
    std::unordered_map<std::string, std::vector<Bar>> data_;
};

}  // namespace tt
