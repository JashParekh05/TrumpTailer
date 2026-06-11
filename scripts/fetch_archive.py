#!/usr/bin/env python3
"""Fetch the maintained Truth Social archive Parquet into data/raw/.

Saved in the archive's native schema (like the committed golden fixture) so the
research pipeline normalizes it through TruthArchiveAdapter exactly as it does
the fixture — keeping the offline and full-sample paths identical.
"""

from __future__ import annotations

from pathlib import Path

import pandas as pd

from trumptailer.ingest.truth_archive import DEFAULT_ARCHIVE_URL

_RAW = Path(__file__).resolve().parents[1] / "data" / "raw"


def main() -> None:
    _RAW.mkdir(parents=True, exist_ok=True)
    df = pd.read_parquet(DEFAULT_ARCHIVE_URL)
    out = _RAW / "truth_posts.parquet"
    df.to_parquet(out, index=False)
    print(f"wrote {len(df)} posts -> {out}")


if __name__ == "__main__":
    main()
