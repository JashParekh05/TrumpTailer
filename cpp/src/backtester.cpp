#include "tt/backtester.hpp"

#include <algorithm>

#include "tt/metrics.hpp"

namespace tt {

BacktestResult EventDrivenBacktester::run(const std::vector<std::string>& symbols,
                                          const std::vector<long long>& event_bar_idx,
                                          const std::vector<double>& scores,
                                          const BarStore& store,
                                          const BacktestConfig& cfg) {
    BacktestResult res;

    for (std::size_t e = 0; e < symbols.size(); ++e) {
        int dir = scores[e] > 0 ? 1 : (scores[e] < 0 ? -1 : 0);
        if (dir == 0 || !store.has_symbol(symbols[e])) continue;

        const auto& bars = store.bars(symbols[e]);
        std::ptrdiff_t entry = static_cast<std::ptrdiff_t>(event_bar_idx[e]) + cfg.latency_bars;
        std::ptrdiff_t exit = entry + cfg.holding_bars;
        if (entry < 0 || exit >= static_cast<std::ptrdiff_t>(bars.size())) continue;

        double ep = bars[entry].close, xp = bars[exit].close;
        double gross = dir * (xp / ep - 1.0);
        double cost = 2.0 * cfg.costs.cost_bps(1.0) / 1e4;  // round trip on unit notional
        res.trades.push_back(Trade{symbols[e], bars[entry].ts_utc_ns, bars[exit].ts_utc_ns,
                                   ep, xp, dir, gross, gross - cost, cost});
    }

    std::sort(res.trades.begin(), res.trades.end(),
              [](const Trade& a, const Trade& b) { return a.exit_ts < b.exit_ts; });

    double equity = 1.0;
    for (const auto& t : res.trades) {
        res.gross_returns.push_back(t.gross_ret);
        res.net_returns.push_back(t.net_ret);
        equity *= (1.0 + t.net_ret);
        res.equity.push_back(equity);
    }

    res.n = static_cast<int>(res.trades.size());
    res.gross_sharpe = sharpe(res.gross_returns);
    res.net_sharpe = sharpe(res.net_returns);
    res.total_net_return = equity - 1.0;
    res.max_drawdown = max_drawdown(res.equity);
    res.hit_rate = hit_rate(res.net_returns);
    res.profit_factor = profit_factor(res.net_returns);
    return res;
}

}  // namespace tt
