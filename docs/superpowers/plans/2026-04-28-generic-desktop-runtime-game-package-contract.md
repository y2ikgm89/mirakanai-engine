# Generic Desktop Runtime Game Package Contract Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let an SDL3 windowed `games/<name>` executable opt into the desktop runtime package lane without hardcoding `sample_desktop_runtime_shell` through every CMake and PowerShell validation step.

**Architecture:** Add a small CMake registration helper for SDL3 desktop runtime game executables, prove it with a non-shell sample that uses `mirakana::SdlDesktopGameHost`, and generalize the desktop runtime package/installed-smoke scripts with an explicit target and smoke-argument contract. Keep the default package path on `sample_desktop_runtime_shell` with DXIL artifact validation, while allowing shader-free windowed games to validate through `NullRenderer` fallback.

**Tech Stack:** C++23, target-based CMake, SDL3 optional desktop runtime lane, `MK_runtime_host_sdl3_presentation`, PowerShell package validation scripts, CTest, game manifests, AI-facing manifest/docs.

---

## File Structure

- Create `games/sample_desktop_runtime_game/main.cpp`, `README.md`, and `game.agent.json` for a minimal non-shell SDL3 windowed desktop runtime game.
- Modify `games/CMakeLists.txt` with a `MK_add_desktop_runtime_game` helper used by both the existing shell sample and the new non-shell sample.
- Modify root `CMakeLists.txt` with `MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET` selection so the desktop runtime install/package lane can choose a registered desktop runtime game target.
- Modify `tools/package-desktop-runtime.ps1` and `tools/validate-installed-desktop-runtime.ps1` so installed smoke validation is target-driven instead of hardcoded to `sample_desktop_runtime_shell`.
- Modify `tools/validate-desktop-game-runtime.ps1` so the focused source-tree lane builds and tests the non-shell proof target.
- Update docs, manifests, skills, and subagent guidance to describe generic per-game desktop runtime package selection without overstating Vulkan/Metal or real-window D3D12 automation.

## Task 1: Red Checks For Generic Selection

- [x] **Step 1: Confirm package selection is currently absent**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected before implementation: the script does not honor the target as a real contract and still enters the existing sample-shell package path.

- [x] **Step 2: Confirm the non-shell windowed target is absent**

Run:

```powershell
cmake --build --preset desktop-runtime --target sample_desktop_runtime_game
```

Expected before implementation: build-system failure because the target has not been registered.

## Task 2: CMake Desktop Runtime Game Contract

- [x] **Step 1: Add `MK_add_desktop_runtime_game`**

Implement a helper inside `games/CMakeLists.txt` that creates an SDL3 desktop runtime executable, links the existing runtime host/presentation/platform targets, copies the SDL3 runtime DLL beside the target after build, records `MK_DESKTOP_RUNTIME_GAME`, and registers a `<target>_smoke` CTest when `SMOKE_ARGS` are provided.

- [x] **Step 2: Convert the existing shell sample**

Use the helper for `sample_desktop_runtime_shell`, preserve its shader artifact custom target, preserve `sample_desktop_runtime_shell_shader_artifacts_smoke`, and keep default `--smoke` behavior unchanged.

- [x] **Step 3: Add install/package target selection**

Add `MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET` as a cache string in root `CMakeLists.txt`. When set, validate that the target exists and was registered as a desktop runtime game, then install that selected executable instead of all source-tree runtime executables.

## Task 3: Non-Shell Windowed Game Proof

- [x] **Step 1: Add `sample_desktop_runtime_game`**

Create a small game executable under `games/sample_desktop_runtime_game` that implements `mirakana::GameApp`, opens through `mirakana::SdlDesktopGameHost`, draws through the host-owned renderer, supports `--smoke`, `--max-frames`, and `--video-driver`, and returns non-zero only when smoke frame counts do not match.

- [x] **Step 2: Add the game manifest and README**

Record `desktop-game-runtime` and `desktop-runtime-release` packaging targets honestly. Note that this proof uses host-owned `NullRenderer` fallback by default and does not require packaged DXIL artifacts.

## Task 4: Generic Package And Installed Validation Scripts

- [x] **Step 1: Generalize `tools/package-desktop-runtime.ps1`**

Add `-GameTarget`, `-SmokeArgs`, and `-RequireD3d12Shaders` parameters. Default to `sample_desktop_runtime_shell` with `--smoke --require-d3d12-shaders`, pass `-DMK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET=<target>` and the DXIL requirement to CMake, run the selected target smoke CTest, install, validate the SDK consumer, validate the installed selected executable, and run CPack.

- [x] **Step 2: Generalize `tools/validate-installed-desktop-runtime.ps1`**

Add matching target, smoke args, and shader requirement parameters. Validate the selected executable exists under `bin`, check `SDL3.dll` on Windows, check `bin/shaders/runtime_shell.*.dxil` only when shader validation is requested, and run the selected installed executable with the provided smoke args.

## Task 5: Documentation And Agent Contract Sync

- [x] Update `docs/superpowers/plans/README.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `docs/specs/game-template.md`, `docs/specs/game-prompt-pack.md`, `docs/specs/generated-game-validation-scenarios.md`, `docs/release.md`, `docs/testing.md`, `docs/dependencies.md`, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, `.claude/skills/gameengine-game-development/SKILL.md`, and `.claude/agents/gameplay-builder.md`.

## Task 6: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run the generic non-shell package proof:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

- [x] Run the default sample-shell package lane:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public/package boundary checks are affected.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record any diagnostic-only host/toolchain blockers explicitly.

## Verification Results

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed after sandbox escalation. The focused lane built the SDL3 desktop runtime host and ran 7/7 tests, including `sample_desktop_runtime_shell_smoke`, `sample_desktop_runtime_shell_shader_artifacts_smoke`, and `sample_desktop_runtime_game_smoke`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: passed. The installed package smoke ran `sample_desktop_runtime_game` with `renderer=null`, `frames=2`, and `game_frames=2`, then generated the desktop runtime ZIP and checksum.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: passed. The default `sample_desktop_runtime_shell` package path still validates the shell smoke and packaged `runtime_shell` DXIL artifacts before installed smoke and CPack.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed.
- Diagnostic-only blockers remain host/toolchain scoped: Vulkan strict shader validation needs DXC SPIR-V CodeGen and `spirv-val`; Metal shader/library checks need Apple `metal`/`metallib`; Apple packaging needs macOS/Xcode tools; Android release/device smoke checks need signing/device setup; strict tidy analysis remains diagnostic when no compile database is present for the active generator.
