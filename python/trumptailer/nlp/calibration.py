"""Calibration of directional forecasts: is a confident 'up' actually up?

`prob_up` maps a (direction, confidence) call to P(up). Brier score and a
reliability table then measure whether those probabilities are honest.
"""

from __future__ import annotations

import numpy as np
import pandas as pd


def prob_up(direction: np.ndarray, confidence: np.ndarray) -> np.ndarray:
    """Map signed direction * confidence to a probability in [0, 1]."""
    return 0.5 + 0.5 * np.asarray(direction, float) * np.asarray(confidence, float)


def brier_score(prob_up_pred: np.ndarray, realized_up: np.ndarray) -> float:
    p = np.asarray(prob_up_pred, float)
    y = np.asarray(realized_up, float)
    return float(np.mean((p - y) ** 2))


def reliability_table(prob_up_pred: np.ndarray, realized_up: np.ndarray, bins: int = 5) -> pd.DataFrame:
    p = np.asarray(prob_up_pred, float)
    y = np.asarray(realized_up, float)
    edges = np.linspace(0.0, 1.0, bins + 1)
    idx = np.clip(np.digitize(p, edges[1:-1]), 0, bins - 1)
    rows = []
    for b in range(bins):
        m = idx == b
        if not m.any():
            continue
        rows.append({"bin": b, "n": int(m.sum()),
                     "mean_pred": float(p[m].mean()), "frac_up": float(y[m].mean())})
    return pd.DataFrame(rows)
