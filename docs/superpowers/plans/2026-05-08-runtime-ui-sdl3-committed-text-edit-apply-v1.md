# Runtime UI SDL3 Committed Text Edit Apply v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Apply copied SDL3 committed text-input rows to a first-party `mirakana_ui` text edit state without moving SDL3 into runtime UI.

**Plan ID:** `runtime-ui-sdl3-committed-text-edit-apply-v1`

**Status:** Completed

**Architecture:** Add a host-independent `mirakana::ui::TextEditState` plus committed text input planning/apply helpers that validate targets, cursor/selection ranges, UTF-8 scalar boundaries, and committed UTF-8 text before mutating state. Keep SDL3 event ownership in `mirakana_platform_sdl3` by adding pure bridge helpers that convert copied `SdlWindowEventKind::text_input` rows into `mirakana::ui::CommittedTextInput` and delegate state application to `mirakana_ui`.

**Tech Stack:** C++23, SDL3 official text input event APIs, `mirakana_ui`, `mirakana_platform_sdl3`, `tests/unit/ui_renderer_tests.cpp`, `tests/unit/sdl3_platform_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `production-ui-importer-platform-adapters`, currently `planned`.
- Runtime UI SDL3 Text Input Event Translation v1 already copies `SDL_EVENT_TEXT_INPUT` text into first-party `SdlWindowEvent::text`.
- Runtime UI SDL3 IME Composition Event Publish v1 publishes copied `SDL_EVENT_TEXT_EDITING` rows through `IImeAdapter`.
- Context7 SDL wiki evidence for `SDL_TextInputEvent` says the event contains UTF-8 input text; SDL best keyboard practices append `e.text.text` to the active text buffer after `SDL_StartTextInput`.

## Constraints

- Do not include SDL3 headers in `mirakana_ui`.
- Do not retain SDL-owned text pointers; use already copied `SdlWindowEvent::text`.
- Do not store native handles or SDL event pointers in `mirakana_ui` state.
- Treat `TextEditState::cursor_byte_offset` and `selection_byte_length` as byte offsets over the current UTF-8 text and validate scalar boundaries before applying committed text.
- Reject empty targets, mismatched targets, invalid cursor/selection ranges, split UTF-8 boundary ranges, and empty or malformed committed text.
- Ignore non-`text_input` rows in the SDL3 bridge without diagnostics or mutation.
- Do not claim full text editing widget behavior, deletion/backspace/navigation keys, candidate UI, IME composition rendering, selection UI, clipboard paste policy, virtual keyboard policy, OS accessibility bridge publication, production shaping, font rasterization, renderer texture upload, or cross-platform native IME parity.
- No new third-party dependencies.
- New behavior must be covered by RED tests before implementation.

## Files

- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
- Modify: `tests/unit/ui_renderer_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify after green: `docs/superpowers/plans/README.md`
- Modify after green: `docs/current-capabilities.md`
- Modify after green: `docs/roadmap.md`
- Modify after green: `docs/ui.md`
- Modify after green: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify after green: `engine/agent/manifest.json`

## Tasks

### Task 1: RED tests for committed text edit state

- [x] Add `TextEditState`, `CommittedTextInput`, `TextEditCommitPlan`, and `TextEditCommitResult` usage tests in `tests/unit/ui_renderer_tests.cpp`.
- [x] Add a test proving committed ASCII text inserts at `cursor_byte_offset` and moves the cursor to the end of the inserted bytes.
- [x] Add a test proving committed UTF-8 text replaces the selected byte range only when the cursor and selection end are UTF-8 scalar boundaries.
- [x] Add a test proving invalid target, mismatched target, out-of-range cursor/selection, split UTF-8 cursor/selection, empty committed text, and malformed committed UTF-8 are rejected without mutating the original state.
- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests` and confirm failure because the new UI text edit APIs do not exist yet.

### Task 2: Host-independent `mirakana_ui` committed text application

- [x] Add public structs `TextEditState`, `CommittedTextInput`, `TextEditCommitPlan`, and `TextEditCommitResult` to `engine/ui/include/mirakana/ui/ui.hpp`.
- [x] Add diagnostic codes for invalid text edit target, mismatched committed text target, invalid edit cursor, invalid edit selection, and invalid committed text.
- [x] Add public functions `plan_committed_text_input(const TextEditState&, const CommittedTextInput&)` and `apply_committed_text_input(const TextEditState&, const CommittedTextInput&)`.
- [x] Implement strict UTF-8 scalar-boundary validation and UTF-8 committed text validation inside `engine/ui/src/ui.cpp`.
- [x] Implement replacement as `state.text[0:cursor_byte_offset] + input.text + state.text[cursor_byte_offset + selection_byte_length:]`, update `cursor_byte_offset`, clear `selection_byte_length`, and preserve the target.
- [x] Run `cmake --build --preset dev --target mirakana_ui_renderer_tests` and `ctest --preset dev --output-on-failure -R "^mirakana_ui_renderer_tests$"`.

### Task 3: SDL3 committed text row bridge

- [x] Add `sdl3_committed_text_from_window_event(const ui::ElementId&, const SdlWindowEvent&)` to the SDL3 platform integration header.
- [x] Add `apply_sdl3_committed_text_event(const ui::TextEditState&, const ui::ElementId&, const SdlWindowEvent&)` to the same header.
- [x] Add tests in `tests/unit/sdl3_platform_tests.cpp` proving copied `SDL_EVENT_TEXT_INPUT` rows apply through `mirakana_ui`, non-`text_input` rows are ignored without diagnostics or mutation, and invalid targets are rejected by the `mirakana_ui` plan.
- [x] Implement the bridge in `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp` without adding SDL3 types to `mirakana_ui`.
- [x] Run `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` and `ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"`.

### Task 4: Documentation, manifest, and validation

- [x] Mark this plan completed with focused validation evidence.
- [x] Update plan registry latest completed slice.
- [x] Update current-truth docs and manifest notes without marking the broad adapter gap ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Prepare the coherent passing slice for a validated commit checkpoint.

## Validation Evidence

| Command | Result |
| --- | --- |
| Context7 SDL wiki lookup for `SDL_TextInputEvent` / `SDL_EVENT_TEXT_INPUT` | PASS - official SDL docs confirm the event text is UTF-8 input text and SDL best keyboard practices append it to the active text buffer. |
| `cmake --build --preset dev --target mirakana_ui_renderer_tests` (RED before implementation) | PASS - failed because `TextEditState`, `CommittedTextInput`, `plan_committed_text_input`, and `apply_committed_text_input` did not exist yet. |
| `cmake --build --preset dev --target mirakana_ui_renderer_tests` | PASS |
| `ctest --preset dev --output-on-failure -R "^mirakana_ui_renderer_tests$"` | PASS - 1/1 test executable passed. |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` | PASS |
| `ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"` | PASS - 1/1 test executable passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS - `unsupported_gaps=11`; `production-ui-importer-platform-adapters` remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset dev -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp -MaxFiles 2` | PASS - `tidy-check: ok (2 files)` with MSVC/UCRT `INCLUDE` configured. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2` | PASS - `tidy-check: ok (2 files)` with MSVC/UCRT `INCLUDE` configured. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS |

## Completion Notes

- Added host-independent `mirakana::ui::TextEditState`, `CommittedTextInput`, `TextEditCommitPlan`, `TextEditCommitResult`, `plan_committed_text_input`, and `apply_committed_text_input`.
- Added strict UTF-8 committed text validation plus byte-offset cursor and selection boundary validation before mutation.
- Added SDL3 bridge helpers `sdl3_committed_text_from_window_event` and `apply_sdl3_committed_text_event` over copied `SdlWindowEventKind::text_input` rows.
- Updated current-truth docs, the master plan ledger, and `engine/agent/manifest.json` while keeping full text editing widget behavior, deletion/backspace/navigation key handling, selection UI, candidate UI, virtual keyboard policy, OS accessibility bridge publication, production shaping/font/rasterization/upload, and cross-platform native IME parity unsupported.
