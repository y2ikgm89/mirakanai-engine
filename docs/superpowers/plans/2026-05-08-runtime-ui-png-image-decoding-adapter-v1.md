# Runtime UI PNG Image Decoding Adapter v1 Implementation Plan (2026-05-08)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional `MK_tools` PNG-backed `mirakana::ui::IImageDecodingAdapter` that routes reviewed PNG bytes through the existing audited libspng decode path.

**Architecture:** Keep `MK_ui` codec-free and host-independent; the dependency adapter lives in `MK_tools` beside `decode_audited_png_rgba8` and is compiled in both default and `asset-importers` builds. `PngImageDecodingAdapter` maps decoded `TextureSourceDocument` RGBA8 rows to `mirakana::ui::ImageDecodeResult`, returns `std::nullopt` on disabled/importer/decode failures, and lets `mirakana::ui::decode_image_request` produce the existing adapter-output diagnostic.

**Tech Stack:** C++23 `MK_tools`, `MK_ui`, existing optional `asset-importers` vcpkg feature (`libspng`), `MK_tools_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`.

---

## Goal

- Add `PngImageDecodingAdapter` to `mirakana/tools/source_image_decode.hpp`.
- Implement `PngImageDecodingAdapter::decode_image` in `source_image_decode.cpp` by calling `decode_audited_png_rgba8`.
- Add a public `MK_tools` dependency on `MK_ui` without making `MK_ui` depend on tools, codecs, or optional dependencies.
- Prove default builds fail closed when `MK_ENABLE_ASSET_IMPORTERS=OFF`.
- Prove the optional `asset-importers` lane decodes a known 1x1 PNG into `ImageDecodePixelFormat::rgba8_unorm`.

## Context

- `Runtime UI Image Decode Request Plan v1` added `ImageDecodeRequestPlan`, `ImageDecodeDispatchResult`, `plan_image_decode_request`, and `decode_image_request` around `IImageDecodingAdapter`, but intentionally left real codec work to adapters.
- `source-image-decode-and-atlas-packing-v1` already added `decode_audited_png_rgba8` using the optional, recorded `libspng` dependency.
- `production-ui-importer-platform-adapters` remains `planned` until production fonts/text/IME/accessibility/image/codecs/importers/platform release work is covered. This slice only narrows the PNG image adapter boundary.

## Constraints

- Do not add new third-party dependencies, image formats, platform SDK calls, SVG/vector parsing, atlas packing, renderer texture upload, mip generation, color management, streaming, or native GPU residency.
- Keep optional dependency installation in `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1`; CMake configure must continue using `VCPKG_MANIFEST_INSTALL=OFF`.
- Keep runtime/game code cooked-artifact-first. This adapter is a reviewed tools/host adapter for UI image decode bytes, not a runtime source-format parsing claim.
- Preserve existing invalid request validation in `MK_ui`; the PNG adapter should not duplicate URI/byte request checks.
- Catch decoder exceptions inside the adapter and return `std::nullopt` so adapter failures stay behind `decode_image_request` diagnostics.

## Done When

- Default `MK_tools_tests` compile and prove `PngImageDecodingAdapter` fails closed when importers are unavailable.
- `asset-importers` `MK_tools_tests` prove the adapter returns a 1x1 RGBA8 `ImageDecodeResult` for the known PNG fixture.
- Invalid PNG bytes return no image and are reported through `decode_image_request` without throwing through the UI adapter boundary.
- Docs, manifest, plan registry, and static checks record `PngImageDecodingAdapter` as a reviewed optional PNG adapter while `production-ui-importer-platform-adapters` remains non-ready for broader image/codecs/SVG/atlas/upload/platform work.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record a concrete host/dependency blocker.

## File Plan

- Modify `engine/tools/include/mirakana/tools/source_image_decode.hpp`: include the UI contract and declare `PngImageDecodingAdapter`.
- Modify `engine/tools/src/source_image_decode.cpp`: implement adapter byte conversion, exception-safe dispatch, and RGBA8 result mapping.
- Modify `engine/tools/CMakeLists.txt`: add the public `MK_ui` dependency for the new public adapter type.
- Modify `tests/unit/tools_tests.cpp`: add RED tests for default fail-closed behavior, optional PNG success, and invalid PNG failure.
- Update docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add `tools_tests.cpp` coverage for `PngImageDecodingAdapter` default fail-closed behavior.
- [x] Add enabled-importer coverage for 1x1 PNG success and invalid PNG failure.
- [x] Run focused default `MK_tools_tests` and record the expected compile failure.

### Task 2: Implement PNG UI Adapter

- [x] Declare `PngImageDecodingAdapter` in `source_image_decode.hpp`.
- [x] Add `MK_ui` as a public `MK_tools` dependency.
- [x] Implement `PngImageDecodingAdapter::decode_image`.
- [x] Run focused default `MK_tools_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, UI docs, dependency docs, roadmap, master plan, registry, agent guidance, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the adapter evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_tools_tests` | Expected failure | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_tools_tests` failed because `MK_tools_tests` could not include `mirakana/ui/ui.hpp` before `MK_tools` exposed the new UI adapter dependency. |
| Focused default `MK_tools_tests` | Pass | Normalized build/test for `MK_tools_tests` passed with `MK_ENABLE_ASSET_IMPORTERS=OFF`, proving fail-closed adapter behavior. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1` | Pass | Asset-importers preset built, 29/29 CTest tests passed, and installed SDK consumer validation passed; enabled tests proved 1x1 PNG decode through `PngImageDecodingAdapter`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok` after docs, manifest, and static JSON contract assertions were updated for `PngImageDecodingAdapter`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok` after Codex/Claude guidance, plan registry, manifest, and source/test evidence assertions were updated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; `production-ui-importer-platform-adapters` remains `planned` with the PNG adapter bounded as a reviewed optional lane. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok` after adding the public `MK_tools` adapter type and public `MK_ui` link. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Initial check requested clang-format for `source_image_decode.hpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` was applied and the rerun reported `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported only line-ending conversion warnings for dirty files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed, including JSON/agent/license/dependency/static checks, default `dev` configure/build, tidy-check, and 29/29 CTest tests. Existing Metal/Apple host evidence remains diagnostic/host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default `dev` configure/build completed after validation. |
