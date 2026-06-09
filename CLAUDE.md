# CLAUDE.md — TrumpTailer

Behavioral guidelines to reduce common LLM coding mistakes, merged with project-specific
instructions. Tradeoff: these guidelines bias toward caution over speed. For trivial
tasks, use judgment.

## 1. Think Before Coding

Don't assume. Don't hide confusion. Surface tradeoffs.

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them. Don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

## 2. Simplicity First

Minimum code that solves the problem. Nothing speculative.

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

## 3. Surgical Changes

Touch only what you must. Clean up only your own mess.

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it. Don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: every changed line should trace directly to the user's request.

## 4. Goal-Driven Execution

Define success criteria. Loop until verified.

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
- [Step] → verify: [check]

Strong success criteria let you loop independently. Weak criteria ("make it work")
require constant clarification.

---

# Project: TrumpTailer

Event-driven quant research engine. Ingests Trump's Truth Social posts + news +
public records, classifies them (NLP), and rigorously measures whether they create
short-lived tradeable edge in US equities/ETFs — event studies, cost-aware backtests,
honest statistics. C++20 core (hot path) + Python (ingestion/NLP/research) via pybind11.

v1 is **backtest-first**: no live trading, no execution code. The engine is event-driven
so the same loop can later consume a live feed ("backtest == live"), but live is
documented-only.

## Build & test

```bash
pip install -e .                  # builds the C++ core via scikit-build-core + CMake
ctest --test-dir build            # C++ unit tests (Catch2)
pytest                            # Python tests
bash scripts/reproduce.sh         # full pipeline: fetch → study → backtest → report
```

## Non-negotiable invariants (methodological honesty)

1. **No look-ahead, ever.**
   - Every fill happens strictly after `event_time + latency`. An after-hours or
     weekend post acts at the *next* session open — never the contemporaneous or
     prior bar.
   - Event-study estimation windows end strictly *before* the event window.
   - `BarStore::bar_asof(symbol, ts)` may only return bars with timestamp ≤ ts.
   - Timestamps are UTC end-to-end; conversion to `America/New_York` market sessions
     happens in exactly one place (TradingCalendar / `research/align.py`).
2. **All headline results are net of transaction costs and latency.** Gross numbers
   may be shown only next to their net counterparts.
3. **Report N (sample size) alongside every t-statistic.** Small N must be visible,
   not hidden.
4. **Count every strategy configuration tried** and feed that N into the deflated
   Sharpe ratio. A negative/marginal deflated Sharpe is a publishable result here,
   not a failure — do not tune until something "works".
5. **Events are selected by the classifier over the full post stream**, never
   hand-picked in hindsight.
