# Editor AI Playtest Evidence Summary v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the operator handoff slice, summarize externally collected playtest validation evidence into deterministic AI/editor rows without running validation commands from editor core.

**Architecture:** Reuse the playtest readiness report, operator handoff rows, validation recipe ids, host gates, and external execution evidence supplied by the operator. Keep the summary read-only and keep command execution in `run-validation-recipe`, package scripts, or CI/operator workflows.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make post-handoff evidence inspectable:

- collect externally supplied validation results for selected recipes
- distinguish passed, failed, blocked, host-gated, and missing evidence
- link evidence back to readiness and operator handoff rows
- keep execution, mutation, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality as explicit non-goals

## Constraints

- Do not execute `run-validation-recipe`, package scripts, or arbitrary shell from editor core.
- Do not mutate manifests, package files, source assets, cooked payloads, or validation metadata.
- Do not evaluate raw manifest command strings or free-form command text.
- Do not expose renderer/RHI/native handles or editor GUI APIs to generated gameplay.
- Do not claim Metal readiness or general renderer quality without host validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish read-only evidence summarization from validation execution.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Evidence Inputs

- [x] Read the playtest readiness report and operator handoff outputs.
- [x] Identify the smallest deterministic evidence-summary model that can aggregate external results without executing commands.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- `EditorAiPlaytestReadinessReportModel` carries the `ready_for_operator_validation` dependency, blocking diagnostics, and host-gate state.
- `EditorAiPlaytestOperatorHandoffModel` carries the selected recipe id, reviewed command display, argv plan data, host gates, blocked reasons, and readiness dependency for each external operator row.
- `tools/run-validation-recipe.ps1 -Mode Execute` returns structured external execution evidence with `status`, `exitCode`, `stdoutSummary`, `stderrSummary`, `commandResults`, `hostGates`, and diagnostics. This slice treats that shape as caller/operator supplied data only.
- The narrow model should live in `mirakana_editor_core`, take operator handoff plus externally supplied evidence rows, and emit deterministic per-recipe summary rows with `passed`, `failed`, `blocked`, `host_gated`, or `missing` status, exit code or summary text, host gates, blockers, readiness dependency, and unsupported-claim diagnostics.
- Non-goals remain editor-core validation execution, package script execution, arbitrary shell or raw/free-form command evaluation, free-form manifest edits, play-in-editor productization, renderer/RHI/native handle exposure, Metal readiness, and renderer quality claims.

### Task 2: RED Checks

- [x] Add failing tests or static checks for read-only evidence rows and unsupported mutation/execution claims.
- [x] Add failing checks rejecting arbitrary shell, free-form manifest edits, raw command evaluation, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

RED evidence:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1`: failed before implementation because `EditorAiPlaytestValidationEvidenceRow`, `EditorAiPlaytestEvidenceStatus`, `EditorAiPlaytestEvidenceSummaryModel`, `EditorAiPlaytestEvidenceSummaryDesc`, `make_editor_ai_playtest_evidence_summary_model`, and `editor_ai_playtest_evidence_status_label` did not exist.

### Task 3: Evidence Summary Implementation

- [x] Implement or tighten the deterministic editor/AI playtest evidence summary model.
- [x] Connect readiness, handoff, and externally supplied validation results as report inputs only.
- [x] Keep mutation and execution delegated to existing reviewed surfaces.

Implementation notes:

- Added `EditorAiPlaytestEvidenceSummaryModel`, `EditorAiPlaytestEvidenceSummaryDesc`, `EditorAiPlaytestValidationEvidenceRow`, `EditorAiPlaytestEvidenceSummaryRow`, `EditorAiPlaytestEvidenceStatus`, `editor_ai_playtest_evidence_status_label`, and `make_editor_ai_playtest_evidence_summary_model` to `mirakana_editor_core`.
- Summary rows are keyed from `EditorAiPlaytestOperatorHandoffModel.command_rows` and mark missing evidence as `missing-external-validation-evidence`.
- Evidence rows are required to be externally supplied; claims that editor core executed validation are converted into blocking diagnostics.
- Evidence summary preserves exit code, stdout/stderr summary text, host gates, blockers, readiness dependency, and unsupported-claim diagnostics without invoking `run-validation-recipe`, package scripts, arbitrary shell, raw/free-form command evaluation, or manifest mutation.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed before implementation with missing evidence summary API symbols.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after implementation (`28/28` CTest).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding the manifest `editor-ai-playtest-evidence-summary` review loop and static JSON checks.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after syncing code/docs/manifest needles for `EditorAiPlaytestEvidenceSummaryModel`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reports `currentActivePlan=docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-queue-v1.md` and `recommendedNextPlan=docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-handoff-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after changing `mirakana_editor_core` public headers.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`, `exitCode: 0`, `durationSeconds: 3.794`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. CTest reported `28/28`; diagnostic-only gates remain Metal tools missing, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, strict tidy compile database availability gated, and direct `cmake` missing on PATH while repository wrappers use resolved VS CMake.
