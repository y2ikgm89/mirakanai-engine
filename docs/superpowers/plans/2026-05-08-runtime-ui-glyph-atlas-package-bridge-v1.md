# Runtime UI Glyph Atlas Package Bridge v1 Implementation Plan (2026-05-08)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Generate deterministic cooked UI glyph atlas package rows from already-rasterized RGBA8 glyph pixels and expose those rows as a `UiRendererGlyphAtlasPalette`.

**Architecture:** Keep `mirakana_ui` independent from font libraries, OS font APIs, text shaping, native handles, and renderer texture upload. Extend the existing first-party `GameEngine.UiAtlas.v1` metadata path with structured glyph rows, let `mirakana_tools` pack supplied rasterized glyph pixels through the existing deterministic RGBA8 atlas packer, let `mirakana_runtime` parse the glyph rows, and let `mirakana_ui_renderer` build the runtime glyph palette from loaded package metadata.

**Tech Stack:** C++23, existing `mirakana_assets` UI atlas metadata, `mirakana_tools` atlas package tooling, `mirakana_runtime` package payload parsing, `mirakana_ui_renderer` palette binding, focused unit tests, existing validation scripts.

---

## Goal

- Add structured glyph rows to `UiAtlasMetadataDocument` and `RuntimeUiAtlasPayload`.
- Add `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas` so runtime package metadata can feed `UiRendererGlyphAtlasPalette` without string conventions.
- Add `PackedUiGlyphAtlasAuthoringDesc`, `author_packed_ui_glyph_atlas_from_rasterized_glyphs`, `plan_packed_ui_glyph_atlas_package_update`, and `apply_packed_ui_glyph_atlas_package_update` to turn already-rasterized RGBA8 glyph pixels into one `GameEngine.CookedTexture.v1` page plus synchronized `GameEngine.UiAtlas.v1` metadata/package rows.
- Update docs, manifest, and static checks so the `production-ui-importer-platform-adapters` gap records host-independent glyph atlas generation while real font loading/rasterization, shaping, renderer upload, platform SDK calls, and native text systems remain unsupported.

## Context

- `Runtime UI Font Image Adapter v1` already lets `mirakana_ui_renderer` consume manually supplied `UiRendererGlyphAtlasPalette` bindings.
- `Runtime UI Font Rasterization Request Plan v1` validates requests around `IFontRasterizerAdapter`, but it does not generate atlas metadata or package texture rows.
- `Runtime UI Decoded Image Atlas Package Bridge v1` already packs validated RGBA8 image rows into cooked UI atlas package artifacts; this slice mirrors that pattern for glyph identity rows instead of image identity rows.
- Existing `GameEngine.UiAtlas.v1` metadata is image-only, so glyph binding currently requires hand-built palette rows or ad hoc conventions. This slice must add structured glyph metadata rather than parsing glyph identity out of strings.

## Constraints

- Do not add dependencies, font files, font rasterization implementations, shaping, bidi, font fallback, glyph metrics, kerning, native IME/text-input sessions, OS accessibility bridge publication, platform SDK calls, renderer/RHI upload, or native handles.
- The glyph bridge accepts already-rasterized RGBA8 glyph pixels only; real font loading/rasterization remains a separate adapter and legal/licensing task.
- Reject empty `font_family`, glyph id `0`, invalid page assets, invalid UV/color rows, duplicate `(font_family, glyph)` bindings, unsupported source/packing labels, invalid output paths, empty glyph sets, non-RGBA8 glyph pixels, and incorrect pixel byte counts.
- Keep package updates deterministic and safe: no filesystem writes happen from `plan_*`, and `apply_*` must leave existing files unchanged when validation fails.

## Done When

- Asset metadata tests prove `GameEngine.UiAtlas.v1` serializes/deserializes structured glyph rows and rejects invalid/duplicate glyph rows.
- Runtime/UI renderer tests prove a loaded runtime package can build a `UiRendererGlyphAtlasPalette` from glyph metadata, and rejects missing/non-texture atlas pages.
- Tools tests prove rasterized glyph RGBA8 rows pack into a cooked texture page and glyph metadata, package index updates include texture and atlas entries, and invalid glyph pixels/path collisions fail closed.
- Static checks require the new API/docs evidence.
- `production-ui-importer-platform-adapters` still does not claim real font loading/rasterization, shaping, native IME/text-input sessions, renderer texture upload, platform SDK behavior, or third-party adapter readiness.
- Focused tests, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice-closing commit.

## File Plan

- Modify `engine/assets/include/mirakana/assets/ui_atlas_metadata.hpp`: add `UiAtlasMetadataGlyph` and glyph diagnostics.
- Modify `engine/assets/src/ui_atlas_metadata.cpp`: validate, serialize, deserialize, and canonicalize glyph rows.
- Modify `engine/runtime/include/mirakana/runtime/asset_runtime.hpp` and `engine/runtime/src/asset_runtime.cpp`: add runtime glyph payload rows.
- Modify `engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp` and `engine/ui_renderer/src/ui_renderer.cpp`: add glyph palette build result/failure and runtime package palette builder.
- Modify `engine/tools/include/mirakana/tools/ui_atlas_tool.hpp` and `engine/tools/src/ui_atlas_tool.cpp`: add packed rasterized glyph atlas authoring/package update/apply contracts.
- Modify `tests/unit/asset_identity_runtime_resource_tests.cpp`, `tests/unit/runtime_tests.cpp`, `tests/unit/ui_renderer_tests.cpp`, and `tests/unit/tools_tests.cpp`: add RED tests for glyph metadata, runtime payload, palette builder, and package bridge behavior.
- Modify `docs/ui.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`, `docs/superpowers/plans/README.md`, `engine/agent/manifest.json`, `tools/check-ai-integration.ps1`, and `tools/check-json-contracts.ps1` after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add `asset_identity_runtime_resource_tests` coverage for `UiAtlasMetadataGlyph` serialization/deserialization and duplicate glyph diagnostics.
- [x] Add `runtime_tests` coverage proving `runtime_ui_atlas_payload` exposes glyph rows and requires glyph page dependencies.
- [x] Add `ui_renderer_tests` coverage proving `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas` builds `UiRendererGlyphAtlasPalette` and rejects non-texture page assets.
- [x] Add `tools_tests` coverage proving `author_packed_ui_glyph_atlas_from_rasterized_glyphs` creates a 2-pixel RGBA8 texture page, glyph metadata rows, and package updates.
- [x] Add `tools_tests` coverage proving invalid glyph pixels and output path collisions fail closed.
- [x] Run focused test targets and record the expected RED compile/test failures.

### Task 2: Implement Metadata, Runtime, Renderer, And Tooling

- [x] Add `UiAtlasMetadataGlyph`, diagnostics, serialization, deserialization, validation, and canonical sorting.
- [x] Add `RuntimeUiAtlasGlyph` and parse glyph rows in `runtime_ui_atlas_payload`.
- [x] Add `UiRendererGlyphAtlasPaletteBuildResult` and `build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas`.
- [x] Add packed rasterized glyph atlas authoring/package update/apply APIs in `mirakana_tools`.
- [x] Run focused tests until green.

### Task 3: Docs, Manifest, And Static Contract Sync

- [x] Update current-truth UI, testing, capabilities, roadmap, AI guidance, gap analysis, master plan, and plan registry text.
- [x] Update `engine/agent/manifest.json` current active/recommended plan text and unsupported gap notes.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require glyph atlas package bridge evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run focused CMake build/test for changed C++ targets.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused tests | Expected failure | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_runtime_tests MK_ui_renderer_tests MK_tools_tests` failed because `UiAtlasMetadataGlyph`, `UiAtlasMetadataDocument::glyphs`, glyph diagnostics, runtime glyph payload rows, glyph palette builder, and packed glyph atlas tooling APIs are not implemented yet. |
| Focused CMake build/test | Pass | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_asset_identity_runtime_resource_tests MK_runtime_tests MK_ui_renderer_tests MK_tools_tests MK_runtime_host_tests MK_core_tests` passed, followed by focused `ctest` for `MK_core_tests`, `MK_asset_identity_runtime_resource_tests`, `MK_runtime_tests`, `MK_runtime_host_tests`, `MK_ui_renderer_tests`, and `MK_tools_tests`. |
| Review hardening focused tests | Pass | Added and passed alias, transactional rollback, existing package-index path validation, and `RootedFileSystem` file/directory removal regression coverage after cpp-reviewer findings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-ui-importer-platform-adapters` remains `planned`; host/platform font, shaping, upload, and accessibility work remains unsupported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok`. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/ui_atlas_metadata.cpp,engine/platform/src/filesystem.cpp,engine/runtime/src/asset_runtime.cpp,engine/tools/src/ui_atlas_tool.cpp,engine/ui_renderer/src/ui_renderer.cpp -MaxFiles 5` completed with existing analyzer warnings and script result `tidy-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok`. |
| `git diff --check` | Pass | Whitespace check passed; Git line-ending warnings are from existing workspace settings. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed. Windows reports Metal/iOS lanes as diagnostic-only / host-gated; CTest passed 29/29. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Full dev build passed after validation. |
