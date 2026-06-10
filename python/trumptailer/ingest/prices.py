"""Price bars -> C++ BarStore, with point-in-time-correct completion stamps.

A daily bar dated D becomes available only at D's session *close*, so its
`ts_utc_ns` is stamped to the close (16:00 ET, or 13:00 on early-close days) via
the C++ TradingCalendar. This is what keeps `bar_asof` honest.
"""

from __future__ import annotations

import numpy as np
import pandas as pd


def load_bars_into_store(store, symbol: str, ts_utc_ns, o, h, l, c, v) -> None:
    """Generic numpy bridge: hand contiguous arrays to the C++ store."""
    store.load_symbol(
        symbol,
        np.ascontiguousarray(ts_utc_ns, dtype=np.int64),
        *[np.ascontiguousarray(x, dtype=np.float64) for x in (o, h, l, c, v)],
    )


def daily_frame_to_store(store, calendar, symbol: str, df: pd.DataFrame) -> None:
    """Load a daily OHLCV frame (indexed by date) stamped at each session close."""
    dates = pd.to_datetime(df.index).tz_localize(None).normalize()
    midday_utc = (dates + pd.Timedelta(hours=12)).tz_localize(
        "America/New_York"
    ).tz_convert("UTC")
    close_ns = np.array(
        [calendar.session_close(int(t)) for t in midday_utc.as_unit("ns").asi8],
        dtype=np.int64,
    )
    load_bars_into_store(
        store, symbol, close_ns,
        df["open"], df["high"], df["low"], df["close"], df["volume"],
    )


def fetch_daily_yf(symbol: str, start: str, end: str) -> pd.DataFrame:
    """Download daily bars via yfinance (lazy import; network required)."""
    import yfinance as yf

    df = yf.download(
        symbol, start=start, end=end, interval="1d",
        auto_adjust=True, progress=False,
    )
    if isinstance(df.columns, pd.MultiIndex):
        df.columns = df.columns.get_level_values(0)
    df.columns = [str(col).lower() for col in df.columns]
    return df[["open", "high", "low", "close", "volume"]]
