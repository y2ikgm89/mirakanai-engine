# 2D Atlas Tilemap Package Authoring v1 Implementation Plan (2026-05-02)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow generated 2D atlas/tilemap package authoring loop after host-gated package streaming execution is validated.

**Architecture:** Reuse first-party `mirakana_assets`, `mirakana_scene`, `mirakana_tools`, `mirakana_ui`, `mirakana_scene_renderer`, and desktop runtime package validation surfaces. Keep atlas/tilemap authoring deterministic and descriptor-driven, keep source image decoding and native GPU sprite batching out of scope unless a focused dependency/legal plan accepts them, and keep gameplay on public `mirakana::` contracts.

**Tech Stack:** C++23, first-party asset/package text contracts, generated game manifests, desktop runtime package recipes, static checks, and focused tests.

---

## Goal

Make generated 2D packages richer without over-claiming production sprite technology:

- describe a first-party atlas/tilemap source contract for generated games
- cook deterministic package payloads through reviewed package helpers
- validate manifest-declared package files and runtime scene validation targets
- keep production atlas packing, source image decoding, native GPU sprite batching, and editor tilemap UX as planned unless implemented by focused follow-up slices

## Constraints

- Do not add third-party image, tilemap, or atlas dependencies without dependency/legal records.
- Do not claim production atlas packing, runtime image decoding, native GPU sprite batching, package streaming readiness, Metal readiness, or general renderer quality.
- Do not expose public renderer/RHI/native handles to gameplay or manifests.

## Done When

- A RED -> GREEN record exists in this plan.
- Schema/static checks distinguish deterministic atlas/tilemap package authoring from production atlas packing and native GPU sprite output.
- Generated 2D package templates and docs remain honest about host gates and unsupported claims.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Implementation Tasks

### Task 1: Inventory 2D Atlas Tilemap Boundaries

- [x] Read existing 2D package scaffold, `mirakana_scene_renderer` sprite submission, `mirakana_ui_renderer` atlas metadata helpers, `mirakana_assets` UI atlas metadata, package tooling, and generated manifest checks.
- [x] Identify the smallest deterministic first-party atlas/tilemap contract that can ship as package data without production packing/native GPU claims.
- [x] Record non-goals before RED checks are added.

Inventory notes:

- The generated 2D desktop package scaffold currently ships one cooked texture, one material, one audio payload, and one `GameEngine.Scene.v1` sprite scene. The committed `sample_2d_desktop_runtime_package` package index has 4 entries and `scene_sprite` / `scene_material` / `material_texture` dependency rows.
- `mirakana_scene_renderer` already submits sprite draw intent through `SceneRenderPacket`, `make_scene_sprite_command`, and `submit_scene_render_packet`; this remains renderer-neutral and does not imply native GPU sprite batching or atlas allocation.
- `mirakana_assets` and `mirakana_tools` already provide `GameEngine.UiAtlas.v1` metadata-only authoring through `UiAtlasMetadataDocument`, `author_cooked_ui_atlas_metadata`, `plan_cooked_ui_atlas_package_update`, and `apply_cooked_ui_atlas_package_update`. The contract explicitly keeps `source_decoding` and `atlas_packing` unsupported.
- `mirakana_ui_renderer` can build `UiRendererImagePalette` rows from runtime UI atlas metadata and submit image sprite placeholders, but that surface is UI-oriented and should not be rebranded as production world-sprite atlas batching.
- `mirakana_scene` Schema v2 currently recognizes `tilemap` as a component type, but the existing Scene v2 runtime package migration rejects `tilemap` as unsupported. A narrow 2D package authoring slice should either add a first-party data-only tilemap package contract or keep Scene v2 tilemap migration blocked until tests define the supported subset.
- The smallest safe contract for this slice is a deterministic first-party 2D atlas/tilemap descriptor emitted by generated 2D packages: explicit cooked atlas metadata or tilemap data rows, declared runtime package files, deterministic package index entries, and runtime scene validation targets. It should reuse existing package update helpers where possible and stop at cooked package data plus renderer-neutral sprite/tile intent.
- Non-goals for this slice: source PNG/JPEG decoding, production atlas packing, native GPU sprite batching, tilemap editor UX, arbitrary tilemap import formats, broad package cooking, broad/background package streaming readiness, renderer/RHI native handles, Metal readiness, and general renderer quality.

### Task 2: RED Checks

- [x] Add failing tests or static checks for generated atlas/tilemap package descriptors and unsupported production claims.
- [x] Add failing checks rejecting runtime image decoding, native GPU sprite batching, package streaming readiness, public native/RHI handles, Metal readiness, and general renderer quality claims.
- [x] Record RED evidence.

RED evidence:

- Added C++ tests that referenced missing public headers `mirakana/assets/tilemap_metadata.hpp` and `mirakana/tools/tilemap_tool.hpp`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed before implementation with missing include errors:
  - `tests/unit/asset_identity_runtime_resource_tests.cpp(7,10): error C1083: include file 'mirakana/assets/tilemap_metadata.hpp': No such file or directory`
  - `tests/unit/tools_tests.cpp(31,10): error C1083: include file 'mirakana/tools/tilemap_tool.hpp': No such file or directory`
- Static descriptor checks were added for `atlasTilemapAuthoringTargets`, rejected unsupported fields such as `sourceImagePath`, `runtimeImageDecoding`, `productionAtlasPacking`, `nativeGpuOutput`, `packageStreamingReady`, `rendererQualityClaim`, `metalReady`, `nativeHandle`, and `rhiHandle`, and required deterministic-package-data mode with unsupported source decoding, atlas packing, and native GPU sprite batching flags.

### Task 3: Authoring And Package Implementation

- [x] Implement or tighten the deterministic first-party atlas/tilemap package contract.
- [x] Update generated 2D package scaffold outputs and committed sample manifests only within the accepted scope.
- [x] Keep gameplay code on public scene/input/UI/audio/package contracts.

Implementation notes:

- Added `mirakana::TilemapMetadataDocument`, `GameEngine.Tilemap.v1` validation/serialization/deserialization, `AssetKind::tilemap`, and `AssetDependencyKind::tilemap_texture`.
- Added `mirakana::author_cooked_tilemap_metadata`, `mirakana::write_cooked_tilemap_metadata`, `mirakana::verify_cooked_tilemap_package_metadata`, `mirakana::plan_cooked_tilemap_package_update`, and `mirakana::apply_cooked_tilemap_package_update`.
- Updated runtime typed payload inspection with `RuntimeTilemapPayload` and package-level diagnostics for atlas texture record kind, atlas page path, and `tilemap_texture` edge coherence.
- Preserved tilemap layer order while keeping tile rows deterministic by id.
- Updated `games/sample_2d_desktop_runtime_package` and `tools/new-game.ps1 -Template DesktopRuntime2DPackage` to ship `runtime/assets/2d/level.tilemap`, package index tilemap rows, `atlasTilemapAuthoringTargets`, tilemap preload keys, and `tilemap` resident resource hints.
- Kept gameplay code on existing public package, scene, UI, input, audio, and renderer-neutral contracts; no renderer/RHI/native handles, source image decoding, production atlas packing, or native GPU sprite batching were added.

### Task 4: Docs Manifest Validation

- [x] Update manifest/docs/static checks.
- [x] Run required validation.
- [x] Advance the plan registry to the next focused slice based on validation evidence.

## Validation Evidence

Record command results here while implementing this plan.

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS (`json-contract-check: ok`).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS (`ai-integration-check: ok`).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS; `currentActivePlan` now points at `docs/superpowers/plans/2026-05-02-3d-prefab-scene-package-authoring-v1.md` and `recommendedNextPlan.path` now points at `docs/superpowers/plans/2026-05-02-editor-ai-package-authoring-diagnostics-v1.md`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS (`public-api-boundary-check: ok`).
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/run-validation-recipe.ps1 -Mode Execute -Recipe agent-contract`: PASS (`status: passed`, `agent-check` and `schema-check` both exit 0).
- Focused build: PASS via CMake dev preset for `mirakana_runtime_tests`, `mirakana_asset_identity_runtime_resource_tests`, and `mirakana_tools_tests`.
- Focused tests: PASS:
  - `out\build\dev\Debug\mirakana_runtime_tests.exe`
  - `out\build\dev\Debug\mirakana_asset_identity_runtime_resource_tests.exe`
  - `out\build\dev\Debug\mirakana_tools_tests.exe`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS (`validate: ok`; CTest `28/28`; diagnostic-only host gates remain Metal tools missing, Apple packaging requires macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy analysis gated by compile database availability).
