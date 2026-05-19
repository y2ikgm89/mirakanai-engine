# Full Repository Quality Gate 1.0 Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `full-repository-quality-gate` for the Engine 1.0 Windows-default ready surface without claiming broad analyzer-profile expansion, release distribution, or full cross-platform execution parity.

**Architecture:** Accept the existing validated quality surface as the 1.0 gate: local full `tools/validate.ps1`, `production-readiness-audit-check`, `check-ci-matrix.ps1`, CI Matrix Contract Check v1, Full Repository Static Analysis CI Contract v1, Linux coverage threshold policy, sanitizer lane documentation, C++23 release package artifact evidence, installed SDK validation, release package artifact validation, strict tidy compile database synthesis, and targeted tidy ergonomics. Keep broader analyzer profile expansion, full cross-platform package execution evidence beyond the reviewed contracts, signing, notarization, upload, release distribution, crash/telemetry backend publication, and non-Windows host execution as explicit future or host-gated release work.

**Tech Stack:** PowerShell 7 repository validation scripts, composed `engine/agent/manifest.json`, GitHub Actions workflow contracts, and existing C++23 build/test targets; this closeout changes docs, manifest fragments, generated manifest output, production readiness audit zero-gap handling, and static guards only.

---

**Plan ID:** `full-repository-quality-gate-1-0-closeout-v1`
**Status:** Completed
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Remove `full-repository-quality-gate` from `unsupportedProductionGaps` once the supported 1.0 Windows-default quality surface is narrowed to the reviewed local validation, static CI contract, release-artifact, coverage, sanitizer, and analyzer evidence that already exists.

## Context

- `full-repository-quality-gate` is the only remaining `unsupportedProductionGaps` row after the production UI closeout.
- Current validation already covers full local `validate.ps1`, strict static contracts, local build, 65 CTest tests, production readiness audit, release package artifact checks, installed SDK checks, CI matrix static policy, Linux coverage threshold policy, and static-analysis CI contract drift guards.
- Reviewed quality evidence includes local full validate, CI Matrix Contract Check v1, Full Repository Static Analysis CI Contract v1, Linux coverage threshold policy, sanitizer lane documentation, C++23 Release Package Artifact CI Evidence v1, Windows release package artifact evidence, installed SDK validation, and release package artifact validation.
- Existing hosted checks remain PR-level evidence for the branch, but the 1.0 ready claim is scoped to reviewed Windows-default local gates plus machine-readable CI workflow contracts, not a promise that every future optional platform/release distribution lane is complete.

## Constraints

- Do not add or weaken CI jobs, branch protection, analyzer warning policy, or package/release scripts in this closeout.
- Do not claim signing, notarization, upload, symbol-server publication, crash/telemetry backend readiness, Apple host execution, Android device matrix readiness, broader analyzer profile expansion, or full cross-platform package execution evidence.
- Keep `tools/check-production-readiness-audit.ps1` as the default audit gate, but let a zero-gap manifest report `unsupported_gaps=0` only after this final closeout plan exists.
- Keep the composed manifest generated from fragments.

## Done When

- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` removes `full-repository-quality-gate` from `unsupportedProductionGaps`, and `engine/agent/manifest.json` is recomposed.
- `tools/check-production-readiness-audit.ps1` reports `unsupported_gaps=0` and keeps shape validation for future rows.
- Static guards assert the full repository quality closeout plan and explicit future/host-gated exclusions instead of requiring a non-ready gap row.
- Current capability docs, roadmap, registry, master plan, and Phase 4 ledger name the zero-gap production manifest and leave no active unsupported production gap.
- Full validation passes or a concrete host/tool blocker is recorded.

## Tasks

- [x] Update manifest fragment and compose output.
- [x] Update production readiness audit zero-gap handling and static guards.
- [x] Update docs, master plan, registry, and Phase 4 ledger.
- [x] Run focused agent/static checks.
- [x] Run full validation for the final manifest/static/docs phase gate.
- [ ] Commit and push the validated checkpoint to PR #120.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS | Recomputed `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Reports `unsupported_gaps=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Static JSON/agent surface contract guard passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Agent-surface drift check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS | Cross-agent instruction parity and budgets passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Text/format gate passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full validation passed; CTest passed 65/65, shader/Apple host gates remain diagnostic-only or host-gated. |
