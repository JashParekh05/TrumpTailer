"""The classification contract: one ClassifiedEvent per (post, routed instrument)."""

from __future__ import annotations

from dataclasses import dataclass

import pandas as pd


@dataclass(frozen=True)
class ClassifiedEvent:
    source_id: str
    ts_utc: pd.Timestamp   # tz-aware UTC (the event clock)
    category: str          # TariffTrade / FedRates / Geopolitical / SingleName
    direction: int         # -1 bearish, 0 neutral, +1 bullish
    magnitude: float       # [0, 1]
    novelty: float         # [0, 1]
    confidence: float      # [0, 1]
    instrument: str        # resolved ticker
    text: str = ""


def events_to_frame(events: list[ClassifiedEvent]) -> pd.DataFrame:
    """Flatten to a frame the research layer can align + study (one row per event)."""
    cols = ["source_id", "ts_utc", "category", "direction", "magnitude",
            "novelty", "confidence", "instrument", "text"]
    df = pd.DataFrame([e.__dict__ for e in events], columns=cols)
    if not df.empty:
        df["ts_utc"] = pd.to_datetime(df["ts_utc"], utc=True)
    return df
