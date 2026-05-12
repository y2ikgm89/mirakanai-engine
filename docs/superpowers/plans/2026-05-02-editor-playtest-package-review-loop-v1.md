# Editor Playtest Package Review Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect editor-facing package candidates, manifest runtime package files, runtime scene validation targets, diagnostics, and desktop run/package review into one AI-operable playtest loop without productizing the full editor.

**Architecture:** Reuse existing editor-core scene/prefab package candidate rows, reviewed package registration apply paths, `runtimeSceneValidationTargets`, `validate-runtime-scene-package`, and manifest-declared validation recipes. Keep this as a review/run workflow over existing models; do not add broad package cooking, runtime source parsing, renderer/RHI residency, package streaming, native handles, Metal readiness, or a Unity/UE-like editor productization claim.

**Tech Stack:** `mirakana_editor_core`, `mirakana_editor`, `game.agent.json`, `tools/register-runtime-package-files.ps1`, `validate-runtime-scene-package`, `run-validation-recipe`, docs, static checks, and validation recipes.

---

## Goal

Make a generated or sample packaged game easier for an agent or editor operator to review before playtest:

- surface reviewed package candidates and manifest registration state
- select runtime scene validation inputs from `runtimeSceneValidationTargets`
- run the non-mutating runtime scene package validation before desktop package smoke
- keep diagnostics tied to manifest/package/scene rows
- preserve host-gated desktop package execution as a separate validation step

## Constraints

- Do not implement broad/dependent package cooking.
- Do not parse source assets at runtime.
- Do not expose renderer, RHI, SDL3, OS, or native handles to gameplay code.
- Do not claim package streaming, editor productization, Metal readiness, or general renderer quality.
- Keep Dear ImGui as the optional developer shell only.

## Done When

- A focused RED -> GREEN record exists in this plan.
- Editor/package review docs and checks describe the ordered review loop.
- Existing package registration apply paths and `runtimeSceneValidationTargets` are used rather than new free-form shell commands.
- Runtime scene package validation is documented and checked as the pre-smoke gate.
- `engine/agent/manifest.json`, roadmap, current capabilities, and plan registry are synchronized.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory Editor And Package Review Surfaces

- [x] Read editor-core package candidate and registration apply models.
- [x] Read visible editor package registration UI wiring.
- [x] Read runtime scene validation target docs and static checks.
- [x] Define the review loop boundary and non-goals.

Inventory notes:

- `SceneAuthoringDocument` already exposes `ScenePackageCandidateRow`, `ScenePackageRegistrationDraftRow`, `ScenePackageRegistrationApplyPlan`, `ScenePackageRegistrationApplyResult`, and `apply_scene_package_registration_to_manifest` for narrow `runtimePackageFiles` updates.
- `mirakana_editor` already renders `Scene Package Candidates`, `Package Registration Draft`, and `Package Registration Apply`, and calls the editor-core apply helper from the `Apply Package Registration` button.
- `runtimeSceneValidationTargets` schema/static checks already require safe game-relative `.geindex` paths, non-empty safe scene asset keys, `packageIndexPath` membership in `runtimePackageFiles`, and no command/native/renderer/source-file fields.
- Gap found during inventory: editor-core did not have a single review/status model that tied package registration state, manifest validation target selection, `validate-runtime-scene-package`, and host-gated desktop smoke into an ordered pre-playtest loop.
- Boundary: add a GUI-independent review/status model and machine-readable manifest review-loop descriptor. Do not run CMake, package scripts, shader tools, desktop runtime, or arbitrary shell from editor core; keep actual validation execution in `validate-runtime-scene-package`, `run-validation-recipe`, and package scripts.

### Task 2: RED Review Loop Checks

- [x] Add failing checks for stale editor/package review guidance.
- [x] Add failing checks that runtime scene package validation is selected from manifest descriptors before desktop smoke.
- [x] Add failing checks rejecting broad package cooking, source runtime parsing, package streaming, renderer/RHI residency, editor productization, native handles, Metal readiness, and general renderer quality claims.
- [x] Record RED evidence.

### Task 3: Review Loop Implementation

- [x] Update editor/package docs and manifest guidance.
- [x] Add or adjust narrow diagnostics/status rows if existing models lack enough review state.
- [x] Keep package registration apply and validation execution separate.
- [x] Keep host-gated desktop package smoke separate from package/scene validation.

Implementation notes:

- Added `mirakana::editor::EditorPlaytestPackageReviewModel` in `editor/core/include/mirakana/editor/playtest_package_review.hpp` and `editor/core/src/playtest_package_review.cpp`.
- The model orders `review-editor-package-candidates -> apply-reviewed-runtime-package-files -> select-runtime-scene-validation-target -> validate-runtime-scene-package -> run-host-gated-desktop-smoke`.
- The model blocks runtime scene validation until reviewed package files have been applied and the selected `runtimeSceneValidationTargets` row points at a registered package index.
- Desktop smoke remains a host-gated step and is not executed from editor core.
- `engine/agent/manifest.json.aiOperableProductionLoop.reviewLoops` now exposes the same ordered loop and unsupported-claim boundary.

### Task 4: Completion And Next Slice

- [x] Mark the editor playtest package review loop ready only inside its validated scope.
- [x] Keep full editor productization, play-in-editor isolation, package streaming, renderer/RHI residency, native handles, Metal, and general renderer quality planned or host-gated.
- [x] Create the next focused plan for renderer resource residency/upload execution or material/shader authoring based on validation evidence.
- [x] Run the full required validation set and record evidence.

## Validation Evidence

Record command results here while implementing this plan.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected because `engine/agent/manifest.json aiOperableProductionLoop` was missing required property `reviewLoops`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected because `docs/current-capabilities.md` did not contain `Editor Playtest Package Review Loop v1`.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` failed as expected after adding editor-core review model tests because `mirakana/editor/playtest_package_review.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding `EditorPlaytestPackageReviewModel` (`CTest 28/28 passed`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed (`json-contract-check: ok`) after manifest review-loop sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after docs/static/editor-core sync.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` passed after manifest sync. Active plan advanced to `docs/superpowers/plans/2026-05-02-renderer-resource-residency-upload-execution-v1.md`; recommended next plan advanced to `docs/superpowers/plans/2026-05-02-material-shader-authoring-loop-v1.md`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed (`public-api-boundary-check: ok`) after adding the editor-core review header.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` passed (`schema-check`, `agent-check`, `api-boundary-check`, `shader-toolchain-check`, `mobile-check`, `validation-recipe-runner-check`, and `agent-context`).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` passed with the editor package review model included (`desktop-gui` CTest `41/41` passed).
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed (`validate: ok`, CTest `28/28` passed).
- DIAGNOSTIC: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` still reports Metal shader tooling as missing (`metal`, `metallib`) and keeps Metal library packaging diagnostic-only.
- DIAGNOSTIC: Apple packaging remains host-blocked on this Windows host because macOS/Xcode tools are unavailable.
- DIAGNOSTIC: Android packaging reports ready, while release signing is not configured and device smoke is not connected.
- DIAGNOSTIC: `tidy-check` reports the compile database availability gate before configure and then `config ok`; strict tidy analysis remains CI/toolchain dependent.
