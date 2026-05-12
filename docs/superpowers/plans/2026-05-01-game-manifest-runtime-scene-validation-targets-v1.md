# Game Manifest Runtime Scene Validation Targets v1 Implementation Plan (2026-05-01)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an AI-readable `game.agent.json` descriptor for explicit runtime scene package validation targets so agents can choose `validate-runtime-scene-package` inputs from a reviewed manifest contract, then hand off directly to packaged 2D playable game generation.

**Architecture:** Keep this descriptor-only. Reuse the existing `validate-runtime-scene-package` command surface; do not execute validation from schema checks, mutate packages, add broad package cooking, parse runtime source assets, or claim renderer/RHI residency.

**Tech Stack:** `schemas/game-agent.schema.json`, generated game manifest scaffolds, sample game manifests, static checks, docs, and `engine/agent/manifest.json`.

---

## Goal

Make generated and sample games declare which cooked package and scene asset should be passed to `validate-runtime-scene-package`:

- optional `runtimeSceneValidationTargets` rows in `game.agent.json`
- each row names a target id, package index path, scene asset key, optional content root, and validation flags
- sample/generated cooked-scene package manifests include descriptor rows matching their runtime package files
- static checks reject unsafe paths, source-file validation targets, free-form command strings, and broad renderer/package/editor claims

## Context

`validate-runtime-scene-package` is now a ready non-mutating command surface. Agents still need a safe game-level manifest descriptor that tells them which `.geindex` and scene asset key are intended for runtime package validation, instead of scraping smoke-test command strings or inventing package paths.

Completion review on 2026-05-02: this slice is governance-heavy and should be treated as the final AI-operability bridge before higher-impact playable-game work. It improves agent reliability, but it does not by itself move the engine much closer to Unity/UE-style completeness. The next slice must therefore create a generated packaged 2D playable workflow that exercises source registration, explicit cook/package, Scene v2 migration, manifest validation targets, runtime package validation, and desktop runtime package validation together.

## Constraints

- Do not add third-party dependencies.
- Do not add a new execution command or shell path.
- Do not mutate package files, source assets, scenes, prefabs, material files, or CMake metadata.
- Do not broaden package cooking, dependent cooking, runtime source parsing, renderer/RHI residency, package streaming, editor productization, native handle exposure, Metal readiness, or general renderer quality claims.
- Keep paths game-relative and package/runtime safe.

## Done When

- A focused RED -> GREEN record exists in this plan.
- `schemas/game-agent.schema.json` accepts and constrains `runtimeSceneValidationTargets`.
- Sample/generated cooked-scene package manifests expose at least one valid runtime scene validation target.
- `tools/new-game.ps1` emits descriptor rows for cooked-scene desktop package templates where the scene package fixture is generated.
- Static checks require the descriptor where appropriate and reject unsupported claims.
- `engine/agent/manifest.json`, docs, plan registry, and `agent-context` mention the descriptor honestly.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused scaffold/static checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.
- The next dated focused plan exists as `docs/superpowers/plans/2026-05-02-2d-packaged-playable-generation-loop-v1.md`.

## Completion Uplift Review

Current honest commercial-engine completion remains low because the repo has strong foundations but not enough normal game-creation workflows. The completion-impact order after this descriptor slice is:

1. `2d-packaged-playable-generation-loop-v1`: generate or scaffold a small packaged 2D game through reviewed source/scene/package command surfaces and validate the runtime package target.
2. `3d-packaged-playable-generation-loop-v1`: do the same for a generated static-mesh 3D package with camera/controller/light/material validation on the D3D12 primary lane and Vulkan strict host gate.
3. `editor-playtest-package-review-loop-v1`: connect editor package candidates, validation targets, diagnostics, and run/play review into one operator workflow over existing editor-core models.
4. `renderer-resource-residency-upload-v1`: move from foundation-only resource/upload planning toward production resource lifetime, residency diagnostics, and upload execution.
5. `material-shader-authoring-v1`: replace scaffold-only material/shader artifacts with first-party material/shader authoring contracts and validation.

This plan should not expand into those systems. It must only make `game.agent.json` capable of pointing at the package/scene validation target that the following playable-game slices will consume.

## Implementation Tasks

### Task 1: Inventory Manifest Contract

**Files:**
- Read: `schemas/game-agent.schema.json`
- Read: `tools/check-ai-integration.ps1`
- Read: `tools/check-json-contracts.ps1`
- Read: `tools/new-game.ps1`
- Read: `games/sample-generated-desktop-runtime-cooked-scene-package/game.agent.json`
- Read: `games/sample-generated-desktop-runtime-material-shader-package/game.agent.json`
- Read: `games/sample_desktop_runtime_game/game.agent.json`
- Read: `docs/ai-game-development.md`
- Read: `docs/current-capabilities.md`

- [x] Define descriptor field names, required fields, optional fields, path rules, and validation flags.
  - Final descriptor property: `runtimeSceneValidationTargets`.
  - Required row fields: `id`, `packageIndexPath`, and `sceneAssetKey`.
  - Optional row fields: `contentRoot`, `validateAssetReferences`, and `requireUniqueNodeNames`.
  - Path rules: game-relative only, no absolute paths, no `..`, no `games/` prefix, and `packageIndexPath` must end in `.geindex`.
  - Validation flags are booleans only; descriptor rows are data, not command strings.
- [x] Decide which existing sample/generated manifests must carry descriptor rows.
  - `games/sample-generated-desktop-runtime-cooked-scene-package/game.agent.json`
  - `games/sample-generated-desktop-runtime-material-shader-package/game.agent.json`
  - `games/sample_desktop_runtime_game/game.agent.json`
  - `games/sample_2d_desktop_runtime_package/game.agent.json`
  - generated `DesktopRuntimeCookedScenePackage` and `DesktopRuntimeMaterialShaderPackage` scaffolds through `tools/new-game.ps1`
- [x] Record non-goals before RED checks are added.
  - No new execution command, shell/eval string, source-file validation target, package mutation, broad dependency cooking, runtime source parsing, renderer/RHI residency, package streaming, editor productization, native handle, Metal readiness, or general renderer quality claim.

### Task 2: RED Schema And Static Checks

**Files:**
- Modify: `schemas/game-agent.schema.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: this plan

- [x] Add failing checks requiring `runtimeSceneValidationTargets` for cooked-scene package manifests.
- [x] Add failing checks rejecting unsafe package paths, missing scene asset keys, command strings, source files, and broad unsupported claims.
- [x] Record RED evidence.

### Task 3: Production Descriptor Implementation

**Files:**
- Modify: `schemas/game-agent.schema.json`
- Modify: `tools/new-game.ps1`
- Modify: selected `games/*/game.agent.json`
- Modify: static checks

- [x] Add the constrained optional schema property.
- [x] Add deterministic descriptor rows to existing sample/generated cooked-scene package manifests.
- [x] Update generated cooked-scene package templates to emit matching rows.
- [x] Keep descriptor rows separate from `runtimePackageFiles` and `validationRecipes`.

### Task 4: Manifest, Docs, And Validation

**Files:**
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: this plan

- [x] Promote only the manifest descriptor contract to ready.
- [x] Keep `validate-runtime-scene-package` as the only reviewed runtime scene validation command surface.
- [x] Run required validation commands and record evidence.
- [x] Mark this plan complete and keep `docs/superpowers/plans/2026-05-02-2d-packaged-playable-generation-loop-v1.md` as the next dated focused plan.

## Validation Evidence

- 2026-05-02 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after static schema guards were added, with `schemas/game-agent.schema.json must define runtimeSceneValidationTargets`.
- 2026-05-02 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected while descriptor docs/manifest sync guards were ahead of implementation, with missing `runtimeSceneValidationTargets` text in `engine/agent/manifest.json`.
- 2026-05-02 GREEN focused checks before final full validation:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`)
- 2026-05-02 final validation after manifest/docs/registry sync:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS; `currentActivePlan` points at `docs/superpowers/plans/2026-05-02-2d-packaged-playable-generation-loop-v1.md` and `recommendedNextPlan.path` points at `docs/superpowers/plans/2026-05-02-3d-packaged-playable-generation-loop-v1.md`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `exitCode: 0`)
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS (`validate: ok`, CTest `28/28` passed)
  - Diagnostic-only host gates remain honest: Metal tools are missing on this Windows host, Apple packaging requires macOS/Xcode, Android release signing/device smoke is not fully configured, and strict tidy analysis still depends on compile database availability.
