# Runtime UI SDL3 Text Input Event Translation v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Translate SDL3 text input and editing events into first-party `mirakana_platform_sdl3` event rows without exposing SDL3 or native handles to UI/game contracts.

**Plan ID:** `runtime-ui-sdl3-text-input-event-translation-v1`

**Status:** Completed

**Architecture:** Keep `mirakana_ui` SDL3-free and keep SDL3 event memory ownership inside `mirakana_platform_sdl3`. Extend `SdlWindowEvent` with copied UTF-8 text rows for committed text and IME editing composition metadata; event publication into the UI text model remains a future adapter slice.

**Tech Stack:** C++23, SDL3 official keyboard/text input event APIs, `mirakana_platform_sdl3`, `tests/unit/sdl3_platform_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `production-ui-importer-platform-adapters`, currently `planned`.
- Runtime UI SDL3 Platform Text Input Adapter v1 can start and stop SDL text input for a caller-owned `SdlWindow`.
- Context7 and local SDL3 headers confirm `SDL_StartTextInput(SDL_Window*)` enables `SDL_EVENT_TEXT_INPUT` and `SDL_EVENT_TEXT_EDITING`; SDL owns event text memory, so retained text must be copied.
- Local SDL3 3.2 headers define `SDL_TextInputEvent::windowID` and UTF-8 `text`, plus `SDL_TextEditingEvent::windowID`, UTF-8 `text`, `start`, and `length`.

## Constraints

- Do not include SDL3 headers in `mirakana_ui` or add SDL/native handles to UI/game contracts.
- Copy SDL event text immediately into first-party storage; do not retain SDL-owned pointers.
- Do not claim text editing model mutation, IME candidate UI, native IME service parity, virtual keyboard policy, shaping, font rasterization, accessibility bridge publication, or cross-platform text services.
- No new third-party dependencies.
- New behavior must be covered by RED tests before implementation.

## Files

- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify after green: `docs/superpowers/plans/README.md`
- Modify after green: `docs/current-capabilities.md`
- Modify after green: `docs/roadmap.md`
- Modify after green: `docs/ui.md`
- Modify after green: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify after green: `engine/agent/manifest.json`

## Tasks

### Task 1: RED tests for SDL3 text input/editing event translation

- [x] Add a test that builds an `SDL_EVENT_TEXT_INPUT` row with `windowID` and UTF-8 text, calls `sdl3_translate_window_event`, and expects `SdlWindowEventKind::text_input`, the copied `window_id`, and copied `text`.
- [x] Add a test that mutates the original SDL text pointer after translation and proves the translated row retained its own copied `std::string`.
- [x] Add a test that builds an `SDL_EVENT_TEXT_EDITING` row with UTF-8 composition text, `start`, and `length`, and expects `SdlWindowEventKind::text_editing` plus copied text and composition range fields.
- [x] Run `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` and confirm failure because the event kinds/fields do not exist yet.

### Task 2: Minimal SDL3 event translation implementation

- [x] Add `text_input` and `text_editing` to `SdlWindowEventKind`.
- [x] Add `std::string text`, `std::int32_t text_edit_start`, and `std::int32_t text_edit_length` to `SdlWindowEvent`.
- [x] Translate `SDL_EVENT_TEXT_INPUT` from `source.text.windowID` and copied `source.text.text`.
- [x] Translate `SDL_EVENT_TEXT_EDITING` from `source.edit.windowID`, copied `source.edit.text`, `source.edit.start`, and `source.edit.length`.
- [x] Treat null SDL text pointers as empty strings.
- [x] Run the focused SDL3 platform test target and CTest lane.

### Task 3: Documentation, manifest, and validation

- [x] Mark this plan completed with focused validation evidence.
- [x] Update plan registry latest completed slice.
- [x] Update current-truth docs and manifest notes without marking the broad adapter gap ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Commit the coherent passing slice.

## Validation Evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` after RED tests | Failed as expected: `SdlWindowEventKind::text_input`, `SdlWindowEventKind::text_editing`, `SdlWindowEvent::text`, `text_edit_start`, and `text_edit_length` did not exist yet. |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` | Passed. |
| `ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"` | Passed: 1/1 tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed; `unsupported_gaps=11`, `production-ui-importer-platform-adapters` remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_window.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2` | Passed with existing warning profile; `tidy-check: ok (2 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; 29/29 default dev tests passed. Apple/Metal host gates remained diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed. |

## Completion Notes

- Added copied first-party text rows to `mirakana::SdlWindowEvent` for SDL3 committed text and editing composition events.
- Kept `mirakana_ui` free of SDL3 headers and native handles.
- Did not mark the broad `production-ui-importer-platform-adapters` gap ready; native IME services, UI-side composition publication, virtual keyboard behavior, text editing model mutation, text shaping, font rasterization, OS accessibility, and broader platform adapters remain follow-up work.
