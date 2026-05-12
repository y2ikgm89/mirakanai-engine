# 3D Prefab Scene Package Authoring v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** After the 2D atlas/tilemap package authoring slice, add a narrow generated 3D prefab/scene package authoring loop without claiming broad importer execution, skeletal animation production, material/shader graphs, live shader generation, Metal readiness, public native/RHI handles, or general renderer quality.

**Architecture:** Reuse `mirakana_scene` Scene/Prefab v2 contracts, reviewed source asset registration and cook/package surfaces, `migrate-scene-v2-runtime-package`, `runtimeSceneValidationTargets`, generated `DesktopRuntime3DPackage` scaffolds, and host-gated desktop package validation. Keep runtime package data cooked and deterministic, keep renderer/RHI upload host-owned, and keep gameplay on public `mirakana::` APIs.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_assets`, `mirakana_tools`, generated game manifests, desktop runtime package recipes, schemas/static checks, focused tests, and docs/manifest synchronization.

---

## Goal

Make generated 3D packages easier for AI agents to author while keeping the claim narrow:

- describe deterministic 3D prefab/scene package authoring inputs
- connect authored Scene/Prefab v2 data to the existing runtime package migration and validation path
- keep package files, runtime scene validation targets, and host-gated package smoke recipes synchronized
- keep broad importer dependency traversal, glTF production import, skeletal animation/GPU skinning, material/shader graph authoring, live shader generation, public native/RHI handles, Metal readiness, and renderer quality as planned or host-gated

## Constraints

- Do not add third-party importer, animation, material, or renderer dependencies without dependency/legal records.
- Do not claim broad/dependent package cooking, runtime source parsing, material/shader graphs, live shader generation, skeletal animation production paths, Metal readiness, or general renderer quality.
- Do not expose public renderer/RHI/native handles to gameplay or manifests.

## Done When

- A RED -> GREEN record exists in this plan.
- Schema/static checks distinguish deterministic 3D prefab/scene package authoring from broad importer or renderer production claims.
- Generated 3D package templates and docs remain honest about host gates and unsupported claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory 3D Prefab Scene Boundaries

- [x] Read existing generated 3D package scaffold, Scene/Prefab v2 contracts, source asset registration/cook/package helpers, Scene v2 runtime package migration, runtime scene validation targets, and desktop package validation scripts.
- [x] Identify the smallest deterministic first-party 3D prefab/scene package authoring contract that can ship as cooked package data without broad importer or renderer claims.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- Existing `DesktopRuntime3DPackage` scaffolding already emits deterministic cooked package fixtures for `texture`, `mesh`, `material`, and `scene` rows plus `runtimeSceneValidationTargets`, `packageStreamingResidencyTargets`, and material/shader package descriptors.
- Existing Scene/Prefab v2 authoring is exposed through `plan_scene_prefab_authoring` and `apply_scene_prefab_authoring` for `create_scene`, `add_node`, `add_or_update_component`, `create_prefab`, and `instantiate_prefab`; it rejects free-form edits and unsupported package/import/render claims.
- Existing Scene v2 package migration is exposed through `plan_scene_v2_runtime_package_migration` and `apply_scene_v2_runtime_package_migration`, reusing `plan_scene_package_update` / `apply_scene_package_update` to produce runtime-loadable Scene v1 package data from selected Scene v2 rows and source registry rows.
- The smallest GREEN contract for this slice should be a manifest/schema/static-checked selected 3D prefab scene authoring descriptor that orders: authored Scene/Prefab v2 command rows, selected source registry rows, Scene v2 runtime package migration, runtime scene package validation, and host-gated desktop package smoke.
- Non-goals for this slice remain broad importer execution, broad/dependent package cooking, runtime source parsing, material/shader graphs, live shader generation, skeletal animation or GPU skinning production paths, renderer/RHI residency ownership, package streaming execution, public native/RHI handles, Metal readiness, editor productization, and general renderer quality claims.

### Task 2: RED Checks

- [x] Add failing tests or static checks for generated 3D prefab/scene package descriptors and unsupported production claims.
- [x] Add failing checks rejecting broad importer execution, broad/dependent package cooking, runtime source parsing, material/shader graphs, live shader generation, skeletal animation/GPU skinning production claims, public native/RHI handles, Metal readiness, and general renderer quality.
- [x] Record RED evidence.

### Task 3: Authoring And Package Implementation

- [x] Implement or tighten the deterministic first-party 3D prefab/scene package authoring contract.
- [x] Update generated 3D package scaffold outputs and committed sample manifests only within the accepted scope.
- [x] Keep gameplay code on public scene/input/package/renderer-neutral contracts.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-02 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed as expected after static/schema guards were added, with `schemas/game-agent.schema.json must define prefabScenePackageAuthoringTargets`.
- 2026-05-02 RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed as expected while docs/manifest/scaffold sync guards were ahead of implementation, first with missing `prefabScenePackageAuthoringTargets` schema text and then with `docs/ai-game-development.md did not contain expected text: prefabScenePackageAuthoringTargets`.
- 2026-05-02 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS after adding `prefabScenePackageAuthoringTargets` schema/static validation, generated/sample 3D descriptor rows, source Scene/Prefab v2 fixtures, selected SourceAssetRegistry rows, and manifest `prefabScenePackageAuthoringLoops`.
- 2026-05-02 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after syncing generated `DesktopRuntime3DPackage` scaffold checks, docs, and AI integration guards.
- 2026-05-02 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1` PASS and emitted the updated machine-readable contract including the 3D prefab/scene package authoring loop and current recommended next plan data.
- 2026-05-02 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` PASS with CTest `28/28`.
- 2026-05-02 GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract` PASS; it ran `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- 2026-05-02 REVIEW FIX: cpp-reviewer identified empty generated 3D source authoring files, sample Scene v2/runtime Scene v1 node mismatch, stale sample scene dependency kinds, missing sample source format declarations, and static-check gaps. Fixed by initializing generated `source/assets/package.geassets`, source Scene/Prefab v2, texture and mesh source files; keeping source Scene v2 node order aligned with runtime node ids; changing sample scene dependency edges to `scene_material` / `scene_mesh`; adding exact source format declarations; and strengthening schema/static checks for non-empty source authoring files, source registry row formats, runtime scene target matching, and Scene/Prefab surface-operation pairing.
- 2026-05-02 REVIEW FIX VALIDATION: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` PASS and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` PASS after the review fixes.
