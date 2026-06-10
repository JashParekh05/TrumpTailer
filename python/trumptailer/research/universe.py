"""Load the tradeable universe config (benchmark, instruments, topic routing)."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path

import yaml

_DEFAULT = Path(__file__).resolve().parents[3] / "config" / "universe.yaml"


@dataclass(frozen=True)
class Universe:
    benchmark: str
    instruments: list[str]
    sector_map: dict[str, list[str]]


def load_universe(path: str | Path = _DEFAULT) -> Universe:
    cfg = yaml.safe_load(Path(path).read_text())
    return Universe(
        benchmark=cfg["benchmark"],
        instruments=list(cfg["instruments"]),
        sector_map={k: list(v) for k, v in cfg.get("sector_map", {}).items()},
    )
