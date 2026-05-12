# Runtime UI IME Composition Publish Plan v1 Implementation Plan (2026-05-07)

**Status:** Completed

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `mirakana_ui` IME composition publish plan so composition updates are validated before an IME adapter receives them.

**Architecture:** Keep `mirakana_ui` independent from OS text-input APIs. The new contract plans and gates publication to the existing `IImeAdapter` boundary, but does not implement Win32/TSF, Cocoa text input, ibus/Fcitx, Android IME, iOS text input, or any native handle bridge.

**Tech Stack:** C++23 `mirakana_ui`, existing first-party UI tests, existing validation scripts.

---

## Goal

- Add `ImeCompositionPublishPlan` / `ImeCompositionPublishResult` to `mirakana/ui/ui.hpp`.
- Add `plan_ime_composition_update` to validate the existing `ImeComposition` value before adapter dispatch.
- Add `publish_ime_composition` to call `IImeAdapter::update_composition` only when the composition is valid.
- Update docs, manifest, and static checks so `production-ui-importer-platform-adapters` records this host-independent IME publish contract while OS IME/text-input adapters remain unsupported.

## Context

- Master plan gap: `production-ui-importer-platform-adapters`.
- Existing `mirakana_ui` already declares `ImeComposition` and `IImeAdapter`, but there is no reviewed plan/result helper to prevent invalid rows from reaching a host adapter.
- `ImeComposition::cursor_index` is treated as a string code-unit/byte index for this host-independent contract. Unicode grapheme navigation, platform preedit ranges, candidate windows, and native text-input sessions remain adapter work.

## Constraints

- Do not add third-party dependencies or platform SDK calls.
- Do not expose OS handles, native text-input objects, or middleware APIs.
- Do not claim OS IME/text-input readiness.
- Keep empty `composition_text` valid so adapters can receive composition-clear updates.
- Keep the implementation deterministic and testable on the default Windows host.

## Done When

- Unit tests prove valid IME composition updates publish once through a supplied adapter.
- Unit tests prove empty targets and cursor indexes beyond `composition_text.size()` block adapter publication.
- `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` (previously `mirakana_ui_renderer_tests`) passes, followed by required static/final validation.
- `production-ui-importer-platform-adapters` remains `planned` or explicitly non-ready for OS IME work while recording this host-independent foundation.

## File Plan

- Modify `engine/ui/include/mirakana/ui/ui.hpp`: add IME publish plan/result contracts, diagnostics, and functions.
- Modify `engine/ui/src/ui.cpp`: implement planning, result status, and safe adapter dispatch.
- Modify `tests/unit/ui_renderer_tests.cpp`: add RED tests for valid publish and invalid diagnostic blocking.
- Modify docs/manifest/static checks after behavior is green.

## Tasks

### Task 1: RED Tests

- [x] Add a capture `IImeAdapter` test helper.
- [x] Add a test for valid IME composition publish through the capture adapter.
- [x] Add a test for invalid empty-target and out-of-range cursor blocking adapter publication.
- [x] Run focused `MK_ui_renderer_tests` and record the expected compile/test failure.

### Task 2: Implement IME Publish Contract

- [x] Add `invalid_ime_target` and `invalid_ime_cursor` diagnostics.
- [x] Add `ImeCompositionPublishPlan` and `ImeCompositionPublishResult`.
- [x] Implement `plan_ime_composition_update`.
- [x] Implement `publish_ime_composition`.
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
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused `MK_ui_renderer_tests` | Expected failure | `cmake --build --preset dev --target MK_ui_renderer_tests` failed because `plan_ime_composition_update`, `publish_ime_composition`, `invalid_ime_target`, and `invalid_ime_cursor` are not implemented yet. |
| Focused `MK_ui_renderer_tests` | Pass | `cmake --build --preset dev --target MK_ui_renderer_tests` and `ctest --preset dev --output-on-failure -R MK_ui_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `production-readiness-audit-check: ok`; gap remains planned for OS/platform adapter work. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `format-check: ok`. |
| `git diff --check` | Pass | No whitespace errors; Git reported LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | `validate: ok`; CTest passed 29/29. Metal/Apple host checks remain diagnostic/host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` completed successfully with the default `dev` preset. |
