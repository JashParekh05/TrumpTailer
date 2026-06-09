"""TrumpTailer: event-driven quant research engine.

C++ core (`trumptailer.core`) + Python ingestion/NLP/research layers.
"""

from trumptailer import _tt_core as core

__version__ = core.__version__

__all__ = ["core"]
