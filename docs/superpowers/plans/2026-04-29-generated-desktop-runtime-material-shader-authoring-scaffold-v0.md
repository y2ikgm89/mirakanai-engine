# Generated Desktop Runtime Material/Shader Authoring Scaffold v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Add a generated desktop-runtime scaffold that includes authorable first-party material and shader source files, host-built D3D12/Vulkan shader artifacts, cooked scene package payloads, and package validation without exposing native/RHI/editor handles to game code.

**Architecture:** Extend the existing generated desktop-runtime cooked-scene package lane instead of creating a new runtime path. `tools/new-game.ps1` emits source material and HLSL shader files plus the existing cooked package payloads; `games/CMakeLists.txt` owns a reusable CMake helper that compiles target-specific shader artifacts when DXC/SPIR-V tools are available or required. Generated gameplay stays on `mirakana::GameApp`, `mirakana::SdlDesktopGameHost`, `RootedFileSystem`, `MK_runtime`, and `MK_scene_renderer` contracts.

**Tech Stack:** PowerShell scaffold generator, CMake, C++23, HLSL compiled by DXC to DXIL and optional Vulkan SPIR-V, `MK_runtime_host_sdl3_presentation`, `MK_runtime`, `MK_scene_renderer`, existing `desktop-game-runtime` and `desktop-runtime-release` validation lanes.

---

## Constraints

- Do not add third-party dependencies.
- Do not introduce runtime shader compilation.
- Do not expose SDL3, OS window handles, GPU handles, D3D12, Vulkan, Metal, Dear ImGui, editor APIs, or RHI backend handles to generated game code.
- Keep Metal/Apple validation host-gated and diagnostic-only on this Windows host.
- Keep shader artifacts honest: generated targets may declare D3D12/Vulkan artifacts only when CMake installs them from host-built outputs; Vulkan artifacts remain gated by DXC SPIR-V CodeGen and `spirv-val`.
- Keep the existing `DesktopRuntimeCookedScenePackage` template as the renderer-neutral NullRenderer fallback proof.

## Done When

- `tools/new-game.ps1 -Template DesktopRuntimeMaterialShaderPackage` creates a package-ready SDL3 desktop runtime game with:
  - `source/materials/lit.material`
  - `shaders/runtime_scene.hlsl`
  - `shaders/runtime_postprocess.hlsl`
  - `runtime/<game>.config`
  - `runtime/<game>.geindex`
  - first-party cooked texture/mesh/material/scene payloads
  - manifest `runtimePackageFiles` derived package files
  - validation recipes for default package, D3D12 shader package, and Vulkan shader package gates
- A committed sample `games/sample-generated-desktop-runtime-material-shader-package` proves the generated scaffold.
- CMake metadata declares and installs target-specific D3D12 shader artifacts by default for the selected package target and Vulkan artifacts only when requested and toolchain-ready.
- Static `agent-check` validates the new template, generated files, manifest honesty, CMake registration, and shader artifact metadata.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host gates recorded.

### Task 1: Close The Previous Slice And Register This Plan

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/superpowers/plans/2026-04-29-editor-prefab-variant-gui-wiring-v0.md`
- Create: this plan

- [x] **Step 1: Move `2026-04-29-editor-prefab-variant-gui-wiring-v0.md` to the completed table.**

Use this result text in `docs/superpowers/plans/README.md`:

```markdown
Editor Prefab Variant GUI Wiring v0 completed: `MK_editor` now adapts `PrefabVariantAuthoringDocument` in the visible Assets panel for new-from-prefab, `.prefabvariant` load/save, override rows, registry-backed diagnostics, composed-prefab review, variant-scoped undo/redo, name/transform/component override actions, and guarded composed-prefab instantiation into the active `SceneAuthoringDocument`. Project create/open resets variant document/history/path/edit buffers; static `agent-check` covers ordered reset and instantiate-gate contracts; focused agent/schema/format/API-boundary/GUI validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with the known sandbox vcpkg 7zip workaround and existing Metal/Apple/signing/device/tidy host gates. Nested prefab propagation, conflict/merge UX, instance links, editor GUI package apply buttons, and play-in-editor isolation remain follow-up work.
```

- [x] **Step 2: Mark this plan as the active slice in `docs/superpowers/plans/README.md`.**

Use this active row:

```markdown
| Active slice | [2026-04-29-generated-desktop-runtime-material-shader-authoring-scaffold-v0.md](2026-04-29-generated-desktop-runtime-material-shader-authoring-scaffold-v0.md) | Add an opt-in generated SDL3 desktop runtime scaffold with first-party material/shader source, cooked scene package payloads, host-built D3D12/Vulkan shader artifact metadata, and selected-target package validation while preserving NullRenderer fallback honesty. |
```

- [x] **Step 3: Update the gap analysis Immediate Next Slice.**

Set the active slice heading to:

```markdown
**Generated Desktop Runtime Material/Shader Authoring Scaffold v0**
```

State that Editor Prefab Variant GUI Wiring v0 is implemented and that the new active work promotes generated desktop runtime games from fixed cooked fixtures to source material plus shader-artifact scaffolds.

### Task 2: Add RED Static Coverage

**Files:**
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add the new template name to the static new-game checks.**

Update the `ValidateSet` expectation and scaffold dry-run checks so `agent-check` requires:

```powershell
"DesktopRuntimeMaterialShaderPackage"
"sample-generated-desktop-runtime-material-shader-package"
"source/materials/lit.material"
"shaders/runtime_scene.hlsl"
"shaders/runtime_postprocess.hlsl"
"MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS"
"MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS"
"--require-d3d12-scene-shaders"
"--require-vulkan-scene-shaders"
```

- [x] **Step 2: Add a disposable scaffold check.**

Create a disposable game with:

```powershell
& (Join-Path $PSScriptRoot "new-game.ps1") -Name "desktop-material-shader-game" -RepositoryRoot $materialShaderScaffoldRoot -Template DesktopRuntimeMaterialShaderPackage | Out-Null
```

Then assert the generated game contains `main.cpp`, `README.md`, `game.agent.json`, `source/materials/lit.material`, `shaders/runtime_scene.hlsl`, `shaders/runtime_postprocess.hlsl`, `runtime/.gitattributes`, the config, index, cooked texture/mesh/material/scene payloads, and CMake registration with `PACKAGE_FILES_FROM_MANIFEST`.

- [x] **Step 3: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm it fails before implementation.**

Expected failure:

```text
ai-integration-check: ... missing DesktopRuntimeMaterialShaderPackage
```

### Task 3: Extend `new-game` For Material/Shader Authoring

**Files:**
- Modify: `tools/new-game.ps1`

- [x] **Step 1: Extend the template validation set.**

Change the parameter to include:

```powershell
[ValidateSet("Headless", "DesktopRuntimePackage", "DesktopRuntimeCookedScenePackage", "DesktopRuntimeMaterialShaderPackage")]
```

- [x] **Step 2: Add source material and shader file generation.**

Add a `New-DesktopRuntimeMaterialShaderPackageFiles` helper that reuses `New-DesktopRuntimeCookedScenePackageFiles`, adds `source/materials/lit.material`, `shaders/runtime_scene.hlsl`, and `shaders/runtime_postprocess.hlsl`, and returns the same `SceneAssetName` for `New-DesktopRuntimeCookedSceneMainCpp`.

The generated `source/materials/lit.material` must use `format=GameEngine.Material.v1` and the same material id, base-color texture id, factors, and texture slot as `runtime/assets/generated/lit.material`.

- [x] **Step 3: Add deterministic HLSL source templates.**

Generate `shaders/runtime_scene.hlsl` with entry points:

```hlsl
VSOut vs_main(VSIn input)
float4 ps_main(VSOut input) : SV_Target0
```

Generate `shaders/runtime_postprocess.hlsl` with entry points:

```hlsl
VSOut vs_postprocess(uint vertex_id : SV_VertexID)
float4 ps_postprocess(VSOut input) : SV_Target0
```

Keep the shader source minimal and compatible with the existing `sample_desktop_runtime_game` shader artifact expectations.

- [x] **Step 4: Add manifest generation.**

Add `New-DesktopRuntimeMaterialShaderManifest` with:

```powershell
sourceFormats = @("first-party-material-source", "hlsl-source", "first-party-cooked-fixture")
currentRuntime = "desktop-windowed-sdl3-host-with-host-built-d3d12-vulkan-shader-artifacts-and-null-fallback"
graphics = "host-built D3D12 DXIL and toolchain-gated Vulkan SPIR-V shader artifacts; NullRenderer fallback remains available"
```

The manifest `runtimePackageFiles` must include only runtime package files, not source material or HLSL authoring files.

- [x] **Step 5: Add registration generation.**

Add `New-DesktopRuntimeMaterialShaderRegistration` that calls:

```cmake
MK_add_desktop_runtime_game(<target>
    SOURCES
        <game>/main.cpp
    GAME_MANIFEST
        games/<game>/game.agent.json
    SMOKE_ARGS
        --smoke
        --require-config
        runtime/<game>.config
        --require-scene-package
        runtime/<game>.geindex
    PACKAGE_SMOKE_ARGS
        --smoke
        --require-config
        runtime/<game>.config
        --require-scene-package
        runtime/<game>.geindex
        --require-d3d12-scene-shaders
        --video-driver
        windows
        --require-d3d12-renderer
        --require-scene-gpu-bindings
        --require-postprocess
    REQUIRES_D3D12_SHADERS
    PACKAGE_FILES_FROM_MANIFEST
)
```

Then link `MK_runtime`, `MK_runtime_rhi`, `MK_scene`, and `MK_scene_renderer`, set D3D12/Vulkan shader artifact properties, and call the reusable CMake helper from Task 4.

### Task 4: Add Reusable CMake Shader Artifact Registration

**Files:**
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Extract a reusable helper inside `if(MK_DESKTOP_RUNTIME_ENABLED)`.**

Add a function named `MK_configure_desktop_runtime_scene_shader_artifacts` with arguments:

```cmake
TARGET
GAME_NAME
SCENE_SHADER_SOURCE
POSTPROCESS_SHADER_SOURCE
```

The helper must set `MK_DESKTOP_RUNTIME_D3D12_SHADER_ARTIFACTS` and `MK_DESKTOP_RUNTIME_VULKAN_SHADER_ARTIFACTS`, create DXIL and SPIR-V custom targets, install selected shader artifacts only when `MK_DESKTOP_RUNTIME_PACKAGE_GAME_TARGET` matches the target, and preserve the existing fatal errors when required toolchain pieces are missing.

- [x] **Step 2: Keep existing `sample_desktop_runtime_game` behavior unchanged.**

Either migrate `sample_desktop_runtime_game` to the helper with identical artifact names:

```text
shaders/sample_desktop_runtime_game_scene.vs.dxil
shaders/sample_desktop_runtime_game_scene.ps.dxil
shaders/sample_desktop_runtime_game_postprocess.vs.dxil
shaders/sample_desktop_runtime_game_postprocess.ps.dxil
```

or leave the existing target-specific block in place and use the helper only for generated material/shader scaffolds.

- [x] **Step 3: Ensure generated target registration can call the helper.**

Generated registration must use:

```cmake
MK_configure_desktop_runtime_scene_shader_artifacts(
    TARGET <target>
    GAME_NAME <game>
    SCENE_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/<game>/shaders/runtime_scene.hlsl"
    POSTPROCESS_SHADER_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/<game>/shaders/runtime_postprocess.hlsl"
)
```

### Task 5: Add The Committed Proof Sample

**Files:**
- Create: `games/sample-generated-desktop-runtime-material-shader-package/`
- Modify: `games/CMakeLists.txt`

- [x] **Step 1: Generate the sample from the new template.**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name sample-generated-desktop-runtime-material-shader-package -Template DesktopRuntimeMaterialShaderPackage
```

Expected target:

```text
sample_generated_desktop_runtime_material_shader_package
```

- [x] **Step 2: Verify generated files.**

The sample directory must contain:

```text
source/materials/lit.material
shaders/runtime_scene.hlsl
shaders/runtime_postprocess.hlsl
runtime/sample-generated-desktop-runtime-material-shader-package.config
runtime/sample-generated-desktop-runtime-material-shader-package.geindex
runtime/assets/generated/base-color.texture.geasset
runtime/assets/generated/triangle.mesh
runtime/assets/generated/lit.material
runtime/assets/generated/packaged-scene.scene
```

- [x] **Step 3: Keep the sample manifest honest.**

`game.agent.json` must state that D3D12 DXIL is host-built, Vulkan SPIR-V is toolchain-gated, Metal is not implemented, runtime uses cooked package files only, and source material/HLSL files are authoring inputs rather than runtime package payloads.

### Task 6: Sync Docs, Manifest, Skills, And Agents

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/workflows.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/specs/game-prompt-pack.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`
- Modify as needed: `.codex/agents/engine-architect.toml`
- Modify as needed: `.claude/agents/engine-architect.md`

- [x] **Step 1: Document the new template and sample.**

Use the exact template name `DesktopRuntimeMaterialShaderPackage` and target name `sample_generated_desktop_runtime_material_shader_package`.

- [x] **Step 2: Keep feature status honest.**

Document that this is not a shader graph, material graph, live shader generator, runtime shader compiler, Metal path, nested prefab system, or public RHI/game API exposure.

- [x] **Step 3: Keep Codex and Claude guidance equivalent.**

Both guidance sets must tell generated-game agents to select the new template only when they need source material/HLSL authoring plus host-built desktop shader artifacts.

### Task 7: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run static and schema checks.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

- [x] **Step 2: Run shader and desktop runtime checks.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package
```

- [x] **Step 3: Run Vulkan strict package validation when the host reports Vulkan SPIR-V ready.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample-generated-desktop-runtime-material-shader-package.config', '--require-scene-package', 'runtime/sample-generated-desktop-runtime-material-shader-package.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess')
```

- [x] **Step 4: Run `cpp-reviewer` and `rendering-auditor` before final validation.**

Use `build-fixer` only if build or package validation fails.

- [x] **Step 5: Run final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS. Static coverage now requires `DesktopRuntimeMaterialShaderPackage`, generated authoring/runtime files, manifest honesty, CMake shader artifact metadata, target-isolated source-tree staging, and package validation recipes. RED was observed before implementation when the new template/static requirements were absent.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after running `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` on the generated C++ sample.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS. No SDL3, OS, GPU, D3D12, Vulkan, Metal, Dear ImGui, editor, or native handle APIs were exposed to game public headers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1`: diagnostic-only PASS with D3D12 DXIL ready, Vulkan SPIR-V ready, DXC SPIR-V CodeGen ready, `dxc` and `spirv-val` found under `C:\VulkanSDK\1.4.341.1\Bin`, and known host gates for missing `metal` and `metallib`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1`: PASS with 12/12 CTest desktop runtime smokes after the known sandbox vcpkg `CreateFileW stdin failed with 5` workaround required an escalated rerun. The focused lane now also explicitly requires `sample_generated_desktop_runtime_material_shader_package` in registered desktop runtime metadata.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package`: PASS after the same known sandbox vcpkg 7zip workaround required an escalated rerun. Installed smoke reported `renderer=d3d12`, `scene_gpu_status=ready`, `postprocess_status=ready`, and `framegraph_passes=2`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_material_shader_package -RequireVulkanShaders -SmokeArgs @('--smoke', '--require-config', 'runtime/sample-generated-desktop-runtime-material-shader-package.config', '--require-scene-package', 'runtime/sample-generated-desktop-runtime-material-shader-package.geindex', '--require-vulkan-scene-shaders', '--video-driver', 'windows', '--require-vulkan-renderer', '--require-scene-gpu-bindings', '--require-postprocess')`: PASS after the same sandbox workaround required an escalated rerun. Installed smoke reported `renderer=vulkan`, `scene_gpu_status=ready`, `postprocess_status=ready`, and `framegraph_passes=2`; SPIR-V artifacts were generated with `-spirv -fspv-target-env=vulkan1.3` and validated with `spirv-val`.
- `build-fixer` read-only investigation confirmed the correct shader bytecode API is `DesktopShaderBytecodeLoadDesc` with `RootedFileSystem`, and the stale generated `DesktopShaderBytecodePairDesc` call shape was fixed in both the template and committed sample.
- `cpp-reviewer` and `rendering-auditor` found no remaining C++/CMake/runtime staging/package collision, renderer/RHI boundary, native handle exposure, or Metal honesty blockers. Their remaining findings were docs/plan completion drift and the missing explicit required desktop-runtime target; both are fixed in this plan and `tools/validate-desktop-game-runtime.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS after the final docs/target-list sync. Validation completed with the known diagnostic-only gates for missing Metal `metal` / `metallib`, Apple packaging/iOS/Metal macOS/Xcode requirements, Android release signing, disconnected Android device smoke, and strict clang-tidy compile-database availability on the active generator.
