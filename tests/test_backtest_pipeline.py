"""P4: end-to-end backtest is net-of-costs and multiple-testing honest."""

from pathlib import Path

from trumptailer import core
from trumptailer.research.run_backtest import build_inputs

_DATA = Path(__file__).resolve().parents[1] / "data" / "golden"


def test_costs_make_net_below_gross():
    symbols, idx, scores, store = build_inputs(
        _DATA / "truth_sample.parquet", _DATA / "prices_daily.parquet"
    )
    cfg = core.BacktestConfig(holding_bars=3, costs=core.LinearCostModel(half_spread_bps=2.0))
    r = core.backtest(symbols, idx, scores, store, cfg)

    assert r.n >= 1
    for t in r.trades:
        assert t.net_ret < t.gross_ret          # every trade pays costs
    gross_equity = 1.0
    for g in r.gross_returns:
        gross_equity *= (1.0 + g)
    assert r.total_net_return < gross_equity - 1.0   # net compounds below gross


def test_deflated_sharpe_is_a_probability():
    symbols, idx, scores, store = build_inputs(
        _DATA / "truth_sample.parquet", _DATA / "prices_daily.parquet"
    )
    configs = [core.BacktestConfig(holding_bars=h) for h in (1, 2, 3, 5)]
    from trumptailer.research.walk_forward import deflated_over_configs

    summary = deflated_over_configs(symbols, idx, scores, store, configs)
    assert summary["n_trials"] == 4
    dsr = summary["deflated_sharpe"]
    assert dsr != dsr or 0.0 <= dsr <= 1.0   # NaN (too few trades) or a valid probability
