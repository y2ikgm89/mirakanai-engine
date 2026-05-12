# Runtime UI Text Edit Command Apply v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `MK_ui` text edit command contract that applies scalar-boundary cursor movement and deletion commands to `TextEditState`.

**Architecture:** Extend the existing `TextEditState` / committed-text path with a first-party command row and plan/apply result types. Keep state offsets as UTF-8 byte offsets, validate existing state text strictly before mutation, and apply only scalar-boundary movement/deletion without introducing SDL3, Dear ImGui, native handles, clipboard, selection UI, grapheme segmentation, or IME candidate UI into `MK_ui`.

**Tech Stack:** C++23, `MK_ui`, `MK_ui_renderer_tests`, PowerShell/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Status:** Completed on 2026-05-08. Slice-closing validation passed; the parent production readiness audit still reports 11 unsupported production gaps.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Current machine-readable pointer: `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points back to the master plan and `recommendedNextPlan.id` is `next-production-gap-selection`.
- Latest completed related slice: `docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-committed-text-edit-apply-v1.md`.
- Existing public API already has `TextEditState`, `CommittedTextInput`, `TextEditCommitPlan`, `TextEditCommitResult`, `plan_committed_text_input`, and `apply_committed_text_input`.
- This slice narrows the runtime UI/input gap by adding text editing commands that hosts can map from keyboard/input systems later.

## Constraints

- Keep `engine/ui` independent from SDL3, OS APIs, editor code, native handles, and middleware.
- Do not claim full text editing widget behavior, selection UI, clipboard, undo/redo, word movement, key repeat, keyboard-layout localization, grapheme-cluster editing, candidate UI, native IME parity, or platform SDK behavior.
- Preserve strict UTF-8 validation and byte-offset naming.
- Add tests before production implementation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header changes.
- Run targeted `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files ...` for touched C++ files when available.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before the slice-closing commit.

## Files

- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
  - Add `TextEditCommandKind`, `TextEditCommand`, `TextEditCommandPlan`, and `TextEditCommandResult`.
  - Add diagnostics for mismatched command targets and invalid command values.
  - Declare `plan_text_edit_command` and `apply_text_edit_command`.
- Modify: `engine/ui/src/ui.cpp`
  - Reuse existing strict UTF-8 helpers.
  - Add previous/next scalar-boundary helpers for validated text.
  - Add command validation and command mutation.
- Modify: `tests/unit/ui_renderer_tests.cpp`
  - Add focused tests for movement, backward/forward deletion, selection deletion, UTF-8 scalar boundaries, and invalid rows.
- Modify: `docs/superpowers/plans/README.md`
  - Add this active/completed slice entry.
- Modify: `docs/roadmap.md`, `docs/current-capabilities.md`, and `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update runtime UI/input capability notes to include command apply and to keep broader text-editing claims unsupported.
- Modify: `engine/agent/manifest.json`
  - Update the `MK_ui`/runtime UI purpose text and current production notes so agents can see the new command surface without promoting the broad gap to ready.

## Done When

- `mirakana::ui::apply_text_edit_command` handles these command kinds:
  - `move_cursor_backward`
  - `move_cursor_forward`
  - `move_cursor_to_start`
  - `move_cursor_to_end`
  - `delete_backward`
  - `delete_forward`
- Movement commands clear `selection_byte_length` and move to strict UTF-8 scalar boundaries.
- Delete commands remove the selected byte range when `selection_byte_length > 0`; otherwise they remove one previous or next UTF-8 scalar when available.
- Boundary no-op commands are treated as valid handled commands with unchanged text.
- Invalid state, target mismatch, invalid cursor/selection byte offsets, malformed existing text, and invalid enum values produce diagnostics and do not mutate state.
- Targeted tests, API boundary check, production readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host blockers.

## Tasks

### Task 1: RED Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests after the existing committed text input tests:

```cpp
MK_TEST("ui text edit command moves cursor by utf8 scalar boundaries") {
    const mirakana::ui::TextEditState state{
        .target = mirakana::ui::ElementId{"chat.input"},
        .text = "a\xE3\x81\x82"
                "b",
        .cursor_byte_offset = 1,
        .selection_byte_length = 0,
    };

    const auto forward = mirakana::ui::apply_text_edit_command(
        state, mirakana::ui::TextEditCommand{state.target, mirakana::ui::TextEditCommandKind::move_cursor_forward});
    MK_REQUIRE(forward.succeeded());
    MK_REQUIRE(forward.applied);
    MK_REQUIRE(forward.state.cursor_byte_offset == 4U);
    MK_REQUIRE(forward.state.selection_byte_length == 0U);

    const auto end = mirakana::ui::apply_text_edit_command(
        forward.state, mirakana::ui::TextEditCommand{state.target, mirakana::ui::TextEditCommandKind::move_cursor_to_end});
    MK_REQUIRE(end.succeeded());
    MK_REQUIRE(end.state.cursor_byte_offset == 5U);

    const auto backward = mirakana::ui::apply_text_edit_command(
        end.state, mirakana::ui::TextEditCommand{state.target, mirakana::ui::TextEditCommandKind::move_cursor_backward});
    MK_REQUIRE(backward.succeeded());
    MK_REQUIRE(backward.state.cursor_byte_offset == 4U);

    const auto start = mirakana::ui::apply_text_edit_command(
        backward.state, mirakana::ui::TextEditCommand{state.target, mirakana::ui::TextEditCommandKind::move_cursor_to_start});
    MK_REQUIRE(start.succeeded());
    MK_REQUIRE(start.state.cursor_byte_offset == 0U);
}
```

Also add tests for `delete_backward`, `delete_forward`, selection deletion, boundary no-op handling, and invalid rows.

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
```

Expected: compile failure because `TextEditCommand` / `TextEditCommandKind` / `apply_text_edit_command` are not declared yet.

### Task 2: Public Command Contract

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`

- [x] **Step 1: Add public types and declarations**

Add `TextEditCommandKind`, `TextEditCommand`, `TextEditCommandPlan`, and `TextEditCommandResult` beside the existing committed text types. Add `plan_text_edit_command` and `apply_text_edit_command` beside the committed text functions.

- [x] **Step 2: Add validation**

Implement `plan_text_edit_command` so it rejects empty state targets, command/state target mismatches, malformed existing UTF-8 text, cursor offsets outside the text or inside a scalar, selection ranges outside the text or ending inside a scalar, and invalid enum values.

- [x] **Step 3: Add apply behavior**

Implement `apply_text_edit_command` with strict scalar-boundary movement/deletion. Set `applied=true` for valid handled commands, including boundary no-op commands, and leave state unchanged when the plan is not ready.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"
```

Expected: pass.

### Task 3: Documentation And Agent Truth

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update current slice references**

Record this slice as the latest completed runtime UI/input child plan after validation succeeds.

- [x] **Step 2: Keep unsupported claims explicit**

State that command application is host-independent and does not claim full text editing widgets, key mapping, selection UI, clipboard, native IME parity, platform text services, or broad production UI readiness.

- [x] **Step 3: Verify docs and manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: pass with the broad remaining unsupported gaps still honestly reported.

### Task 4: Slice Validation And Commit

**Files:**
- All files touched by this plan.

- [x] **Step 1: Format and focused checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp
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
git add engine/ui/include/mirakana/ui/ui.hpp engine/ui/src/ui.cpp tests/unit/ui_renderer_tests.cpp docs/superpowers/plans/2026-05-08-runtime-ui-text-edit-command-apply-v1.md docs/superpowers/plans/README.md docs/roadmap.md docs/current-capabilities.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md engine/agent/manifest.json
git commit -m "feat: apply text edit commands"
```

Expected: commit succeeds only after validation is green.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_ui_renderer_tests` | Passed | RED compile failure observed before implementation, then focused target passed after implementation and review fixes. |
| `ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"` | Passed | Focused runtime UI tests passed, including UTF-8 scalar movement, selected-range deletion, delete-forward selection, boundary no-ops, and invalid rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting check passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public `MK_ui` header changed; boundary check passed after API and contract comment updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp` | Passed | Targeted shared C++ check passed; existing warnings are reported by clang-tidy but wrapper returned `tidy-check: ok (2 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/docs schema validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent truth validation passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit passed and still reports `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Final default gate passed on 2026-05-08; host-gated Apple/Metal diagnostics remain diagnostic-only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate build passed on 2026-05-08. |
