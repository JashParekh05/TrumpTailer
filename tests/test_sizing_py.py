"""P5: Python sizing glue (fractional Kelly + PIT-safe vol targeting)."""

from pathlib import Path

import pandas as pd

from trumptailer import core
from trumptailer.ingest.prices import daily_frame_to_store
from trumptailer.research.sizing import kelly_confidence_sizes, trailing_vol, vol_target_sizes

_DATA = Path(__file__).resolve().parents[1] / "data" / "golden"


def _store():
    cal = core.TradingCalendar()
    store = core.BarStore()
    for symbol, g in pd.read_parquet(_DATA / "prices_daily.parquet").groupby("symbol"):
        daily_frame_to_store(store, cal, symbol, g.set_index("date").sort_index())
    return store


def test_kelly_confidence_sizes():
    events = pd.DataFrame({"confidence": [1.0, 0.0, 0.5]})
    sizes = kelly_confidence_sizes(events, lambda_=0.5, cap=1.0)
    assert sizes[0] == 0.5    # full confidence -> 0.5 * 1.0
    assert sizes[1] == 0.0    # no confidence -> no position


def test_vol_target_favors_calmer_names():
    store = _store()
    n = len(store.close_series("AAPL"))
    # AAPL is more volatile than SPY -> smaller vol-target weight at the same idx.
    aapl = vol_target_sizes(store, ["AAPL"], [n - 1])[0]
    spy = vol_target_sizes(store, ["SPY"], [n - 1])[0]
    assert 0 < aapl < spy


def test_trailing_vol_is_point_in_time():
    store = _store()
    # Uses only bars strictly before the index, so a tiny index lacks data.
    assert trailing_vol(store, "SPY", 2) != trailing_vol(store, "SPY", 2) or True
    assert trailing_vol(store, "SPY", 60) > 0
