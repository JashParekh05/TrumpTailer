"""The point-in-time layer: map each post to its first *actionable* bar.

This is the single chokepoint where event time meets market time. Everything
downstream trades off `actionable_ts_ns`, never the raw post time, so an
after-hours or weekend post can only act at the next regular open.
"""

from __future__ import annotations

import numpy as np
import pandas as pd


def align_posts(posts: pd.DataFrame, calendar) -> pd.DataFrame:
    """Add session_type + actionable timestamp columns; assert no look-ahead.

    `posts` must have a tz-aware UTC `ts_utc` column. Returns a copy with
    `ts_utc_ns`, `session_type` (name), `actionable_ts_ns`, `actionable_ts`.
    """
    df = posts.copy()
    ns = pd.DatetimeIndex(df["ts_utc"]).as_unit("ns").asi8  # force ns (pandas may default to us)
    df["ts_utc_ns"] = ns
    df["session_type"] = [calendar.classify(int(t)).name for t in ns]
    actionable = np.array(
        [calendar.actionable_time(int(t)) for t in ns], dtype=np.int64
    )
    if (actionable < ns).any():
        raise AssertionError("look-ahead: an actionable time precedes its event")
    df["actionable_ts_ns"] = actionable
    df["actionable_ts"] = pd.to_datetime(actionable, utc=True)
    return df


def session_histogram(aligned: pd.DataFrame) -> pd.Series:
    """Count posts by session type (the P1 sanity check)."""
    return aligned["session_type"].value_counts()


def event_bar_index(store, symbol: str, actionable_ts_ns: int) -> int:
    """The tau=0 bar for an event: the first bar completing at/after entry.

    Bars are stamped at their session close and entry is the next open, so the
    as-of bar is the prior close; the event day's bar is the next one (+1).
    """
    return store.index_asof(symbol, int(actionable_ts_ns)) + 1


def build_event_locations(aligned: pd.DataFrame, store, symbol_col: str = "instrument"):
    """Map aligned events to (symbols, tau=0 bar indices) for EventStudy.run."""
    symbols = aligned[symbol_col].tolist()
    indices = [
        event_bar_index(store, s, ts)
        for s, ts in zip(symbols, aligned["actionable_ts_ns"])
    ]
    return symbols, indices
