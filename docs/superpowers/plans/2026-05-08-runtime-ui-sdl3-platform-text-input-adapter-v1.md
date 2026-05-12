# Runtime UI SDL3 Platform Text Input Adapter v1 Implementation Plan (2026-05-08)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an optional SDL3-backed implementation of `mirakana::ui::IPlatformIntegrationAdapter` for first-party platform text-input begin/end requests.

**Plan ID:** `runtime-ui-sdl3-platform-text-input-adapter-v1`

**Status:** Completed

**Architecture:** Keep `MK_ui` SDL3-free and keep generated games on `mirakana::ui` contracts. Add a narrow adapter in `MK_platform_sdl3` that owns the SDL3 call boundary, maps validated UI text bounds into `SDL_Rect`, sets the SDL text input area, starts/stops SDL text input for the caller-owned `SdlWindow`, and exposes no native handles through the UI contract.

**Tech Stack:** C++23, `MK_ui`, `MK_platform_sdl3`, SDL3 official text-input APIs, `tests/unit/sdl3_platform_tests.cpp`.

---

## Context

- Active roadmap: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`.
- Target unsupported gap: `production-ui-importer-platform-adapters`, currently `planned`.
- Existing `MK_ui` provides `PlatformTextInputSessionPlan`, `PlatformTextInputSessionResult`, `PlatformTextInputEndPlan`, `PlatformTextInputEndResult`, `plan_platform_text_input_session`, `begin_platform_text_input`, `plan_platform_text_input_end`, and `end_platform_text_input`.
- Existing `MK_platform_sdl3` owns SDL runtime/window/input/clipboard/cursor/file-dialog adapters and already keeps `SDL_Window*` behind `SdlWindow`.
- Context7 SDL3 docs confirm `SDL_StartTextInput` / `SDL_StopTextInput` should be paired, `SDL_SetTextInputArea` sets the IME suggestion area, and these calls should be made on the main thread.

## Constraints

- Do not add SDL3 includes or native handles to `engine/ui`.
- Do not expose `SDL_Window*`, OS handles, IME native objects, virtual keyboard objects, or platform SDK objects to game/UI public contracts.
- Do not implement text shaping, bidirectional reordering, font loading/rasterization, OS accessibility publication, image decoding, IME composition event publication, or virtual keyboard policy.
- Do not add new third-party dependencies; SDL3 is already the optional desktop runtime/platform dependency.
- New behavior must have failing tests before implementation.

## Files

- Create: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Create: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
- Modify: `engine/platform/sdl3/CMakeLists.txt`
- Modify: `tests/unit/sdl3_platform_tests.cpp`
- Modify after green: `docs/superpowers/plans/README.md`
- Modify after green: `docs/current-capabilities.md`
- Modify after green: `docs/roadmap.md`
- Modify after green: `docs/ui.md`
- Modify after green: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify after green: `engine/agent/manifest.json`

## Tasks

### Task 1: RED tests for SDL3 platform text input begin/end

**Files:**
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Add failing tests**

Add the new public header include and SDL keyboard include:

```cpp
#include "mirakana/platform/sdl3/sdl_ui_platform_integration.hpp"

#include <SDL3/SDL_keyboard.h>
```

Add tests near the existing SDL3 window/platform tests:

```cpp
MK_TEST("sdl3 platform integration adapter begins text input with a window text area") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{"SDL Text Input Test", mirakana::WindowExtent{320, 240}});
    mirakana::SdlPlatformIntegrationAdapter adapter(window);

    const mirakana::ui::PlatformTextInputRequest request{
        mirakana::ui::ElementId{"chat.input"},
        mirakana::ui::Rect{4.0F, 8.0F, 160.0F, 24.0F},
    };

    const auto result = mirakana::ui::begin_platform_text_input(adapter, request);

    MK_REQUIRE(result.succeeded());
    auto* native = reinterpret_cast<SDL_Window*>(window.native_window().value);
    MK_REQUIRE(SDL_TextInputActive(native));

    SDL_Rect area{};
    int cursor = -1;
    MK_REQUIRE(SDL_GetTextInputArea(native, &area, &cursor));
    MK_REQUIRE(area.x == 4);
    MK_REQUIRE(area.y == 8);
    MK_REQUIRE(area.w == 160);
    MK_REQUIRE(area.h == 24);
    MK_REQUIRE(cursor == 0);

    const auto end_result = mirakana::ui::end_platform_text_input(adapter, request.target);
    MK_REQUIRE(end_result.succeeded());
    MK_REQUIRE(!SDL_TextInputActive(native));
}

MK_TEST("sdl3 platform integration adapter rejects invalid direct requests before calling SDL") {
    mirakana::SdlRuntime runtime(mirakana::SdlRuntimeDesc{.video_driver_hint = "dummy"});
    mirakana::SdlWindow window(mirakana::WindowDesc{"SDL Text Input Invalid Test", mirakana::WindowExtent{320, 240}});
    mirakana::SdlPlatformIntegrationAdapter adapter(window);

    bool threw = false;
    try {
        adapter.begin_text_input(mirakana::ui::PlatformTextInputRequest{
            mirakana::ui::ElementId{},
            mirakana::ui::Rect{0.0F, 0.0F, 10.0F, 10.0F},
        });
    } catch (const std::invalid_argument& error) {
        threw = std::string{error.what()} == "platform text input target must not be empty";
    }

    MK_REQUIRE(threw);
    auto* native = reinterpret_cast<SDL_Window*>(window.native_window().value);
    MK_REQUIRE(!SDL_TextInputActive(native));
}
```

- [x] **Step 2: Run test to verify it fails**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
```

Expected: FAIL because `mirakana/platform/sdl3/sdl_ui_platform_integration.hpp` and `SdlPlatformIntegrationAdapter` do not exist.

### Task 2: Implement the SDL3 adapter

**Files:**
- Create: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`
- Create: `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp`
- Modify: `engine/platform/sdl3/CMakeLists.txt`

- [x] **Step 1: Add public adapter header**

Create `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp`:

```cpp
// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/sdl3/sdl_window.hpp"
#include "mirakana/ui/ui.hpp"

namespace mirakana {

class SdlPlatformIntegrationAdapter final : public ui::IPlatformIntegrationAdapter {
  public:
    explicit SdlPlatformIntegrationAdapter(SdlWindow& window) noexcept;

    void begin_text_input(const ui::PlatformTextInputRequest& request) override;
    void end_text_input(const ui::ElementId& target) override;

  private:
    SdlWindow* window_{nullptr};
};

} // namespace mirakana
```

- [x] **Step 2: Add implementation**

Create `engine/platform/sdl3/src/sdl_ui_platform_integration.cpp` with:

- direct validation through `mirakana::ui::plan_platform_text_input_session` and `mirakana::ui::plan_platform_text_input_end`;
- `SDL_SetTextInputArea(window, &area, 0)` before `SDL_StartTextInput`;
- `SDL_StopTextInput(window)` and `SDL_SetTextInputArea(window, nullptr, 0)` on end;
- stable `std::invalid_argument` for first-party request validation failures;
- `std::runtime_error("<operation> failed: <SDL_GetError>")` for SDL API failures.

- [x] **Step 3: Wire CMake**

Modify `engine/platform/sdl3/CMakeLists.txt`:

```cmake
add_library(MK_platform_sdl3
    src/sdl_clipboard.cpp
    src/sdl_cursor.cpp
    src/sdl_file_dialog.cpp
    src/sdl_input.cpp
    src/sdl_ui_platform_integration.cpp
    src/sdl_window.cpp
)

target_link_libraries(MK_platform_sdl3
    PUBLIC
        MK_platform
        MK_ui
    PRIVATE
        SDL3::SDL3
)
```

- [x] **Step 4: Run focused tests**

Run:

```powershell
cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests
ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"
```

Expected: PASS.

### Task 3: Sync docs, manifest, and active plan pointers

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ui.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Update capability truth**

Record the new narrow capability as optional SDL3 platform text-input begin/end adapter evidence. Keep `production-ui-importer-platform-adapters` non-ready because text shaping, font rasterization, OS accessibility bridge publication, IME composition event publication, virtual keyboard policy, broader image codecs, and arbitrary importer adapters remain unsupported.

- [x] **Step 2: Update plan registry and manifest**

Point `currentActivePlan` back to the master plan after completion. Add the new public header to the `MK_platform_sdl3` public header list if present, and update the `MK_platform_sdl3` purpose text to mention `SdlPlatformIntegrationAdapter`.

- [x] **Step 3: Run static/docs checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: PASS; production audit still reports known unsupported gaps unless a later audit intentionally reclassifies the row.

### Task 4: Final validation and commit

**Files:**
- All files changed in this slice.

- [x] **Step 1: Run final validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: PASS or a concrete host/tool blocker recorded in this plan.

- [ ] **Step 2: Commit**

Run:

```powershell
git status --short
git add docs/current-capabilities.md docs/roadmap.md docs/ui.md docs/superpowers/plans/README.md docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md docs/superpowers/plans/2026-05-08-runtime-ui-sdl3-platform-text-input-adapter-v1.md engine/agent/manifest.json engine/platform/sdl3/CMakeLists.txt engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_ui_platform_integration.hpp engine/platform/sdl3/src/sdl_ui_platform_integration.cpp tests/unit/sdl3_platform_tests.cpp
git diff --cached --check
git commit -m "feat: add sdl3 platform text input adapter"
```

Expected: clean commit containing only this slice.

## Done When

- `SdlPlatformIntegrationAdapter` implements `mirakana::ui::IPlatformIntegrationAdapter` in the optional SDL3 platform target.
- Focused SDL3 platform tests prove begin/end dispatch starts/stops SDL text input and sets the SDL text input area.
- Direct invalid adapter requests fail before calling SDL.
- `MK_ui` remains SDL3-free.
- Docs, plan registry, master plan notes, and manifest agree on the exact supported and unsupported scope.
- Final validation evidence is recorded below.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED `cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests` | Expected failure | Failed on missing `mirakana/platform/sdl3/sdl_ui_platform_integration.hpp` before implementation. |
| `cmake --build --preset desktop-runtime --target MK_sdl3_platform_tests` | Pass | Focused SDL3 platform build. |
| `ctest --preset desktop-runtime --output-on-failure -R "^MK_sdl3_platform_tests$"` | Pass | Focused SDL3 platform tests. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Pass | Repository formatter. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Formatting gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public header/API boundary gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Preset desktop-runtime -Files engine/platform/sdl3/src/sdl_ui_platform_integration.cpp,tests/unit/sdl3_platform_tests.cpp -MaxFiles 2` | Pass | Targeted optional SDL3 compile-database lane; reports existing style warnings but ends `tidy-check: ok (2 files)`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | Schema validation; interim active-child pointer failure was resolved by returning `currentActivePlan` to the master plan before slice close. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Agent manifest/docs consistency. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Unsupported gap audit still reports `unsupported_gaps=11` and keeps `production-ui-importer-platform-adapters` planned. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Default completion gate; CTest 29/29 passed, Apple/Metal host gates remained diagnostic-only, and production audit still reports `unsupported_gaps=11`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Commit gate. |
