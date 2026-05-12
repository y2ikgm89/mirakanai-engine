# Generated 3D Native UI Overlay Package Smoke Implementation Plan (2026-05-09)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a selected generated `DesktopRuntime3DPackage` package smoke that proves a generated 3D game can submit a first-party `MK_ui` HUD through the host-owned native UI overlay path on the D3D12 desktop package lane.

**Architecture:** Reuse the existing `MK_ui` / `MK_ui_renderer` document, layout, renderer-submission, and `SdlDesktopPresentation` native UI overlay contracts. The generated 3D sample and `tools/new-game.ps1 -Template DesktopRuntime3DPackage` will add a `--require-native-ui-overlay` flag, compile/load `runtime_ui_overlay.hlsl` artifacts, pass overlay shaders into the host-owned scene renderer descriptor, emit package-visible `ui_overlay_*` and `hud_*` counters, and fail closed when required counters are not positive or not frame-exact. This is a narrow D3D12-selected proof; it must not claim production text shaping, font rasterization, texture UI atlases, runtime source image decoding, public native/RHI handles, Metal readiness, or broad generated 3D production readiness.

**Tech Stack:** C++23, CMake desktop-runtime target metadata, PowerShell validation/static checks, `MK_ui`, `MK_ui_renderer`, `MK_runtime_host_sdl3_presentation`, D3D12 DXIL shader artifacts, optional strict Vulkan artifact metadata without ready-claim expansion.

---

**Plan ID:** `generated-3d-native-ui-overlay-package-smoke-v1`
**Status:** Completed.
**Master Plan:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)

## Context

- `engine/agent/manifest.json.aiOperableProductionLoop.recommendedNextPlan.id` returned to `next-production-gap-selection` after Generated 3D Shadow Morph Composition Package Smoke v1.
- `unsupportedProductionGaps[3d-playable-vertical-slice]` is still not ready, even though `--require-playable-3d-slice`, gameplay systems, postprocess depth input, directional shadow, and selected D3D12 shadow+morph receiver package smokes are complete.
- The committed generated 3D sample already links `MK_ui` and `MK_ui_renderer`, but its manifest currently says the generated 3D package has only a headless UI contract and does not prove host-owned native UI overlay output.
- `sample_desktop_runtime_game` and `sample_2d_desktop_runtime_package` already prove native UI overlay reporting through `SdlDesktopPresentationReport::native_ui_overlay_*` fields; this slice should reuse that public host-owned path instead of adding new renderer/RHI APIs.

## Constraints

- Keep gameplay code on public `mirakana::ui`, `mirakana::IRenderer`, and package/runtime APIs only.
- Keep SDL3, D3D12, Vulkan, native window handles, and RHI handles inside runtime-host/presentation adapters.
- Do not add third-party dependencies.
- Keep the new smoke separate from shadow+morph composition; `--require-shadow-morph-composition` remains its own selected D3D12 run.
- Keep texture-backed UI atlas proof out of this slice. This slice proves colored HUD box overlay only.
- Add tests/checks before implementation where feasible. Run focused build/package smoke before broad validation.

## Done When

- `games/sample_generated_desktop_runtime_3d_package` accepts `--require-native-ui-overlay`.
- The selected installed D3D12 package smoke reports:
  - `hud_boxes=<expected_frames>`
  - `ui_overlay_requested=1`
  - `ui_overlay_status=ready`
  - `ui_overlay_ready=1`
  - `ui_overlay_sprites_submitted=<expected_frames>`
  - `ui_overlay_draws=<expected_frames>`
- `tools/validate-installed-desktop-runtime.ps1` enforces those fields whenever smoke args include `--require-native-ui-overlay`.
- `tools/new-game.ps1 -Template DesktopRuntime3DPackage` emits the same flag, shader artifacts, docs, manifest recipe, and static markers for generated games.
- `engine/agent/manifest.json`, docs, skills, static checks, and plan registry describe this as a narrow D3D12-selected generated 3D UI overlay smoke.
- Focused sample build, installed package smoke, static checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass or record concrete host/tool blockers.

## File Structure

- Modify `games/sample_generated_desktop_runtime_3d_package/main.cpp`: option parsing, HUD document, UI renderer submission counters, shader bytecode loading, presentation descriptor wiring, status output, fail-closed validation.
- Add `games/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl`: generated 3D copy of the simple native UI overlay shader with `vs_native_ui_overlay` and `ps_native_ui_overlay`.
- Modify `games/CMakeLists.txt`: extend `MK_configure_desktop_runtime_scene_shader_artifacts` with optional `UI_OVERLAY_SHADER_SOURCE` support and pass it for the generated 3D sample.
- Modify `games/sample_generated_desktop_runtime_3d_package/game.agent.json` and `README.md`: add shader artifacts, recipe, command, and unsupported boundaries.
- Modify `tools/new-game.ps1`: propagate the generated 3D template changes.
- Modify `tools/validate-installed-desktop-runtime.ps1`: parse `--require-native-ui-overlay` and validate package smoke fields.
- Modify `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1`: lock the scaffold/sample/manifest/docs markers.
- Modify `engine/agent/manifest.json`, `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/specs/generated-game-validation-scenarios.md`, `.agents/skills/gameengine-game-development/SKILL.md`, `.claude/skills/gameengine-game-development/SKILL.md`, and `docs/superpowers/plans/README.md`: synchronize the ready boundary.

### Task 1: RED Validation And Static Markers

**Files:**
- Test: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Test: `tools/validate-installed-desktop-runtime.ps1`
- Test: `tools/check-json-contracts.ps1`
- Test: `tools/check-ai-integration.ps1`

- [ ] **Step 1: Run current generated 3D package smoke with the new flag and record failure**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-compute-morph-skin', '--require-compute-morph-async-telemetry', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-d3d12-scene-shaders', '--video-driver', 'windows', '--require-d3d12-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice', '--require-native-ui-overlay')
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected now: FAIL because `--require-native-ui-overlay` is unknown or because required `ui_overlay_*` fields are absent.

- [ ] **Step 2: Add static-check expectations before implementation**

Add checks that will initially fail until later tasks add markers:

```powershell
Require-Text -Path 'games/sample_generated_desktop_runtime_3d_package/main.cpp' -Needle '--require-native-ui-overlay'
Require-Text -Path 'games/sample_generated_desktop_runtime_3d_package/game.agent.json' -Needle 'installed-d3d12-3d-native-ui-overlay-smoke'
Require-Text -Path 'tools/new-game.ps1' -Needle 'runtime_ui_overlay.hlsl'
```

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected now: FAIL on missing generated 3D native UI overlay markers.

### Task 2: Generated 3D HUD And Runtime Flag

**Files:**
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`

- [ ] **Step 1: Add `--require-native-ui-overlay` option**

Add `bool require_native_ui_overlay{false};` to `DesktopRuntimeOptions`, parse the flag, and update usage. Selecting the flag must imply D3D12/Vulkan renderer, scene GPU bindings, postprocess, renderer quality gates, and playable 3D smoke:

```cpp
if (arg == "--require-native-ui-overlay") {
    options.require_native_ui_overlay = true;
    options.require_d3d12_scene_shaders = true;
    options.require_d3d12_renderer = !options.require_vulkan_renderer;
    options.require_scene_gpu_bindings = true;
    options.require_postprocess = true;
    options.require_renderer_quality_gates = true;
    options.require_playable_3d_slice = true;
    continue;
}
```

- [ ] **Step 2: Add a generated 3D HUD**

Add a small `mirakana::ui::UiDocument` to the game class:

```cpp
mirakana::ui::UiDocument hud_;
mirakana::UiRendererTheme theme_;
std::size_t hud_boxes_submitted_{0};
bool ui_ok_{false};
bool ui_text_updates_ok_{true};
```

Build one `hud.status` label/box in `on_start`, update its text with frame/mesh evidence, then submit it each frame through:

```cpp
const auto layout =
    mirakana::ui::solve_layout(hud_, mirakana::ui::ElementId{"hud.root"}, mirakana::ui::Rect{0.0F, 0.0F, 320.0F, 180.0F});
const auto submission = mirakana::ui::build_renderer_submission(hud_, layout);
const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer_, submission, mirakana::UiRenderSubmitDesc{.theme = &theme_});
hud_boxes_submitted_ += hud_submit.boxes_submitted;
```

- [ ] **Step 3: Emit and validate HUD counters**

Print `hud_boxes=<value>` on the existing status line. When `--require-native-ui-overlay` is set, fail unless `hud_boxes == options.max_frames`.

### Task 3: Shader Artifacts And Host-Owned Overlay Wiring

**Files:**
- Add: `games/sample_generated_desktop_runtime_3d_package/shaders/runtime_ui_overlay.hlsl`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/CMakeLists.txt`

- [ ] **Step 1: Add generated 3D native UI overlay shader**

Create `runtime_ui_overlay.hlsl` with entry points:

```hlsl
NativeUiOverlayVertexOut vs_native_ui_overlay(NativeUiOverlayVertexIn input) { ... }
float4 ps_native_ui_overlay(NativeUiOverlayVertexOut input) : SV_Target { ... }
```

Use the same simple colored-sprite contract as `games/sample_desktop_runtime_game/shaders/runtime_ui_overlay.hlsl`; do not add texture atlas support in this slice.

- [ ] **Step 2: Extend CMake shader helper**

Add optional `UI_OVERLAY_SHADER_SOURCE` support to `MK_configure_desktop_runtime_scene_shader_artifacts`. When provided, append:

```cmake
shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.dxil
shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.dxil
shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.vs.spv
shaders/${MK_SCENE_SHADER_TARGET}_ui_overlay.ps.spv
```

Compile DXIL with `-E vs_native_ui_overlay -T vs_6_0` and `-E ps_native_ui_overlay -T ps_6_0`; compile SPIR-V with the existing `-spirv -fspv-target-env=vulkan1.3` pattern.

- [ ] **Step 3: Wire shader bytecode into presentation descriptors**

Load `sample_generated_desktop_runtime_3d_package_ui_overlay.vs/ps` artifacts and pass them to `SdlDesktopPresentationD3d12SceneRendererDesc` / `SdlDesktopPresentationVulkanSceneRendererDesc`:

```cpp
.native_ui_overlay_vertex_shader = to_presentation_shader_bytecode(native_ui_overlay_bytecode.vertex_shader),
.native_ui_overlay_fragment_shader = to_presentation_shader_bytecode(native_ui_overlay_bytecode.fragment_shader),
.enable_native_ui_overlay = options.require_native_ui_overlay,
```

Do not set `enable_native_ui_overlay_textures`.

- [ ] **Step 4: Emit and validate overlay counters**

Print:

```text
ui_overlay_requested ui_overlay_status ui_overlay_ready ui_overlay_sprites_submitted ui_overlay_draws
```

Fail closed when `--require-native-ui-overlay` is set and:

```cpp
!report.native_ui_overlay_requested ||
!report.native_ui_overlay_ready ||
report.native_ui_overlay_sprites_submitted != options.max_frames ||
report.native_ui_overlay_draws != options.max_frames
```

### Task 4: Generated Scaffold Parity

**Files:**
- Modify: `tools/new-game.ps1`

- [ ] **Step 1: Add template constants, shader source, flag, and wiring**

Mirror the committed sample changes in the `DesktopRuntime3DPackage` template:

```cpp
constexpr std::string_view kRuntimeNativeUiOverlayVertexShaderPath{"shaders/__TARGET_NAME___ui_overlay.vs.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayFragmentShaderPath{"shaders/__TARGET_NAME___ui_overlay.ps.dxil"};
```

The generated code must parse `--require-native-ui-overlay`, load DXIL/SPIR-V overlay artifacts, enable the host-owned native UI overlay, emit the same `hud_*` and `ui_overlay_*` counters, and fail closed with the same field checks.

- [ ] **Step 2: Update generated manifest and README templates**

Add `installed-d3d12-3d-native-ui-overlay-smoke`, overlay shader artifacts, and unsupported boundaries:

```text
This proves selected D3D12 generated 3D HUD box overlay output only; texture UI atlases, production text/font/image, Metal, native handles, and broad generated 3D production readiness remain unsupported.
```

### Task 5: Validators, Manifest, Docs, And Skills

**Files:**
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `tools/check-json-contracts.ps1`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

- [ ] **Step 1: Enforce installed package fields**

In `tools/validate-installed-desktop-runtime.ps1`, parse `--require-native-ui-overlay` and require:

```powershell
Assert-FieldEquals $fields 'ui_overlay_requested' '1'
Assert-FieldEquals $fields 'ui_overlay_status' 'ready'
Assert-FieldEquals $fields 'ui_overlay_ready' '1'
Assert-PositiveField $fields 'ui_overlay_sprites_submitted'
Assert-PositiveField $fields 'ui_overlay_draws'
Assert-PositiveField $fields 'hud_boxes'
```

- [ ] **Step 2: Add static contract checks**

Update `tools/check-ai-integration.ps1` and `tools/check-json-contracts.ps1` so missing flag, recipe, shader artifacts, or unsupported-boundary text fails default validation.

- [ ] **Step 3: Synchronize AI-facing truth**

Update manifest/docs/skills to state that the generated 3D package now has a selected D3D12 native UI overlay HUD box smoke. Keep unsupported:

```text
production text shaping, font rasterization, glyph atlas generation, texture UI atlas proof for generated 3D, runtime source image decoding, public native/RHI handles, Metal readiness, broad generated 3D production readiness
```

### Task 6: Focused And Full Validation

**Files:**
- Test: generated 3D package target and repository validation tools.

- [ ] **Step 1: Run focused build**

Run:

```powershell
cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug
```

Expected: PASS.

- [ ] **Step 2: Run installed D3D12 native UI overlay smoke**

Run:

```powershell
$smokeArgs = @('--smoke', '--require-config', 'runtime/sample_generated_desktop_runtime_3d_package.config', '--require-scene-package', 'runtime/sample_generated_desktop_runtime_3d_package.geindex', '--require-primary-camera-controller', '--require-transform-animation', '--require-morph-package', '--require-compute-morph', '--require-compute-morph-normal-tangent', '--require-compute-morph-skin', '--require-compute-morph-async-telemetry', '--require-quaternion-animation', '--require-package-streaming-safe-point', '--require-gameplay-systems', '--require-d3d12-scene-shaders', '--video-driver', 'windows', '--require-d3d12-renderer', '--require-scene-gpu-bindings', '--require-postprocess', '--require-postprocess-depth-input', '--require-renderer-quality-gates', '--require-playable-3d-slice', '--require-native-ui-overlay')
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -SmokeArgs $smokeArgs
```

Expected: PASS with `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, `ui_overlay_draws=2`, and `hud_boxes=2`.

- [ ] **Step 3: Run static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Expected: PASS.

- [ ] **Step 4: Run completion gates**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1
```

Expected: PASS or record concrete host/tool blockers.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED installed D3D12 package smoke with `--require-native-ui-overlay` | Pass | Pre-implementation package smoke failed as expected with `unknown argument: --require-native-ui-overlay`. |
| `cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_3d_package --config Debug` | Pass | Focused Debug build completed after CMake shader metadata and overlay wiring changes. |
| Installed D3D12 native UI overlay package smoke | Pass | Explicit smoke args and metadata-selected `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` both passed with `ui_overlay_requested=1`, `ui_overlay_status=ready`, `ui_overlay_ready=1`, `ui_overlay_sprites_submitted=2`, `ui_overlay_draws=2`, and `hud_boxes=2`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Generated `DesktopRuntime3DPackage` scaffold contract includes the native UI overlay flag, shader source, recipe, and static markers. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Audit stayed at `unsupported_gaps=11`; no broad ready claims were promoted. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Passed after formatting `games/sample_generated_desktop_runtime_3d_package/main.cpp`. |
| `git diff --check` | Pass | Whitespace check passed; Git emitted LF-to-CRLF working-copy warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full validation passed: schema/agent/production-readiness/toolchain/public API/tidy smoke/dev build/29 CTest checks; Metal and Apple lanes remained diagnostic-only/host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Default dev build completed successfully after validation. |

## Self-Review

- Spec coverage: covers generated 3D sample behavior, generated scaffold parity, package validation, manifest/docs/skills, static checks, and validation gates.
- Placeholder scan: no placeholder tasks; all commands and expected fields are explicit.
- Scope check: excludes texture UI atlas, font/text production, Metal, native handles, and broad generated 3D readiness; these require later child plans.
