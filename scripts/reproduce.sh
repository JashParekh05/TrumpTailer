#!/usr/bin/env bash
# One-command pipeline: build -> (optional fetch) -> research + backtest -> report.
# Runs offline on the committed golden fixtures by default.
set -euo pipefail
cd "$(dirname "$0")/.."

echo "[1/3] building C++ core (pip install -e .)"
pip install -e . >/dev/null

if [[ "${TT_FETCH:-0}" == "1" ]]; then
  echo "[2/3] fetching real data (TT_FETCH=1)"
  python3 scripts/fetch_archive.py
  python3 scripts/fetch_prices.py
  POSTS=data/raw/truth_posts.parquet
  PRICES=data/raw/prices_daily.parquet
else
  echo "[2/3] using committed golden fixtures (set TT_FETCH=1 for the full sample)"
  POSTS=data/golden/truth_sample.parquet
  PRICES=data/golden/prices_daily.parquet
fi

echo "[3/3] generating report"
python3 -m trumptailer.viz.report --posts "$POSTS" --prices "$PRICES"
echo "done -> reports/report.md"
