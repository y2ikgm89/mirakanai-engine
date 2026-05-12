# Runtime UI Clipboard Text Request v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a host-independent `MK_ui` clipboard text request contract plus a narrow SDL3 adapter bridge over the existing `SdlClipboard`.

**Architecture:** Keep `MK_ui` independent from SDL3 and `engine/platform` by defining a first-party `IClipboardTextAdapter` boundary with request/plan/result rows. Keep concrete SDL3 clipboard calls inside `MK_platform_sdl3` by adapting `SdlClipboard` to that UI boundary, preserving SDL3 main-thread clipboard behavior behind the platform adapter.

**Tech Stack:** C++23, `MK_ui`, `MK_platform_sdl3`, `MK_ui_renderer_tests`, `MK_sdl3_platform_tests`, PowerShell/PowerShell 7 (pwsh) scripts under `tools/` (validate + static checks).

**Status:** Completed on 2026-05-08. Focused UI/SDL3/core build/test, targeted tidy, schema/agent/audit checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed; the production readiness audit still honestly reports `unsupported_gaps=11`.

---

## Context

- Parent roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Current machine-readable pointer: `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points back to the master plan and `recommendedNextPlan.id` is `next-production-gap-selection` after this child slice.
- Latest related slice: `docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-text-edit-command-key-events-v1.md`.
- `engine/platform` already exposes `mirakana::IClipboard`, `mirakana::MemoryClipboard`, and optional `mirakana::SdlClipboard`.
- SDL3 official clipboard text APIs use UTF-8 C strings, return `bool` for `SDL_SetClipboardText`, allocate `SDL_GetClipboardText` results that must be freed with `SDL_free`, and are main-thread operations.

## Constraints

- Keep `engine/ui` independent from SDL3, OS APIs, platform headers, editor code, native handles, and middleware.
- Do not move `mirakana::IClipboard` into `MK_ui`; add a UI-owned adapter boundary instead.
- Allow empty clipboard text writes so callers can clear text intentionally.
- Validate strict UTF-8 for non-native UI contract text; invalid adapter-returned text must be diagnosed before consumers treat it as ready.
- Do not claim full text editing widget behavior, copy/cut/paste commands, selection UI, word movement, grapheme-cluster editing, rich clipboard MIME formats, virtual keyboard policy, native IME parity, or broad production UI/platform readiness.
- Add tests before production implementation.
- Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` after public header changes.
- Run focused `MK_ui_renderer_tests` and `MK_sdl3_platform_tests`, targeted tidy, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` before commit.

## Files

- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
  - Add `clipboard` to `AdapterBoundary`.
  - Add `IClipboardTextAdapter`.
  - Add `ClipboardTextWriteRequest`, `ClipboardTextReadRequest`, plan/result structs, diagnostics, and public helper declarations.
- Modify: `engine/ui/src/ui.cpp`
  - Add clipboard contract metadata.
  - Implement write/read plan validation and dispatch helpers.
  - Validate strict UTF-8 for write request text and adapter-returned read text.
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_clipboard.hpp`
  - Add `SdlClipboardTextAdapter` implementing `mirakana::ui::IClipboardTextAdapter` over `SdlClipboard`.
- Modify: `engine/platform/sdl3/src/sdl_clipboard.cpp`
  - Implement the SDL3 UI clipboard adapter bridge.
- Modify: `tests/unit/ui_renderer_tests.cpp`
  - Add focused UI contract tests for valid write/read, clear writes, invalid rows, empty clipboard reads, and invalid adapter output.
- Modify: `tests/unit/core_tests.cpp`
  - Update the renderer-independent adapter contract order assertion to include the new `clipboard` boundary.
- Modify: `tests/unit/sdl3_platform_tests.cpp`
  - Add focused SDL3 adapter bridge coverage using `SdlRuntime` dummy video and `SdlClipboard`.
- Modify docs/manifest:
  - `docs/superpowers/plans/README.md`
  - `docs/roadmap.md`
  - `docs/current-capabilities.md`
  - `docs/ui.md`
  - `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
  - `engine/agent/manifest.json`

## Done When

- `MK_ui` exposes first-party clipboard text read/write request plan/result rows without depending on `engine/platform`.
- Write requests reject empty targets and malformed UTF-8, but allow empty valid UTF-8 text as an explicit clear.
- Read requests reject empty targets before adapter dispatch.
- Read dispatch reports `has_text=false` and empty `text` as a valid empty-clipboard result.
- Adapter-returned read text is diagnosed when it is not strict UTF-8.
- `SdlClipboardTextAdapter` maps validated UI clipboard dispatch to `SdlClipboard::set_text`, `has_text`, and `text` without exposing SDL3/native handles through `MK_ui`.
- Targeted tests, API boundary check, production readiness audit, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record exact host blockers.

## Tasks

### Task 1: RED UI Contract Tests

**Files:**
- Modify: `tests/unit/ui_renderer_tests.cpp`

- [x] **Step 1: Add failing tests**

Add tests beside the existing platform text input / text edit tests:

```cpp
MK_TEST("ui clipboard text write request dispatches valid text and clear rows") {
    CapturingClipboardTextAdapter adapter;

    const auto write = mirakana::ui::write_clipboard_text(
        adapter, mirakana::ui::ClipboardTextWriteRequest{mirakana::ui::ElementId{"chat.input"}, "copy me"});
    MK_REQUIRE(write.succeeded());
    MK_REQUIRE(write.written);
    MK_REQUIRE(adapter.text == "copy me");

    const auto clear = mirakana::ui::write_clipboard_text(
        adapter, mirakana::ui::ClipboardTextWriteRequest{mirakana::ui::ElementId{"chat.input"}, ""});
    MK_REQUIRE(clear.succeeded());
    MK_REQUIRE(adapter.text.empty());
}
```

Also add tests for invalid target/malformed UTF-8 write rejection, read success, empty clipboard read success, invalid read target rejection, and invalid adapter-returned UTF-8 diagnostics.

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
```

Expected: compile failure because `IClipboardTextAdapter`, clipboard request/result types, and helper functions are not declared yet.

### Task 2: UI Clipboard Contract

**Files:**
- Modify: `engine/ui/include/mirakana/ui/ui.hpp`
- Modify: `engine/ui/src/ui.cpp`

- [x] **Step 1: Add public types and declarations**

Add `IClipboardTextAdapter`, `ClipboardTextWriteRequest`, `ClipboardTextReadRequest`, `ClipboardTextWritePlan`, `ClipboardTextWriteResult`, `ClipboardTextReadPlan`, and `ClipboardTextReadResult` beside the other adapter request/result rows. Add `plan_clipboard_text_write`, `write_clipboard_text`, `plan_clipboard_text_read`, and `read_clipboard_text`.

- [x] **Step 2: Add validation and dispatch**

Implement write validation for non-empty target and strict UTF-8 text. Implement read validation for non-empty target. For read dispatch, call `has_clipboard_text()` first; when false, return success with `has_text=false` and empty text. When true, call `clipboard_text()` and diagnose invalid UTF-8 with `invalid_clipboard_text_result`.

- [x] **Step 3: Verify GREEN**

Run:

```powershell
cmake --build --preset dev --target MK_ui_renderer_tests
ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"
```

Expected: pass.

### Task 3: SDL3 Clipboard Adapter Bridge

**Files:**
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_clipboard.hpp`
- Modify: `engine/platform/sdl3/src/sdl_clipboard.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Add failing SDL3 adapter test**

Add a test after `sdl3 clipboard reads writes and clears text` that creates `SdlClipboardTextAdapter` over `SdlClipboard`, writes text through `mirakana::ui::write_clipboard_text`, reads through `mirakana::ui::read_clipboard_text`, and clears by writing empty text.

- [x] **Step 2: Verify RED**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
```

Expected: compile failure because `SdlClipboardTextAdapter` is not declared yet.

- [x] **Step 3: Implement SDL3 adapter bridge**

Add `SdlClipboardTextAdapter` as a thin bridge over `SdlClipboard`. It should forward `set_clipboard_text`, `has_clipboard_text`, and `clipboard_text` without catching `SdlClipboard` exceptions.

- [x] **Step 4: Verify GREEN**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"
```

Expected: pass.

### Task 4: Documentation And Agent Truth

**Files:**
- Modify docs/manifest listed above.

- [x] **Step 1: Update current slice references**

Record this slice as the latest completed runtime UI/platform adapter child plan after validation succeeds.

- [x] **Step 2: Keep unsupported claims explicit**

State that this is only a text clipboard request/adapter boundary and SDL3 text clipboard bridge. Keep copy/cut/paste widgets, selection UI, rich clipboard MIME types, word/grapheme editing, candidate UI, OS accessibility, concrete Win32/Cocoa/Linux/Android/iOS text services, and broad production UI readiness unsupported.

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
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_clipboard.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2
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
git add engine/ui/include/mirakana/ui/ui.hpp engine/ui/src/ui.cpp engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_clipboard.hpp engine/platform/sdl3/src/sdl_clipboard.cpp tests/unit/ui_renderer_tests.cpp tests/unit/sdl3_platform_tests.cpp docs/superpowers/plans/2026-05-08-runtime-ui-clipboard-text-request-plan-v1.md docs/superpowers/plans/README.md docs/roadmap.md docs/current-capabilities.md docs/ui.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md engine/agent/manifest.json
git commit -m "feat: add ui clipboard text requests"
```

Expected: commit succeeds only after validation is green.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target MK_ui_renderer_tests` | Passed | RED failed as expected before implementation; GREEN build passed after adding the UI clipboard contract and again after formatting. |
| `ctest --preset dev --output-on-failure -R "^MK_ui_renderer_tests$"` | Passed | Focused UI clipboard contract tests passed after formatting. |
| `cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests` | Passed | RED failed as expected before `SdlClipboardTextAdapter`; GREEN build passed after adding the adapter bridge and again after formatting. |
| `ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"` | Passed | Focused SDL3 clipboard adapter bridge tests passed after formatting. |
| `ctest --preset dev --output-on-failure -R "^MK_core_tests$"` | Passed | Renderer-independent adapter contract order test passed after adding the `clipboard` boundary expectation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Formatting gate passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public UI and SDL3 header boundary check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/ui/src/ui.cpp,tests/unit/ui_renderer_tests.cpp -MaxFiles 2` | Passed | Targeted UI contract tidy lane passed; wrapper reported existing warnings but `tidy-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_clipboard.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2` | Passed | Targeted SDL3 adapter tidy lane passed; wrapper reported existing warnings but `tidy-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest/schema truth gate passed after aligning the child plan completion state with `currentActivePlan`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent truth gate passed after aligning the child plan completion state with `currentActivePlan`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Audit still honestly reports `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Final default gate passed after updating the core adapter-contract test expectation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Commit gate passed. |
