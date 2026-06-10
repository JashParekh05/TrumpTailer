"""Truth Social archive adapter — the primary v1 (backtest) source.

Truth Social has no official API. This reads the maintained public archive
Parquet (CNN mirror, ~5-min updates). For offline tests/reproducibility, pass a
local path to a committed snapshot. The live route (`truthbrush_live`) is a
separate adapter behind the same interface.
"""

from __future__ import annotations

from typing import Iterable

import pandas as pd

from trumptailer.ingest.base import IngestAdapter, Post

DEFAULT_ARCHIVE_URL = "https://ix.cnn.io/data/truth-social/truth_archive.parquet"

# Engagement fields carried through to meta (used later as a magnitude proxy).
_ENGAGEMENT = ["replies_count", "reblogs_count", "favourites_count"]


class TruthArchiveAdapter(IngestAdapter):
    source = "truth_social"

    def __init__(self, path_or_url: str | None = None):
        self.path = path_or_url or DEFAULT_ARCHIVE_URL

    def fetch(self) -> Iterable[Post]:
        df = pd.read_parquet(self.path)
        created = pd.to_datetime(df["created_at"], utc=True)
        for i, row in df.reset_index(drop=True).iterrows():
            meta = {k: row[k] for k in _ENGAGEMENT if k in df.columns}
            yield Post(
                source=self.source,
                source_id=str(row["id"]),
                ts_utc=created.iloc[i],
                text=str(row.get("content", "")),
                url=str(row.get("url", "")),
                meta=meta,
            )
