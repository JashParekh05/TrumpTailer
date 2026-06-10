"""Driver: posts -> (rule-labeled) events -> market-model event study by category.

Rule labeling is a deliberate P2 placeholder; the NLP classifier (P3) replaces
`label_posts` behind the same (category, instrument) output. Defaults to the
committed golden fixtures so it runs offline.
"""

from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd

from trumptailer import core
from trumptailer.ingest.prices import daily_frame_to_store
from trumptailer.ingest.truth_archive import TruthArchiveAdapter
from trumptailer.research.align import align_posts, build_event_locations
from trumptailer.research.universe import Universe, load_universe

_ROOT = Path(__file__).resolve().parents[3]
_GOLDEN_POSTS = _ROOT / "data" / "golden" / "truth_sample.parquet"
_GOLDEN_PRICES = _ROOT / "data" / "golden" / "prices_daily.parquet"

# Coarse keyword rules (P2). One labeled row per (post, routed instrument).
_RULES = {
    "TariffTrade": ["tariff", "china", "trade deficit", "import", "liberation day"],
    "FedRates": ["fed", "powell", "rate", "interest"],
}


def label_posts(posts: pd.DataFrame, universe: Universe) -> pd.DataFrame:
    rows = []
    for _, p in posts.iterrows():
        text = str(p["text"]).lower()
        for category, keywords in _RULES.items():
            if any(k in text for k in keywords):
                for symbol in universe.sector_map.get(category, [universe.benchmark]):
                    rows.append({**p.to_dict(), "category": category, "instrument": symbol})
                break
    return pd.DataFrame(rows)


def load_price_store(prices_path: Path, calendar) -> core.BarStore:
    df = pd.read_parquet(prices_path)
    store = core.BarStore()
    for symbol, g in df.groupby("symbol"):
        daily_frame_to_store(store, calendar, symbol, g.set_index("date").sort_index())
    return store


def run(posts_path: Path, prices_path: Path, cfg: core.EventStudyConfig) -> pd.DataFrame:
    universe = load_universe()
    calendar = core.TradingCalendar()
    store = load_price_store(prices_path, calendar)

    posts = TruthArchiveAdapter(str(posts_path)).to_frame()
    labeled = align_posts(label_posts(posts, universe), calendar)

    out = []
    for category, grp in labeled.groupby("category"):
        symbols, idx = build_event_locations(grp, store)
        r = core.event_study(symbols, idx, store, universe.benchmark, cfg)
        out.append(
            {"category": category, "n": r.n, "caar": round(r.caar_total, 4),
             "t_caar": round(r.t_caar, 2)}
        )
    return pd.DataFrame(out)


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--posts", type=Path, default=_GOLDEN_POSTS)
    ap.add_argument("--prices", type=Path, default=_GOLDEN_PRICES)
    ap.add_argument("--car-end", type=int, default=3)
    args = ap.parse_args()
    cfg = core.EventStudyConfig(est_start=-60, est_end=-5, car_start=0, car_end=args.car_end)
    result = run(args.posts, args.prices, cfg)
    print(result.to_string(index=False))
    print("\n(N reported with every t-stat; small N is expected on the golden fixture.)")


if __name__ == "__main__":
    main()
