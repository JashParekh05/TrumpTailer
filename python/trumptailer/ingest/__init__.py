"""Ingestion adapters: normalize every source to the `Post` schema."""

from trumptailer.ingest.base import IngestAdapter, Post, posts_to_frame

__all__ = ["IngestAdapter", "Post", "posts_to_frame"]
