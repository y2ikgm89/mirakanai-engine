# Runtime UI SDL3 Text Edit Command Key Events v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Map copied SDL3 key-press rows into first-party `mirakana_ui` text edit commands and apply them to `TextEditState` without moving SDL3 into `mirakana_ui`.

**Architecture:** Keep command mutation in `mirakana_ui` through `apply_text_edit_command`. Extend `mirakana_platform` key identity with the small set required for text editing, map SDL3 keycodes into those first-party keys, and add optional `mirakana_platform_sdl3` helper functions that convert `SdlWindowEvent` key rows into `TextEditCommand` rows. Repeated SDL key-down rows are accepted by the text-edit helper because repeat policy is owned by the host/SDL event source; `SdlWindow::handle_event` continues to ignore repeat for ordinary `VirtualInput` pressed/released state.

**Tech Stack:** C++23, `mirakana_platform`, `mirakana_platform_sdl3`, `mirakana_ui`, `sdl3_platform_tests`, PowerShell/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Status:** Completed on 2026-05-08. Focused SDL3 platform build/test, targeted tidy, schema/agent/audit checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed; the production readiness audit still honestly reports `unsupported_gaps=11`.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Latest related slice: `docs/superpowers/plans/2026-05-08-runtime-ui-text-edit-command-apply-v1.md`.
- Existing `mirakana_ui` command contract has `TextEditCommandKind`, `TextEditCommand`, `plan_text_edit_command`, and `apply_text_edit_command`.
- Existing `mirakana_platform_sdl3` text bridge already translates SDL text input, text editing, candidate events, IME composition publication, and committed text application.

## Constraints

- Keep `engine/ui` independent from SDL3, OS APIs, native handles, editor code, and middleware.
- Do not claim full text editing widgets, selection UI, clipboard, word movement, grapheme-cluster editing, keyboard-layout localization, platform input glyph generation, virtual keyboard policy, or native IME parity.
- Do not change `SdlWindow::handle_event` ordinary `VirtualInput` repeat behavior.
- Add tests before production implementation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header changes.
- Run focused `mirakana_sdl3_platform_tests` through the `desktop-runtime` preset, targeted tidy, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before commit.

## Files

- Modify: `engine/platform/include/mirakana/platform/input.hpp`
  - Add first-party key ids for `backspace`, `delete_key`, `home`, and `end`.
- Modify: `engine/platform/sdl3/src/sdl_input.cpp`
  - Map SDL3 keycodes for the new first-party keys.
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
  - Preserve SDL key-row window ids so existing window filtering also applies to key rows.
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
  - Declare SDL3 text edit command conversion and apply helpers.
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
  - Implement key-row conversion and apply through `mirakana::ui::apply_text_edit_command`.
- Modify: `engine/runtime/src/session_services.cpp`
  - Add stable runtime key names/parsing for the new public key ids so rebinding capture and presentation do not reject captured text-edit keys.
- Modify: `tests/unit/sdl3_platform_tests.cpp`
  - Add focused tests for keycode mapping, command conversion, repeated key rows, command application, and ignored rows.
- Modify: `tests/unit/runtime_tests.cpp`
  - Add focused tests for runtime key serialization/deserialization, rebinding capture, and rebinding presentation rows using the new public key ids.
- Modify docs/manifest:
  - `docs/superpowers/plans/README.md`
  - `docs/roadmap.md`
  - `docs/current-capabilities.md`
  - `docs/ui.md`
  - `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - `engine/agent/manifest.json`

## Done When

- SDL3 keycode mapping recognizes Backspace, Delete, Home, and End as first-party keys.
- `sdl3_text_edit_command_from_window_event` returns:
  - `move_cursor_backward` for Left
  - `move_cursor_forward` for Right
  - `move_cursor_to_start` for Home
  - `move_cursor_to_end` for End
  - `delete_backward` for Backspace
  - `delete_forward` for Delete
- The conversion helper ignores key-up, non-key, and non-text-editing key rows.
- Repeated SDL key-down rows are accepted by the text-edit helper and still remain ignored by ordinary `VirtualInput` handling.
- SDL key rows preserve `window_id` so existing `SdlWindow::handle_event` filtering can reject rows from another window.
- Runtime input action/rebinding key name, parse, capture, and presentation paths recognize the new public key ids.
- `apply_sdl3_text_edit_command_event` mutates valid `TextEditState` rows through `mirakana_ui` and returns unchanged state with no diagnostics for ignored rows.
- Targeted tests, API boundary check, production readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host blockers.

## Tasks

### Task 1: RED Tests

**Files:**
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests for SDL3 keycode mapping and text edit command conversion/application beside the existing SDL3 text input tests.

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests
```

Expected: compile failure because the new `Key` values and `sdl3_text_edit_command_from_window_event` / `apply_sdl3_text_edit_command_event` helpers are not declared yet.

### Task 2: SDL3 Command Bridge

**Files:**
- Modify: `engine/platform/include/mirakana/platform/input.hpp`
- Modify: `engine/platform/sdl3/src/sdl_input.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`

- [x] **Step 1: Extend first-party key ids**

Add `backspace`, `delete_key`, `home`, and `end` before `count` in `Key`.

- [x] **Step 2: Map SDL3 keycodes**

Map `SDLK_BACKSPACE`, `SDLK_DELETE`, `SDLK_HOME`, and `SDLK_END` in `sdl3_key_to_key`.

- [x] **Step 3: Add command conversion helpers**

Implement `sdl3_text_edit_command_from_window_event` and `apply_sdl3_text_edit_command_event` in the SDL3 UI integration bridge. Accept repeated key-down rows for command generation, but ignore key-up/non-key/unknown rows.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests
ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"
```

Expected: pass.

### Task 3: Documentation And Agent Truth

**Files:**
- Modify docs/manifest listed above.

- [x] **Step 1: Update current slice references**

Record this slice as the latest completed runtime UI/input child plan after validation succeeds.

- [x] **Step 2: Keep unsupported claims explicit**

State that this is only SDL3 key-row command conversion/application and does not claim full text editing widgets, selection UI, clipboard, word movement, grapheme-cluster editing, keyboard-layout localization, platform glyph generation, or native IME parity.

- [x] **Step 3: Verify docs and manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: pass with broad unsupported gaps still honestly reported.

### Task 4: Slice Validation And Commit

**Files:**
- All files touched by this plan.

- [x] **Step 1: Format and focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/session_services.cpp,tests/unit/runtime_tests.cpp -MaxFiles 2
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_window.cpp,engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 3
```

Expected: pass, or record an exact missing-tool blocker.

- [x] **Step 2: Full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: pass.

- [x] **Step 3: Commit**

Run:

```powershell
git status --short
git add engine/platform/include/mirakana/platform/input.hpp engine/platform/sdl3/src/sdl_input.cpp engine/platform/sdl3/src/sdl_window.cpp engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp engine/platform/sdl3/src/sdl_ui_platform_integration.cpp engine/runtime/src/session_services.cpp tests/unit/runtime_tests.cpp tests/unit/sdl3_platform_tests.cpp docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-text-edit-command-key-events-v1.md docs/superpowers/plans/README.md docs/roadmap.md docs/current-capabilities.md docs/ui.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md engine/agent/manifest.json
git commit -m "feat: map sdl text edit commands"
```

Expected: commit succeeds only after validation is green.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_runtime_tests` | Passed | Added review-follow-up coverage for runtime key names/parsing, rebinding capture, and presentation rows for Backspace/Delete/Home/End. |
| `ctest --preset dev --output-on-failure -R "^mirakana_runtime_tests$"` | Passed | Focused runtime tests passed after public `Key` ids were made valid in runtime action/rebinding paths. |
| `cmake --build --preset desktop-runtime --target mirakana_sdl3_platform_tests` | Passed | RED compile failure observed before implementation, then focused target passed after implementation, repeat-boundary regression coverage, window-id filtering coverage, and review-follow-up tests. |
| `ctest --preset desktop-runtime --output-on-failure -R "^mirakana_sdl3_platform_tests$"` | Passed | Focused SDL3 platform tests passed, including selected edit command keys, unknown/ignored rows, repeated key-down command acceptance, ordinary `VirtualInput` repeat ignore behavior, text-edit diagnostics forwarding, and key-row `window_id` filtering. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting check passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public `mirakana_platform` key ids and SDL3 bridge header changed; boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/session_services.cpp,tests/unit/runtime_tests.cpp -MaxFiles 2` | Passed | Review-follow-up runtime key-name/rebinding path check passed; existing clang-tidy warnings were reported but the wrapper returned `tidy-check: ok (2 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_window.cpp,engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 3` | Passed | Targeted SDL3 compile-database check passed; existing clang-tidy warnings were reported but the wrapper returned `tidy-check: ok (3 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/docs schema validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent truth validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit passed and still reports `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Final default gate passed on 2026-05-08; host-gated Apple/Metal diagnostics remain diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate build passed on 2026-05-08. |
