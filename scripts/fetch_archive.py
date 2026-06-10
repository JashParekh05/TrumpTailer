#!/usr/bin/env python3
"""Fetch the maintained Truth Social archive Parquet into data/raw/."""

from __future__ import annotations

from pathlib import Path

from trumptailer.ingest.truth_archive import DEFAULT_ARCHIVE_URL, TruthArchiveAdapter
from trumptailer.ingest.base import posts_to_frame

_RAW = Path(__file__).resolve().parents[1] / "data" / "raw"


def main() -> None:
    _RAW.mkdir(parents=True, exist_ok=True)
    df = posts_to_frame(TruthArchiveAdapter(DEFAULT_ARCHIVE_URL).fetch())
    out = _RAW / "truth_posts.parquet"
    df.to_parquet(out, index=False)
    print(f"wrote {len(df)} posts -> {out}")


if __name__ == "__main__":
    main()
