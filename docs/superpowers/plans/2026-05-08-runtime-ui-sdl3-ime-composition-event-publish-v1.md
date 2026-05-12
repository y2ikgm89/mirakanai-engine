# Runtime UI SDL3 IME Composition Event Publish v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Publish copied SDL3 text editing composition rows through the first-party `mirakana_ui` `IImeAdapter` contract without moving SDL3 into runtime UI.

**Plan ID:** `runtime-ui-sdl3-ime-composition-event-publish-v1`

**Status:** Completed

**Architecture:** Keep SDL3 event ownership and UTF-8 event interpretation inside `mirakana_platform_sdl3`, then hand a validated `mirakana::ui::ImeComposition` to `mirakana_ui` publication helpers. SDL3 reports editing cursor positions in UTF-8 character units; the bridge converts that position into the byte offset expected by the current `ImeComposition::cursor_index` validation.

**Tech Stack:** C++23, SDL3 official text editing event APIs, `mirakana_platform_sdl3`, `mirakana_ui`, `tests/unit/sdl3_platform_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `production-ui-importer-platform-adapters`, currently `planned`.
- Runtime UI IME Composition Publish Plan v1 already provides host-independent `ImeCompositionPublishPlan`, `ImeCompositionPublishResult`, `plan_ime_composition_update`, and `publish_ime_composition`.
- Runtime UI SDL3 Text Input Event Translation v1 already copies `SDL_EVENT_TEXT_EDITING` text and range metadata into `SdlWindowEvent`.
- Context7 SDL wiki evidence for `SDL_TextEditingEvent` says `start` is a UTF-8 character position and `length` is a UTF-8 character count.

## Constraints

- Do not include SDL3 headers in `mirakana_ui` or expose SDL/native handles to UI/game code.
- Do not retain SDL-owned text pointers; use already copied `SdlWindowEvent::text`.
- Convert SDL UTF-8 character cursor positions to byte offsets before calling `mirakana_ui`.
- Treat unset negative SDL cursor rows as the start of the copied composition and oversized rows as the end.
- Ignore non-`text_editing` rows without adapter dispatch.
- Do not claim committed text insertion, text editing model mutation, candidate UI, native IME service parity, virtual keyboard policy, OS accessibility bridge publication, cross-platform text services, production shaping, font rasterization, renderer texture upload, or new third-party adapters.
- No new third-party dependencies.
- New behavior must be covered by RED tests before implementation.

## Files

- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify after green: `docs/superpowers/plans/README.md`
- Modify after green: `docs/current-capabilities.md`
- Modify after green: `docs/roadmap.md`
- Modify after green: `docs/ui.md`
- Modify after green: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify after green: `engine/agent/manifest.json`

## Tasks

### Task 1: RED tests for SDL3 text editing composition publication

- [x] Add a capturing `IImeAdapter` test helper in `tests/unit/sdl3_platform_tests.cpp`.
- [x] Add a test proving `sdl3_ime_composition_from_window_event` converts an `SDL_EVENT_TEXT_EDITING` row with UTF-8 text and `start=1` into `ImeComposition::cursor_index == 3`.
- [x] Add a test proving `publish_sdl3_ime_composition_event` dispatches a valid text editing row through `mirakana::ui::publish_ime_composition`.
- [x] Add a test proving non-editing rows are ignored without adapter dispatch.
- [x] Add a test proving unset negative cursor rows clamp to `0` and oversized cursor rows clamp to `composition_text.size()`.
- [x] Run `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` and confirm failure because the new bridge functions do not exist yet.

### Task 2: Minimal SDL3 IME composition bridge

- [x] Add `sdl3_ime_composition_from_window_event(const ui::ElementId&, const SdlWindowEvent&)` to the SDL3 platform integration header.
- [x] Add `publish_sdl3_ime_composition_event(ui::IImeAdapter&, const ui::ElementId&, const SdlWindowEvent&)` to the same header.
- [x] Implement UTF-8 character index to byte offset conversion in the SDL3 integration source without rejecting malformed byte rows more strictly than the existing text copy bridge.
- [x] Return `std::nullopt` and no-op publication for non-`text_editing` rows.
- [x] Delegate validation and adapter dispatch to `mirakana::ui::publish_ime_composition`.
- [x] Run the focused SDL3 platform test target and CTest lane.

### Task 3: Documentation, manifest, and validation

- [x] Mark this plan completed with focused validation evidence.
- [x] Update plan registry latest completed slice.
- [x] Update current-truth docs and manifest notes without marking the broad adapter gap ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Prepare the coherent passing slice for a validated commit checkpoint.

## Validation Evidence

| Command | Result |
| --- | --- |
| Context7 SDL wiki lookup for `SDL_TextEditingEvent` / `SDL_TextEditingCandidatesEvent` | PASS - official SDL docs confirmed text editing `start`/`length` are UTF-8 character units and candidates are copied row data owned by SDL. |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` before implementation | RED - failed as expected because `mirakana::sdl3_ime_composition_from_window_event` and `mirakana::publish_sdl3_ime_composition_event` did not exist. |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` | PASS. |
| `ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"` | PASS - 1/1 SDL3 platform test passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | PASS - audit still reports `unsupported_gaps=11`; the broad `production-ui-importer-platform-adapters` gap remains `planned`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2` | PASS after setting the MSVC/UCRT `INCLUDE` environment in ordinary PowerShell. The first run proved the host shell lacked system include paths for Visual Studio `clang-tidy` (`crtdbg.h` not found), not a source issue. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS - `validate: ok`, 29/29 dev CTest tests passed, production audit remains `unsupported_gaps=11`; Metal/Apple lanes remain diagnostic host gates on Windows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | PASS. |

## Completion Notes

- SDL3 text editing composition rows can now be transformed from copied `SdlWindowEvent` rows into first-party `mirakana::ui::ImeComposition` values and published through `mirakana::ui::IImeAdapter`.
- The bridge converts SDL UTF-8 character cursor positions into byte offsets before UI validation. Negative cursor rows clamp to `0`; oversized cursor rows clamp to the copied composition byte length.
- Non-`text_editing` rows are ignored without diagnostics or adapter dispatch. Invalid UI targets are rejected by the existing `mirakana_ui` publication helper.
- This slice intentionally does not implement committed text insertion, text model mutation, candidate UI, native IME parity, virtual keyboard policy, accessibility bridge publication, cross-platform text services, production shaping, font rasterization, renderer texture upload, or third-party adapters.
