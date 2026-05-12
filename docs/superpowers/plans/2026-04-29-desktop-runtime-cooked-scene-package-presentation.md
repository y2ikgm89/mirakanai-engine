# Desktop Runtime Cooked Scene Package Presentation Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make the non-shell SDL3 desktop runtime game package prove a cooked scene package load and renderer submission path, not only executable/config bundling.

**Architecture:** Keep gameplay on public `mirakana::GameApp`, `mirakana::RootedFileSystem`, `mirakana::runtime::RuntimeAssetPackage`, `mirakana::instantiate_runtime_scene_render_data`, `mirakana::submit_scene_render_packet`, and `mirakana::IRenderer`. Add first-party cooked texture, mesh, material, and scene payload files to the sample package and make the installed smoke require that the executable loads the package and submits the scene through the host-owned renderer. This slice does not add new RHI handles to gameplay code, does not require D3D12 DXIL artifacts, and does not add new dependencies.

**Tech Stack:** C++23, `mirakana_runtime`, `mirakana_scene`, `mirakana_scene_renderer`, `mirakana_runtime_host_sdl3_presentation`, desktop-runtime package metadata, `RootedFileSystem`, `NullRenderer` fallback-compatible scene submission.

---

## Context

- `sample_desktop_runtime_game` already proves `SdlDesktopGameHost`, finite `--smoke`, target-driven desktop runtime packaging, and installed config-file validation.
- `mirakana_runtime` can load cooked package indexes and typed runtime payloads through `IFileSystem`.
- `mirakana_scene_renderer` can instantiate a cooked scene package into a `Scene`, `SceneMaterialPalette`, and `SceneRenderPacket`, then submit it through an `IRenderer`.
- The production gap is connecting installed desktop runtime package files to the same cooked content path a real game executable would use.

## Constraints

- Keep public API names under `mirakana::`.
- Do not expose SDL3, OS, GPU, D3D12, Vulkan, Metal, RHI backend, Dear ImGui, or editor-private handles to gameplay code.
- Do not add third-party dependencies or external asset licenses.
- Preserve deterministic `NullRenderer` fallback and the existing source-tree `--smoke` behavior.
- Keep `runtimePackageFiles` in `game.agent.json` mirrored by `games/CMakeLists.txt` `PACKAGE_FILES`.

## Done When

- [x] `sample_desktop_runtime_game` declares packaged cooked texture, mesh, material, scene, and package-index files.
- [x] Installed package smoke passes `--require-scene-package runtime/sample_desktop_runtime_game.geindex`.
- [x] The sample loads the package through `RootedFileSystem`, instantiates the cooked scene render data, submits at least one scene mesh through `IRenderer`, and reports deterministic scene submit counters.
- [x] Docs, roadmap, gap analysis, engine manifest, game manifest, Codex/Claude game-development skills, and gameplay-builder subagents describe the capability honestly.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`, selected `sample_desktop_runtime_game` package validation, relevant agent/schema checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact blockers are recorded.

---

### Task 1: Package Contract RED

**Files:**
- Modify: `games/CMakeLists.txt`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Create: `games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex`
- Create: `games/sample_desktop_runtime_game/runtime/assets/desktop-runtime/base-color.texture.geasset`
- Create: `games/sample_desktop_runtime_game/runtime/assets/desktop-runtime/triangle.mesh`
- Create: `games/sample_desktop_runtime_game/runtime/assets/desktop-runtime/unlit.material`
- Create: `games/sample_desktop_runtime_game/runtime/assets/desktop-runtime/packaged-scene.scene`

- [x] **Step 1: Add package-file declarations and installed smoke requirement**

Add the cooked package index and payload files to both `runtimePackageFiles` and `PACKAGE_FILES`. Extend the selected package smoke args for `sample_desktop_runtime_game` to:

```text
--smoke --require-config runtime/sample_desktop_runtime_game.config --require-scene-package runtime/sample_desktop_runtime_game.geindex
```

- [x] **Step 2: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected before implementation: FAIL because `sample_desktop_runtime_game` does not accept `--require-scene-package`.

### Task 2: Sample Runtime Scene Loading

**Files:**
- Modify: `games/sample_desktop_runtime_game/main.cpp`

- [x] **Step 1: Add package loading helpers**

Add `--require-scene-package <path>` parsing. Load the package relative to the executable directory through `mirakana::RootedFileSystem`, then call:

```cpp
auto package = mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{path, {}});
auto instance = mirakana::instantiate_runtime_scene_render_data(package.package, packaged_scene_asset_id());
```

Return exit code `4` for missing/invalid package data, matching the existing required-config failure style.

- [x] **Step 2: Submit the cooked scene in the game frame**

Store the `RuntimeSceneRenderInstance` in the `GameApp`. During each frame, after the existing sprite draw, submit:

```cpp
auto submitted = mirakana::submit_scene_render_packet(renderer_, scene.render_packet,
    mirakana::SceneRenderSubmitDesc{mirakana::Color{0.8F, 0.35F, 0.15F, 1.0F}, &scene.material_palette});
```

Track submitted mesh count and material color resolution so smoke mode can fail if the package did not drive renderer work.

- [x] **Step 3: Verify GREEN**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game
```

Expected after implementation: PASS with output that includes `scene_meshes=2` for two smoke frames and `scene_materials=2`.

### Task 3: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `games/sample_desktop_runtime_game/README.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Synchronize capability claims**

Describe `sample_desktop_runtime_game` as a non-shell desktop runtime package proof that bundles config plus cooked scene package files and validates package load/scene submission through `SdlDesktopGameHost` with `NullRenderer` fallback.

- [x] **Step 2: Keep future work honest**

Keep D3D12 uploaded mesh/material GPU binding, Vulkan/Metal game-window presentation, editor authoring, and broader cooked asset conventions listed as follow-up unless this slice validates them.

### Task 4: Verification

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record diagnostic-only blockers explicitly.

## Validation Evidence

Completed on Windows host on 2026-04-29.

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` first failed after package metadata changes with `unknown argument: --require-scene-package`, proving the installed smoke requirement was not implemented yet. The first sandboxed run also hit the known vcpkg `7z2600-x64.7z` `CreateFileW stdin failed with 5 (Access is denied.)` blocker; the same command succeeded when rerun with elevated execution.
- GREEN focused package validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game` passed and the installed executable reported `scene_meshes=2` and `scene_materials=2`.
- Focused source-tree validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1` passed after the same known sandbox vcpkg blocker was rerun with elevated execution.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Review follow-up: `cpp-reviewer` identified the sample frame counter type mismatch and incomplete scene-to-mesh dependency edge metadata; both were fixed before final validation.
- Final validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Diagnostic-only blockers remain: Metal `metal` / `metallib` missing, Apple packaging requires macOS/Xcode (`xcodebuild` / `xcrun` missing), Android release signing not configured, Android device smoke not connected, and strict clang-tidy remains diagnostic-only when the Visual Studio generator lacks the expected compile database at tidy-check time.
