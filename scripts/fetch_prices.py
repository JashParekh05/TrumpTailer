#!/usr/bin/env python3
"""Fetch daily OHLCV for the universe into data/raw/prices_daily.parquet."""

from __future__ import annotations

import argparse
from pathlib import Path

import pandas as pd

from trumptailer.ingest.prices import fetch_daily_yf
from trumptailer.research.universe import load_universe

_RAW = Path(__file__).resolve().parents[1] / "data" / "raw"


def main() -> None:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--start", default="2021-01-01")
    ap.add_argument("--end", default="2026-06-01")
    args = ap.parse_args()

    universe = load_universe()
    frames = []
    for symbol in universe.instruments:
        df = fetch_daily_yf(symbol, args.start, args.end).reset_index()
        df.columns = [str(c).lower() for c in df.columns]
        df = df.rename(columns={df.columns[0]: "date"})
        df["date"] = pd.to_datetime(df["date"]).dt.tz_localize(None).dt.normalize()
        df["symbol"] = symbol
        frames.append(df[["date", "open", "high", "low", "close", "volume", "symbol"]])

    _RAW.mkdir(parents=True, exist_ok=True)
    out = pd.concat(frames, ignore_index=True)
    out.to_parquet(_RAW / "prices_daily.parquet", index=False)
    print(f"wrote {len(out)} rows for {out.symbol.nunique()} symbols -> {_RAW / 'prices_daily.parquet'}")


if __name__ == "__main__":
    main()
