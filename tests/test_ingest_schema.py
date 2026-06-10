"""TruthArchiveAdapter normalizes the archive Parquet into the Post schema."""

from pathlib import Path

from trumptailer.ingest.base import Post
from trumptailer.ingest.truth_archive import TruthArchiveAdapter

GOLDEN = Path(__file__).resolve().parents[1] / "data" / "golden" / "truth_sample.parquet"


def test_adapter_normalizes_archive():
    adapter = TruthArchiveAdapter(str(GOLDEN))
    posts = list(adapter.fetch())
    assert len(posts) >= 1
    p = posts[0]
    assert isinstance(p, Post)
    assert p.source == "truth_social"
    assert p.ts_utc.tzinfo is not None  # tz-aware UTC
    assert "favourites_count" in p.meta  # engagement carried to meta


def test_to_frame_is_sorted_and_typed():
    df = TruthArchiveAdapter(str(GOLDEN)).to_frame()
    assert list(df.columns) == ["source", "source_id", "ts_utc", "text", "url", "meta"]
    assert df["ts_utc"].is_monotonic_increasing
    assert str(df["ts_utc"].dt.tz) == "UTC"
