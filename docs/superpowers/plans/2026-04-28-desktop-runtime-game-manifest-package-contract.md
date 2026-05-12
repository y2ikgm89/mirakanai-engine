# Desktop Runtime Game Manifest Package Contract Implementation Plan (2026-04-28)

**Goal:** Make every registered `games/<name>` desktop runtime package target trace back to its `game.agent.json`, and make manifest claims for `desktop-game-runtime` / `desktop-runtime-release` fail early when CMake package registration or validation recipes drift.

**Architecture:** Add a required `GAME_MANIFEST` field to `MK_add_desktop_runtime_game`, persist it into `desktop-runtime-games.json`, and validate that metadata against source-tree and installed game manifests. Keep the public game API in `mirakana::` and keep SDL3, OS handles, GPU/RHI backend handles, Dear ImGui, and editor APIs inside host/build/package tooling.

**Tech Stack:** C++23, target-based CMake, PowerShell contract checks, generated desktop runtime package metadata, existing SDL3 optional desktop runtime lane.

## Constraints

- Do not introduce third-party dependencies.
- Do not expose SDL3, native window/GPU handles, D3D12/Vulkan/Metal handles, Dear ImGui, or editor APIs to game public APIs.
- Preserve headless samples and existing default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1` behavior.
- Keep the slice focused on package/manifest contract validation, not new rendering backend capability.

## Tasks

1. **RED: static manifest/package eligibility check**
   - Extend `tools/check-json-contracts.ps1` so `desktop-runtime-release` manifests require `desktop-game-runtime`, a `MK_add_desktop_runtime_game(<target>)` registration, finite `SMOKE_ARGS`, a matching `GAME_MANIFEST`, and an appropriate package validation recipe.
   - Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` and record the expected failure before adding `GAME_MANIFEST` to CMake registrations.

2. **CMake metadata contract**
   - Add required `GAME_MANIFEST` to `MK_add_desktop_runtime_game`.
   - Store it as `MK_DESKTOP_RUNTIME_GAME_MANIFEST`.
   - Emit `gameManifest` in each `desktop-runtime-games.json` registered game entry.
   - Add `GAME_MANIFEST` to `sample_desktop_runtime_shell` and `sample_desktop_runtime_game`.

3. **Runtime/package validation**
   - Add shared PowerShell helpers that validate metadata `gameManifest` paths, load source-tree or installed manifests, and verify `target` plus required package targets.
   - Use those helpers in `tools/validate-desktop-game-runtime.ps1`, `tools/package-desktop-runtime.ps1`, and `tools/validate-installed-desktop-runtime.ps1`.

4. **Docs and agent guidance**
   - Update `docs/testing.md`, `docs/ai-game-development.md`, `docs/dependencies.md`, `docs/release.md`, `docs/roadmap.md`, and `engine/agent/manifest.json`.
   - Sync Codex/Claude game-development guidance and gameplay-builder subagents where the new manifest/package contract affects generated games.

## Validation

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Done When

- `desktop-runtime-games.json` records `gameManifest` for each registered desktop runtime game.
- Static JSON contract checks catch manifest/package mismatches before CMake package lanes.
- Source-tree and installed package validation confirm selected target metadata maps to the correct `game.agent.json`.
- The default shell package and non-shell package proof still pass.

## Results

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` failed after the static manifest/package eligibility check was added because existing `MK_add_desktop_runtime_game` calls did not declare `GAME_MANIFEST`.
- GREEN: `MK_add_desktop_runtime_game` now requires `GAME_MANIFEST`, CMake emits `gameManifest` into `desktop-runtime-games.json`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` validates desktop runtime package manifests against CMake registration, finite `SMOKE_ARGS`, matching manifest paths, and package validation recipes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: passed after vcpkg tool extraction required running outside the sandbox. The generated `desktop-runtime-games.json` recorded both sample manifests and 7/7 focused tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`: passed after sandbox escalation. Installed validation verified the selected installed manifest, ran the non-shell smoke with `renderer=null`, `frames=2`, and `game_frames=2`, then generated the desktop runtime ZIP.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1`: passed after sandbox escalation. The default shell package kept metadata-driven DXIL shader-artifact validation and installed manifest validation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed with existing diagnostic-only host/toolchain blockers for Vulkan SPIR-V validation, Metal shader/library checks, Apple packaging, Android signing/device smoke, and strict tidy compile database availability.
- Post-review hardening: CMake and PowerShell path checks now both require `GAME_MANIFEST` / `gameManifest` to match exactly `games/<name>/game.agent.json`, rejecting nested or parent-directory forms before metadata is trusted.
- Post-review validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-installed-desktop-runtime.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed after the path hardening. The post-review package smoke again reported `renderer=null`, `frames=2`, and `game_frames=2` for `sample_desktop_runtime_game`.
