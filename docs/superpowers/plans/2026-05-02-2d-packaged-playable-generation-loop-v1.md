# 2D Packaged Playable Generation Loop v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make AI-generated packaged 2D games use reviewed source, scene, cook, package, manifest, runtime validation, and desktop runtime package validation surfaces instead of isolated sample fixtures.

**Architecture:** Reuse existing command surfaces first: `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `validate-runtime-scene-package`, `register-runtime-package-files`, and `run-validation-recipe`. Keep the first production uplift narrow: one deterministic generated 2D package with sprite, camera, input, HUD intent, audio cue intent, cooked package files, manifest runtime scene validation targets, and package smoke validation. Do not implement broad dependency cooking, package streaming, renderer/RHI residency, production atlas packing, tilemap editor UX, material/shader graphs, live shader generation, editor productization, native handles, Metal readiness, or general renderer quality.

**Tech Stack:** C++23 generated game scaffolding, `tools/new-game.ps1`, `game.agent.json`, `mirakana_scene`, `mirakana_assets`, `mirakana_tools`, `mirakana_runtime`, `mirakana_runtime_scene`, existing desktop runtime package scripts, static checks, docs, and validation recipes.

---

## Goal

Raise practical game-engine completeness by proving that an agent can create a packaged 2D playable game through reviewed engine contracts:

- create or scaffold a 2D packaged game manifest with runtime package files and runtime scene validation targets
- register first-party source rows for sprite texture, material, audio cue metadata, and Scene v2 authoring inputs where applicable
- cook explicitly selected registered source assets into deterministic cooked artifacts and `.geindex` rows
- migrate supported Scene v2 rows to the current runtime-loadable Scene v1 package surface
- validate the explicit runtime `.geindex` plus scene asset through `validate-runtime-scene-package`
- package and smoke the selected desktop runtime target through reviewed validation recipes

## Context

The engine has a ready `2d-playable-source-tree` recipe, a host-gated `2d-desktop-runtime-package` proof, and a strong AI command surface set. What is missing for higher completion is a normal generated-game loop that composes those pieces into one validated packaged 2D game workflow. This plan should convert the current foundations into a repeatable agent-facing path before moving to generated 3D production work.

## Constraints

- Do not add third-party dependencies or assets.
- Do not parse source assets at runtime; gameplay consumes cooked packages only.
- Do not make broad dependency cooking ready. Dependencies must be selected explicitly until a later focused plan implements dependency traversal.
- Do not claim production atlas packing, tilemap editor UX, native GPU sprite output, renderer/RHI residency, package streaming, editor productization, public native/RHI handles, Metal readiness, or general renderer quality.
- Keep generated game code on public `mirakana::` APIs.
- Keep desktop runtime proofs host-gated where SDL3/vcpkg or renderer backends are required.

## Done When

- A focused RED -> GREEN record exists in this plan.
- A generated or sample packaged 2D game has a manifest, runtime package files, and `runtimeSceneValidationTargets`.
- Focused tests or static checks prove the generated game includes deterministic package files, explicit validation target rows, and no source authoring files in `runtimePackageFiles`.
- The workflow validates through `validate-runtime-scene-package` before package smoke.
- `engine/agent/manifest.json`, docs, static checks, generated-game guidance, and `agent-context` expose the workflow honestly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused tests/checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory The Existing 2D And Package Paths

**Files:**
- Read: `tools/new-game.ps1`
- Read: `games/sample_2d_playable_foundation/game.agent.json`
- Read: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Read: `games/sample-generated-desktop-runtime-cooked-scene-package/game.agent.json`
- Read: `tests/unit/tools_tests.cpp`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `docs/specs/generated-game-validation-scenarios.md`
- Read: `docs/ai-game-development.md`

- [x] Define the generated 2D packaged workflow boundary and list reused command surfaces.
  - Reuse `register-source-asset`, `cook-registered-source-assets`, `migrate-scene-v2-runtime-package`, `validate-runtime-scene-package`, `register-runtime-package-files`, and `run-validation-recipe` as the reviewed workflow surfaces.
  - The first GREEN implementation is a deterministic generated desktop-runtime 2D package scaffold, not broad dependency cooking or a runtime source parser.
- [x] Decide whether to extend `DesktopRuntimeCookedScenePackage` or add a separate 2D packaged template.
  - Add a separate `DesktopRuntime2DPackage` template. The existing `DesktopRuntimeCookedScenePackage` remains a generic cooked mesh/scene scaffold; forcing 2D sprite/HUD/audio semantics into it would make the recipe contract ambiguous.
- [x] Define package files, scene asset key, validation target id, and validation recipe names.
  - Generated package files: `runtime/<game>.config`, `runtime/<game>.geindex`, `runtime/assets/2d/player.texture.geasset`, `runtime/assets/2d/player.material`, `runtime/assets/2d/jump.audio.geasset`, and `runtime/assets/2d/playable.scene`.
  - Runtime scene validation target id: `packaged-2d-scene`.
  - Scene asset key: `<game>/scenes/packaged-2d-scene`.
  - Validation recipes: `desktop-game-runtime`, `desktop-runtime-release-target`, and `installed-2d-package-smoke`.
- [x] Record non-goals before RED checks are added.
  - No broad dependency cooking, runtime source parsing, package streaming, renderer/RHI residency, production atlas/tilemap/native GPU sprite output, editor productization, material/shader graph, live shader generation, native handles, Metal readiness, or general renderer quality claim.

### Task 2: RED Checks For Generated 2D Package Workflow

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: focused tests if a C++ helper is introduced
- Modify: this plan

- [x] Add failing static checks requiring the generated 2D packaged game manifest shape.
- [x] Add failing checks requiring `runtimeSceneValidationTargets` to match package files and scene asset key.
- [x] Add failing checks rejecting source files in `runtimePackageFiles`, broad dependency cooking, renderer/RHI residency, package streaming, material/shader graph, live shader generation, editor productization, native handle, and Metal claims.
- [x] Record RED evidence.

### Task 3: Generate Or Scaffold The Packaged 2D Game Path

**Files:**
- Modify: `tools/new-game.ps1`
- Modify or create: `games/<selected-2d-package-game>/game.agent.json`
- Modify or create: `games/<selected-2d-package-game>/main.cpp`
- Modify or create: selected runtime package fixture files under `games/<selected-2d-package-game>/runtime/`
- Modify: `games/CMakeLists.txt` if a new sample target is added

- [x] Emit deterministic runtime config, `.geindex`, texture/material/audio/scene package files, and validation target rows.
- [x] Keep source authoring inputs out of `runtimePackageFiles`.
- [x] Keep gameplay code on public `mirakana::` APIs and cooked package/runtime scene access.
- [x] Ensure package smoke args consume the generated package path and scene package path.

### Task 4: Validation Command Composition

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `engine/agent/manifest.json`

- [x] Document the ordered workflow: register source rows, cook selected rows, migrate Scene v2, validate runtime scene package, package/smoke.
- [x] Keep `validate-runtime-scene-package` as the reviewed non-mutating package/scene check before desktop smoke.
- [x] Keep package smoke host-gated and separate from package/scene validation.
- [x] Update static checks so stale generated-game guidance fails.

### Task 5: Plan Completion And Next Slice

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.json`
- Modify: this plan

- [x] Mark this 2D generated package workflow ready only inside its validated scope.
- [x] Keep production 2D atlas/tilemap/native GPU output planned or host-gated.
- [x] Create the next focused plan for generated packaged 3D gameplay or editor playtest package review, based on validation evidence.
- [x] Run the full required validation set and record evidence.

## Validation Evidence

- 2026-05-02 RED:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: FAIL as expected with `engine/agent/manifest.json 2d-desktop-runtime-package recipe must allow DesktopRuntime2DPackage`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: FAIL as expected with `engine manifest commands.newGame must expose DesktopRuntime2DPackage`.
- 2026-05-02 GREEN focused checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS; generated `DesktopRuntime2DPackage` scaffold emitted deterministic config, `.geindex`, cooked player texture/material, cooked jump audio, orthographic sprite scene, `runtimeSceneValidationTargets`, public `mirakana::` input/UI/audio/package code, and `PACKAGE_FILES_FROM_MANIFEST`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
- 2026-05-02 final validation after docs/manifest/registry sync:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS; `currentActivePlan` points at `docs/superpowers/plans/2026-05-02-3d-packaged-playable-generation-loop-v1.md` and `recommendedNextPlan.path` points at `docs/superpowers/plans/2026-05-02-editor-playtest-package-review-loop-v1.md`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `exitCode: 0`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS (`validate: ok`, CTest `28/28` passed)
  - Diagnostic-only host gates remain honest: Metal tools are missing on this Windows host, Apple packaging requires macOS/Xcode, Android release signing/device smoke is not fully configured, and strict tidy analysis still depends on compile database availability.
