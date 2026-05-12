# Touch Gesture Recognizer v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party touch gesture recognizer so games can consume named tap, double-tap, long-press, pan, swipe, pinch, rotate, and canceled gesture events without SDL3 or native handles.

**Architecture:** `MK_platform` remains the owner of platform-neutral virtual input. `VirtualPointerInput` tracks touch cancellation distinctly from release, and `TouchGestureRecognizer` derives deterministic value-type gesture events from active/released/canceled `PointerKind::touch` pointers. `MK_platform_sdl3` maps `SDL_EVENT_FINGER_CANCELED` to the first-party canceled pointer path while continuing to ignore touch-generated virtual mouse rows.

**Tech Stack:** C++23, `MK_platform`, optional SDL3 backend, TDD unit tests, repository PowerShell validation.

---

## Context

The production master plan keeps broad input middleware out of current ready claims while naming touch gesture recognizers such as swipe/rotate as follow-up work. The previous touch primitive slice added `VirtualPointerInput::touch_gesture()` for active touch count, centroid, centroid delta, and pinch scale. Official SDL3 touch guidance uses dedicated finger events, notes normalized but unclamped finger coordinates, and says touch-generated virtual mouse events should be ignored via `SDL_TOUCH_MOUSEID` when touch and mouse are distinct. Android and UIKit gesture guidance both treat pointer ids/cancel events as first-class and model continuous gestures through explicit began/changed/ended/canceled states.

## Constraints

- Keep the recognizer in first-party `MK_platform`; do not expose SDL3, UIKit, Android, OS handles, or UI middleware types.
- Preserve deterministic value-type output suitable for tests and generated games.
- Keep runtime input-action integration as a later slice; this slice only produces named gesture events.
- Do not add dependencies.
- Keep named gesture scope narrow: tap, double-tap, long-press, pan, swipe, pinch, rotate, and cancel state.

## Done When

- `VirtualPointerInput` has distinct canceled pointer state and SDL3 finger cancel reaches it.
- `TouchGestureRecognizer` emits discrete tap/double-tap/long-press events and continuous pan/pinch/rotate events with deterministic phases.
- Swipe events derive from released one-finger movement using explicit distance and velocity thresholds.
- Docs, roadmap, AI guidance, and manifest describe named touch gesture recognition as ready while action-map binding and broader input middleware remain follow-up.
- Focused tests, public API boundary checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record exact blockers.

## Tasks

### Task 1: RED Tests For Named Gesture API

**Files:**
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] Add a failing `MK_core_tests` case for tap, double-tap, and long-press using `TouchGestureRecognizer`.
- [x] Add a failing `MK_core_tests` case for pan begin/change/end plus swipe on release.
- [x] Add a failing `MK_core_tests` case for pinch, rotate, and cancel phases.
- [x] Update the SDL3 touch-cancel test to expect `pointer_canceled` instead of `pointer_released`.
- [x] Run `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests` and confirm failure is caused by missing gesture/cancel APIs.

### Task 2: Pointer Cancel Contract

**Files:**
- Modify: `engine/platform/include/mirakana/platform/input.hpp`
- Modify: `engine/platform/src/input.cpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Test: `tests/unit/sdl3_platform_tests.cpp`

- [x] Add `PointerState::canceled`, `VirtualPointerInput::cancel(PointerId)`, and `VirtualPointerInput::pointer_canceled(PointerId)`.
- [x] Add `SdlWindowEventKind::pointer_canceled`.
- [x] Map `SDL_EVENT_FINGER_CANCELED` to `pointer_canceled`.
- [x] Dispatch `pointer_canceled` through `SdlWindow::handle_event`.
- [x] Run `cmake --build --preset dev --target MK_sdl3_platform_tests` and `ctest --preset dev -R MK_sdl3_platform_tests --output-on-failure`.

### Task 3: Touch Gesture Recognizer

**Files:**
- Modify: `engine/platform/include/mirakana/platform/input.hpp`
- Modify: `engine/platform/src/input.cpp`
- Test: `tests/unit/core_tests.cpp`

- [x] Add `TouchGestureKind`, `TouchGesturePhase`, `TouchGestureEvent`, `TouchGestureRecognizerConfig`, and `TouchGestureRecognizer`.
- [x] Implement one-finger discrete recognition for tap, double-tap, and long-press.
- [x] Implement one-finger continuous pan and release-time swipe.
- [x] Implement two-finger pinch and rotate recognition with began/changed/ended/canceled phases.
- [x] Run `cmake --build --preset dev --target MK_core_tests` and `ctest --preset dev -R MK_core_tests --output-on-failure`.

### Task 4: Current-Truth Docs And Manifest

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generated: `engine/agent/manifest.json`

- [x] Update docs to state named touch gesture recognition is ready in `MK_platform`.
- [x] Keep action-map gesture sources, runtime/game rebinding panels, glyphs, per-device profiles, and broad input middleware as follow-up.
- [x] Update manifest fragments only, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.

### Task 5: Validation

**Commands:**
- `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests`
- `ctest --preset dev -R "MK_core_tests|MK_sdl3_platform_tests" --output-on-failure`
- Direct clang-format dry-run on touched C++ files
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

- [x] Run focused build/test.
- [x] Run formatting and API boundary checks.
- [x] Run full repository validation.
- [x] Record validation evidence and any host-gated diagnostic-only rows in this plan.

## Validation Evidence

- RED: `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests` failed before implementation because `TouchGestureRecognizerConfig`, `TouchGestureRecognizer`, `TouchGestureKind`, `TouchGesturePhase`, and pointer cancel APIs were not present.
- PASS: `git diff --check -- ...` on the touched slice files exited 0.
- PASS: direct `clang-format --dry-run --Werror` on touched C++ files exited 0.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` reported `public-api-boundary-check: ok`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/src/input.cpp,engine/platform/sdl3/src/sdl_window.cpp` reported `tidy-check: ok (2 files)`; existing SDL3 include/designated-initializer and array-index warnings remain warning-only.
- PASS: `cmake --build --preset dev --target MK_core_tests MK_sdl3_platform_tests` built both targets.
- PASS: `ctest --preset dev -R "MK_core_tests|MK_sdl3_platform_tests" --output-on-failure` passed 2/2 tests.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` reported `validate: ok`; CTest passed 51/51. Windows host diagnostics still report Metal shader tools and Apple host evidence as host-gated/diagnostic-only, matching the current repository gates.
