# API And Package Boundary Cleanup Implementation Plan (2026-04-27)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix the review findings so public APIs, CMake policy, package compatibility, and installed dependency resolution match the project's clean pre-release architecture.

**Architecture:** Tighten validation first, then remove SDL types from installed/public API surfaces. Keep optional third-party dependencies behind vcpkg manifest features while making installed CMake packages self-describing when those optional targets are exported.

**Tech Stack:** C++23, CMake, CTest, CPack, PowerShell validation scripts, vcpkg manifest features, SDL3 optional desktop adapter.

---

### Task 1: Strengthen Policy Checks

**Files:**
- Modify: `tools/check-public-api-boundaries.ps1`
- Modify: `tools/check-cpp-standard-policy.ps1`
- Modify: `tools/check-dependency-policy.ps1`

- [x] **Step 1: Write failing checks**

Add checks that reject SDL/ImGui/Vulkan/Metal/Objective-C/Android/native UI symbols in public headers, require CMake 3.30 or newer for configured module/import-std policy, require `ExactVersion`, and require asset-importers dependency discovery in `GameEngineConfig.cmake.in`.

- [x] **Step 2: Run checks to verify they fail**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

Expected before implementation: SDL public API, CMake minimum/compatibility, and missing asset-importers dependency checks fail.

- [x] **Step 3: Keep checks narrow**

Only forbid native SDK and third-party surface leaks in public/installable headers. Do not block first-party opaque handles such as `std::uintptr_t` value wrappers.

### Task 2: Remove SDL Types From Public SDL3 Adapter Headers

**Files:**
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_window.hpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_file_dialog.hpp`
- Modify: `engine/platform/sdl3/include/mirakana/platform/sdl3/sdl_cursor.hpp`
- Modify: `engine/platform/sdl3/src/sdl_window.cpp`
- Modify: `engine/platform/sdl3/src/sdl_file_dialog.cpp`
- Modify: `engine/platform/sdl3/src/sdl_cursor.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `tests/unit/sdl3_platform_tests.cpp`

- [x] **Step 1: Introduce first-party opaque wrappers**

Replace public `SDL_Window*`, `SDL_Event`, `SDL_FileDialogType`, and `SDL_DialogFileFilter` surface types with first-party methods or private implementation details. Keep SDL includes in `.cpp` or test files only.

- [x] **Step 2: Preserve event behavior through source-private translation**

Move SDL event handling into implementation-owned functions so tests still cover keyboard, pointer, gamepad, lifecycle, resize, and close behavior without making SDL event types part of installed headers.

- [x] **Step 3: Run GUI tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

Expected: `mirakana_sdl3_platform_tests` and `mirakana_sdl3_audio_tests` pass with no public SDL header exposure.

### Task 3: Clean CMake And Installed Package Contract

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `cmake/GameEngineConfig.cmake.in`
- Modify: `docs/cpp-standard.md`
- Modify: `docs/dependencies.md`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Align CMake minimum with active feature policy**

Set the project and presets to CMake 3.30 or newer so `CMAKE_CXX_MODULE_STD` and module scanning policy are not advertised below their official support floor.

- [x] **Step 2: Remove false package compatibility**

Change generated package version compatibility from `SameMinorVersion` to `ExactVersion` while the project explicitly does not preserve backward compatibility.

- [x] **Step 3: Make optional package dependencies discoverable**

Add an installed package flag for asset-importers and call `find_dependency(SPNG CONFIG)` and `find_dependency(fastgltf CONFIG)` only when the installed export actually contains those private link dependencies. Keep miniaudio header-only and private.

- [x] **Step 4: Validate installed packages**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
```

Expected: default Release package and optional asset-importers lane still build and test.

### Task 4: Final Verification

**Files:**
- No additional source files expected.

- [x] **Step 1: Run default validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: all default checks and tests pass.

- [x] **Step 2: Run focused optional validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1
```

Expected: all pass, with only documented diagnostic-only shader/mobile blockers remaining.

**Verification note (2026-04-27):** default/package policy implementation was already present when this plan was resumed. Focused validation passed with:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-cpp-standard-policy.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-asset-importers.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`

`build-gui` and `build-asset-importers` required unsandboxed execution because vcpkg tool extraction hit sandboxed `CreateFileW stdin` access denial; both passed after rerun. Remaining shader/mobile/toolchain gaps are diagnostic-only host capability blockers tracked by validation output.
