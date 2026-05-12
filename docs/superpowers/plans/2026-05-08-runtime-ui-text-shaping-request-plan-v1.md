# Runtime UI Text Shaping Request Plan v1 Implementation Plan (2026-05-08)

**Plan ID:** `runtime-ui-text-shaping-request-plan-v1`  
**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` text shaping request plan so first-party text shaping requests and adapter-returned runs are validated before/after `ITextShapingAdapter` dispatch. Validated in `MK_ui_renderer_tests`.

**Architecture:** Keep `mirakana_ui` independent from shaping libraries, OS text APIs, font fallback systems, bidi engines, line-breaking engines, renderer texture upload, and native handles. The new contract plans and gates calls to the existing `ITextShapingAdapter` boundary, but does not shape text itself or add a dependency.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `TextShapingRequestPlan` / `TextShapingResult` to `mirakana/ui/ui.hpp`.
- Add `plan_text_shaping_request` to validate an existing `TextLayoutRequest` value before adapter dispatch.
- Add `shape_text_run` to call `ITextShapingAdapter::shape_text` only when the request is valid and diagnose invalid adapter-returned `TextLayoutRun` rows.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent text shaping adapter boundary while production shaping implementation remains adapter work.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` already declares `TextLayoutRequest`, `TextLayoutRun`, and `ITextShapingAdapter`, but there is no reviewed request/result helper to prevent invalid rows from reaching a text shaping adapter.
- Existing `MonospaceTextLayoutPolicy` remains a deterministic fallback/test layout policy. This plan is the reviewed boundary for a future real shaping adapter, not a replacement for production shaping libraries.

## Constraints

- Do not add third-party dependencies, platform SDK calls, font files, shaping code, bidi reordering, font fallback, production line breaking, renderer upload, or atlas allocation.
- Do not expose OS handles, font-library handles, shaping-engine handles, middleware APIs, SDL3, Dear ImGui, or native renderer resources.
- Treat empty or adapter-unsafe `text`, empty or adapter-unsafe `font_family`, and negative or non-finite `max_width` as invalid request data for this host-independent gate.
- Treat adapter results as invalid when no runs are returned, any run has empty/adapter-unsafe text, any run has invalid or non-positive bounds, or concatenated run text does not match the request text.
- Keep `TextDirection` and `TextWrapMode` policy decisions at the adapter contract boundary; this slice does not claim bidirectional reordering or production line breaking readiness.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid text shaping requests dispatch once through a supplied adapter and return adapter runs.
- Unit tests prove invalid text, invalid font family, and invalid max width block adapter dispatch.
- Unit tests prove missing, malformed, or text-mismatched adapter runs are reported without claiming shaping success.
- `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passes, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for production text shaping while recording this host-independent request boundary without shaping implementations.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add text shaping request plan/result contracts, diagnostics, and functions.
- Modify `engine/ui/src/ui.cpp`: implement request planning, result status, safe adapter dispatch, and output validation.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid dispatch, invalid request diagnostic blocking, and invalid adapter run diagnostics.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add capture and invalid `ITextShapingAdapter` test helpers.
- [x] Add a test for valid text shaping request dispatch through the capture adapter.
- [x] Add a test for invalid empty/newline text, invalid font family, and invalid max width blocking adapter dispatch.
- [x] Add a test for missing, invalid-bound, and text-mismatched adapter output reporting.
- [x] Run focused `MK_ui_renderer_tests` and record the expected compile/test failure.

### Task 2: Implement Text Shaping Request Contract

- [x] Add text shaping diagnostics (`invalid_text_shaping_text`, `invalid_text_shaping_font_family`, `invalid_text_shaping_max_width`, `invalid_text_shaping_result`).
- [x] Add `TextShapingRequestPlan` and `TextShapingResult`.
- [x] Implement `plan_text_shaping_request`.
- [x] Implement `shape_text_run` with adapter run validation.
- [x] Run focused `MK_ui_renderer_tests`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, UI docs, roadmap, master plan, registry, agent guidance, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new API/docs evidence.
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
| RED focused `MK_ui_renderer_tests` | Expected failure | `cmake --build --preset dev --target MK_ui_renderer_tests` failed because `plan_text_shaping_request`, `shape_text_run`, and text shaping diagnostics are not implemented yet. |
| Focused `MK_ui_renderer_tests` | Pass | `cmake --build --preset dev --target MK_ui_renderer_tests` and `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; `production-ui-importer-platform-adapters` remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | `public-api-boundary-check: ok` after adding public `mirakana_ui` text shaping request APIs. |
| Focused `tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp` | Pass | `tidy-check: ok (2 files)`; existing warning profile remains non-blocking. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Failed once before formatting `ui.cpp`; passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| Post-format focused `MK_ui_renderer_tests` | Pass | `cmake --build --preset dev --target MK_ui_renderer_tests` and `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed after formatting. |
| Subagent code review | Addressed | Review identified the untracked plan file, unchecked final validation, and thin edge-case tests; the plan file is part of the stage set, final validation remains gated below, and focused empty/unsafe/NaN request and adapter-run tests were added. |
| Review coverage focused `MK_ui_renderer_tests` | Pass | `cmake --build --preset dev --target MK_ui_renderer_tests` and `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed after adding empty text, unsafe font, NaN width, empty run text, and unsafe run text cases. |
| `git diff --check` | Pass | No whitespace errors; Git reported expected CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; 29/29 CTest tests passed, with Apple/Metal host diagnostics correctly reported as non-blocking on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` completed the default dev configure/build successfully. |
