# Editor AI Playtest Remediation Queue v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the evidence-summary slice, turn externally supplied playtest evidence failures and blockers into deterministic read-only remediation rows without executing commands or mutating game/editor data.

**Architecture:** Reuse the evidence summary, operator handoff rows, host gates, blocked reasons, unsupported claim diagnostics, and existing validation recipe ids. Keep remediation as prioritized diagnostics and next-action descriptions only; implementation, command execution, package scripts, and manifest edits stay on existing reviewed surfaces.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make post-evidence remediation inspectable:

- classify failed, blocked, missing, and host-gated evidence into deterministic remediation rows
- preserve recipe id, handoff command row id, host gate, and blocker references
- distinguish read-only remediation planning from fixing, executing, or mutating
- keep unsupported renderer/editor/productization claims explicit

## Constraints

- Do not execute `run-validation-recipe`, package scripts, arbitrary shell, or raw command text from editor core.
- Do not mutate manifests, package files, source assets, cooked payloads, evidence records, or validation metadata.
- Do not evaluate raw manifest command strings or free-form command text.
- Do not expose renderer/RHI/native handles or editor GUI APIs to generated gameplay.
- Do not claim Metal readiness or general renderer quality without focused host validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish read-only remediation queue rows from validation execution and mutation.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Remediation Inputs

- [x] Read the evidence-summary outputs, operator handoff rows, validation recipe preflight rows, and unsupported claim diagnostics.
- [x] Identify the smallest deterministic remediation queue model that can classify external failures without executing commands.
- [x] Record non-goals before RED checks are added.

Task 1 inventory notes:

- Minimal model: `EditorAiPlaytestRemediationQueueModel` in `mirakana_editor_core`, built only from `EditorAiPlaytestEvidenceSummaryModel` rows.
- Rows classify non-passing evidence only: `failed -> investigate_failure`, `blocked -> resolve_blocker`, `missing -> collect_missing_evidence`, and `host_gated -> satisfy_host_gate`.
- Rows preserve recipe id, evidence status, exit code/summary, host gates, blockers, readiness dependency, remediation category, and deterministic next-action text.
- Non-goals remain explicit: no editor-core validation execution, package script execution, arbitrary shell, raw/free-form command evaluation, manifest mutation, evidence mutation, fix execution, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, or renderer quality claim.

### Task 2: RED Checks

- [x] Add failing tests or static checks for read-only remediation rows and unsupported execution/mutation claims.
- [x] Add failing checks rejecting arbitrary shell, raw/free-form command evaluation, free-form manifest edits, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

### Task 3: Remediation Queue Implementation

- [x] Implement or tighten the deterministic editor/AI playtest remediation queue model.
- [x] Connect evidence summary, handoff, host gates, and blockers as report inputs only.
- [x] Keep mutation, fixes, and execution delegated to existing reviewed surfaces.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding remediation queue tests because `EditorAiPlaytestRemediationQueueModel`, `EditorAiPlaytestRemediationQueueDesc`, `EditorAiPlaytestRemediationCategory`, `make_editor_ai_playtest_remediation_queue_model`, and `editor_ai_playtest_remediation_category_label` were not implemented.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after adding static checks with `engine/agent/manifest.json aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks with `engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS after implementing `EditorAiPlaytestRemediationQueueModel` (`28/28` CTest).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after manifest `reviewLoops` and static checks were synchronized (`json-contract-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after docs/static check synchronization (`ai-integration-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS after advancing `currentActivePlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-handoff-v1.md` and `recommendedNextPlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-remediation-evidence-review-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS after public `mirakana_editor_core` header updates (`public-api-boundary-check: ok`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS with `status: passed`, `agent-check` PASS, and `schema-check` PASS.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS after remediation queue implementation, docs/manifest/static checks, active-plan advancement, CMake build, and CTest.
