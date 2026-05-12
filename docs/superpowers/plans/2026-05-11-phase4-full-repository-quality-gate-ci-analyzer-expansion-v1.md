# Phase 4 Full Repository Quality Gate CI And Analyzer Expansion v1 (2026-05-11)

**Plan ID:** `phase4-full-repository-quality-gate-ci-analyzer-expansion-v1`  
**Gap:** `full-repository-quality-gate`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Active program ledger  

## Goal

Extend **CI build/package execution evidence** and **static analyzer coverage** beyond the current `static-analysis` Ubuntu tidy lane and Windows release artifact asserts—without weakening the existing `-Strict` tidy contract or conflating local `tools/check-tidy.ps1 -Files` ergonomics with the full gate.

## Current baseline (reference)

- Full-repo tidy CI job + artifact upload (`static-analysis-tidy-logs`).
- Targeted file lane: `tools/check-tidy.ps1 -Files` for agent iteration.
- `tools/check-ci-matrix.ps1` guards workflow drift.

## Expansion tracks

1. **CI execution evidence** — additional jobs or steps that build/package selected targets and upload logs/binaries per matrix policy (no secrets in repo).
2. **Analyzer profiles** — optional secondary `.clang-tidy` profile or staged check enablement with warning budgets documented in plan evidence tables.
3. **Sanitizer / coverage** — keep Linux coverage thresholds authoritative; document host requirements for optional lanes.

## Role separation (must stay explicit)

| Lane | Purpose |
| --- | --- |
| `check-tidy.ps1` default / CI `static-analysis` | Full-repository gate |
| `check-tidy.ps1 -Files` | Fast, changed-file agent loop; never replaces full gate claims |
| Future expanded profile | Must be opt-in until warning-clean under project policy |

## Constraints

- `AGENTS.md` PowerShell 7 wrappers remain the canonical entrypoints.
- Analyzer expansion must not silently broaden `ready` claims in `engine/agent/manifest.json`.

## Linked slices

| Topic | Plan | Status |
| --- | --- | --- |
| (pending) | — | Add row per landed Phase 4 child plan |
