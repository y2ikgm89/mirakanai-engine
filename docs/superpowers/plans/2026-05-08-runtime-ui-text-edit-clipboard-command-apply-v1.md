# Runtime UI Text Edit Clipboard Command Apply v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `MK_ui` text edit clipboard command contract for copy, cut, and paste over validated `TextEditState` rows and the existing `IClipboardTextAdapter`.

**Architecture:** Keep shortcut mapping, native clipboard ownership, and widget selection UI outside `MK_ui`; this slice only applies caller-selected clipboard commands to first-party text edit state. Reuse the existing strict UTF-8 text edit validation and clipboard adapter boundary so SDL3 remains an optional platform bridge instead of a dependency of `engine/ui`.

**Tech Stack:** C++23, `MK_ui`, optional `MK_platform_sdl3`, `MK_ui_renderer_tests`, `MK_sdl3_platform_tests`, PowerShell/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Status:** Completed.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Current machine-readable pointer: `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` returned to the production completion master plan after this slice completed.
- Latest prerequisite slice: `docs/superpowers/plans/2026-05-08-runtime-ui-clipboard-text-request-plan-v1.md`.
- `MK_ui` already has `TextEditState`, `CommittedTextInput`, `TextEditCommand`, `IClipboardTextAdapter`, `write_clipboard_text`, and `read_clipboard_text`.
- `MK_platform_sdl3` already exposes `SdlClipboardTextAdapter` over `SdlClipboard`.

## Constraints

- Keep `engine/ui` independent from SDL3, OS APIs, native handles, editor code, Dear ImGui, and middleware.
- Do not add Ctrl/Cmd shortcut mapping in this slice; platform key modifiers are not represented in `SdlWindowEvent`.
- Copy/cut require a non-empty valid forward selection. Paste may be a no-op when the adapter reports no text.
- Preserve strict UTF-8 validation for state text, selection boundaries, selected text, and adapter-returned paste text.
- Do not claim full text editing widgets, selection UI, rich clipboard formats, word movement, grapheme-cluster editing, platform shortcut localization, native IME/text services, or broad production UI readiness.
- Add tests before production implementation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header changes.
- Run focused tests, targeted tidy, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before commit.

## Files

- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
  - Add `TextEditClipboardCommandKind`, `TextEditClipboardCommand`, `TextEditClipboardCommandPlan`, and `TextEditClipboardCommandResult`.
  - Add public declarations for `plan_text_edit_clipboard_command` and `apply_text_edit_clipboard_command`.
  - Add diagnostics for mismatched clipboard command target and unsupported clipboard command kind.
- Modify: `engine/ui/src/ui.cpp`
  - Validate text edit state and clipboard command rows.
  - Apply copy, cut, and paste by reusing `IClipboardTextAdapter` through the existing clipboard read/write helper behavior.
- Modify: `tests/unit/ui_renderer_tests.cpp`
  - Add focused RED/GREEN coverage for copy, cut, paste, no-text paste, invalid rows, and invalid adapter text.
- Modify: `tests/unit/sdl3_platform_tests.cpp`
  - Add focused SDL3 adapter composition coverage using `SdlClipboardTextAdapter` and `apply_text_edit_clipboard_command`.
- Modify docs/manifest:
  - `docs/superpowers/plans/README.md`
  - `docs/roadmap.md`
  - `docs/current-capabilities.md`
  - `docs/ui.md`
  - `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - `engine/agent/manifest.json`

## Done When

- `MK_ui` exposes first-party text edit clipboard command plan/result rows without depending on `engine/platform`.
- Copy writes the selected UTF-8 substring to `IClipboardTextAdapter` and preserves text edit state.
- Cut writes the selected UTF-8 substring to the adapter, removes the selection, leaves the cursor at the selection start, and clears selection length.
- Paste reads text through `IClipboardTextAdapter`, replaces the current selection or inserts at the cursor when text is present, advances the cursor by pasted byte length, and clears selection length.
- Paste with no clipboard text succeeds as a state-preserving no-op.
- Invalid target, mismatched command target, invalid command kind, malformed state text/cursor/selection, empty copy/cut selection, and invalid adapter paste text are diagnosed before mutation.
- SDL3 composition proves `SdlClipboardTextAdapter` works with the host-independent command helper without adding platform shortcut mapping.
- Targeted tests, API boundary check, production readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host blockers.

## Tasks

### Task 1: RED UI Clipboard Command Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests near the existing text edit and clipboard tests:

```cpp
MK_TEST("ui text edit clipboard command copies selected text without mutating state") {
    CapturingClipboardTextAdapter adapter;
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "hello world",
        .cursor_byte_offset = 6,
        .selection_byte_length = 5,
    };

    const auto result = mirakana::ui::apply_text_edit_clipboard_command(
        adapter,
        state,
        mirakana::ui::TextEditClipboardCommand{.target = state.target,
                                         .kind = mirakana::ui::TextEditClipboardCommandKind::copy_selection});

    MK_REQUIRE(result.succeeded());
    MK_REQUIRE(result.state.text == state.text);
    MK_REQUIRE(result.state.cursor_byte_offset == 6U);
    MK_REQUIRE(result.state.selection_byte_length == 5U);
    MK_REQUIRE(adapter.text == "world");
}
```

Also add tests for cut selection removal, paste insertion/replacement, paste no-text no-op, invalid target/mismatch/selection/kind rejection, and invalid adapter UTF-8 paste diagnostics.

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
```

Expected: compile failure because `TextEditClipboardCommand*` public types and helpers are not declared.

### Task 2: UI Clipboard Command Contract

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`
- Modify: `tests/unit/core_tests.cpp` if adapter/diagnostic contract assertions require updates.

- [x] **Step 1: Add public types and declarations**

Add `TextEditClipboardCommandKind`, `TextEditClipboardCommand`, `TextEditClipboardCommandPlan`, `TextEditClipboardCommandResult`, `plan_text_edit_clipboard_command`, and `apply_text_edit_clipboard_command`.

- [x] **Step 2: Implement validation**

Validate non-empty state target, command target equality, strict UTF-8 state text, cursor boundary, selection range/boundary, copy/cut non-empty selection, and supported command kind.

- [x] **Step 3: Implement dispatch**

Use `write_clipboard_text` for copy/cut selected text. Use `read_clipboard_text` for paste; no clipboard text is a successful no-op, and invalid adapter text keeps the original state with diagnostics.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"
```

Expected: pass.

### Task 3: SDL3 Clipboard Adapter Composition

**Files:**
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Add SDL3 composition test**

Add a test beside the existing `SdlClipboardTextAdapter` coverage that uses `apply_text_edit_clipboard_command` with `SdlClipboardTextAdapter` to cut selected text and paste it into another state.

- [x] **Step 2: Verify GREEN**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"
```

Expected: pass.

### Task 4: Documentation And Agent Truth

**Files:**
- Modify docs/manifest listed above.

- [x] **Step 1: Update active/completed slice references**

Record this slice as active while in progress and latest completed after validation succeeds. Keep `currentActivePlan` synchronized with the plan status.

- [x] **Step 2: Preserve unsupported claims**

State that this is only a host-independent command application helper over caller-owned `TextEditState` and `IClipboardTextAdapter`. Keep shortcut mapping, selection UI, rich clipboard formats, word/grapheme movement, native text services, candidate UI, OS accessibility, and broad UI readiness unsupported.

- [x] **Step 3: Verify docs and manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: pass with broad unsupported gaps still honestly reported.

### Task 5: Slice Validation And Commit

**Files:**
- All files touched by this plan.

- [x] **Step 1: Format and focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp -MaxFiles 2
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files tests/unit/sdl3_platform_tests.cpp -MaxFiles 1
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
git add engine/ui/include/mirakana/ui/ui.hpp engine/ui/src/ui.cpp tests/unit/ui_renderer_tests.cpp tests/unit/sdl3_platform_tests.cpp docs/superpowers/plans/2026-05-08-runtime-ui-text-edit-clipboard-command-apply-v1.md docs/superpowers/plans/README.md docs/roadmap.md docs/current-capabilities.md docs/ui.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md engine/agent/manifest.json
git commit -m "feat: apply ui text edit clipboard commands"
```

Expected: commit succeeds only after validation is green.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_ui_renderer_tests` | RED failed as expected | Initial compile failed before the new `TextEditClipboardCommand*` public API existed. |
| `cmake --build --preset dev --target MK_ui_renderer_tests` | Passed | GREEN UI clipboard command contract build. |
| `ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"` | Passed | Focused UI clipboard command tests. |
| `cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests` | Passed | SDL3 adapter composition build. |
| `ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"` | Passed | Focused SDL3 adapter composition tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | Applied repository formatting. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public UI header changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp -MaxFiles 2` | Passed | Targeted UI tidy lane; existing warnings reported, wrapper completed ok. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files tests/unit/sdl3_platform_tests.cpp -MaxFiles 1` | Passed | Targeted SDL3 test tidy lane; existing warnings reported, wrapper completed ok. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/schema truth gate after returning `currentActivePlan` to the master plan. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent truth gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Reports `unsupported_gaps=11`; broad unsupported gaps remain honest. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Final default gate before `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`; diagnostic-only Apple/Metal host gates remain. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate. |
