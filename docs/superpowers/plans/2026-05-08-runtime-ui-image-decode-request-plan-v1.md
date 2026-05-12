# Runtime UI Image Decode Request Plan v1 Implementation Plan (2026-05-08)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` image decode request plan so UI image decode bytes and adapter output are validated before/after `IImageDecodingAdapter` dispatch.

**Architecture:** Keep `mirakana_ui` independent from image codecs, SVG/vector parsers, platform imaging APIs, renderer texture upload, atlas packing, and native handles. The new contract validates first-party byte requests and adapter-returned RGBA8 image rows, but does not decode images itself or add a dependency.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `ImageDecodeRequestPlan` / `ImageDecodeDispatchResult` to `mirakana/ui/ui.hpp`.
- Add `plan_image_decode_request` to validate the existing `ImageDecodeRequest` value before adapter dispatch.
- Add `decode_image_request` to call `IImageDecodingAdapter::decode_image` only when the request is valid and diagnose missing or invalid adapter-returned `ImageDecodeResult` rows.
- Make `ImageDecodeResult` explicit about its pixel format so the host-independent validator can reject ambiguous pixel buffers.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent image decode boundary while real source image decoding, SVG/vector decoding, codec dependencies, atlas packing, and renderer upload remain adapter work.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` declares `ImageDecodeRequest`, `ImageDecodeResult`, and `IImageDecodingAdapter`, but there is no reviewed request/result helper to prevent invalid byte requests or invalid decoded images from being treated as usable.
- Existing `mirakana_ui_renderer` consumes already-cooked image bindings and UI atlas metadata; this plan does not generate textures, decode source files, or pack atlases.

## Constraints

- Do not add third-party dependencies, platform SDK calls, image codec code, SVG/vector parsing, image files, or renderer texture upload.
- Do not implement source image decoding, image dimension probing, atlas packing, mip generation, color management, streaming, or native GPU residency.
- Treat empty `asset_uri`, newline-bearing `asset_uri`, and empty request bytes as invalid request data for this host-independent gate.
- Treat adapter results as invalid when the adapter returns no image, zero dimensions, unsupported/unknown pixel format, or a pixel byte count that does not match the declared format and dimensions.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid image decode requests dispatch once through a supplied adapter and return the adapter image.
- Unit tests prove empty or invalid `asset_uri` and empty byte payloads block adapter dispatch.
- Unit tests prove missing adapter output and invalid adapter dimensions/pixels are reported without claiming decode success.
- `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passes, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for real image decoding/codecs/atlas packing while recording this host-independent request boundary without image decoding implementations.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add decoded image pixel-format metadata, image decode request plan/result contracts, diagnostics, and functions.
- Modify `engine/ui/src/ui.cpp`: implement request planning, result status, and safe adapter dispatch/output validation.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid dispatch, invalid request blocking, missing adapter output, and invalid decoded image diagnostics.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add capture and invalid `IImageDecodingAdapter` test helpers.
- [x] Add a test for valid image decode request dispatch through the capture adapter.
- [x] Add a test for invalid empty/newline asset URI and empty bytes blocking adapter dispatch.
- [x] Add a test for missing adapter output and invalid decoded dimensions/pixels reporting.
- [x] Run focused `MK_ui_renderer_tests` and record the expected compile/test failure.

### Task 2: Implement Image Decode Request Contract

- [x] Add image decode diagnostics (`invalid_image_decode_uri`, `empty_image_decode_bytes`, `invalid_image_decode_result`).
- [x] Add decoded image pixel-format metadata.
- [x] Add `ImageDecodeRequestPlan` and `ImageDecodeDispatchResult`.
- [x] Implement `plan_image_decode_request`.
- [x] Implement `decode_image_request` with adapter output validation.
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
| RED focused `MK_ui_renderer_tests` | Expected failure | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_ui_renderer_tests` failed because `plan_image_decode_request`, `decode_image_request`, `ImageDecodePixelFormat`, and image decode diagnostics are not implemented yet. |
| Focused `MK_ui_renderer_tests` | Pass | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_ui_renderer_tests` and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `tools/check-json-contracts.ps1` accepted the image decode request plan/docs/API evidence and non-ready importer/platform gap wording. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `tools/check-ai-integration.ps1` accepted manifest, docs, Codex/Claude agent guidance, and API/test evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-ui-importer-platform-adapters` remains `planned`; the audit reported 11 unsupported gaps and `production-readiness-audit-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Repository format wrapper reported `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported only line-ending conversion warnings for modified tracked files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation completed with 29/29 CTest tests passed; Apple/Metal diagnostics remained host-gated on Windows as expected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev preset configured and MSBuild completed all default targets. |
