# Runtime UI SDL3 Text Editing Candidate Event Translation v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Translate SDL3 IME text editing candidate events into first-party `mirakana_platform_sdl3` event rows without retaining SDL-owned string pointers.

**Plan ID:** `runtime-ui-sdl3-text-editing-candidate-event-translation-v1`

**Status:** Completed

**Architecture:** Keep SDL3-specific IME candidate payloads inside `mirakana_platform_sdl3` and expose copied UTF-8 candidate rows through the existing first-party `SdlWindowEvent` contract. This extends the SDL3 platform adapter gap only; UI-side composition publishing, native IME services, and candidate UI remain follow-up work.

**Tech Stack:** C++23, SDL3 official event APIs, `mirakana_platform_sdl3`, `tests/unit/sdl3_platform_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `production-ui-importer-platform-adapters`, currently `planned`.
- Runtime UI SDL3 Text Input Event Translation v1 already copies `SDL_EVENT_TEXT_INPUT` and `SDL_EVENT_TEXT_EDITING` text rows into `SdlWindowEvent`.
- Context7 confirms SDL3 event-owned text memory must be copied when retained. Local SDL3 3.2 headers define `SDL_TextEditingCandidatesEvent` as `event.edit_candidates.*` with `windowID`, `const char * const *candidates`, `num_candidates`, `selected_candidate`, and `horizontal`.

## Constraints

- Do not include SDL3 headers in `mirakana_ui` or add SDL/native handles to UI/game contracts.
- Copy SDL candidate strings immediately into first-party storage; do not retain SDL-owned pointers.
- Keep null candidate lists as empty candidate rows and preserve deterministic metadata.
- Do not claim text editing model mutation, candidate UI, native IME service parity, virtual keyboard policy, shaping, font rasterization, accessibility bridge publication, or cross-platform text services.
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

### Task 1: RED tests for SDL3 editing candidate event translation

- [x] Add a test that builds an `SDL_EVENT_TEXT_EDITING_CANDIDATES` row with two UTF-8 candidate strings, `windowID`, selected candidate index, and horizontal list metadata.
- [x] Mutate the source candidate buffers after translation and prove the translated row retained copied `std::string` values.
- [x] Add a test that treats a null SDL candidate list as an empty first-party candidate vector.
- [x] Run `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` and confirm failure because the event kind/fields do not exist yet.

### Task 2: Minimal SDL3 event translation implementation

- [x] Add `text_editing_candidates` to `SdlWindowEventKind`.
- [x] Add copied candidate storage and metadata fields to `SdlWindowEvent`.
- [x] Translate `SDL_EVENT_TEXT_EDITING_CANDIDATES` from `source.edit_candidates.windowID`, copied candidates, `selected_candidate`, and `horizontal`.
- [x] Remove the inaccurate `noexcept` from `sdl3_translate_window_event`, because copied text and candidate rows allocate first-party storage.
- [x] Run the focused SDL3 platform test target and CTest lane.

### Task 3: Documentation, manifest, and validation

- [x] Mark this plan completed with focused validation evidence.
- [x] Update plan registry latest completed slice.
- [x] Update current-truth docs and manifest notes without marking the broad adapter gap ready.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Commit the coherent passing slice.

## Validation Evidence

| Command | Result |
| --- | --- |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` after RED tests | Failed as expected: `SdlWindowEventKind::text_editing_candidates`, `SdlWindowEvent::text_editing_candidates`, `selected_text_editing_candidate`, and `text_editing_candidates_horizontal` did not exist yet. |
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

- Added copied first-party SDL3 IME candidate rows to `mirakana::SdlWindowEvent`.
- Kept `mirakana_ui` free of SDL3 headers and native handles.
- Did not mark the broad `production-ui-importer-platform-adapters` gap ready; native IME services, UI-side composition/candidate publication, virtual keyboard behavior, text editing model mutation, text shaping, font rasterization, OS accessibility, and broader platform adapters remain follow-up work.
