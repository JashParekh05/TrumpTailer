"""Optional FinBERT direction backend (dormant unless torch+transformers exist).

Keeping this behind a lazy import means the pipeline, tests, and CI run fully
offline with the deterministic lexicon backend; FinBERT only activates when the
[nlp] extra is installed and the model is available locally.
"""

from __future__ import annotations

from typing import Callable

_MODEL = "ProsusAI/finbert"


def is_available() -> bool:
    try:
        import torch  # noqa: F401
        import transformers  # noqa: F401
    except ImportError:
        return False
    return True


def make_backend() -> Callable[[str], tuple[int, float]]:
    """Return a (text) -> (direction, confidence) callable backed by FinBERT.

    Raises RuntimeError if the optional dependencies are not installed.
    """
    if not is_available():
        raise RuntimeError("FinBERT backend requires the [nlp] extra (torch+transformers)")

    from transformers import pipeline

    clf = pipeline("text-classification", model=_MODEL, top_k=None)
    label_to_dir = {"positive": 1, "negative": -1, "neutral": 0}

    def backend(text: str) -> tuple[int, float]:
        scores = {d["label"].lower(): d["score"] for d in clf(text[:512])[0]}
        best = max(scores, key=scores.get)
        return label_to_dir.get(best, 0), float(scores[best])

    return backend
