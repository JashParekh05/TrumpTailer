"""Matplotlib figures for the report. Headless (Agg); each returns its path."""

from __future__ import annotations

from pathlib import Path

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402
import numpy as np  # noqa: E402


def plot_car(result, title: str, path: Path) -> Path:
    """Cumulative abnormal return vs event-time offset (the 'is there signal' figure)."""
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.axhline(0, color="k", lw=0.8)
    ax.plot(result.offsets, np.array(result.caar) * 100, marker="o", color="#b22222")
    ax.set_xlabel("event-time offset (trading days)")
    ax.set_ylabel("cumulative abnormal return (%)")
    ax.set_title(f"{title}  (N={result.n})")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


def plot_alpha_decay(result, path: Path) -> Path:
    """Per-offset average abnormal return (AAR): where the move happens, then dies."""
    fig, ax = plt.subplots(figsize=(7, 4))
    ax.axhline(0, color="k", lw=0.8)
    ax.bar(result.offsets, np.array(result.aar) * 100, color="#4169e1", alpha=0.8)
    ax.set_xlabel("event-time offset (trading days)")
    ax.set_ylabel("average abnormal return (%)")
    ax.set_title(f"Alpha decay: per-day abnormal return  (N={result.n})")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path


def plot_equity(result, path: Path) -> Path:
    """Net-of-cost equity curve with drawdown shading."""
    eq = np.array(result.equity)
    fig, ax = plt.subplots(figsize=(7, 4))
    if len(eq):
        x = np.arange(len(eq))
        peak = np.maximum.accumulate(eq)
        ax.plot(x, eq, color="#2e8b57", label="net equity")
        ax.fill_between(x, eq, peak, color="#cd5c5c", alpha=0.25, label="drawdown")
        ax.legend()
    ax.set_xlabel("trade #")
    ax.set_ylabel("equity (start = 1.0)")
    ax.set_title(f"Backtest equity, net of costs  (net Sharpe={result.net_sharpe:.2f}, "
                 f"N={result.n})")
    ax.grid(alpha=0.3)
    fig.tight_layout()
    fig.savefig(path, dpi=120)
    plt.close(fig)
    return path
