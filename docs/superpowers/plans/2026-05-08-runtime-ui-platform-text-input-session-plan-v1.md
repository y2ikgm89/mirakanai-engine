# Runtime UI Platform Text Input Session Plan v1 Implementation Plan (2026-05-08)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` platform text-input session plan so text-input begin/end requests are validated before a platform integration adapter receives them.

**Architecture:** Keep `mirakana_ui` independent from native text-input APIs. The new contract plans and gates calls to the existing `IPlatformIntegrationAdapter` boundary, but does not implement Win32/TSF, Cocoa text input, ibus/Fcitx, Android/iOS IME sessions, native text-input object/session ownership, virtual keyboard control, or native handle bridging.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `PlatformTextInputSessionPlan` / `PlatformTextInputSessionResult` to `mirakana/ui/ui.hpp`.
- Add `PlatformTextInputEndPlan` / `PlatformTextInputEndResult` for explicit session end validation.
- Add `plan_platform_text_input_session`, `begin_platform_text_input`, `plan_platform_text_input_end`, and `end_platform_text_input`.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent text-input session boundary while native platform sessions remain unsupported.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` already declares `PlatformTextInputRequest` and `IPlatformIntegrationAdapter`, but callers can dispatch empty targets or invalid text bounds directly.
- Existing IME composition publication is separate: composition rows describe preedit text, while platform text-input sessions describe when a host adapter should begin or end native text entry for a first-party UI element.

## Constraints

- Do not add third-party dependencies, platform SDK calls, native text-input object/session ownership, virtual keyboard APIs, or OS handles.
- Do not claim native IME/text-input session readiness.
- Treat an empty target id as invalid for begin and end.
- Treat non-finite, negative, zero-width, or zero-height text bounds as invalid for begin.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid begin requests dispatch once through a supplied `IPlatformIntegrationAdapter`.
- Unit tests prove invalid begin requests block adapter dispatch and report target/bounds diagnostics.
- Unit tests prove valid end requests dispatch once and invalid end targets are blocked.
- `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passes, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for native platform session work while recording this host-independent foundation.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add diagnostics, platform text-input plan/result contracts, and helper declarations.
- Modify `engine/ui/src/ui.cpp`: implement planning, result status, and safe adapter dispatch.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid begin/end dispatch and invalid diagnostic blocking.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add a capture `IPlatformIntegrationAdapter` test helper.
- [x] Add a test for valid text-input begin dispatch through the capture adapter.
- [x] Add a test for invalid begin target/bounds blocking adapter dispatch.
- [x] Add a test for valid and invalid text-input end dispatch behavior.
- [x] Run focused `MK_ui_renderer_tests`.

### Task 2: Implement Platform Text-Input Session Contract

- [x] Add `invalid_platform_text_input_target` and `invalid_platform_text_input_bounds` diagnostics.
- [x] Add begin/end plan and result contracts.
- [x] Implement `plan_platform_text_input_session` and `begin_platform_text_input`.
- [x] Implement `plan_platform_text_input_end` and `end_platform_text_input`.
- [x] Run focused `MK_ui_renderer_tests`.

### Task 3: Docs And Static Contract Sync

- [x] Update current capabilities, UI docs, roadmap, master plan, registry, and manifest.
- [x] Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` to require the new API/docs evidence.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 4: Final Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_ui_renderer_tests` | Expected failure | Direct `cmake --build --preset dev --target MK_ui_renderer_tests` first hit the known Windows `Path`/`PATH` MSBuild environment issue; rerunning through `Invoke-CheckedCommand` with normalized process environment failed because `plan_platform_text_input_session`, `begin_platform_text_input`, `plan_platform_text_input_end`, `end_platform_text_input`, `invalid_platform_text_input_target`, and `invalid_platform_text_input_bounds` were not implemented yet. |
| Focused `MK_ui_renderer_tests` | Pass | Normalized `Invoke-CheckedCommand $tools.CMake --build --preset dev --target MK_ui_renderer_tests` and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`; new manifest gap/docs/header/source evidence was accepted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`; first run caught missing `PlatformTextInputSessionPlan` wording in `docs/ai-game-development.md`, then passed after the guidance was corrected. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-ui-importer-platform-adapters` remains `planned`; unsupported gap count remains 11. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Required public-header boundary check after changing `mirakana/ui/ui.hpp`. |
| Targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp -MaxFiles 2` completed with exit code 0; existing non-blocking diagnostics were printed for the touched UI files. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Initial run caught the new declaration wrapping; after formatting correction, the repository format check passed. |
| `git diff --check` | Pass | Whitespace check passed; Git printed existing LF-to-CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; CTest reported 29/29 passing. Windows host-gated Apple/Metal diagnostics remain explicit non-ready gates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Full dev preset configure/generate/build completed successfully before the slice commit. |
