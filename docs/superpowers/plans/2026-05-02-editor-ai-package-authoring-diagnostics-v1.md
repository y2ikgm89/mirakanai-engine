# Editor AI Package Authoring Diagnostics v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the 3D prefab/scene package authoring slice, add a narrow editor/AI diagnostics loop that reviews authored package candidates, manifest descriptors, package payload diagnostics, and validation recipe status without turning the editor into a broad package cooker or play-in-editor product.

**Architecture:** Reuse `mirakana_editor_core` package candidate/review models, `game.agent.json` descriptor fields, `mirakana_runtime` package inspection reports, `validate-runtime-scene-package`, generated game manifests, and static checks. Keep mutation on the existing reviewed apply surfaces and keep native renderer/RHI handles out of editor-facing diagnostics.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_runtime`, `mirakana_tools`, manifest schemas, docs/static checks, focused tests, and host-gated desktop runtime validation recipes.

---

## Goal

Make AI/editor package review more inspectable without expanding authority:

- summarize candidate package files, descriptor rows, payload diagnostics, and recipe gates in one deterministic model
- distinguish dry-run package review from package mutation and desktop smoke execution
- keep existing reviewed apply surfaces as the only mutation paths
- keep play-in-editor isolation, broad package cooking, renderer/RHI residency ownership, native handles, Metal readiness, and renderer quality as follow-up work

## Constraints

- Do not add arbitrary shell, free-form JSON patching, broad package cooking, or runtime source parsing.
- Do not expose `IRhiDevice`, backend handles, swapchain frames, descriptor handles, or editor-private GUI APIs to generated gameplay.
- Do not claim play-in-editor productization, Metal readiness, allocator/GPU budget enforcement, or general renderer quality.

## Done When

- A RED -> GREEN record exists in this plan.
- Schema/static checks distinguish diagnostics/review from package mutation and desktop smoke execution.
- Docs and manifest keep unsupported editor/productization and renderer/RHI claims explicit.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Existing Review Diagnostics

- [x] Read editor package candidate/review models, runtime package diagnostics, validation recipe runner outputs, and generated game manifest descriptor checks.
- [x] Identify the smallest deterministic diagnostics model that can summarize package authoring state without mutating files.
- [x] Record non-goals before RED checks are added.

### Task 2: RED Checks

- [x] Add failing tests or static checks for package candidate diagnostics, descriptor summary rows, and unsupported mutation/execution claims.
- [x] Add failing checks rejecting arbitrary shell, free-form manifest edits, broad package cooking, runtime source parsing, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, and general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Diagnostics Implementation

- [x] Implement or tighten a deterministic editor/AI package diagnostics model.
- [x] Connect runtime package inspection and validation recipe status as report inputs only.
- [x] Keep mutation delegated to existing reviewed apply surfaces.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- Inventory: `editor/core/include/mirakana/editor/playtest_package_review.hpp`, `editor/core/src/playtest_package_review.cpp`, `tests/unit/editor_core_tests.cpp`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `schemas/engine-agent.schema.json`, and `engine/agent/manifest.json` already owned the editor playtest package review boundary. The diagnostics slice can safely extend `aiOperableProductionLoop.reviewLoops` without adding a new `game.agent.json` descriptor.
- Non-goals recorded: diagnostics do not execute arbitrary shell, free-form manifest edits, broad package cooking, runtime source parsing, play-in-editor productization, renderer/RHI handle exposure, Metal readiness, or renderer quality claims.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding tests for `EditorAiPackageAuthoringDiagnosticsModel` because `make_editor_ai_package_authoring_diagnostics_model`, `EditorAiPackageAuthoringDiagnosticsDesc`, and `EditorAiPackageAuthoringDiagnosticStatus` did not exist.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json aiOperableProductionLoop` did not expose `editor-ai-package-authoring-diagnostics`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `engine/agent/manifest.json aiOperableProductionLoop` did not expose `editor-ai-package-authoring-diagnostics`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding `EditorAiPackageAuthoringDiagnosticsModel` and diagnostics-only tests (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed after adding the manifest `reviewLoops` row and static checks for diagnostics-only `mutates=false` / `executes=false`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after adding the manifest `reviewLoops` row and agent-facing static checks.
- Registry/manifest: `engine/agent/manifest.json.currentActivePlan` advanced to `docs/superpowers/plans/2026-05-02-editor-ai-validation-recipe-preflight-v1.md`; `recommendedNextPlan` advanced to `docs/superpowers/plans/2026-05-02-editor-ai-playtest-readiness-report-v1.md`; `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, and `docs/roadmap.md` were synchronized.
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed and reported `currentActivePlan=docs/superpowers/plans/2026-05-02-editor-ai-validation-recipe-preflight-v1.md` plus `recommendedNextPlan=docs/superpowers/plans/2026-05-02-editor-ai-playtest-readiness-report-v1.md`.
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed (`CTest 28/28 passed`).
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the `mirakana_editor_core` public header update.
- Verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed.
- Final verification: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. CTest reported `28/28` passing. Diagnostic-only host gates remained explicit: Metal tools missing, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy compile database availability remains gated.
