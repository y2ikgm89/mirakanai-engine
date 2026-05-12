# Editor AI Validation Recipe Preflight v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After editor/AI package authoring diagnostics, add a narrow validation-recipe preflight loop that lets editor/AI surfaces explain selected manifest validation recipes and host gates without executing arbitrary commands or turning the editor into play-in-editor productization.

**Architecture:** Reuse `run-validation-recipe` dry-run planning, manifest `validationRecipes`, package/scene descriptor rows, editor-core diagnostics models, and existing package validation scripts. Keep execution on the reviewed validation runner or external operator workflow, and keep editor core free of shell execution, renderer/RHI handles, and native backend ownership.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_tools`, manifest schemas/static checks, docs, focused tests, and host-gated desktop runtime validation recipes.

---

## Goal

Make validation readiness inspectable before execution:

- map selected manifest descriptors to reviewed validation recipe dry-run plans
- summarize host gates, missing recipe ids, blocked execution requirements, and unsupported claims
- distinguish preflight diagnostics from actual validation execution
- keep mutation and execution delegated to existing reviewed command surfaces

## Constraints

- Do not evaluate raw manifest command strings or arbitrary shell.
- Do not execute package scripts from editor core.
- Do not expose `IRhiDevice`, backend handles, swapchain frames, descriptor handles, or native handles.
- Do not claim play-in-editor productization, Metal readiness, allocator/GPU budget enforcement, or general renderer quality.

## Done When

- A RED -> GREEN record exists in this plan.
- Static checks distinguish validation preflight from command execution and package mutation.
- Docs and manifest keep host gates and unsupported editor/productization claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Validation Preflight Inputs

- [x] Read validation recipe runner dry-run output, editor package diagnostics models, and manifest descriptor rows.
- [x] Identify the smallest deterministic preflight model that reports host gates without executing commands.
- [x] Record non-goals before RED checks are added.

### Task 2: RED Checks

- [x] Add failing tests or static checks for validation recipe preflight rows and unsupported execution claims.
- [x] Add failing checks rejecting raw shell execution, free-form manifest commands, play-in-editor productization, native/RHI handle exposure, Metal readiness, and renderer quality claims.
- [x] Record RED evidence.

### Task 3: Preflight Implementation

- [x] Implement or tighten the deterministic editor/AI validation preflight model.
- [x] Connect selected manifest descriptor rows to reviewed validation dry-run plans.
- [x] Keep execution delegated to `run-validation-recipe` and package scripts outside editor core.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

- Task 1 inventory: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS and confirmed `currentActivePlan` was this plan while `recommendedNextPlan` was `docs/superpowers/plans/2026-05-02-editor-ai-playtest-readiness-report-v1.md`. `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode DryRun -Recipe agent-contract` returned a dry-run plan for `agent-check` and `schema-check`; selected D3D12 package dry-run reported host gate `d3d12-windows-primary`; `shader-toolchain` dry-run reported `vulkan-strict` and `metal-apple`.
- Non-goals recorded for this slice: arbitrary shell/raw manifest command evaluation, package script execution from editor core, play-in-editor productization, renderer/RHI/native handle exposure, Metal readiness, and general renderer quality.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding preflight tests because `make_editor_ai_validation_recipe_preflight_model` and `EditorAiValidationRecipePreflightDesc` were not implemented.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected after adding static checks, first with the preflight contract not fully synchronized in tests/docs/manifest.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after adding static checks with `engine manifest aiOperableProductionLoop must expose one editor-ai-validation-recipe-preflight review loop`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS after implementing `EditorAiValidationRecipePreflightModel` (`28/28` CTest).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after adding manifest `editor-ai-validation-recipe-preflight`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after docs/static check synchronization.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS after advancing `currentActivePlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-readiness-report-v1.md` and `recommendedNextPlan` to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-operator-handoff-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` PASS after public `mirakana_editor_core` header changes.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS (`status: passed`, `agent-check` and `schema-check` both exit 0).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS after final test/docs/static synchronization (`28/28` CTest).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` PASS. Diagnostic-only host gates remain explicit: Metal tools are missing on this host, Apple packaging requires macOS/Xcode, Android release signing/device smoke are not fully configured, and strict tidy analysis is gated by compile database availability.
