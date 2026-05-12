# Editor AI Playtest Operator Handoff v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the read-only playtest readiness report, create a deterministic operator handoff that lists the reviewed external validation commands an operator may run, without executing those commands from editor core.

**Architecture:** Reuse the playtest readiness report, validation recipe preflight rows, manifest `validationRecipes`, and `run-validation-recipe` dry-run plans. Keep command execution in the existing validation runner or package scripts, and keep editor/core free of shell execution, package mutation, renderer/RHI handles, and native backend ownership.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make the operator handoff inspectable before validation execution:

- summarize selected external validation commands from reviewed dry-run plans
- preserve host-gate acknowledgements and blocked reasons
- distinguish handoff generation from command execution
- keep package scripts and validation runners outside editor core

## Constraints

- Do not execute `run-validation-recipe`, package scripts, arbitrary shell, or raw manifest command strings from editor core.
- Do not mutate manifests, package files, source assets, cooked payloads, or validation metadata.
- Do not expose native renderer/RHI handles, backend frames, or editor GUI APIs to generated gameplay.
- Do not claim play-in-editor productization, Metal readiness, allocator/GPU budget enforcement, or general renderer quality.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish operator handoff rows from mutation and validation execution.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Handoff Inputs

- [x] Read playtest readiness report outputs, validation recipe preflight rows, and `run-validation-recipe` dry-run fields.
- [x] Identify the smallest deterministic handoff model that can list operator commands without executing them.
- [x] Record non-goals before RED checks are added.

Inventory notes: `EditorAiPlaytestReadinessReportModel` already carries `ready_for_operator_validation`, blocking diagnostics, and host gates; `EditorAiValidationRecipePreflightModel` carries selected manifest recipe rows, host gates, blockers, and reviewed dry-run availability; `tools/run-validation-recipe.ps1 -Mode DryRun` returns reviewed `command`, `argv`, and `commandPlan` data without execution. The narrow model should only summarize recipe id, reviewed command display or argv plan data, host gates, blockers, and readiness dependency for an external operator.

Non-goals for this slice: arbitrary shell execution, raw manifest command evaluation, validation execution from editor core, package script execution from editor core, manifest/package mutation, free-form manifest edits, play-in-editor productization, renderer/RHI/native handle exposure, Metal readiness, and general renderer quality claims.

### Task 2: RED Checks

- [x] Add failing tests or static checks for operator handoff rows and unsupported execution/mutation claims.
- [x] Add failing checks rejecting arbitrary shell, raw manifest command strings, package script execution from editor core, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

### Task 3: Handoff Implementation

- [x] Implement or tighten the deterministic editor/AI playtest operator handoff model.
- [x] Connect readiness report and validation preflight outputs as handoff inputs only.
- [x] Keep mutation and execution delegated to existing reviewed surfaces.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed before implementation because `EditorAiPlaytestOperatorHandoffModel`, `EditorAiPlaytestOperatorHandoffDesc`, and `make_editor_ai_playtest_operator_handoff_model` did not exist, and `EditorAiValidationRecipePreflightRow` did not yet carry command display / argv plan fields.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed before manifest sync with `engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-handoff review loop`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after implementation (`28/28` CTest tests).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after manifest/static-check sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs/manifest/static-check sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed after advancing `currentActivePlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-evidence-summary-v1.md` and `recommendedNextPlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-queue-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after changing the public `mirakana_editor_core` header.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`status: passed`, `exitCode: 0`, `durationSeconds: 3.881`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with CTest `28/28`; diagnostic-only host gates remain Metal tools missing, Apple packaging requiring macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy compile database availability gated.
