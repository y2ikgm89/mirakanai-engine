# Touch Gesture Input Foundation v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a small first-party touch gesture foundation over existing pointer input so games can consume touch pan and pinch primitives without SDL3/native handles.

**Architecture:** `MK_platform` keeps raw touch contacts in `VirtualPointerInput` and exposes a deterministic `TouchGestureSnapshot` derived from active `PointerKind::touch` pointers. `MK_platform_sdl3` continues translating official SDL3 finger events into pointer rows, ignores SDL touch-generated virtual mouse rows, and maps touch cancel rows to pointer release.

**Tech Stack:** C++23, `MK_platform`, optional SDL3 backend, focused unit tests, repository PowerShell validation.

---

## Context

The production master plan keeps ready claims narrow and allows clean breaking changes when MVP-era contracts block production behavior. Current docs list low-level touch pointers as supported while `touch gestures` remain follow-up. SDL3 official documentation reports finger coordinates as normalized and not clamped, and notes touch may emit virtual mouse rows; applications that distinguish touch from mouse should ignore mouse rows with `SDL_TOUCH_MOUSEID`.

## Constraints

- Keep platform contracts first-party and host-independent.
- Do not expose SDL3 handles or move SDL3 into runtime/UI contracts.
- Do not add third-party dependencies.
- Do not claim broad gesture recognition; this slice covers touch contact count, centroid, centroid delta, and two-touch pinch scale primitives only.
- Keep higher-level named swipe/rotate recognizers, radial stick deadzones, response curves, device assignment, and per-device profiles as follow-up work.

## Done When

- `VirtualPointerInput` exposes a deterministic touch gesture snapshot over active touch pointers.
- SDL3 touch cancel releases the matching touch pointer.
- SDL3 touch-generated virtual mouse rows do not create duplicate mouse pointer input.
- Current capabilities, roadmap, architecture, AI game guidance, and composed manifest describe the new ready surface and remaining follow-ups.
- Focused tests and final repository validation run or record concrete blockers.

## Tasks

### Task 1: Touch Gesture Snapshot API

**Files:**
- Modify: `engine/platform/include/mirakana/platform/input.hpp`
- Modify: `engine/platform/src/input.cpp`
- Test: `tests/unit/core_tests.cpp`

- [x] Add a failing `MK_core_tests` case that presses two touch pointers, advances a frame, moves both pointers, and expects `touch_gesture()` to report `touch_count=2`, centroid movement, and `pinch_scale=2.0`.
- [x] Implement `TouchGestureSnapshot` and `VirtualPointerInput::touch_gesture()`.
- [x] Keep the calculation deterministic by using sorted active touch pointers and only reporting pinch scale when exactly two touches have a positive previous distance.

### Task 2: SDL3 Touch Event Hardening

**Files:**
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Test: `tests/unit/sdl3_platform_tests.cpp`

- [x] Add failing `MK_sdl3_platform_tests` cases for `SDL_EVENT_FINGER_CANCELED` and `SDL_TOUCH_MOUSEID` mouse rows.
- [x] Map `SDL_EVENT_FINGER_CANCELED` to `pointer_released`.
- [x] Ignore `SDL_TOUCH_MOUSEID` mouse button and motion rows so touch and mouse state do not duplicate the same physical touch.

### Task 3: Documentation And Manifest Truth

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generated: `engine/agent/manifest.json`

- [x] Update current-truth docs to say touch gesture primitives are ready while named gestures/rotation and broader input middleware remain follow-up.
- [x] Update manifest fragments only, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 4: Validation

**Commands:**
- `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests`
- `ctest --preset dev -R "MK_core_tests|MK_sdl3_platform_tests" --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

- [x] Run focused build/test after implementation.
- [x] Run API boundary and full validation before reporting completion, or record the exact local blocker.

## Validation Evidence

- `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests` passed.
- `ctest --preset dev -R "MK_core_tests|MK_sdl3_platform_tests" --output-on-failure` passed, 2/2 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Direct `clang-format --dry-run --Werror` on touched C++ files passed.
- `git diff --check -- <slice files>` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; 50/50 tests passed. Metal and Apple host diagnostics remained host-gated/diagnostic-only on this Windows host.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` remains blocked by pre-existing formatting issues in `engine/runtime/include/mirakana/runtime/resource_runtime.hpp` and `engine/runtime/src/resource_runtime.cpp` around `commit_runtime_resident_package_unmount_v2`; touched C++ files pass the direct clang-format dry-run above.
