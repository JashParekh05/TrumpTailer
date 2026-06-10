"""P6: the report generator runs offline and produces its artifacts."""

from trumptailer.viz.report import build_report


def test_build_report_offline(tmp_path):
    out = build_report(tmp_path)
    assert out["report"].exists()
    assert out["report"].read_text().startswith("# TrumpTailer")
    for key in ("car", "alpha_decay", "equity"):
        assert out[key].exists() and out[key].stat().st_size > 0
