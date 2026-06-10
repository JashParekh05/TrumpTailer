"""Driver: posts -> classify -> align -> backtest, gross vs net + deflated Sharpe.

Defaults to the committed golden fixtures so it runs offline. On the real
archive + full price history this produces the headline P4 result.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd

from trumptailer import core
from trumptailer.ingest.prices import daily_frame_to_store
from trumptailer.ingest.truth_archive import TruthArchiveAdapter
from trumptailer.nlp.classifier import classify_posts
from trumptailer.nlp.schema import events_to_frame
from trumptailer.research.align import align_posts, build_event_locations
from trumptailer.research.signals import baseline_scores
from trumptailer.research.universe import load_universe
from trumptailer.research.walk_forward import deflated_over_configs

_ROOT = Path(__file__).resolve().parents[3]
_GOLDEN_POSTS = _ROOT / "data" / "golden" / "truth_sample.parquet"
_GOLDEN_PRICES = _ROOT / "data" / "golden" / "prices_daily.parquet"


def build_inputs(posts_path: Path, prices_path: Path):
    universe = load_universe()
    cal = core.TradingCalendar()
    store = core.BarStore()
    for symbol, g in pd.read_parquet(prices_path).groupby("symbol"):
        daily_frame_to_store(store, cal, symbol, g.set_index("date").sort_index())

    posts = TruthArchiveAdapter(str(posts_path)).to_frame()
    events = align_posts(events_to_frame(classify_posts(posts, universe)), cal)
    symbols, idx = build_event_locations(events, store)
    return symbols, idx, baseline_scores(events), store


def run(posts_path: Path, prices_path: Path) -> dict:
    symbols, idx, scores, store = build_inputs(posts_path, prices_path)
    configs = [core.BacktestConfig(holding_bars=h) for h in (1, 2, 3, 5)]
    return deflated_over_configs(symbols, idx, scores, store, configs)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--posts", type=Path, default=_GOLDEN_POSTS)
    ap.add_argument("--prices", type=Path, default=_GOLDEN_PRICES)
    args = ap.parse_args()
    summary = run(args.posts, args.prices)
    print("Backtest summary (net of costs):")
    for k, v in summary.items():
        print(f"  {k}: {v}")
    print("\nN trades reported with the Sharpe; deflated Sharpe accounts for "
          f"{summary.get('n_trials', '?')} configs tried.")


if __name__ == "__main__":
    main()
