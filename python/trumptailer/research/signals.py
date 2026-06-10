"""Turn classified events into backtester scores.

The baseline uses the classified direction (sign decides the trade; sizing is
P5). A trained model would expose the same shape: a float score per event whose
sign is the position direction.
"""

from __future__ import annotations

import numpy as np
import pandas as pd


def baseline_scores(events: pd.DataFrame) -> np.ndarray:
    """Score = signed direction, scaled by confidence so neutral events drop out."""
    return (events["direction"].astype(float)
            * (0.25 + 0.75 * events["confidence"].astype(float))).to_numpy()
