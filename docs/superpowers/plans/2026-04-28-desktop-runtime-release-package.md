# Desktop Runtime Release Package Implementation Plan (2026-04-28)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the editor-independent desktop runtime shell and its generated D3D12 DXIL artifacts installable and packageable through an explicit release lane.

**Architecture:** Keep the default `release` package lean and source-tree-friendly. Add a focused desktop-runtime package lane that enables SDL3 runtime host targets without Dear ImGui or `MK_editor`, builds `sample_desktop_runtime_shell` in Release, installs the executable, SDL3 runtime DLL, and generated `bin/shaders/runtime_shell.*.dxil`, then validates the installed smoke path with `--require-d3d12-shaders`. Do not expose SDL, OS, GPU, D3D12, or editor handles to gameplay or public game APIs.

**Tech Stack:** C++23, target-based CMake, Visual Studio multi-config builds, CPack ZIP, PowerShell validation scripts, optional SDL3/vcpkg `desktop-runtime` feature, Windows SDK `dxc` build-time shader compilation.

---

## File Structure

- Modify root/engine/game CMake to separate `MK_ENABLE_DESKTOP_RUNTIME` from the broader `MK_ENABLE_DESKTOP_GUI` editor lane.
- Modify `games/CMakeLists.txt` to install generated DXIL artifacts and SDL3 runtime artifacts for `sample_desktop_runtime_shell`.
- Modify `CMakePresets.json` to add a `desktop-runtime-release` configure/build/test/package lane.
- Modify `vcpkg.json` and dependency policy checks to add a SDL3-only `desktop-runtime` feature.
- Add `tools/validate-installed-desktop-runtime.ps1`.
- Add `tools/package-desktop-runtime.ps1`.
- Modify `package.json` to expose the new package/validation command.
- Modify `docs/release.md`, `docs/testing.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `.agents/skills/gameengine-game-development/SKILL.md`, `.claude/skills/gameengine-game-development/SKILL.md`, and `games/sample_desktop_runtime_shell/README.md`.
- Modify this plan and `docs/superpowers/plans/README.md`.

## Task 1: Installed Desktop Runtime Validation

**Files:**
- Add: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `package.json`

- [x] **Step 1: Add installed-runtime validation script**

Create `tools/validate-installed-desktop-runtime.ps1` with:

- parameters: `InstallPrefix`, `BuildConfig`,
- checks for `bin/sample_desktop_runtime_shell.exe` on Windows or `bin/sample_desktop_runtime_shell` elsewhere,
- checks for `bin/shaders/runtime_shell.vs.dxil` and `bin/shaders/runtime_shell.ps.dxil`,
- on Windows, checks for `bin/SDL3.dll`,
- runs the installed sample with `--smoke --require-d3d12-shaders`,
- prints `installed-desktop-runtime-validation: ok`.

- [x] **Step 2: Verify the validation fails before install rules**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/release
```

Expected before CMake install rules: fail with a missing installed `sample_desktop_runtime_shell` or missing shader artifact.

- [x] **Step 3: Add package scripts to `package.json`**

Add:

- `validate-installed-desktop-runtime`: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1`
- `package-desktop-runtime`: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`

## Task 2: CMake Install And Package Lane

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `CMakePresets.json`
- Add: `tools/package-desktop-runtime.ps1`

- [x] **Step 1: Install desktop runtime shader artifacts**

When `sample_desktop_runtime_shell` and `sample_desktop_runtime_shell_shaders` exist, install:

- `$<TARGET_FILE_DIR:sample_desktop_runtime_shell>/shaders/runtime_shell.vs.dxil`
- `$<TARGET_FILE_DIR:sample_desktop_runtime_shell>/shaders/runtime_shell.ps.dxil`

to `${CMAKE_INSTALL_BINDIR}/shaders`. Keep hosts without `dxc` configurable for normal desktop runtime builds, but make the focused package lane fail configure through `MK_REQUIRE_DESKTOP_RUNTIME_DXIL=ON`.

- [x] **Step 2: Install SDL3 runtime artifact for the desktop runtime package**

When `WIN32` and `SDL3::SDL3` exist, install `$<TARGET_FILE:SDL3::SDL3>` to `${CMAKE_INSTALL_BINDIR}`. Keep this target-scoped to the optional desktop runtime path; do not add new dependencies.

- [x] **Step 3: Add desktop runtime release presets**

Add:

- configure preset `desktop-runtime` with `MK_ENABLE_DESKTOP_RUNTIME=ON`, vcpkg `desktop-runtime` feature, `BUILD_TESTING=ON`, and binary dir `out/build/desktop-runtime`,
- configure preset `desktop-runtime-release` with `MK_ENABLE_DESKTOP_RUNTIME=ON`, `MK_REQUIRE_DESKTOP_RUNTIME_DXIL=ON`, vcpkg `desktop-runtime` feature, `BUILD_TESTING=ON`, and binary dir `out/build/desktop-runtime-release`,
- build preset `desktop-runtime-release` with configuration `Release`,
- test preset `desktop-runtime-release` with configuration `Release`,
- package preset `desktop-runtime-release` using ZIP and configuration `Release`.

- [x] **Step 4: Add package script**

Create `tools/package-desktop-runtime.ps1` that:

1. configures `desktop-runtime-release`,
2. builds the full `desktop-runtime-release` preset so all install targets exist,
3. runs CTest for `MK_runtime_host_tests|MK_runtime_host_sdl3_tests|MK_sdl3_platform_tests|MK_sdl3_audio_tests|sample_desktop_runtime_shell(_shader_artifacts)?_smoke`,
4. installs to `out/install/desktop-runtime-release`,
5. runs `validate-installed-sdk.ps1` against that prefix with a separate consumer build dir,
6. runs `validate-installed-desktop-runtime.ps1`,
7. runs `cpack --preset desktop-runtime-release`,
8. prints `desktop-runtime-package: ok`.

## Task 3: Documentation And Manifest Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/release.md`
- Modify: `docs/testing.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `games/sample_desktop_runtime_shell/README.md`

- [x] **Step 1: Register active slice**

Set this plan as Active during implementation and move it to Completed after validation.

- [x] **Step 2: Update readiness claims**

Docs and manifest must claim only:

- default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1` remains the current installed SDK package lane,
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` is the optional desktop runtime release lane,
- it packages `sample_desktop_runtime_shell`, SDL3 runtime DLL on Windows, and generated D3D12 DXIL artifacts with `dxc` required by the focused package lane,
- `desktop-runtime` uses SDL3 only and does not require Dear ImGui or `MK_editor`,
- Vulkan/Metal desktop runtime packages remain follow-up work.

## Task 4: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/release` before install rules and record the expected failure.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if any public CMake package or public header boundary changes.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Verification Results

- Expected red check: `tools/validate-installed-desktop-runtime.ps1 -InstallPrefix out/install/release` failed before the install rules because the default SDK install did not contain `sample_desktop_runtime_shell`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed after sandbox escalation for vcpkg tool extraction; CTest covered `MK_runtime_host_tests`, `MK_runtime_host_sdl3_tests`, `MK_sdl3_platform_tests`, `MK_sdl3_audio_tests`, `sample_desktop_runtime_shell_smoke`, and `sample_desktop_runtime_shell_shader_artifacts_smoke`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: passed after sandbox escalation for vcpkg tool extraction; built `desktop-runtime-release`, installed `sample_desktop_runtime_shell.exe`, `SDL3.dll`, and `bin/shaders/runtime_shell.{vs,ps}.dxil`, validated the installed SDK consumer, ran installed sample smoke with `--require-d3d12-shaders`, and generated `out/build/desktop-runtime-release/GameEngine-0.1.0-Windows-AMD64.zip`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Diagnostic-only blockers remain Vulkan SPIR-V toolchain, Metal toolchain, Apple/Xcode packaging, strict tidy compile database, and optional signing/device readiness.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package.ps1`: passed for the existing default SDK package lane.
