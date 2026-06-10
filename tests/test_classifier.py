"""P3: classifier behaviour + the classifier->event-study reproduction."""

from pathlib import Path

import numpy as np
import pandas as pd

from trumptailer import core
from trumptailer.ingest.prices import daily_frame_to_store
from trumptailer.ingest.truth_archive import TruthArchiveAdapter
from trumptailer.nlp.calibration import brier_score, prob_up
from trumptailer.nlp.classifier import (
    classify_category,
    classify_posts,
    lexicon_direction,
    load_events_config,
)
from trumptailer.nlp.novelty import jaccard_novelty
from trumptailer.nlp.schema import events_to_frame
from trumptailer.research.align import align_posts, build_event_locations
from trumptailer.research.universe import load_universe

_DATA = Path(__file__).resolve().parents[1] / "data" / "golden"


def test_category_rules():
    cfg = load_events_config()
    assert classify_category("Massive TARIFFS on China!", cfg) == "TariffTrade"
    assert classify_category("Powell must cut the interest rate", cfg) == "FedRates"
    assert classify_category("Great round of golf today", cfg) is None  # non-event


def test_direction_lexicon_sign():
    cfg = load_events_config()
    d_bear, _ = lexicon_direction("New tariffs, retaliation, recession risk", cfg)
    d_bull, _ = lexicon_direction("Record growth, historic deal, booming economy", cfg)
    assert d_bear == -1
    assert d_bull == 1


def test_novelty_drops_for_repeats():
    nov = jaccard_novelty(["tariffs on china now", "tariffs on china now", "totally unrelated weather"])
    assert nov[0] == 1.0          # nothing prior
    assert nov[1] < 0.2           # exact repeat -> low novelty
    assert nov[2] > 0.8           # distinct -> high novelty


def test_brier_rewards_calibration():
    direction = np.array([1, 1, -1, -1])
    confidence = np.array([0.8, 0.8, 0.8, 0.8])
    p = prob_up(direction, confidence)              # [.9,.9,.1,.1]
    good = brier_score(p, np.array([1, 1, 0, 0]))   # confident & correct
    bad = brier_score(p, np.array([0, 0, 1, 1]))    # confident & wrong
    assert good < bad


def test_selection_drops_non_events():
    universe = load_universe()
    posts = TruthArchiveAdapter(str(_DATA / "truth_sample.parquet")).to_frame()
    events = events_to_frame(classify_posts(posts, universe))
    # The 'Mar-a-Lago weekend' post matches no category and must be excluded.
    assert "113" not in set(events["source_id"])
    assert set(events["category"]) <= {"TariffTrade", "FedRates", "SingleName"}


def test_classifier_events_reproduce_negative_tariff_car():
    """Classifier-selected tariff events reproduce P2's negative abnormal CAR."""
    universe = load_universe()
    cal = core.TradingCalendar()

    prices = pd.read_parquet(_DATA / "prices_daily.parquet")
    store = core.BarStore()
    for symbol, g in prices.groupby("symbol"):
        daily_frame_to_store(store, cal, symbol, g.set_index("date").sort_index())

    posts = TruthArchiveAdapter(str(_DATA / "truth_sample.parquet")).to_frame()
    events = events_to_frame(classify_posts(posts, universe))
    tariff = align_posts(events[events["category"] == "TariffTrade"], cal)

    symbols, idx = build_event_locations(tariff, store)
    cfg = core.EventStudyConfig(est_start=-60, est_end=-5, car_start=0, car_end=3)
    r = core.event_study(symbols, idx, store, universe.benchmark, cfg)

    assert r.n >= 1
    assert r.caar_total < 0.0  # tariff posts -> negative abnormal return, as in P2
