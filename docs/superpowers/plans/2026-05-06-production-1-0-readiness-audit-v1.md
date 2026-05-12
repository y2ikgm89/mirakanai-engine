# Production 1.0 Readiness Audit v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an automated audit gate that reports the remaining 1.0 unsupported production gaps from `engine/agent/manifest.json` without converting them into false ready claims.

**Status:** Completed on 2026-05-06

**Architecture:** Treat `engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps` as the machine-readable source of truth. The audit script validates every gap row has an id, status, required-before-ready claims, and notes, summarizes statuses for humans, and runs in default validation as a governance check rather than a readiness pass/fail on the existence of known gaps.

**Tech Stack:** PowerShell 7, JSON parsing through `ConvertFrom-Json`, `tools/common.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, `tools/check-ai-integration.ps1`.

---

## Context

- The master plan's next-candidate section still mentions several slices that have since completed.
- The manifest currently lists 11 `unsupportedProductionGaps`; those rows are the honest non-ready surface.
- The 1.0 closeout path needs an audit before any final closeout can claim the remaining gaps are either implemented, host-gated, or explicitly out of scope.

## Constraints

- Do not delete unsupported gap rows just to reduce the count.
- Do not claim 1.0 readiness in this slice.
- Do not require CI-only host evidence locally.
- End with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: Static Contract RED

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] Add assertions for `tools/check-production-readiness-audit.ps1`, `production-readiness-audit-check`, the new plan registry entry, and docs/manifest text for `Production 1.0 Readiness Audit v1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Record the expected RED failure.

### Task 2: Audit Script And Validation Wiring

**Files:**
- Create: `tools/check-production-readiness-audit.ps1`
- Modify: `package.json`
- Modify: `tools/validate.ps1`

- [x] Parse `engine/agent/manifest.json`.
- [x] Validate every `unsupportedProductionGaps` row has a non-empty `id`, `status`, `requiredBeforeReadyClaim`, and `notes`.
- [x] Reject duplicate gap ids and unexpected statuses outside the current status vocabulary.
- [x] Print `production-readiness-audit: unsupported_gaps=<count>`, per-status counts, per-gap rows, and `production-readiness-audit-check: ok`.
- [x] Add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` and include it in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

### Task 3: Docs, Manifest, And Master Plan

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `docs/workflows.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] Document that the audit checks manifest truth, not 1.0 readiness.
- [x] Replace the stale next-candidate text in the master plan with current remaining gap categories.
- [x] Keep `currentActivePlan` on the master plan and `recommendedNextPlan.id` as `next-production-gap-selection`.

### Task 4: Validation

**Commands:**
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

- [x] Verify the audit prints the current unsupported gap count and exits 0.
- [x] Verify `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passes.
- [x] Verify `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Done When

- The remaining unsupported production gaps are audited by a repository command.
- Default validation fails on malformed or stale unsupported gap rows.
- Current truth docs and the master plan describe the remaining 1.0 gap categories without claiming completion.

## Validation Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed because `engine/agent/manifest.json aiOperableProductionLoop.recommendedNextPlan.reason` did not contain `Production 1.0 Readiness Audit v1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` reported `unsupported_gaps=11`, status counts for implemented/planned/partly-ready rows, per-gap required claim counts, and `production-readiness-audit-check: ok`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` then failed because `docs/workflows.md` did not contain the literal `Production 1.0 Readiness Audit v1` marker required by the new static contract.
- FIXED: Added the explicit workflow marker.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with `validate: ok`; default validation ran `production-readiness-audit-check`, reported `unsupported_gaps=11`, kept Apple/Metal host blockers diagnostic-only, rebuilt the dev preset, and ran `ctest` with 29/29 tests passing.
