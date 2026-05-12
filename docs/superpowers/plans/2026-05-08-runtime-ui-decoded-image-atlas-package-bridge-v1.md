# Runtime UI Decoded Image Atlas Package Bridge v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a host-independent `mirakana_tools` bridge that turns validated `mirakana::ui::ImageDecodeResult` rows into a deterministic packed UI atlas texture page plus `GameEngine.UiAtlas.v1` package metadata update.

**Architecture:** Keep `mirakana_ui` codec-free and keep renderer texture upload out of scope. The new tool API lives in `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`, reuses `mirakana::pack_sprite_atlas_rgba8_max_side`, emits a first-party `GameEngine.CookedTexture.v1` page payload, and reuses the existing cooked UI atlas package update validation so decoded image rows can be dry-run/applied after validation with package index evidence.

**Tech Stack:** C++23, `mirakana_ui`, `mirakana_assets`, `mirakana_tools`, existing in-memory filesystem tests, existing CMake/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

---

## Goal

Close the current UI image pipeline loophole after Runtime UI PNG Image Decoding Adapter v1: decoded RGBA8 UI images can be produced, and UI atlas metadata/package update helpers exist, but there is no reviewed bridge that packs decoded UI images into a package texture page and metadata rows together.

## Context

- `Runtime UI Image Decode Request Plan v1` validates `ImageDecodeRequest` and adapter output.
- `Runtime UI PNG Image Decoding Adapter v1` adds `PngImageDecodingAdapter` over the audited PNG path.
- `source-image-decode-and-atlas-packing-v1` already provides deterministic `pack_sprite_atlas_rgba8_max_side`.
- `mirakana_tools` already owns `author_cooked_ui_atlas_metadata`, `plan_cooked_ui_atlas_package_update`, and `apply_cooked_ui_atlas_package_update`, but those require caller-authored page/image rows and an existing texture page entry.

## Constraints

- Do not add new third-party dependencies, codecs, SVG/vector parsing, platform SDK calls, or native renderer/RHI handles.
- Do not claim renderer texture upload, GPU residency, package streaming, font rasterization, text shaping, IME, OS accessibility, or production UI quality.
- Keep the transaction safe: if decoded image validation, packing, metadata validation, or package index validation fails, `apply_*` must leave prior files unchanged.
- Keep `production-ui-importer-platform-adapters` non-ready because font loading/rasterization, shaping, native IME/accessibility, broader codecs, SVG/vector, and renderer upload remain unfinished.

## Done When

- `mirakana_tools` exposes packed UI atlas authoring and package update types/functions for decoded `ImageDecodeResult` rows.
- Unit tests prove successful pack/package update, invalid decoded image rejection, package path collision rejection, and transactional apply rollback.
- Docs, plan registry, master plan, manifest, and static contract checks record the bridge without widening ready claims.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass; commit only this slice and preserve pre-existing unrelated dirty files.

## Files

- Modify: `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp`
- Modify: `engine/tools/src/ui_atlas_tool.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ui.md`
- Modify: `docs/architecture.md`
- Modify: `docs/dependencies.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `tools/check-ai-integration.ps1`

## Tasks

### Task 1: Red Tests

- [x] Add `tools_tests.cpp` coverage for `author_packed_ui_atlas_from_decoded_images` with two RGBA8 decoded images that produce one texture page, metadata rows, and non-zero UVs.
- [x] Add `tools_tests.cpp` coverage for `plan_packed_ui_atlas_package_update` writing three dry-run changed files: page texture, atlas metadata, and package index.
- [x] Add `tools_tests.cpp` coverage for invalid decoded image pixel format/byte count and package path collision diagnostics.
- [x] Add `tools_tests.cpp` coverage proving `apply_packed_ui_atlas_package_update` does not mutate prior files when a later validation fails.
- [x] Run `cmake --build --preset dev --target MK_tools_tests` and confirm failure before the new API exists.

### Task 2: Public API

- [x] Add `PackedUiAtlasImageDesc`, `PackedUiAtlasAuthoringDesc`, `PackedUiAtlasAuthoringResult`, `PackedUiAtlasPackageUpdateDesc`, `PackedUiAtlasPackageApplyDesc`, and `PackedUiAtlasPackageUpdateResult` to `ui_atlas_tool.hpp`.
- [x] Add `author_packed_ui_atlas_from_decoded_images`, `plan_packed_ui_atlas_package_update`, and `apply_packed_ui_atlas_package_update` declarations.
- [x] Keep all new public types value-only and free of platform, renderer, GPU, and third-party handles.

### Task 3: Implementation

- [x] Convert each `ImageDecodeResult` RGBA8 pixel vector into `SpriteAtlasPackingItemView` rows after validating non-empty resource identity and exact `width * height * 4` byte counts.
- [x] Call `pack_sprite_atlas_rgba8_max_side`, build a single `TextureSourceDocument` page, emit a `GameEngine.CookedTexture.v1` page payload, and map pack placements to UV rows.
- [x] Build a `CookedUiAtlasAuthoringDesc` with `source_decoding="decoded-image-adapter"` and `atlas_packing="deterministic-sprite-atlas-rgba8-max-side"`, then reuse `author_cooked_ui_atlas_metadata` for validation.
- [x] In `plan_packed_ui_atlas_package_update`, parse the current package index, add/update the page texture entry, reject path collisions, then call `plan_cooked_ui_atlas_package_update` with the augmented index.
- [x] In `apply_packed_ui_atlas_package_update`, read the index, dry-run first, and only write the returned changed files when the whole plan succeeds.

### Task 4: Documentation And Contract Checks

- [x] Mark this plan completed with validation evidence.
- [x] Update plan registry and master plan current verdict/completed slice lists.
- [x] Update docs and `engine/agent/manifest.json` to describe the new package bridge while preserving unsupported claims.
- [x] Update `tools/check-json-contracts.ps1` and `tools/check-ai-integration.ps1` to assert the new API, docs, and manifest evidence.

### Task 5: Validation And Commit

- [x] Run focused `MK_tools_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` and format if needed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Stage only this slice and commit as `feat: add runtime ui decoded image atlas bridge`.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_tools_tests` | PASS | Builds `mirakana_assets`, `mirakana_tools`, and `MK_tools_tests` after adding packed UI atlas APIs. |
| `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_tools_tests` | PASS | Proves packed decoded-image authoring/package update plus invalid decoded image and path-collision diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | Verifies manifest, plan, docs, and static contract tokens for the decoded image atlas package bridge. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Verifies AI-facing manifest, command surfaces, docs, and game guidance stay synchronized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS | Keeps `production-ui-importer-platform-adapters` planned/non-ready and records remaining unsupported gaps. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Confirms the new `mirakana_tools` public API does not expose backend, platform, or third-party handles. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Passed after applying repository `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` to the implementation source. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed; 29/29 CTest tests passed. Metal and Apple diagnostics remain host-gated/non-fatal on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS | Full dev build completed through the repository wrapper. |

## Status

**Status:** Completed.
