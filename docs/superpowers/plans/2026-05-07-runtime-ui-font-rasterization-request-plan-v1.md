# Runtime UI Font Rasterization Request Plan v1 Implementation Plan (2026-05-07)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` font rasterization request plan so glyph rasterization requests are validated before a font adapter receives them.

**Architecture:** Keep `mirakana_ui` independent from font libraries, OS font APIs, glyph atlas allocators, renderer texture upload, and font licensing systems. The new contract plans and gates publication to the existing `IFontRasterizerAdapter` boundary, but does not load fonts, rasterize glyphs itself, allocate atlas pages, or expose native handles.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `FontRasterizationRequestPlan` / `FontRasterizationResult` to `mirakana/ui/ui.hpp`.
- Add `plan_font_rasterization_request` to validate the existing `FontRasterizationRequest` value before adapter dispatch.
- Add `rasterize_font_glyph` to call `IFontRasterizerAdapter::rasterize_glyph` only when the request is valid and diagnose invalid adapter-returned `GlyphAtlasAllocation` rows.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent font rasterization boundary while real font loading/rasterization/atlas generation remains adapter work.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` already declares `FontRasterizationRequest`, `GlyphAtlasAllocation`, and `IFontRasterizerAdapter`, but there is no reviewed request/result helper to prevent invalid rows from reaching a font adapter.
- Current `mirakana_ui_renderer` consumes already-cooked glyph atlas metadata through `UiRendererGlyphAtlasPalette`; this plan does not generate that metadata or upload textures.

## Constraints

- Do not add third-party dependencies, platform SDK calls, font files, or font licensing records.
- Do not implement text shaping, bidirectional reordering, font fallback, glyph metrics, glyph atlas packing, or renderer texture upload.
- Do not expose OS handles, font-library handles, glyph-atlas internals, middleware APIs, or native renderer resources.
- Treat `glyph == 0`, empty `font_family`, and non-positive or non-finite `pixel_size` as invalid request data for this host-independent gate.
- Treat adapter allocations with a mismatched glyph or invalid/non-positive atlas bounds as `invalid_font_allocation`.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid font rasterization requests dispatch once through a supplied adapter and return the adapter allocation.
- Unit tests prove empty font family, zero glyph id, and invalid pixel size block adapter publication.
- Unit tests prove invalid adapter-returned allocations are reported without claiming adapter success.
- `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` (formerly `mirakana_ui_renderer_tests`) passes, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for real font loading/rasterization/glyph atlas work while recording this host-independent boundary without font loading/rasterization implementations.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add font rasterization request plan/result contracts, diagnostics, and functions.
- Modify `engine/ui/src/ui.cpp`: implement planning, result status, and safe adapter dispatch.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid dispatch, invalid request diagnostic blocking, and invalid adapter allocation diagnostics.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add a capture `IFontRasterizerAdapter` test helper.
- [x] Add a test for valid font rasterization request dispatch through the capture adapter.
- [x] Add a test for invalid empty-family, zero-glyph, and invalid pixel-size blocking adapter publication.
- [x] Add a test for invalid adapter-returned glyph atlas allocation reporting.
- [x] Run focused `MK_ui_renderer_tests` and record the expected compile/test failure.

### Task 2: Implement Font Rasterization Request Contract

- [x] Add `invalid_font_family`, `invalid_font_glyph`, `invalid_font_pixel_size`, and `invalid_font_allocation` diagnostics.
- [x] Add `FontRasterizationRequestPlan` and `FontRasterizationResult`.
- [x] Implement `plan_font_rasterization_request`.
- [x] Implement `rasterize_font_glyph` with adapter allocation validation.
- [x] Run focused `MK_ui_renderer_tests`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, UI docs, roadmap, master plan, registry, agent guidance, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new API/docs evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_ui_renderer_tests` | Expected failure | `cmake --build --preset dev --target MK_ui_renderer_tests` failed because `plan_font_rasterization_request`, `rasterize_font_glyph`, `invalid_font_family`, `invalid_font_glyph`, and `invalid_font_pixel_size` are not implemented yet. |
| RED adapter allocation `MK_ui_renderer_tests` | Expected failure | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_ui_renderer_tests` failed because `invalid_font_allocation` was not implemented yet. |
| Focused `MK_ui_renderer_tests` | Pass | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_ui_renderer_tests` and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; `production-ui-importer-platform-adapters` remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok` after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format to the touched UI C++ files. |
| `git diff --check` | Pass | No whitespace errors; Git reported existing CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; all 29 CTest tests passed, including `MK_ui_renderer_tests`. Diagnostic host gates remained explicit for Metal/Apple lanes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default `dev` preset configure/build completed. |
