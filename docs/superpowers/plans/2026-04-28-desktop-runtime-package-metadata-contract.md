# Desktop Runtime Package Metadata Contract Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make registered desktop runtime game package metadata explicit so validation and packaging scripts can select `games/<name>` targets from CMake-declared smoke and artifact requirements instead of target-name conventions.

**Architecture:** Extend `MK_add_desktop_runtime_game` with package metadata target properties and generate a small desktop-runtime metadata JSON file at configure time. The focused desktop runtime and package scripts read that file to discover registered games, smoke args, and shader-artifact requirements while preserving the default `sample_desktop_runtime_shell` DXIL package lane and shader-free `sample_desktop_runtime_game` proof.

**Tech Stack:** C++23 policy unchanged, target-based CMake, PowerShell validation scripts, SDL3 optional desktop runtime lane, existing game manifests and AI-facing docs.

---

## File Structure

- Modify `games/CMakeLists.txt` to store desktop runtime target metadata: source-tree smoke args, installed package smoke args, and D3D12 shader-artifact requirement.
- Modify root `CMakeLists.txt` to validate selected package target metadata and write `desktop-runtime-games.json` into the active build directory, then install it into `share/GameEngine`.
- Modify `tools/common.ps1` with small metadata reader helpers.
- Modify `tools/validate-desktop-game-runtime.ps1` to build/test registered desktop runtime games from generated metadata.
- Modify `tools/package-desktop-runtime.ps1` and `tools/validate-installed-desktop-runtime.ps1` to resolve smoke args and shader-artifact requirements from metadata, with explicit script arguments only adding or replacing the supported pieces.
- Update docs, manifest, and agent guidance only where the metadata contract changes the workflow.

## Task 1: Red Metadata Check

**Files:**
- Modify: `tools/validate-desktop-game-runtime.ps1`

- [x] **Step 1: Require generated metadata in the focused lane**

After `cmake --preset desktop-runtime`, read `out/build/desktop-runtime/desktop-runtime-games.json`. Fail if it is missing, if it lacks `sample_desktop_runtime_shell`, if it lacks `sample_desktop_runtime_game`, or if any registered game lacks source-tree smoke args.

- [x] **Step 2: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

Expected before CMake metadata generation: failure because `desktop-runtime-games.json` is not produced yet. Also verify the installed selected-target guard by reproducing that a reused install prefix could previously run stale `sample_desktop_runtime_shell` after packaging `sample_desktop_runtime_game`.

## Task 2: CMake Metadata Contract

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`

- [x] **Step 1: Store target metadata**

Extend `MK_add_desktop_runtime_game` with:

- `PACKAGE_SMOKE_ARGS` multi-value argument, defaulting to `SMOKE_ARGS`
- `REQUIRES_D3D12_SHADERS` option
- target properties `MK_DESKTOP_RUNTIME_SMOKE_ARGS`, `MK_DESKTOP_RUNTIME_PACKAGE_SMOKE_ARGS`, and `MK_DESKTOP_RUNTIME_REQUIRES_D3D12_SHADERS`
- global property `MK_DESKTOP_RUNTIME_GAMES`

Require `SMOKE_ARGS` for every registered desktop runtime game. Set `sample_desktop_runtime_shell` package smoke args to `--smoke --require-d3d12-shaders` and mark it `REQUIRES_D3D12_SHADERS`. Keep `sample_desktop_runtime_game` shader-free with package smoke args inherited from `--smoke`.

- [x] **Step 2: Generate metadata JSON**

Write `desktop-runtime-games.json` into `CMAKE_BINARY_DIR` with:

- `schemaVersion: 1`
- `selectedPackageTarget`
- `registeredGames[]` entries containing `target`, `smokeArgs`, `packageSmokeArgs`, and `requiresD3d12Shaders`

Validate that a selected package target is registered and has non-empty package smoke args. Install the JSON file into `share/GameEngine` for installed validation.

- [x] **Step 3: Move default DXIL requirement into target metadata**

Set `MK_REQUIRE_DESKTOP_RUNTIME_DXIL` default to `OFF` in the `desktop-runtime-release` preset, and make the shell target metadata require DXIL when that target is selected. Preserve explicit `-DMK_REQUIRE_DESKTOP_RUNTIME_DXIL=ON` as an override.

- [x] **Step 4: Verify GREEN for metadata**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
```

Expected after implementation: focused lane reads metadata, builds registered desktop runtime game targets dynamically, and runs the existing runtime host and sample smoke tests.

## Task 3: Package Scripts Use Metadata

**Files:**
- Modify: `tools/common.ps1`
- Modify: `tools/package-desktop-runtime.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`

- [x] **Step 1: Add metadata readers**

Add helpers that read `desktop-runtime-games.json`, select a registered game by target, and return package smoke args plus D3D12 shader-artifact requirement.

- [x] **Step 2: Update package script**

After configure, read build metadata for the selected target. Require metadata `selectedPackageTarget` to match the requested target. If `-SmokeArgs` is not provided, use metadata `packageSmokeArgs`. If `-RequireD3d12Shaders` is not provided, use metadata `requiresD3d12Shaders`. Clean the fixed desktop-runtime install prefix before installing so stale files cannot satisfy the installed smoke. Keep `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` defaulting to `sample_desktop_runtime_shell`.

- [x] **Step 3: Update installed validation**

Read installed metadata from `share/GameEngine/desktop-runtime-games.json` when available. Use `selectedPackageTarget` when `-GameTarget` is omitted, reject explicit target mismatches, then use selected target package smoke args and shader-artifact requirement; `-SmokeArgs` replaces smoke args and `-RequireD3d12Shaders` adds a shader-artifact requirement.

- [x] **Step 4: Verify package lanes**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
```

Expected: non-shell package validates with metadata `--smoke` and no DXIL requirement; default shell package validates with metadata `--smoke --require-d3d12-shaders` and installed DXIL artifacts.

## Task 4: Documentation And Agent Contract Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Register this slice**

Add this plan as active during implementation and move it to completed after validation.

- [x] **Step 2: Update workflow wording**

Describe registered desktop runtime game metadata as the source of package smoke args and shader-artifact requirements. Keep claims narrow: D3D12 real-window automation remains shell/manual-oriented, and Vulkan/Metal presentation remains follow-up.

## Task 5: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` only if public headers or backend interop are changed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record any diagnostic-only host/toolchain blockers explicitly.

## Verification Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` failed before metadata generation with `Desktop runtime game metadata was not generated`.
- RED: after packaging `sample_desktop_runtime_game`, `tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/desktop-runtime-release` previously passed by running stale `sample_desktop_runtime_shell`.
- GREEN: `tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/desktop-runtime-release -GameTarget sample_desktop_runtime_shell` now fails when installed metadata selects `sample_desktop_runtime_game`.
- GREEN: `tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/desktop-runtime-release` now runs metadata-selected `sample_desktop_runtime_game`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed 7/7 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: passed; installed smoke reported `renderer=null`, `frames=2`, and `game_frames=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: passed; default shell package kept metadata-driven shader-artifact smoke and installed shell smoke.
- Post-review hardening: `tools/package-desktop-runtime.ps1` now resolves and verifies `out/install/desktop-runtime-release` under the repo install root, removes it before `cmake --install`, and prevents stale same-target executable/shader files from satisfying installed validation.
- Clean install proof: after `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`, `out/install/desktop-runtime-release/bin` contained only `sample_desktop_runtime_game.exe` among desktop runtime game executables; after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`, it contained only `sample_desktop_runtime_shell.exe`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed with 18/18 default tests passing.
- Diagnostic-only blockers: Vulkan strict shader validation is blocked by missing DXC SPIR-V CodeGen and `spirv-val`; Metal shader/library checks are blocked by missing `metal` and `metallib`; Apple packaging is blocked on this Windows host by missing macOS/Xcode `xcodebuild`/`xcrun`; Android release signing is not configured and no Android device is connected for device smoke; strict tidy reports the active generator compile database blocker while config remains ok.
