"""Novelty = how different a post is from recent ones (offline, deterministic).

A repeated tariff threat is mostly priced in; a genuinely new statement is not.
We use 1 - max token-Jaccard similarity to the previous `window` posts. The
optional embedding backend (FinBERT/sentence-transformers) can replace this
later behind the same signature.
"""

from __future__ import annotations

import re
from typing import Sequence

_WORD = re.compile(r"[a-z]{3,}")


def _tokens(text: str) -> set[str]:
    return set(_WORD.findall(text.lower()))


def jaccard_novelty(texts: Sequence[str], window: int = 20) -> list[float]:
    token_sets = [_tokens(t) for t in texts]
    out: list[float] = []
    for i, cur in enumerate(token_sets):
        prev = token_sets[max(0, i - window):i]
        if not cur or not prev:
            out.append(1.0)
            continue
        sim = max(
            (len(cur & p) / len(cur | p)) if (cur | p) else 0.0 for p in prev
        )
        out.append(round(1.0 - sim, 6))
    return out
