# Runtime UI SDL3 Text Edit Clipboard Shortcuts v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Map copied SDL3 key-down shortcut rows for copy, cut, and paste into first-party `MK_ui` text edit clipboard commands and apply them through `IClipboardTextAdapter` without moving SDL3 into `MK_ui`.

**Architecture:** Keep text mutation and clipboard validation in `MK_ui` through `apply_text_edit_clipboard_command`. Copy SDL3 `SDL_KeyboardEvent::key` and `SDL_KeyboardEvent::mod` into the SDL3-specific `SdlWindowEvent` row as plain integral fields, then add optional `MK_platform_sdl3` helpers that recognize Ctrl/GUI + C/X/V and build `TextEditClipboardCommand` rows. This keeps platform shortcut policy in the SDL3 bridge and keeps `engine/ui` independent from platform headers, native handles, and SDL3.

**Tech Stack:** C++23, SDL3, `MK_platform_sdl3`, `MK_ui`, `MK_sdl3_platform_tests`, PowerShell/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Status:** Completed.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Active gap row: `production-ui-importer-platform-adapters`.
- Prerequisite slices:
  - `docs/superpowers/plans/2026-05-08-runtime-ui-clipboard-text-request-plan-v1.md`
  - `docs/superpowers/plans/2026-05-08-runtime-ui-text-edit-clipboard-command-apply-v1.md`
  - `docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-text-edit-command-key-events-v1.md`
- Official SDL3 documentation confirms `SDL_KeyboardEvent` exposes `event.key.key` and `event.key.mod`, and `SDL_Keymod` provides `SDL_KMOD_CTRL` and `SDL_KMOD_GUI` modifier masks.

## Constraints

- Keep `engine/ui` independent from SDL3, OS APIs, native handles, editor code, Dear ImGui, and middleware.
- Keep this slice limited to copied SDL3 shortcut rows. Do not add full text editing widgets, selection rendering, word/grapheme movement, rich clipboard formats, native text services, or candidate UI.
- Do not add public `mirakana::Key` entries for C/X/V solely for shortcut detection; the shortcut helper can use the copied SDL keycode because it lives in the SDL3 bridge.
- Require a shortcut modifier: Ctrl or GUI. Ignore rows with Alt to avoid treating AltGr or application/menu chord rows as clipboard shortcuts.
- Repeated key-down rows are accepted by the helper; repeat suppression remains host policy.
- Add tests before production implementation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header changes.
- Run focused `MK_sdl3_platform_tests` through the `desktop-runtime` preset, targeted tidy, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before commit.

## Files

- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
  - Add copied SDL keycode and key modifier fields to `SdlWindowEvent`.
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
  - Copy `source.key.key` and `source.key.mod` for key-down/key-up rows.
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
  - Declare `sdl3_text_edit_clipboard_command_from_window_event`.
  - Declare `apply_sdl3_text_edit_clipboard_command_event`.
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
  - Map Ctrl/GUI + C/X/V key-down rows into `TextEditClipboardCommandKind`.
  - Apply mapped commands through `mirakana::ui::apply_text_edit_clipboard_command`.
- Modify: `tests/unit/sdl3_platform_tests.cpp`
  - Add focused RED/GREEN coverage for modifier copying, shortcut command conversion, ignored rows, and adapter-backed apply behavior.
- Modify docs/manifest:
  - `docs/superpowers/plans/README.md`
  - `docs/roadmap.md`
  - `docs/current-capabilities.md`
  - `docs/ui.md`
  - `docs/testing.md`
  - `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - `engine/agent/manifest.json`
- Modify validation wrapper after full validation exposed a generator-target hang:
  - `tools/test.ps1`

## Done When

- SDL3 key rows copy raw SDL keycode and key modifier state into `SdlWindowEvent` without exposing SDL headers from public first-party UI APIs.
- `sdl3_text_edit_clipboard_command_from_window_event` returns:
  - `copy_selection` for Ctrl/GUI + C key-down rows.
  - `cut_selection` for Ctrl/GUI + X key-down rows.
  - `paste_text` for Ctrl/GUI + V key-down rows.
- The conversion helper ignores key-up, non-key, unknown shortcut keys, rows without Ctrl/GUI, and rows with Alt.
- `apply_sdl3_text_edit_clipboard_command_event` composes with `SdlClipboardTextAdapter` / `IClipboardTextAdapter`, mutates valid `TextEditState` rows only through `MK_ui`, and returns unchanged state with no diagnostics for ignored rows.
- Ready claims remain narrow: this is selected SDL3 shortcut mapping only, not full platform shortcut localization, text editing widgets, selection UI, rich clipboard formats, native text services, or broad runtime UI readiness.
- Targeted tests, API boundary check, production readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host blockers.

## Tasks

### Task 1: RED SDL3 Shortcut Tests

**Files:**
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests beside the existing SDL3 text edit and clipboard tests:

```cpp
MK_TEST("sdl3 key rows copy raw keycode and shortcut modifiers") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    const auto translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});

    MK_REQUIRE(translated.kind == mirakana::SdlWindowEventKind::key_pressed);
    MK_REQUIRE(translated.key == mirakana::Key::unknown);
    MK_REQUIRE(translated.sdl_keycode == SDLK_C);
    MK_REQUIRE((translated.key_modifiers & SDL_KMOD_CTRL) != 0U);
}

MK_TEST("sdl3 platform integration maps shortcut rows to text edit clipboard commands") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    auto translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    auto command =
        mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);

    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->target == mirakana::ui::ElementId{"chat.input"});
    MK_REQUIRE(command->kind == mirakana::ui::TextEditClipboardCommandKind::copy_selection);

    event.key.key = SDLK_X;
    event.key.mod = SDL_KMOD_GUI;
    translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditClipboardCommandKind::cut_selection);

    event.key.key = SDLK_V;
    event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_SHIFT;
    translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    command = mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated);
    MK_REQUIRE(command.has_value());
    MK_REQUIRE(command->kind == mirakana::ui::TextEditClipboardCommandKind::paste_text);
}

MK_TEST("sdl3 platform integration ignores non clipboard shortcut rows") {
    SDL_Event event{};
    event.type = SDL_EVENT_KEY_UP;
    event.key.key = SDLK_C;
    event.key.mod = SDL_KMOD_CTRL;

    auto translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(!mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
                    .has_value());

    event = SDL_Event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_C;
    translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(!mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
                    .has_value());

    event.key.mod = SDL_KMOD_CTRL | SDL_KMOD_ALT;
    translated =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    MK_REQUIRE(!mirakana::sdl3_text_edit_clipboard_command_from_window_event(mirakana::ui::ElementId{"chat.input"}, translated)
                    .has_value());
}

MK_TEST("sdl3 platform integration applies clipboard shortcuts through ge ui state") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlClipboard clipboard;
    mirakana::SdlClipboardTextAdapter adapter{clipboard};
    clipboard.clear();

    SDL_Event event{};
    event.type = SDL_EVENT_KEY_DOWN;
    event.key.key = SDLK_X;
    event.key.mod = SDL_KMOD_CTRL;

    const auto cut_event =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState cut_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello world",
        .cursor_byte_offset = 6,
        .selection_byte_length = 5,
    };

    const auto cut = mirakana::apply_sdl3_text_edit_clipboard_command_event(
        adapter, cut_state, mirakana::ui::ElementId{"chat.input"}, cut_event);

    MK_REQUIRE(cut.succeeded());
    MK_REQUIRE(cut.applied);
    MK_REQUIRE(cut.state.text == "hello ");
    MK_REQUIRE(clipboard.text() == "world");

    event.key.key = SDLK_V;
    const auto paste_event =
        mirakana::sdl3_translate_window_event(mirakana::SdlRawEventHandle{&event}, mirakana::WindowExtent{.width = 320, .height = 240});
    const mirakana::ui::TextEditState paste_state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "say ",
        .cursor_byte_offset = 4,
        .selection_byte_length = 0,
    };

    const auto paste = mirakana::apply_sdl3_text_edit_clipboard_command_event(
        adapter, paste_state, mirakana::ui::ElementId{"chat.input"}, paste_event);

    MK_REQUIRE(paste.succeeded());
    MK_REQUIRE(paste.applied);
    MK_REQUIRE(paste.state.text == "say world");
    MK_REQUIRE(paste.state.cursor_byte_offset == 9U);
}
```

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
```

Expected: compile failure because `SdlWindowEvent::sdl_keycode`, `SdlWindowEvent::key_modifiers`, `sdl3_text_edit_clipboard_command_from_window_event`, and `apply_sdl3_text_edit_clipboard_command_event` are not declared yet.

### Task 2: SDL3 Shortcut Bridge

**Files:**
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Modify: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`

- [x] **Step 1: Copy keycode and modifiers**

Add to `SdlWindowEvent`:

```cpp
std::int32_t sdl_keycode{0};
std::uint16_t key_modifiers{0};
```

Set them in the SDL key event translation:

```cpp
translated.sdl_keycode = source.key.key;
translated.key_modifiers = source.key.mod;
```

- [x] **Step 2: Add public SDL3 bridge declarations**

Declare:

```cpp
[[nodiscard]] std::optional<ui::TextEditClipboardCommand>
sdl3_text_edit_clipboard_command_from_window_event(const ui::ElementId& target, const SdlWindowEvent& event);

[[nodiscard]] ui::TextEditClipboardCommandResult
apply_sdl3_text_edit_clipboard_command_event(ui::IClipboardTextAdapter& adapter, const ui::TextEditState& state,
                                             const ui::ElementId& target, const SdlWindowEvent& event);
```

- [x] **Step 3: Implement shortcut conversion**

Use SDL3 keymod and keycode constants only inside the SDL3 implementation file:

```cpp
[[nodiscard]] bool has_sdl3_text_edit_shortcut_modifier(std::uint16_t modifiers) noexcept {
    const auto mods = static_cast<SDL_Keymod>(modifiers);
    const bool shortcut = (mods & (SDL_KMOD_CTRL | SDL_KMOD_GUI)) != 0;
    const bool alt = (mods & SDL_KMOD_ALT) != 0;
    return shortcut && !alt;
}

[[nodiscard]] std::optional<ui::TextEditClipboardCommandKind>
text_edit_clipboard_command_kind_from_sdl_keycode(std::int32_t keycode) noexcept {
    switch (keycode) {
    case SDLK_C:
        return ui::TextEditClipboardCommandKind::copy_selection;
    case SDLK_X:
        return ui::TextEditClipboardCommandKind::cut_selection;
    case SDLK_V:
        return ui::TextEditClipboardCommandKind::paste_text;
    default:
        return std::nullopt;
    }
}
```

Then build the command only for `SdlWindowEventKind::key_pressed` rows with a valid shortcut modifier and mapped keycode.

- [x] **Step 4: Implement apply helper**

If conversion returns no command, return a state-preserving result:

```cpp
return ui::TextEditClipboardCommandResult{
    .applied = false,
    .state = state,
    .diagnostics = {},
};
```

Otherwise call:

```cpp
return ui::apply_text_edit_clipboard_command(adapter, state, *command);
```

- [x] **Step 5: Verify GREEN**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"
```

Expected: pass.

### Task 3: Documentation and Manifest Truth

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ui.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update active/completed records**

Record this slice as active while in progress and latest completed after validation succeeds. Keep `currentActivePlan`, `recommendedNextPlan`, and the plan registry synchronized.

- [x] **Step 2: Preserve unsupported claims**

State that this is only selected SDL3 shortcut mapping for text edit clipboard commands. Keep full platform shortcut localization, selection UI, rich clipboard formats, word/grapheme editing, native text services, candidate UI, and broad runtime UI readiness unsupported.

- [x] **Step 3: Verify docs and manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: pass; production readiness audit still reports the remaining non-ready gap rows.

### Task 4: Formatting, Static Checks, Full Validation, Commit

**Files:**
- All files touched by this slice.

- [x] **Step 1: Format/static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
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

- [ ] **Step 3: Commit**

Run:

```powershell
git status --short
git add engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp engine/platform/sdl3/src/sdl_window.cpp engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp engine/platform/sdl3/src/sdl_ui_platform_integration.cpp tests/unit/sdl3_platform_tests.cpp docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-text-edit-clipboard-shortcuts-v1.md docs/superpowers/plans/README.md docs/roadmap.md docs/current-capabilities.md docs/ui.md docs/testing.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md engine/agent/manifest.json tools/test.ps1
git commit -m "feat: map sdl3 text edit clipboard shortcuts"
```

Expected: commit succeeds only after validation is green.

## Validation Evidence

| Command | Status | Notes |
| --- | --- | --- |
| `cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests` | Passed | RED failed first on missing fields/helpers; GREEN focused SDL3 platform build passed after implementation, tidy cleanup, and review follow-up. |
| `ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"` | Passed | Focused SDL3 shortcut bridge tests passed after implementation, tidy cleanup, and the ignored-row apply coverage follow-up. |
| `cpp-reviewer staged diff review` | Addressed | Reviewer found `engine/agent/manifest.json` had no staged capability update and ignored-row apply coverage was thin; fixed by updating manifest truth rows and adding explicit ignored apply-result coverage. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | Formatting applied successfully. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting gate passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public SDL3 bridge header change accepted by boundary gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_window.cpp,engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 3` | Passed | Targeted changed-file analyzer lane passed; inherited SDL3/test warnings remain non-fatal, and the new optional-access warning was removed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Plan/manifest schema gate passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent truth gate passed after restoring required master-plan historical evidence needles and adding the SDL3 clipboard shortcut manifest rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Gap truth gate passed and still reports `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` | Passed | Direct CTest preset wrapper passed 29/29 after replacing the Visual Studio `RUN_TESTS` target path. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Default repository gate passed after the CTest wrapper fix; production audit still reports `unsupported_gaps=11`, with Metal and Apple lanes host-gated as diagnostic-only on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate passed after validation. |

## Validation Blocker Notes

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` initially timed out, then failed in the default `dev` test lane because the Visual Studio-generated `RUN_TESTS` target held `MK_platform_process_tests` until CTest's 1500 second timeout while direct CTest preset execution passed.
- `ctest --preset dev --output-on-failure -R "^MK_platform_process_tests$" --timeout 60` passed, and `ctest --preset dev --output-on-failure --timeout 300` passed 29/29.
- `tools/test.ps1` now builds `dev` through the resolved CMake tool and then invokes the resolved CTest tool directly with `--preset dev --output-on-failure --timeout 300`, matching the documented CTest preset lane and avoiding the generator-specific `RUN_TESTS` wrapper hang.
