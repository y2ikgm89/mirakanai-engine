# Static Analysis Sharded CI v1 (2026-05-20)

**Plan ID:** `static-analysis-sharded-ci-v1`
**Status:** Completed docs/governance slice
**Owner:** Codex
**Completed phase:** Phase 1 - Sharded full repository static analysis

## Goal

Reduce hosted `Full Repository Static Analysis` wall-clock time without reducing clang-tidy coverage or weakening diagnostics.

## Context

- The existing hosted lane runs strict full-repository clang-tidy through `tools/check-tidy.ps1 -Strict -Preset ci-linux-tidy -Jobs 0`.
- Official clang-tidy documentation recommends parallel execution for large projects, and GitHub Actions supports matrix jobs for parallel work.
- `clang-tidy-diff.py` filters diagnostics to changed lines but still analyzes whole files, so it is not the root speed fix for full-repository analysis.

## Constraints

- Keep full repository clang-tidy coverage for selected static-analysis lanes.
- Keep `--warnings-as-errors=*`, first-party header filtering, config verification, and preserved actionable diagnostics.
- Do not make branch-protection-required checks path-filtered; keep `PR Gate` as the aggregate decision.
- Do not change `engine/agent/manifest.json` by hand.

## Phase 1 - Sharded Full Static Lane

### Done When

- `tools/check-tidy.ps1` supports deterministic full-list sharding through `-ShardCount` and zero-based `-ShardIndex`.
- The hosted `static-analysis` job runs four strict shards through GitHub Actions matrix strategy.
- `tools/check-ci-matrix.ps1` statically guards the matrix and wrapper arguments.
- Docs and agent surfaces explain the sharded lane and focused local `-Files` loop.
- Focused static checks pass, followed by `tools/validate.ps1`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` after RED guard | FAIL | Missing `strategy:` in `static-analysis` job before implementation. |
| Context7 `/kitware/cmake` docs query | PASS | Confirmed CMake `CMAKE_CXX_CLANG_TIDY`, `CMAKE_EXPORT_COMPILE_COMMANDS`, and compiler-launcher surfaces. |
| Context7 `/websites/ccache_dev` docs query | PASS | Confirmed ccache is compile-cache focused, so sharding clang-tidy work is the primary wall-clock fix rather than relying on ccache. |
| Official GitHub Actions docs | PASS | Matrix strategy is the official parallelization surface for jobs. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS | Guards four static-analysis shards and wrapper arguments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-surface needles updated for sharded static analysis. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Agent files stay inside size/parity budgets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Manifest compose output unchanged and valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | C++ and tracked text formatting valid. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -ShardCount 4 -ShardIndex 0 -MaxFiles 1` | PASS | Sharded wrapper selected 60 of 238 files and analyzed one smoke TU. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/physics/src/physics3d.cpp -ShardCount 2 -ShardIndex 0` | EXPECTED FAIL | Proved focused `-Files` cannot be combined with multi-shard mode. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full local validation passed; diagnostic-only Apple/Metal host gates unchanged. |
