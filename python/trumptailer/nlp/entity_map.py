"""Resolve a post to instruments: named companies first, else topic routing.

A curated name->ticker map is more accurate than NER for a known universe and
trivially auditable. If no company is named, route by the event category via
the universe's sector_map.
"""

from __future__ import annotations

from trumptailer.research.universe import Universe

# Curated company / nickname -> ticker. Extend as the universe grows.
COMPANIES = {
    "apple": "AAPL",
    "tim cook": "AAPL",
    "iphone": "AAPL",
    "tesla": "TSLA",
    "elon": "TSLA",
    "nvidia": "NVDA",
    "jensen": "NVDA",
}


def detect_companies(text: str) -> list[str]:
    low = text.lower()
    hits = [ticker for name, ticker in COMPANIES.items() if name in low]
    return list(dict.fromkeys(hits))  # de-dup, keep order


def route_instruments(text: str, category: str, universe: Universe) -> list[str]:
    named = detect_companies(text)
    if named:
        return named
    return list(universe.sector_map.get(category, [universe.benchmark]))
