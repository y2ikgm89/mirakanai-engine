# Phase 4 Full Repository Quality Gate CI And Analyzer Expansion v1 (2026-05-11)

**Plan ID:** `phase4-full-repository-quality-gate-ci-analyzer-expansion-v1`  
**Gap:** `full-repository-quality-gate`  
**Parent:** [2026-05-10-unsupported-production-gaps-orchestration-program-v1.md](2026-05-10-unsupported-production-gaps-orchestration-program-v1.md)  
**Status:** Completed for the Engine 1.0 Windows-default closeout; future release/analyzer expansion ledger

## Goal

Extend **CI build/package execution evidence** and **static analyzer coverage** beyond the current `static-analysis` Ubuntu tidy lane and Windows release artifact asserts—without weakening the existing `-Strict` tidy contract or conflating local `tools/check-tidy.ps1 -Files` ergonomics with the full gate.

**1.0 closeout:** [Full Repository Quality Gate 1.0 Closeout v1](2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md) accepts the reviewed local validation, CI matrix static contract, static-analysis CI contract, Linux coverage threshold policy, sanitizer lane documentation, release package artifact evidence, and installed SDK validation as the Engine 1.0 Windows-default ready surface. Broader analyzer profiles, full cross-platform package execution evidence, signing, notarization, upload, release distribution, crash/telemetry backend publication, and non-Windows host execution remain future or host-gated release work.

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
| CI PR check root-cause hardening | [2026-05-13-ci-pr-check-root-cause-hardening-v1.md](2026-05-13-ci-pr-check-root-cause-hardening-v1.md) | Completed |
| Full repository quality 1.0 closeout | [2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md](2026-05-18-full-repository-quality-gate-1-0-closeout-v1.md) | Completed |
