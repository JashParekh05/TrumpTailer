"""NLP layer: classify posts into events (category, direction, magnitude, novelty)."""

from trumptailer.nlp.schema import ClassifiedEvent, events_to_frame
from trumptailer.nlp.classifier import classify_posts, load_events_config

__all__ = ["ClassifiedEvent", "events_to_frame", "classify_posts", "load_events_config"]
