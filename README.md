# TrumpTailer

**Does Donald Trump's Truth Social activity create short-lived, tradeable edge in
US equities — and does it survive transaction costs?**

An event-driven quantitative research engine that ingests Trump's posts (plus news
and public records), classifies them with NLP, and rigorously measures the market
reaction with **event studies, cost-aware backtests, and honest statistics** —
including a deflated Sharpe ratio and strict no-look-ahead discipline.

> **C++20 core (hot path) + Python (ingestion / NLP / research), bridged by pybind11.**
> v1 is research-only — no live trading. The engine is event-driven, so the same
> loop could later consume a live feed ("backtest == live"); execution is out of scope.

---

## Headline result (reproducible)

On the committed golden fixture — the April 2025 *"Liberation Day"* tariff shock —
the engine recovers, through a pipeline that **cannot peek at future bars**:

- **Tariff posts → Apple's abnormal return vs SPY: CAAR ≈ −14% over the event window**
  (Apple is heavily China-exposed, so it fell far beyond its ~0.91 beta to the market).
- Entry is taken **strictly after** the after-close announcement — never the
  contemporaneous bar.

```bash
bash scripts/reproduce.sh      # build → research → backtest → reports/report.md
```

This regenerates the three figures that tell the story: the **CAR curve**, the
**per-day alpha-decay**, and the **net-of-cost equity curve**.

---

## Where the quant lives

| Layer | What it does | Implementation |
|---|---|---|
| **Event study** | Market-model abnormal returns (α/β), CAAR, cross-sectional t-stats — *is there signal?* | C++ (`event_study`) |
| **Deflated Sharpe** | Probabilistic Sharpe + correction for the number of strategy configs tried — *is it real or data-mined?* | C++ (`metrics`) |
| **Kelly sizing** | Fractional Kelly, vol-targeting, correlation-aware portfolio vol, drawdown kill-switch | C++ (`sizing`) |
| **Cost-aware backtest** | Per-event entry/exit net of half-spread + commission + √-impact slippage | C++ (`backtester`) |
| **No-look-ahead gate** | UTC→NYSE session alignment; after-hours/weekend posts act at the *next* open | C++ (`clock`) + `research/align.py` |
| **NLP classification** | Category + direction + novelty over the *full* post stream (no hindsight selection) | Python (`nlp/`) |

The systems language earns its place in the replay/stat hot path; the alpha is in
the **modeling, sizing, and honest measurement** — which is exactly the point.

## Non-negotiable invariants (see `CLAUDE.md`)

1. **No look-ahead, ever.** Every fill is strictly after `event_time + latency`;
   estimation windows end strictly before the event; `bar_asof` only returns bars
   with `ts ≤ query`. Timestamps are UTC end-to-end.
2. **All headline results are net of costs and latency.**
3. **Report N alongside every t-statistic.** Small N stays visible.
4. **Count every configuration tried** and feed it into the deflated Sharpe. A
   marginal/negative deflated Sharpe is a *publishable* result, not a failure.
5. **Events are selected by the classifier over the full stream**, never hand-picked.

## Build & test

```bash
pip install -e .            # builds the C++ core via scikit-build-core + CMake
python -m pytest            # Python tests
make test-cpp               # C++ unit tests (Catch2 via ctest)
```

C++ dependencies are deliberately tiny: **pybind11 + Catch2 only**. Parquet/CSV are
read in Python (`pyarrow`) and handed to C++ as numpy arrays; the statistics
(OLS market model, t-stats, normal CDF/PPF, deflated Sharpe, Kelly) are hand-rolled.

## NLP backend

The classifier is **offline-first**: an auditable lexicon/rules backend runs in CI
with zero downloads and full determinism. **FinBERT** (`ProsusAI/finbert`) is wired
as an optional direction backend that activates only when the `[nlp]` extra
(`pip install -e .[nlp]`) is installed — keeping the core pipeline reproducible.

## Data sourcing (honest)

Truth Social has no official API. v1 research is backed by the maintained public
archive Parquet; the live route (`truthbrush` / commercial streams) sits behind the
same `IngestAdapter` interface, built but dormant. Prices via `yfinance` (daily for
full history; minute bars only recently — so alpha-decay at fine resolution is
framed honestly). Fetch the full sample with `TT_FETCH=1 bash scripts/reproduce.sh`.

## Layout

```
cpp/          C++20 core (clock, bar_store, event_study, backtester, sizing, metrics) + pybind11
python/trumptailer/
  ingest/     Truth Social archive + price adapters (normalized Post schema)
  nlp/        classifier (category/direction/novelty), optional FinBERT, calibration
  research/   align (the PIT chokepoint), universe, event-study/backtest drivers, sizing
  viz/        plots + the report generator
data/golden/  tiny committed fixtures so tests + reproduce.sh run offline
scripts/      fetch_archive.py, fetch_prices.py, reproduce.sh
```

## Roadmap (documented only)

Swap the archive adapter for a live feed and add an execution venue behind the cost
model — the event-driven loop runs unchanged. Not implemented in v1 by design.
