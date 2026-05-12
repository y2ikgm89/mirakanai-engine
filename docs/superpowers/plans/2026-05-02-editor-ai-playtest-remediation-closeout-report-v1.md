# Editor AI Playtest Remediation Closeout Report v1 Implementation Plan (2026-05-02)

> **Status note (2026-05-02): Retired by [AI-Operable Production Loop Clean Uplift v1](2026-05-02-ai-operable-production-loop-clean-uplift-v1.md).** The milestone decided there should be no separate remediation closeout-report row model. Closeout is represented by rerunning `EditorAiPlaytestEvidenceSummaryModel` after external remediation and observing `all_required_evidence_passed`, `remediation_required=false`, and `handoff_required=false`.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the remediation-evidence-review slice, summarize remediation closeout status without executing commands or mutating game/editor data.

**Architecture:** Reuse remediation evidence review rows, remediation handoff rows, remediation queue rows, host gates, blockers, unsupported claim diagnostics, and validation recipe ids. Keep closeout rows read-only; fixes, command execution, package scripts, evidence collection, evidence mutation, remediation mutation, and manifest edits stay on existing reviewed surfaces or external operator workflow.

**Tech Stack:** C++23, `mirakana_editor_core`, manifest static checks, docs, focused tests, and existing validation recipes.

---

## Goal

Make remediation closeout inspectable:

- summarize which remediation items are closed, still failing, blocked, missing evidence, or host-gated
- preserve recipe id, remediation category, evidence status, host gate, blocker, and readiness dependency references
- distinguish closeout reporting from fixing, executing, evidence mutation, remediation mutation, or manifest/package mutation
- keep unsupported renderer/editor/productization claims explicit

## Constraints

- Do not execute `run-validation-recipe`, package scripts, arbitrary shell, or raw command text from editor core.
- Do not mutate manifests, package files, source assets, cooked payloads, evidence records, remediation records, or validation metadata.
- Do not evaluate raw manifest command strings or free-form command text.
- Do not expose renderer/RHI/native handles or editor GUI APIs to generated gameplay.
- Do not claim Metal readiness or general renderer quality without focused host validation evidence.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish read-only remediation closeout rows from validation execution, evidence mutation, remediation mutation, fixes, and mutation.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Remediation Closeout Inputs

- [ ] Read the remediation evidence review outputs, remediation handoff rows, remediation queue rows, and unsupported claim diagnostics.
- [ ] Identify the smallest deterministic remediation closeout report model that can summarize final status without executing commands.
- [ ] Record non-goals before RED checks are added.

### Task 2: RED Checks

- [ ] Add failing tests or static checks for read-only remediation closeout rows and unsupported execution/mutation/fix/evidence-mutation/remediation-mutation claims.
- [ ] Add failing checks rejecting arbitrary shell, raw/free-form command evaluation, free-form manifest edits, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and renderer quality claims.
- [ ] Record RED evidence.

### Task 3: Remediation Closeout Report Implementation

- [ ] Implement or tighten the deterministic editor/AI playtest remediation closeout report model.
- [ ] Connect remediation evidence review, remediation handoff, remediation queue, host gates, and blockers as report inputs only.
- [ ] Keep mutation, fixes, evidence collection, evidence mutation, remediation mutation, and execution delegated to existing reviewed surfaces.

### Task 4: Docs Manifest Validation

- [ ] Update manifest/docs/static checks.
- [ ] Run required validation.
- [ ] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.
