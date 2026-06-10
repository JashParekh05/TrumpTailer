"""Golden reproduction: the April 2025 'Liberation Day' tariff shock.

Apple is heavily China-exposed, so the tariff announcement (2025-04-02, after
close) drove it down well beyond its ~0.9 beta to SPY. The market-model event
study must recover that large negative abnormal CAR — locking the engine + the
point-in-time plumbing against regressions. (n=1: reported, t undefined.)
"""

from pathlib import Path

import pandas as pd

from trumptailer import core
from trumptailer.ingest.prices import daily_frame_to_store

GOLDEN = Path(__file__).resolve().parents[1] / "data" / "golden" / "prices_daily.parquet"


def _load_store():
    df = pd.read_parquet(GOLDEN)
    cal = core.TradingCalendar()
    store = core.BarStore()
    for symbol, g in df.groupby("symbol"):
        g = g.set_index("date").sort_index()
        daily_frame_to_store(store, cal, symbol, g)
    return store, cal


def test_liberation_day_aapl_abnormal_crash():
    store, cal = _load_store()

    # Tariffs announced 2025-04-02 ~17:00 ET (after close) -> acts next open.
    post_ns = pd.Timestamp("2025-04-02T21:00:00Z").as_unit("ns").value
    actionable = cal.actionable_time(int(post_ns))
    idx = store.index_asof("AAPL", actionable) + 1  # event-day (Apr 3) close bar

    cfg = core.EventStudyConfig(est_start=-60, est_end=-5, car_start=0, car_end=3)
    r = core.event_study(["AAPL"], [idx], store, "SPY", cfg)

    assert r.n == 1
    assert r.aar[0] < -0.03            # Apr 3 abnormal return sharply negative (~-5%)
    assert r.caar_total < -0.10        # 4-day abnormal CAR ~ -14%, matches documented crash
    # tau=0 entry is strictly after the post (no look-ahead)
    assert actionable > post_ns
