# Editor AI Playtest Remediation Handoff v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the remediation-queue slice, turn read-only remediation rows into deterministic external handoff rows without executing commands or mutating game/editor data.

**Architecture:** Reuse the remediation queue, evidence summary, operator handoff rows, host gates, blockers, unsupported claim diagnostics, and validation recipe ids. Keep handoff rows as external-operator guidance only; fixes, command execution, package scripts, and manifest edits stay on existing reviewed surfaces.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make remediation handoff inspectable:

- map remediation rows to deterministic external-owner/action rows
- preserve recipe id, evidence status, host gate, blocker, and readiness dependency references
- distinguish handoff guidance from fixing, executing, or mutating
- keep unsupported renderer/editor/productization claims explicit

## Constraints

- Do not execute `run-validation-recipe`, package scripts, arbitrary shell, or raw command text from editor core.
- Do not mutate manifests, package files, source assets, cooked payloads, evidence records, remediation records, or validation metadata.
- Do not evaluate raw manifest command strings or free-form command text.
- Do not expose renderer/RHI/native handles or editor GUI APIs to generated gameplay.
- Do not claim Metal readiness or general renderer quality without focused host validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish read-only remediation handoff rows from validation execution, fixes, and mutation.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Remediation Handoff Inputs

- [x] Read the remediation queue outputs, evidence summary rows, operator handoff rows, and unsupported claim diagnostics.
- [x] Identify the smallest deterministic remediation handoff model that can guide external work without executing commands.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- Existing inputs are `EditorAiPlaytestOperatorHandoffModel` command rows, `EditorAiPlaytestEvidenceSummaryModel` evidence rows, and `EditorAiPlaytestRemediationQueueModel` remediation rows in `mirakana_editor_core`.
- The smallest handoff slice should consume only externally supplied `EditorAiPlaytestRemediationQueueModel` rows, preserving recipe id, evidence status, remediation category, exit code or evidence summary, host gates, blockers, readiness dependency, diagnostic text, and unsupported claims.
- The handoff output should add deterministic external-owner/action-kind guidance and handoff text for an external operator; it must not add command execution, package script execution, shell/raw command evaluation, data mutation, evidence/remediation mutation, or fix execution.
- Non-goals remain explicit: no editor GUI productization, play-in-editor isolation, validation execution from editor core, package script execution from editor core, arbitrary shell/raw/free-form command evaluation, free-form manifest edits, evidence mutation, remediation mutation, fix execution, broad package cooking, runtime source parsing, renderer/RHI/native handle exposure, Metal readiness, or renderer quality claims.

### Task 2: RED Checks

- [x] Add failing tests or static checks for read-only remediation handoff rows and unsupported execution/mutation/fix claims.
- [x] Add failing checks rejecting arbitrary shell, raw/free-form command evaluation, free-form manifest edits, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

### Task 3: Remediation Handoff Implementation

- [x] Implement or tighten the deterministic editor/AI playtest remediation handoff model.
- [x] Connect remediation queue, evidence summary, handoff, host gates, and blockers as report inputs only.
- [x] Keep mutation, fixes, and execution delegated to existing reviewed surfaces.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed before implementation because `EditorAiPlaytestRemediationHandoffModel`, `EditorAiPlaytestRemediationHandoffDesc`, `EditorAiPlaytestRemediationHandoffActionKind`, `make_editor_ai_playtest_remediation_handoff_model`, and `editor_ai_playtest_remediation_handoff_action_kind_label` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed before manifest sync with `engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-handoff review loop`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before code/docs sync because `editor/core/include/mirakana/editor/playtest_package_review.hpp` was missing `EditorAiPlaytestRemediationHandoffDesc`, `EditorAiPlaytestRemediationHandoffModel`, `EditorAiPlaytestRemediationHandoffActionKind`, and `make_editor_ai_playtest_remediation_handoff_model`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS after implementing `EditorAiPlaytestRemediationHandoffModel` (`28/28` CTest).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after manifest `editor-ai-playtest-remediation-handoff` review loop and static checks were synchronized (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after code/docs/manifest needles for `EditorAiPlaytestRemediationHandoffModel` were synchronized (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS and reports `currentActivePlan=docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-evidence-review-v1.md` with `recommendedNextPlan.path=docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-closeout-report-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS after adding public `mirakana_editor_core` remediation handoff declarations (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS with `status=passed`, `agent-check` exit code 0, and `schema-check` exit code 0.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS with `validate: ok`; diagnostic-only host gates remain Metal tools missing, Apple packaging requires macOS/Xcode, strict tidy compile database availability gated, and direct `cmake` missing on PATH while repository wrappers use resolved Visual Studio CMake.
