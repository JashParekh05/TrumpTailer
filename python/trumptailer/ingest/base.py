"""The ingestion contract: one normalized record type, one adapter interface.

Every source (Truth Social, news, public records) is reduced to `Post` so the
research layer never sees source-specific shapes.
"""

from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Iterable

import pandas as pd


@dataclass(frozen=True)
class Post:
    source: str            # e.g. "truth_social"
    source_id: str         # stable id within the source
    ts_utc: pd.Timestamp   # tz-aware UTC; the canonical event clock
    text: str
    url: str = ""
    meta: dict = field(default_factory=dict)


class IngestAdapter(ABC):
    """Yields normalized `Post`s. Subclasses set `source` and implement fetch()."""

    source: str = "unknown"

    @abstractmethod
    def fetch(self) -> Iterable[Post]:
        ...

    def to_frame(self) -> pd.DataFrame:
        return posts_to_frame(self.fetch())


def posts_to_frame(posts: Iterable[Post]) -> pd.DataFrame:
    rows = [
        {
            "source": p.source,
            "source_id": p.source_id,
            "ts_utc": p.ts_utc,
            "text": p.text,
            "url": p.url,
            "meta": p.meta,
        }
        for p in posts
    ]
    df = pd.DataFrame(rows, columns=["source", "source_id", "ts_utc", "text", "url", "meta"])
    if not df.empty:
        df["ts_utc"] = pd.to_datetime(df["ts_utc"], utc=True)
        df = df.sort_values("ts_utc", ignore_index=True)
    return df
