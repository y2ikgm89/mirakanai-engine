# Editor AI Playtest Readiness Report v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After validation recipe preflight, aggregate package diagnostics and recipe preflight rows into a deterministic read-only playtest readiness report for AI/editor workflows without executing package scripts or claiming play-in-editor productization.

**Architecture:** Reuse `EditorAiPackageAuthoringDiagnosticsModel`, the validation recipe preflight model, manifest descriptor rows, package payload diagnostics, and host-gate status. Keep mutation and execution on existing reviewed surfaces outside the report model.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make the final pre-playtest decision inspectable:

- aggregate package candidate, descriptor, payload, validation recipe, and host-gate diagnostics into one read-only report
- distinguish ready, blocked, and host-gated reasons for a selected packaged game
- keep report generation separate from manifest mutation and validation execution
- keep play-in-editor isolation, renderer/RHI ownership, Metal readiness, and renderer quality as follow-up work

## Constraints

- Do not evaluate arbitrary shell or raw manifest command strings.
- Do not mutate `game.agent.json`, package files, source assets, cooked payloads, or validation metadata.
- Do not execute `run-validation-recipe` or package scripts from editor core.
- Do not expose native renderer/RHI handles, backend frames, or editor GUI APIs to generated gameplay.
- Do not claim play-in-editor productization, Metal readiness, allocator/GPU budget enforcement, or general renderer quality.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish read-only readiness reporting from mutation and validation execution.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Readiness Inputs

- [x] Read editor AI package diagnostics and validation recipe preflight outputs.
- [x] Identify the smallest deterministic readiness report model that can aggregate status without mutating files.
- [x] Record non-goals before RED checks are added.

### Task 2: RED Checks

- [x] Add failing tests or static checks for read-only readiness report rows and unsupported mutation/execution claims.
- [x] Add failing checks rejecting arbitrary shell, free-form manifest edits, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

### Task 3: Readiness Report Implementation

- [x] Implement or tighten the deterministic editor/AI playtest readiness report model.
- [x] Connect package diagnostics and validation preflight outputs as report inputs only.
- [x] Keep mutation and execution delegated to existing reviewed surfaces.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

- Task 1 inventory: reused the completed `EditorAiPackageAuthoringDiagnosticsModel` and `EditorAiValidationRecipePreflightModel` contracts, `run-validation-recipe` dry-run/preflight status as caller-supplied data, manifest `validationRecipes`, `runtimeSceneValidationTargets`, and `prefabScenePackageAuthoringTargets`. Non-goals remained mutation, validation execution, arbitrary shell/raw command evaluation, package script execution, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- RED C++ checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed before the readiness API existed, with `EditorAiPlaytestReadinessReportDesc`, `EditorAiPlaytestReadinessReportModel`, and `make_editor_ai_playtest_readiness_report_model` unresolved in `tests/unit/editor_core_tests.cpp`.
- RED static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed while docs/tests/manifest needles for the readiness contract were unsynchronized; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed because `engine/agent/manifest.json.aiOperableProductionLoop.reviewLoops` did not yet expose `editor-ai-playtest-readiness-report`.
- GREEN implementation: `EditorAiPlaytestReadinessReportModel` now aggregates package diagnostics and validation preflight rows in `mirakana_editor_core`, reports ready/blocked/host-gated status and `ready_for_operator_validation`, and converts mutation, validation execution, arbitrary shell, free-form manifest edits, package script execution, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality requests into blocking diagnostics.
- GREEN focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS with `28/28` CTest tests passing.
- Static validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS.
- Agent context: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS after advancing `currentActivePlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-handoff-v1.md` and `recommendedNextPlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-evidence-summary-v1.md`.
- Public API validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS after adding public editor-core readiness report types.
- Recipe validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS with `status=passed`, running `agent-check` and `schema-check`.
- Full validation before recording this evidence: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with `28/28` tests passing. Diagnostic-only gates remained Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy gated by compile database availability.
