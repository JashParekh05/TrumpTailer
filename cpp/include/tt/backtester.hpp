#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "tt/bar_store.hpp"
#include "tt/costs.hpp"

namespace tt {

struct Trade {
    std::string symbol;
    std::int64_t entry_ts = 0, exit_ts = 0;
    double entry_px = 0, exit_px = 0;
    int direction = 0;
    double gross_ret = 0, net_ret = 0, cost = 0;
};

struct BacktestConfig {
    int holding_bars = 3;   // bars held from entry to exit
    int latency_bars = 0;   // bars between the event bar and entry (>= 0)
    LinearCostModel costs{};
};

struct BacktestResult {
    std::vector<Trade> trades;        // chronological by exit
    std::vector<double> equity;       // net, compounded from 1.0
    std::vector<double> gross_returns;
    std::vector<double> net_returns;
    double gross_sharpe = 0, net_sharpe = 0;
    double total_net_return = 0, max_drawdown = 0, hit_rate = 0, profit_factor = 0;
    int n = 0;
};

// Event-driven per-trade backtest. Each event opens a position at
// event_bar_idx + latency, in the sign of its score, closed holding_bars later;
// returns are net of round-trip costs. Entry is never before the event bar, so
// the no-look-ahead guarantee from alignment carries through.
class EventDrivenBacktester {
public:
    static BacktestResult run(const std::vector<std::string>& symbols,
                              const std::vector<long long>& event_bar_idx,
                              const std::vector<double>& scores, const BarStore& store,
                              const BacktestConfig& cfg);
};

}  // namespace tt
