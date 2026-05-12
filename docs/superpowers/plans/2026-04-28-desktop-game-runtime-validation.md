# Desktop Game Runtime Validation Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow, repeatable validation lane for the editor-independent desktop game runtime shell so `games/sample_desktop_runtime_shell` can run as a finite smoke test without relying on the full editor GUI lane as the only proof.

**Architecture:** Keep gameplay code on `mirakana::GameApp` and `mirakana::IRenderer`. Keep SDL3 selection in the optional host executable. Add command-line smoke options to the sample, register a CTest smoke that uses the SDL dummy video driver, and add a PowerShell 7 `tools/*.ps1` wrapper that builds only the runtime-host tests and sample target before running the focused CTest regex.

**Tech Stack:** C++23, CMake, `MK_runtime_host`, `MK_runtime_host_sdl3`, `MK_platform_sdl3`, `MK_renderer`, SDL3 optional desktop GUI lane, PowerShell validation wrapper.

---

## File Structure

- Modify `games/sample_desktop_runtime_shell/main.cpp` for `--smoke`, `--max-frames`, and `--video-driver` options.
- Modify `games/CMakeLists.txt` to register `sample_desktop_runtime_shell_smoke` under `MK_ENABLE_DESKTOP_GUI`.
- Add `tools/validate-desktop-game-runtime.ps1`.
- Modify `package.json`, `games/sample_desktop_runtime_shell/README.md`, `games/sample_desktop_runtime_shell/game.agent.json`, `docs/testing.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `engine/agent/manifest.json`, and game-development skills to advertise the focused lane.

## Task 1: Sample Smoke Contract

**Files:**
- Modify: `games/sample_desktop_runtime_shell/main.cpp`
- Modify: `games/sample_desktop_runtime_shell/README.md`

- [x] **Step 1: Add finite smoke options**

Add argument parsing for:

- `--smoke`: sets `max_frames` to a small finite value and uses SDL video driver hint `dummy` unless the caller overrides it.
- `--max-frames <n>`: runs through `DesktopRunConfig::max_frames`.
- `--video-driver <name>`: passes a video driver hint into `SdlRuntimeDesc`.

Keep default execution interactive: no max frame cap, normal SDL video driver choice, close window or press Escape to stop.

- [x] **Step 2: Keep gameplay on renderer contracts**

Change the sample game object to depend on `mirakana::IRenderer&` instead of `mirakana::NullRenderer&`. The executable can still instantiate `NullRenderer` as the current fallback.

## Task 2: Focused CTest And Script

**Files:**
- Modify: `games/CMakeLists.txt`
- Add: `tools/validate-desktop-game-runtime.ps1`
- Modify: `package.json`

- [x] **Step 1: Register sample smoke CTest**

Under `MK_ENABLE_DESKTOP_GUI`, register:

```cmake
add_test(NAME sample_desktop_runtime_shell_smoke COMMAND sample_desktop_runtime_shell --smoke)
```

- [x] **Step 2: Add focused wrapper**

Add `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` backed by a PowerShell script that configures `desktop-gui`, builds `MK_runtime_host_tests`, `MK_runtime_host_sdl3_tests`, and `sample_desktop_runtime_shell`, then runs CTest for `MK_runtime_host_tests|MK_runtime_host_sdl3_tests|sample_desktop_runtime_shell_smoke`.

## Task 3: Documentation And Manifest Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `games/sample_desktop_runtime_shell/game.agent.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`

- [x] **Step 1: Register active slice**

Set this plan as the active slice in the registry.

- [x] **Step 2: Update validation guidance**

Document that `desktop-game-runtime` is a focused optional validation lane for the game runtime shell and sample smoke, while `build-gui` remains the broader editor/SDL3 lane.

## Task 4: Verification

**Files:**
- No direct file edits.

- [x] **Step 1: Run focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

Expected: runtime host tests, SDL3 event-pump tests, and `sample_desktop_runtime_shell_smoke` pass.

- [x] **Step 2: Run default validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: default validation remains green.
