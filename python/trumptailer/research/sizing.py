"""Position sizing: fractional Kelly (confidence-scaled) and volatility targeting.

Both are point-in-time safe: trailing vol uses only bars strictly before the
event bar. Sizes feed core.sized_equity, which also enforces a drawdown stop.
"""

from __future__ import annotations

import numpy as np
import pandas as pd

from trumptailer import core


def trailing_vol(store, symbol: str, event_idx: int, window: int = 21) -> float:
    """Realized return vol over `window` bars ending strictly before the event."""
    closes = store.close_series(symbol)
    hi = int(event_idx)                       # exclusive of the event bar
    lo = max(1, hi - window)
    seg = closes[lo - 1:hi]
    if len(seg) < 3:
        return float("nan")
    return float(np.std(np.diff(np.log(seg)), ddof=1))


def kelly_confidence_sizes(events: pd.DataFrame, lambda_: float = 0.5, cap: float = 1.0) -> np.ndarray:
    """Fractional Kelly using classifier confidence as the edge proxy."""
    return np.array(
        [core.fractional_kelly(float(c), lambda_, cap) for c in events["confidence"]]
    )


def vol_target_sizes(store, symbols, event_bar_idx, sigma_target: float = 0.02,
                     cap: float = 1.0) -> np.ndarray:
    out = []
    for symbol, idx in zip(symbols, event_bar_idx):
        sigma = trailing_vol(store, symbol, int(idx))
        out.append(core.vol_target_weight(sigma_target, sigma, cap) if sigma == sigma else 0.0)
    return np.array(out)
