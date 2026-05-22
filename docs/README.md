# Documentation

This directory keeps the current project documentation plus historical validation records. For current engineering decisions, prefer the documents listed below over older per-day notes.

## Read First

1. [design/ARCHITECTURE.md](design/ARCHITECTURE.md) - current source-level architecture.
2. [guides/PLAYER_FEATURES_USAGE_VALIDATION.md](guides/PLAYER_FEATURES_USAGE_VALIDATION.md) - user-facing features, CLI checks and validation commands.
3. [plans/CROSS_PLATFORM_MASTER_TASKLIST.md](plans/CROSS_PLATFORM_MASTER_TASKLIST.md) - current status and remaining TODOs.
4. [reports/README.md](reports/README.md) - validation report index.
5. [records/VERSION.md](records/VERSION.md) - version and release notes.

## Directory Purpose

| Directory | Purpose |
| --- | --- |
| `guides/` | Setup, usage and feature validation. |
| `design/` | Current architecture and focused design decisions. |
| `plans/` | Current roadmap and remaining work. |
| `reports/` | Local check reports and release readiness evidence. |
| `records/` | Version, changelog and development log. |
| `workflows/` | Regression, formatting and source-reading workflows. |
| `reference/` | External notes and structured reference data. |
| `analysis/` | Current gap analysis plus historical investigation notes. |
| `interpretation/` | Project reading notes. |

## Current Documentation Policy

- Keep current docs short and source-grounded.
- Do not preserve obsolete early architecture text in the main entry documents.
- Historical day-by-day reports can stay as records, but they are not the source of truth for current status.
- When code changes behavior, update the nearest current guide/architecture/plan document first; append detailed evidence to `reports/` only when validation was actually run.
