"""Orchestrate classification: category (rules) + direction + novelty + routing.

Selection happens here over the *full* post stream: posts matching no event
category are dropped, never hand-picked in hindsight (a core invariant). The
direction backend defaults to the offline lexicon; pass a FinBERT backend to
override just the direction call.
"""

from __future__ import annotations

from pathlib import Path
from typing import Callable, Optional

import pandas as pd
import yaml

from trumptailer.nlp.entity_map import detect_companies, route_instruments
from trumptailer.nlp.novelty import jaccard_novelty
from trumptailer.nlp.schema import ClassifiedEvent
from trumptailer.research.universe import Universe

_DEFAULT_CFG = Path(__file__).resolve().parents[3] / "config" / "events.yaml"


def load_events_config(path: str | Path = _DEFAULT_CFG) -> dict:
    return yaml.safe_load(Path(path).read_text())


def classify_category(text: str, cfg: dict) -> Optional[str]:
    low = text.lower()
    for category, keywords in cfg["categories"].items():
        if any(k in low for k in keywords):
            return category
    if detect_companies(text):
        return "SingleName"
    return None


def lexicon_direction(text: str, cfg: dict) -> tuple[int, float]:
    low = text.lower()
    bull = sum(1 for k in cfg["direction"]["bullish"] if k in low)
    bear = sum(1 for k in cfg["direction"]["bearish"] if k in low)
    if bull == bear:
        return 0, 0.0
    score = (bull - bear) / (bull + bear)
    return (1 if score > 0 else -1), abs(score)


def classify_posts(
    posts: pd.DataFrame,
    universe: Universe,
    cfg: Optional[dict] = None,
    direction_backend: Optional[Callable[[str], tuple[int, float]]] = None,
) -> list[ClassifiedEvent]:
    cfg = cfg or load_events_config()
    novelties = jaccard_novelty(posts["text"].astype(str).tolist())

    events: list[ClassifiedEvent] = []
    for (_, row), novelty in zip(posts.iterrows(), novelties):
        text = str(row["text"])
        category = classify_category(text, cfg)
        if category is None:
            continue  # not an event of interest
        if direction_backend is not None:
            direction, confidence = direction_backend(text)
        else:
            direction, confidence = lexicon_direction(text, cfg)
        magnitude = max(0.0, min(1.0, confidence * novelty))
        for instrument in route_instruments(text, category, universe):
            events.append(
                ClassifiedEvent(
                    source_id=str(row["source_id"]),
                    ts_utc=row["ts_utc"],
                    category=category,
                    direction=direction,
                    magnitude=magnitude,
                    novelty=novelty,
                    confidence=confidence,
                    instrument=instrument,
                    text=text,
                )
            )
    return events
