"""No-look-ahead at the Python boundary (mirrors the C++ clock PIT test)."""

import pandas as pd

from trumptailer import core
from trumptailer.research.align import align_posts, session_histogram


def _posts() -> pd.DataFrame:
    ts = pd.to_datetime(
        [
            "2025-04-02T18:00:00Z",  # Wed 14:00 ET -> regular hours
            "2025-04-03T00:00:00Z",  # Wed 20:00 ET -> after hours
            "2025-04-05T18:00:00Z",  # Saturday    -> closed
            "2025-04-02T06:00:00Z",  # Wed 02:00 ET -> pre-market
        ],
        utc=True,
    )
    return pd.DataFrame(
        {
            "source": "truth_social",
            "source_id": ["1", "2", "3", "4"],
            "ts_utc": ts,
            "text": ["a", "b", "c", "d"],
            "url": "",
        }
    )


def test_actionable_never_precedes_post():
    out = align_posts(_posts(), core.TradingCalendar())
    assert (out["actionable_ts_ns"] >= out["ts_utc_ns"]).all()


def test_weekend_maps_to_next_regular_open():
    cal = core.TradingCalendar()
    out = align_posts(_posts(), cal)
    sat = out[out["ts_utc"] == pd.Timestamp("2025-04-05T18:00:00Z")].iloc[0]
    assert sat["session_type"] == "Closed"
    assert cal.classify(int(sat["actionable_ts_ns"])) == core.SessionType.RegularHours
    monday = pd.Timestamp(int(sat["actionable_ts_ns"]), tz="UTC").tz_convert(
        "America/New_York"
    )
    assert monday.strftime("%Y-%m-%d") == "2025-04-07"  # next trading day


def test_after_hours_acts_after_that_days_close():
    cal = core.TradingCalendar()
    out = align_posts(_posts(), cal)
    ah = out[out["ts_utc"] == pd.Timestamp("2025-04-03T00:00:00Z")].iloc[0]
    assert ah["session_type"] == "AfterHours"
    assert int(ah["actionable_ts_ns"]) > cal.session_close(int(ah["ts_utc_ns"]))


def test_session_histogram_totals():
    out = align_posts(_posts(), core.TradingCalendar())
    assert session_histogram(out).sum() == 4
