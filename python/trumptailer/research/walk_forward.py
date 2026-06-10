"""Multiple-testing-honest evaluation.

Every holding-period (or threshold) we try is a trial. We run them all, then
deflate the best one's Sharpe by the number of trials and the spread of their
Sharpes — so a strategy that only looks good because we searched hard is exposed.
"""

from __future__ import annotations

import numpy as np

from trumptailer import core


def deflated_over_configs(symbols, event_bar_idx, scores, store, configs) -> dict:
    """Run each BacktestConfig, then report the best config's deflated Sharpe."""
    results = [core.backtest(symbols, event_bar_idx, scores, store, c) for c in configs]
    sharpes = [r.net_sharpe for r in results]
    if not results:
        return {}

    best = int(np.argmax(sharpes))
    best_r = results[best]
    var_sr = float(np.var(sharpes, ddof=1)) if len(sharpes) > 1 else 0.0
    dsr = (
        core.deflated_sharpe(list(best_r.net_returns), len(configs), max(var_sr, 1e-9))
        if best_r.n >= 3
        else float("nan")  # too few trades to deflate honestly
    )
    return {
        "n_trials": len(configs),
        "best_holding_bars": configs[best].holding_bars,
        "net_sharpe": round(best_r.net_sharpe, 4),
        "gross_sharpe": round(best_r.gross_sharpe, 4),
        "deflated_sharpe": dsr,
        "n_trades": best_r.n,
    }
