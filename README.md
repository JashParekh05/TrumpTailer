# TrumpTailer

**Does Trump's Truth Social activity create short-lived, tradeable edge in US
equities — and does it survive transaction costs?**

An event-driven quant research engine. Python ingests posts/news/public records and
classifies them with NLP; a C++20 core (via pybind11) runs the event studies,
cost-aware event-driven backtests, position sizing, and statistics needed to answer
the question honestly — including deflated Sharpe ratios and strict no-look-ahead
discipline.

> v1 is research-only: no live trading. The engine is event-driven so the same loop
> can later consume a live feed, but execution is out of scope.

## Quick start

```bash
pip install -e .[dev]      # builds the C++ core (CMake + pybind11 via scikit-build-core)
pytest                     # Python tests
make test-cpp              # C++ unit tests (Catch2)
```

*(Full README — methodology, results, and reproduction — lands with the v1 report.)*
