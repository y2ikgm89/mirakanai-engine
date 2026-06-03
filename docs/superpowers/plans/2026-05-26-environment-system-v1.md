# 2026-05-26 Environment System v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `environment-system-v1`

**Status:** Active.

**Goal:** Add a first-class MIRAIKANAI Environment System for sky, sun, moon, ambient lighting, reflections, fog, clouds, rain, snow, storms, time-of-day, weather presets, and quality tiers without preserving backward compatibility.

**Architecture:** Introduce a clean `MK_environment` module for backend-neutral value contracts, deterministic text IO, validation, and package rows. Keep `MK_scene` limited to environment references, `MK_scene_renderer` responsible for converting scene plus environment data into renderer packets, and `MK_renderer` / `MK_rhi` responsible for backend-neutral render planning and backend-private execution. No native handles, legacy desktop middleware, Dear ImGui, or backend resources are exposed through environment public APIs.

**Tech Stack:** C++23, `MK_environment`, `MK_assets`, `MK_scene`, `MK_scene_renderer`, `MK_renderer`, `MK_rhi`, `MK_runtime_scene`, `MK_runtime_host_win32_presentation`, D3D12, Vulkan, Metal host gates, HLSL, SPIR-V, future MSL, PowerShell validation tools, optional OpenEXR/KTX-class HDR environment import after dependency and license review.

---

## Status

This plan is selected as the active `environment-system-v1` milestone. `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points to this file, `recommendedNextPlan.id = environment-system-v1`, and `unsupportedProductionGaps = []` remains unchanged.

The 2026-06-03 PR1 foundation implements the `MK_environment` value contract, validation, CMake/export wiring, public API boundary coverage, and focused tests. The PR2 text IO/package-row slice adds deterministic `GameEngine.EnvironmentProfile.v1` text IO, `AssetKind::environment_profile`, source registry/import metadata planning, cooked `GameEngine.CookedEnvironmentProfile.v1` artifacts, and `.geindex` package rows without runtime source parsing. PR3 adds scene/runtime scene environment profile binding. PR4 adds value-only render packet and renderer policy planning. PR5 adds physical-sky policy and shader-contract evidence. PR6 adds first-party cooked-texture sky-lighting/IBL policy rows without new import dependencies. PR7 adds height-fog/aerial-perspective policy rows plus a reviewed depth-aware HLSL shader contract. PR8 adds `RhiPostprocessFrameRenderer` first-stage uniform binding, height-fog constant packing, and a D3D12 WARP-safe depth-aware readback proof for the height-fog shader path. PR9 adds physical-sky constant packing, stable D3D12 constant binding metadata, and a selected D3D12 WARP-safe fullscreen shader readback proof. PR10 adds selected D3D12 package-visible height-fog evidence through `sample_desktop_runtime_game --require-environment-fog-evidence`, `environment_fog_status=ready`, `environment_fog_constants_binding=4`, and `environment_fog_constants_byte_size=256`. It does not claim physical-sky Vulkan/package proof, height-fog Vulkan runtime proof, editor authoring, cloud/rain/volumetric fog readiness, Vulkan readiness, Metal readiness, broad optimization, or broad `environment_ready`.

Active execution must keep this as one milestone with reviewable PR slices. Do not change validation recipes, package counters, optional OpenEXR/KTX dependency records, or broad environment readiness claims beyond the exact implementation evidence that has landed. `014-gameCodeGuidance.json` may only describe landed evidence and must preserve unsupported rows for missing package/runtime/backend proof.

## 2026-06-03 Project Alignment Review

- Current desktop presentation is first-party Win32 through `MK_runtime_host_win32_presentation`; removed SDL3 and other legacy desktop middleware must stay absent.
- Current visible editor shell is first-party retained `MK_editor` / `MK_editor_core`; environment authoring must use editor-core retained models before any visible first-party editor panel claim.
- Current production-completion state is the selection gate with `unsupportedProductionGaps = []`; this plan is a post-1.0 candidate, not a 1.0 blocker.
- D3D12 remains the primary Windows evidence lane, strict Vulkan proof must name Vulkan validation/toolchain requirements, and Metal remains Apple-host-gated unless a focused host recipe proves the selected feature.
- Package-visible evidence must use narrow counters such as `environment_fog_status`, `environment_cloud_layer_status`, and backend-specific evidence rows; never add a broad `environment_ready` claim.

## Official Source Baseline

Before implementation, re-check the exact current documents for the APIs touched in that implementation slice. The 2026-06-03 planning baseline used these official or primary sources plus Context7 checks for D3D12, Vulkan, and OpenEXR:

- Unreal Engine environmental lighting with fog, clouds, sky, and atmosphere: <https://dev.epicgames.com/documentation/en-us/unreal-engine/environmental-light-with-fog-clouds-sky-and-atmosphere-in-unreal-engine>
- Unreal Engine Sky Atmosphere: <https://dev.epicgames.com/documentation/unreal-engine/sky-atmosphere-component-in-unreal-engine?lang=en-US>
- Unreal Engine Exponential Height Fog: <https://dev.epicgames.com/documentation/unreal-engine/exponential-height-fog-in-unreal-engine?lang=en-US>
- Unreal Engine Volumetric Cloud Component: <https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine?lang=en-US>
- Unreal Engine Sky Lights: <https://dev.epicgames.com/documentation/unreal-engine/sky-lights-in-unreal-engine?lang=en-US>
- Unity Cubemaps: <https://docs.unity3d.com/Manual/class-Cubemap-introduction.html>
- Unity Cubemap texture import/convolution settings: <https://docs.unity3d.com/Manual/texture-type-cubemap.html>
- Unity HDRP Sky: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/sky.html>
- Unity HDRP Visual Environment: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/Override-Visual-Environment.html>
- Unity HDRP Fog: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/fog.html>
- Unity HDRP Clouds: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/clouds-in-hdrp.html>
- Unity HDRP Volumetric Clouds override: <https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/Override-Volumetric-Clouds.html>
- Godot Environment and post-processing: <https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html>
- Godot Volumetric Fog and fog volumes: <https://docs.godotengine.org/en/stable/tutorials/3d/volumetric_fog.html>
- Hillaire 2020, "A Scalable and Production Ready Sky and Atmosphere Rendering Technique": <https://diglib.eg.org/items/8a3e5350-18b3-46bd-9274-3add5af88c75>
- Microsoft Direct3D 11 texture/cubemap resource overview: <https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro>
- Microsoft DDS cubemap layout: <https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-file-layout-for-cubic-environment-maps>
- Microsoft HLSL semantics: <https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics>
- Vulkan synchronization examples: <https://docs.vulkan.org/guide/latest/synchronization_examples.html>
- D3D12 ResourceBarrier reference: <https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier>
- D3D12 Root Signatures overview: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview>
- D3D12 resource binding flow: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control>
- OpenEXR documentation: <https://openexr.com/>

Implementation slices must treat these as design input, not as copied implementation. D3D12 descriptor heaps, root signatures, command lists, fences, and resource barriers stay backend-private. Vulkan synchronization2 image layout transitions and shader-read barriers must be proven through strict validation before Vulkan readiness. OpenEXR or KTX-class HDR environment import requires dependency, license, vcpkg feature, importer tests, and package evidence before any ready claim.

## Clean-Break Policy

- Do not preserve compatibility with the current ad hoc sky/lighting assumptions.
- Do not add deprecated aliases, duplicate environment APIs, or migration shims.
- Do not overload `LightComponent` with sky, weather, cloud, or environment settings.
- Do not expose D3D12, Vulkan, Metal, legacy desktop middleware, Dear ImGui, or raw RHI handles through `MK_environment`.
- Do not claim production sky, fog, cloud, rain, weather, IBL, or renderer parity until tests and package-visible evidence prove that exact feature.
- Do not infer Vulkan or Metal readiness from D3D12 evidence. Each backend needs its own evidence row.
- Keep gameplay-facing runtime code on first-party public contracts only.

## Current Engine Baseline

The repository already has useful foundations:

- `MK_scene` has camera, light, mesh, and sprite renderer components.
- `LightComponent` supports directional, point, and spot lights with color, intensity, range, cone values, and shadow intent.
- `MK_renderer` has postprocess policy rows and a current `fog` enum value, but fog is not yet a proven rendered feature.
- D3D12 and strict Vulkan package smokes already prove selected scene GPU binding, depth-aware postprocess, and narrow directional shadow filtering in sample lanes.
- `MK_runtime_host_win32_presentation` already owns host-private Windows desktop presentation and package-visible diagnostics.

This plan promotes those foundations into a general environment system without turning existing smoke paths into broad claims early.

## Proposed Module Boundaries

| Module | Responsibility |
| --- | --- |
| `MK_environment` | Environment profile value APIs, validation, text IO, package rows, time-of-day and weather planning, no GPU execution. |
| `MK_scene` | Environment profile reference on Scene v2 and runtime scene package metadata only. |
| `MK_scene_renderer` | Builds `EnvironmentRenderPacket` from scene, camera, lights, and environment profile values. |
| `MK_renderer` | Backend-neutral environment render policies, pass plans, quality budgets, diagnostics, and shader contract layouts. |
| `MK_rhi` | Backend-neutral resource, descriptor, render-pass, compute, barrier, and readback contracts needed by environment rendering. |
| `MK_rhi_d3d12` | D3D12 execution evidence with backend-private resources, descriptor heaps, root signatures, barriers, and WARP-safe tests. |
| `MK_rhi_vulkan` | Strict Vulkan execution evidence with synchronization2, image layouts, descriptor sets, SPIR-V validation, and host gates. |
| `MK_rhi_metal` | Apple-host-gated Metal evidence only after Xcode/Metal tools are available. |
| `MK_runtime_host_win32_presentation` | Package-visible environment status fields while keeping native presentation resources private. |
| `MK_editor_core` / `MK_editor` | Reviewed environment authoring model and visible panel after core contracts are stable. |

## Public Contract Shape

The initial public API should use explicit value objects and fail-closed diagnostics:

```cpp
namespace mirakana {

enum class EnvironmentSkyModel : std::uint8_t {
    none = 0,
    color,
    gradient,
    hdri,
    physical_atmosphere,
};

enum class EnvironmentWeatherKind : std::uint8_t {
    clear = 0,
    cloudy,
    rain,
    storm,
    snow,
    foggy,
    dust,
    ash,
};

enum class EnvironmentPrecipitationKind : std::uint8_t {
    none = 0,
    rain,
    snow,
    sleet,
    hail,
    ash,
    dust,
};

struct EnvironmentSunMoonDesc {
    Vec3 direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 color{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float illuminance_lux{100000.0F};
    float angular_radius_radians{0.00465F};
    bool visible_disk{true};
    bool affects_atmosphere{true};
    bool affects_clouds{true};
    bool casts_environment_shadows{true};
};

struct EnvironmentAtmosphereDesc {
    float planet_radius_km{6360.0F};
    float atmosphere_height_km{100.0F};
    float rayleigh_density{1.0F};
    float mie_density{1.0F};
    float mie_anisotropy{0.8F};
    float ozone_density{1.0F};
    float ground_albedo{0.3F};
};

struct EnvironmentFogDesc {
    bool enabled{false};
    float density{0.0F};
    float height_falloff{0.2F};
    Vec3 albedo{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float anisotropy{0.0F};
    float sky_affect{1.0F};
};

struct EnvironmentPrecipitationDesc {
    EnvironmentPrecipitationKind kind{EnvironmentPrecipitationKind::none};
    float intensity{0.0F};
    float particle_radius_meters{0.001F};
    float fall_speed_meters_per_second{9.0F};
    Vec3 wind_velocity_meters_per_second{};
    bool surface_wetness{false};
    bool collision_splashes{false};
    bool occlusion_required{true};
};

struct EnvironmentProfileDesc {
    std::string id;
    EnvironmentSkyModel sky_model{EnvironmentSkyModel::physical_atmosphere};
    EnvironmentWeatherKind weather{EnvironmentWeatherKind::clear};
    EnvironmentSunMoonDesc sun;
    EnvironmentSunMoonDesc moon;
    EnvironmentAtmosphereDesc atmosphere;
    EnvironmentFogDesc fog;
    EnvironmentPrecipitationDesc precipitation;
};

} // namespace mirakana
```

The final names can change during implementation, but the shape should remain explicit, deterministic, and backend-neutral.

## Quality And Evidence Counters

Package-visible evidence should use narrow counters rather than a broad `environment_ready` claim:

- `environment_profile_status`
- `environment_profile_diagnostics`
- `environment_sky_status`
- `environment_atmosphere_status`
- `environment_sun_lights`
- `environment_moon_lights`
- `environment_sky_lighting_status`
- `environment_fog_status`
- `environment_volumetric_fog_status`
- `environment_cloud_layer_status`
- `environment_volumetric_cloud_status`
- `environment_precipitation_status`
- `environment_surface_wetness_status`
- `environment_weather_blend_status`
- `environment_d3d12_evidence_ready`
- `environment_vulkan_evidence_ready`
- `environment_metal_host_evidence`

Each counter must be tied to a validation recipe and exact implementation evidence.

---

## Task 1: Official-Practice Design Record

**Files:**
- Create: `docs/specs/2026-05-26-environment-system-v1-design.md`

- [x] **Step 1: Write the design record**

Create a design record that summarizes the official source baseline, clean-break policy, module boundaries, public API family, and feature phase order from this plan. Include the source URLs listed above and explicitly state that no broad readiness claim exists before validation evidence lands.

- [x] **Step 2: Review against current engine boundaries**

Check the design against:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
```

Expected: The output confirms `MK_scene`, `MK_scene_renderer`, `MK_renderer`, `MK_rhi`, and `MK_runtime_host_win32_presentation` are the correct integration boundaries.

- [x] **Step 3: Drift check**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: PASS, or only pre-existing unrelated text-format issues are reported with exact paths.

**2026-06-03 Task 1 Evidence:** Added `docs/specs/2026-05-26-environment-system-v1-design.md`; compose-backed `tools/agent-context.ps1 -ContextProfile Minimal` confirmed `currentActivePlan = docs/superpowers/plans/2026-05-26-environment-system-v1.md`, `recommendedNextPlan.id = environment-system-v1`, `MK_environment` depends only on `MK_math`, and the integration boundaries remain `MK_scene`, `MK_scene_renderer`, `MK_renderer`, `MK_rhi`, and `MK_runtime_host_win32_presentation`. `tools/check-text-format.ps1` passed.

## Task 2: `MK_environment` Contract Foundation

**Files:**
- Create: `engine/environment/include/mirakana/environment/environment_profile.hpp`
- Create: `engine/environment/src/environment_profile.cpp`
- Create: `engine/environment/CMakeLists.txt`
- Modify: root `CMakeLists.txt`
- Create: `tests/unit/environment_tests.cpp`
- Modify: root `CMakeLists.txt` test target list

- [x] **Step 1: Add RED tests for validation**

Add tests that require:

- valid Earth-like default profile succeeds;
- duplicate or empty profile ids fail;
- invalid sun/moon directions fail;
- negative atmosphere height fails;
- invalid fog density fails;
- invalid rain/snow intensity fails;
- native/backend/editor token strings in profile ids fail.

- [x] **Step 2: Add the public contract**

Implement the clean value types shown in the "Public Contract Shape" section, plus:

```cpp
enum class EnvironmentProfileDiagnosticCode : std::uint8_t;
struct EnvironmentProfileDiagnostic;
struct EnvironmentProfileValidationResult;

[[nodiscard]] EnvironmentProfileValidationResult
validate_environment_profile(const EnvironmentProfileDesc& desc);

[[nodiscard]] bool is_valid_environment_profile(const EnvironmentProfileDesc& desc) noexcept;
```

- [x] **Step 3: Build and test the focused target**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment"
```

Expected: `MK_environment_tests` passes.

**2026-06-03 PR1 Evidence:** RED failed on missing `mirakana/environment/environment_profile.hpp`; GREEN added `MK_environment` and validation APIs. Focused evidence passed: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_environment_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R MK_environment_tests`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, and `tools/check-tidy.ps1 -Files engine/environment/src/environment_profile.cpp,tests/unit/environment_tests.cpp`.

## Task 3: Environment Text IO And Package Rows

**Files:**
- Create: `engine/environment/include/mirakana/environment/environment_io.hpp`
- Create: `engine/environment/src/environment_io.cpp`
- Modify: `tests/unit/environment_tests.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_kind.hpp`
- Modify: `engine/tools/src/asset_import_plan.cpp`

- [x] **Step 1: Add RED text IO tests**

Require deterministic round-trip serialization for `GameEngine.EnvironmentProfile.v1` with stable key ordering:

```text
GameEngine.EnvironmentProfile.v1
id=default_outdoor
sky.model=physical_atmosphere
weather.kind=clear
sun.direction=0,-1,0
sun.illuminance_lux=100000
fog.enabled=false
precipitation.kind=none
```

- [x] **Step 2: Add package asset kind**

Add `AssetKind::environment_profile` and ensure package rows can reference cooked environment profile content without runtime source parsing.

- [x] **Step 3: Validate malformed documents**

Reject unknown schema names, invalid enums, duplicate keys, non-finite floats, negative precipitation values, and native/backend/editor tokens in authored ids.

- [x] **Step 4: Focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_environment_tests MK_assets_tests MK_tools_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "environment|asset|tools"
```

Expected: Focused tests pass.

**2026-06-03 PR2 Evidence:** RED failed on missing `mirakana/environment/environment_io.hpp`; GREEN added deterministic `GameEngine.EnvironmentProfile.v1` serialization/deserialization, enum text conversion, fail-closed malformed document parsing, `AssetKind::environment_profile`, source registry/import metadata planning, cooked `GameEngine.CookedEnvironmentProfile.v1` artifact emission, package assembly/index rows, and runtime-scene diagnostic kind labels. Focused evidence passed: `tools/cmake.ps1 --build --preset dev --target MK_environment_tests MK_assets_tests MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_environment_tests|MK_assets_tests|MK_tools_tests)$"`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, and targeted `tools/check-tidy.ps1 -Files ...` for 14 changed C++ files. The broader `ctest -R "environment|asset|tools"` pattern is intentionally too broad on this worktree unless all matching test executables are built first; the exact three-target focused CTest lane is the Task 3 evidence.

## Task 4: Scene Environment Binding

**Files:**
- Modify: `engine/scene/include/mirakana/scene/schema_v2.hpp`
- Modify: `engine/scene/src/schema_v2.cpp`
- Modify: `engine/scene/include/mirakana/scene/render_packet.hpp`
- Modify: `engine/scene/src/render_packet.cpp`
- Modify: `engine/runtime_scene/include/mirakana/runtime_scene/runtime_scene.hpp`
- Modify: `engine/runtime_scene/src/runtime_scene.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/scene_schema_v2_tests.cpp`
- Modify: `tests/unit/runtime_scene_tests.cpp`
- Modify: `tests/unit/tools_tests.cpp`

- [x] **Step 1: Add RED scene binding tests**

Require authored scenes to carry exactly one selected environment profile reference when a runtime environment is requested. Missing profile references should be diagnostics, not implicit defaults.

- [x] **Step 2: Add scene reference rows**

Add scene rows equivalent to:

```text
environment.profile=environment/default_outdoor.environment
environment.required=true
```

- [x] **Step 3: Keep `LightComponent` clean**

Do not add sky, weather, cloud, rain, or atmosphere fields to `LightComponent`. Scene lights remain physical scene lights; environment sun/moon lives in `MK_environment`.

- [x] **Step 4: Focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_assets_tests MK_core_tests MK_scene_schema_v2_tests MK_runtime_scene_tests MK_tools_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_assets_tests|MK_core_tests|MK_scene_schema_v2_tests|MK_runtime_scene_tests|MK_tools_tests)$"
```

Expected: Scene and runtime scene tests pass.

**2026-06-03 PR3 Evidence:** RED failed on missing `SceneEnvironmentReference`, `Scene::set_environment`, `Scene::environment`, and `SceneRenderPacket::environment`; GREEN added scene-level environment profile references in `Scene`, deterministic `GameEngine.Scene.v1` and `GameEngine.Scene.v2` serialization/deserialization rows, render packet propagation, runtime scene `environment_profile` reference validation, missing-required-environment diagnostics, asset identity audit rows, source registry/import metadata `scene_environment_profile` dependencies, scene package dependency validation, and Scene v2 runtime package migration projection. `LightComponent` remains free of sky/weather/cloud/rain/atmosphere fields. Focused evidence passed: `tools/cmake.ps1 --build --preset dev --target MK_assets_tests MK_core_tests MK_scene_schema_v2_tests MK_runtime_scene_tests MK_tools_tests`, `tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_assets_tests|MK_core_tests|MK_scene_schema_v2_tests|MK_runtime_scene_tests|MK_tools_tests)$"`, targeted `tools/check-tidy.ps1 -Files ...`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, and full `tools/validate.ps1` with 86/86 tests passed.

## Task 5: Environment Render Packet Planning

**Files:**
- Create: `engine/scene_renderer/include/mirakana/scene_renderer/environment_renderer.hpp`
- Create: `engine/scene_renderer/src/environment_renderer.cpp`
- Modify: `engine/scene_renderer/CMakeLists.txt`
- Create: `tests/unit/scene_environment_renderer_tests.cpp`

- [x] **Step 1: Add RED packet tests**

Require `build_environment_render_packet` to combine camera, scene lights, and environment profile into deterministic render rows. Reject conflicting sun directions when a scene directional light is explicitly bound to the environment sun but points elsewhere.

- [x] **Step 2: Add packet value types**

Expose value rows:

```cpp
struct EnvironmentAtmosphereRenderRow;
struct EnvironmentSkyRenderRow;
struct EnvironmentFogRenderRow;
struct EnvironmentCloudRenderRow;
struct EnvironmentPrecipitationRenderRow;
struct EnvironmentRenderPacket;

[[nodiscard]] EnvironmentRenderPacket
build_environment_render_packet(const SceneRenderPacket& scene_packet,
                                const EnvironmentProfileDesc& environment);
```

- [x] **Step 3: Keep renderer-free behavior**

This task must not create GPU resources, compile shaders, or call `IRenderer`.

- [x] **Step 4: Focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_scene_environment_renderer_tests MK_scene_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_scene_environment_renderer_tests|MK_scene_renderer_tests)$"
```

Expected: Scene renderer environment tests pass.

**2026-06-03 PR4 Task 5 Evidence:** RED failed on missing `mirakana/scene_renderer/environment_renderer.hpp`; GREEN added value-only `EnvironmentRenderPacket`, atmosphere/sky/fog/cloud/precipitation/celestial/scene-light rows, explicit scene-light sun/moon binding rows outside `LightComponent`, conflict diagnostics for bound sun/moon direction mismatches, and no renderer/RHI/GPU resource calls. Focused evidence passed: `tools/cmake.ps1 --build --preset dev --target MK_scene_environment_renderer_tests MK_renderer_environment_policy_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_scene_environment_renderer_tests|MK_renderer_environment_policy_tests)$"`.

## Task 6: Renderer Environment Policy Foundation

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/environment_policy.hpp`
- Create: `engine/renderer/src/environment_policy.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Create: `tests/unit/renderer_environment_policy_tests.cpp`

- [x] **Step 1: Add RED policy tests**

Require fail-closed diagnostics for missing scene color, missing scene depth when fog or cloud shadowing needs it, missing backend shader evidence, excessive raymarch step budgets, unsupported backend inheritance, and public native handle claims.

- [x] **Step 2: Add policy types**

Add:

```cpp
enum class EnvironmentRenderFeature : std::uint8_t;
enum class EnvironmentPolicyDiagnosticCode : std::uint8_t;
struct EnvironmentPolicyDesc;
struct EnvironmentPolicyPlan;

[[nodiscard]] EnvironmentPolicyPlan plan_environment_render_policy(const EnvironmentPolicyDesc& desc);
```

- [x] **Step 3: Keep it backend-neutral**

The policy plan may mention D3D12, Vulkan, and Metal evidence rows by name, but it must not include native structs, native enums, or native handles.

- [x] **Step 4: Focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_environment_policy_tests MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "^(MK_renderer_environment_policy_tests|MK_renderer_tests)$"
```

Expected: Renderer policy tests pass.

**2026-06-03 PR4 Task 6 Evidence:** GREEN added backend-neutral `EnvironmentRenderFeature`, `EnvironmentPolicyDesc`, `EnvironmentPolicyPlan`, deterministic feature rows, and fail-closed diagnostics for missing scene color/depth, missing backend shader/validation evidence, excessive raymarch budgets, unsupported backend inheritance, and native-handle claims. This is policy planning only; it does not create PSOs, root signatures, descriptor layouts, shader artifacts, render passes, Vulkan/Metal/D3D12 backend execution, package-visible counters, or broad `environment_ready`.

## Task 7: Physical Sky And Sun Disk

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/physical_sky_policy.hpp`
- Create: `engine/renderer/src/physical_sky_policy.cpp`
- Create: `shaders/environment/physical_sky.hlsl`
- Create: `tests/shaders/environment_physical_sky.hlsl`
- Modify: `tests/unit/renderer_environment_policy_tests.cpp`
- Modify: `tools/check-shader-toolchain.ps1` only if a new shader artifact class needs validation.

- [x] **Step 1: Re-check official sky sources**

Re-check Unreal Sky Atmosphere and Hillaire 2020 before writing shader constants. Record source URLs in the phase evidence.

- [x] **Step 2: Add RED tests for sky policy**

Require deterministic validation for Rayleigh density, Mie density, Mie anisotropy, ozone density, planet radius, atmosphere height, sun disk radius, sample budget, and aerial perspective mode.

- [x] **Step 3: Implement policy and shader contract**

Add backend-neutral constant layout rows for transmittance, sky-view, aerial perspective, and multiple-scattering LUT intent. Do not allocate LUT textures in this task unless the same PR adds D3D12 or Vulkan readback proof.

- [x] **Step 4: Validate shader planning**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "physical_sky|renderer_environment"
```

Expected: Policy tests pass; shader toolchain either reports ready or a concrete host/tool blocker.

**2026-06-03 PR5 Task 7 Evidence:** GREEN added backend-neutral `PhysicalSkyPolicyDesc`, `PhysicalSkyPolicyPlan`, deterministic constant-buffer layout rows, transmittance / sky-view / aerial-perspective / multiple-scattering LUT intent rows, fail-closed diagnostics for invalid Rayleigh/Mie/ozone/planet/sun/sample/evidence/native-handle/LUT-allocation requests, and `shaders/environment/physical_sky.hlsl` plus `tests/shaders/environment_physical_sky.hlsl` shader-contract sources. The phase re-checked Epic's official Sky Atmosphere documentation (`https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere?application_version=4.27`), Sebastien Hillaire's 2020 production atmosphere paper (`https://sebh.github.io/publications/egsr2020.pdf`), Microsoft HLSL constant-buffer and semantic docs (`https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-constants`, `https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics`), and Context7 `/khronosgroup/vulkan-docs` synchronization/image-layout evidence before fixing the shader contract. Focused validation passed for `MK_renderer_physical_sky_policy_tests`; DXC compiled the contract to DXIL and Vulkan SPIR-V for vertex/pixel entry points, and `spirv-val --target-env vulkan1.3` passed for both SPIR-V artifacts. `tools/check-shader-toolchain.ps1` reported D3D12 DXIL and Vulkan SPIR-V ready, with Metal tools missing as diagnostic-only on this Windows host. This is physical-sky policy and shader-contract evidence only; it does not allocate LUT textures, create PSOs, descriptor layouts, render passes, shader package artifacts, backend resources, package-visible environment counters, Vulkan readiness, Metal readiness, or broad `environment_ready`.

**2026-06-04 PR9 Task 7 Evidence:** GREEN added `physical_sky_constants_binding`, `physical_sky_constants_byte_size`, and `pack_physical_sky_constants` so the reviewed `PhysicalSkyConstants` layout has an explicit 256-byte D3D12 constant-buffer packing path. The D3D12 RHI proof compiles a fullscreen physical-sky shader using `SV_VertexID`, `SV_Position`, and `SV_Target`, binds the packed constants through a uniform-buffer descriptor at binding `0`, draws on Microsoft WARP, copies the render target to a readback buffer, and asserts a deterministic sky gradient plus descriptor-write, descriptor-set, draw, texture-copy, and buffer-write counters. The slice re-checked Microsoft HLSL semantics (`https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics`) and Context7 `/microsoft/directxshadercompiler` HLSL cbuffer/register evidence before adding the proof. Focused validation passed for `MK_renderer_physical_sky_policy_tests` and `MK_d3d12_rhi_tests`; DXC and `spirv-val --target-env vulkan1.3` still validate the physical-sky contract sources. This is a selected D3D12 WARP-safe readback proof only; it does not allocate LUT textures, add shader package artifacts, expose package-visible environment counters, prove Vulkan runtime execution, prove Metal readiness, prove backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 8: Sky Lighting And IBL

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/environment_lighting_policy.hpp`
- Create: `engine/renderer/src/environment_lighting_policy.cpp`
- Not modified in PR6 no-new-dependency mode: `engine/assets/include/mirakana/assets/asset_kind.hpp`, `engine/tools/src/asset_import_plan.cpp`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`
- Modify: `vcpkg.json` only if EXR/KTX support is selected for this phase.

- [x] **Step 1: Choose dependency mode**

Select one mode for this phase:

- no new dependency: use existing first-party cooked texture payloads and HDRI metadata only;
- optional OpenEXR feature: add EXR source import through a vcpkg feature and legal records;
- optional KTX feature: add KTX environment map package support through a vcpkg feature and legal records.

- [x] **Step 2: Add RED IBL policy tests**

Require separation of visual sky from lighting sky, reflection cubemap size, irradiance rows, radiance mip rows, roughness mip count, HDR clamp exposure policy, and package evidence rows.

- [x] **Step 3: Implement policy**

Add a renderer policy that produces deterministic rows for ambient lighting and reflection fallback without requiring the sky to be visible in the main camera.

- [x] **Step 4: Dependency validation when selected**

If a dependency is added, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

Expected: Dependency and legal checks pass, or dependency bootstrap records a concrete host blocker.

**2026-06-03 PR6 Task 8 Evidence:** GREEN selected the no-new-dependency mode: use existing first-party cooked texture payloads plus HDR metadata only, and do not add OpenEXR/KTX import, dependency/legal/vcpkg records, source importer changes, runtime capture, cubemap upload, descriptor creation, image-layout transitions, PSO/render-pass creation, package-visible environment counters, or backend execution in this slice. `MK_renderer` now exposes `EnvironmentLightingPolicyDesc`, `EnvironmentLightingPolicyPlan`, `EnvironmentReflectionCubemapDesc`, `EnvironmentIrradianceDesc`, `EnvironmentRadianceDesc`, `EnvironmentHdrClampPolicyDesc`, `plan_environment_lighting_policy`, and `has_environment_lighting_diagnostic`. The policy produces deterministic cubemap, irradiance SH, radiance mip/roughness, package-evidence, and HDR clamp rows while keeping visual sky and lighting sky independently declared and failing closed for unsupported import dependency requests, invalid cubemap size/mips/asset/HDR/package evidence, unsupported visual-lighting coupling, runtime capture, backend execution, and native-handle access. Official context re-checked Unreal Sky Lights (`https://dev.epicgames.com/documentation/unreal-engine/sky-lights-in-unreal-engine?lang=en-US`), Unity Cubemaps and cubemap import convolution (`https://docs.unity3d.com/Manual/class-Cubemap-introduction.html`, `https://docs.unity3d.com/Manual/texture-type-cubemap.html`), Microsoft Direct3D 11 cubemap/mipmap resource docs (`https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-resources-textures-intro`, `https://learn.microsoft.com/en-us/windows/win32/direct3ddds/dds-file-layout-for-cubic-environment-maps`), and Context7 `/khronosgroup/vulkan-docs` image layout/descriptor evidence before implementation. Focused validation passed for `MK_renderer_environment_lighting_policy_tests`, `MK_renderer_environment_policy_tests`, and `MK_renderer_tests`; dependency validation is no-op beyond confirming no dependency was selected and preserving dependency/legal files unchanged. This is sky-lighting/IBL policy evidence only, not physical-sky Vulkan/package proof, runtime cubemap capture, importer support, renderer/RHI environment execution, Vulkan readiness, Metal readiness, or broad `environment_ready`.

## Task 9: Height Fog And Aerial Perspective

**Files:**
- Modify: `CMakeLists.txt`
- Modify: `engine/renderer/CMakeLists.txt`
- Create: `engine/renderer/include/mirakana/renderer/environment_fog_policy.hpp`
- Create: `engine/renderer/src/environment_fog_policy.cpp`
- Create: `shaders/environment/height_fog.hlsl`
- Create: `tests/shaders/environment_height_fog.hlsl`
- Create: `tests/unit/renderer_environment_fog_policy_tests.cpp`

- [x] **Step 1: Add RED fog policy tests**

The tests must prove `fog` is no longer only an enum value. Require depth input, finite density, valid height falloff, valid sky affect, valid anisotropy, shader evidence, and exact pass budget rows.

- [x] **Step 2: Implement policy**

Use existing postprocess depth input foundations. PR7 adds policy/shader-contract evidence only; PR10 is the first package-visible promotion for the selected D3D12 lane, so `environment_fog_status=ready` remains valid only for `sample_desktop_runtime_game --require-environment-fog-evidence` on D3D12.

- [x] **Step 3: Add D3D12 readback proof**

Add a focused D3D12 WARP-safe readback test proving fog changes at least two known depths differently. Do not use subjective screenshot comparison as the only proof.

- [ ] **Step 4: Add Vulkan host-gated proof**

Add a strict Vulkan proof guarded by explicit SPIR-V artifact environment variables and validation-layer readiness.

**2026-06-03 PR7 Task 9 Evidence:** GREEN added backend-neutral `EnvironmentFogPolicyDesc`, `EnvironmentFogPolicyPlan`, `EnvironmentFogConstantLayoutRow`, `EnvironmentFogDepthInputRow`, `EnvironmentFogPassBudgetRow`, `EnvironmentFogPostprocessRow`, `plan_environment_fog_policy`, and `has_environment_fog_diagnostic`. The policy validates supported height-fog/aerial-perspective modes, finite density/falloff/height offset/distance/color values, sky affect, anisotropy, sample-step budget, scene-depth evidence, shader-contract evidence, and fail-closed volumetric-fog/backend/native-handle requests while producing exact one-pass fullscreen-triangle budget rows and `PostprocessEffectKind::fog` rows. `shaders/environment/height_fog.hlsl` and `tests/shaders/environment_height_fog.hlsl` add the reviewed depth-aware HLSL contract using `SV_VertexID`, `SV_Position`, `SV_Target0`, explicit `packoffset` constant rows, and sampled scene depth. Official context re-checked Unreal Exponential Height Fog (`https://dev.epicgames.com/documentation/unreal-engine/exponential-height-fog-in-unreal-engine?lang=en-US`), Unity HDRP Fog (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/fog.html`), Godot Environment and post-processing fog guidance (`https://docs.godotengine.org/en/stable/tutorials/3d/environment_and_post_processing.html`), Microsoft HLSL semantics (`https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics`), and Context7 `/khronosgroup/vulkan-docs` depth-attachment-to-fragment-shader sampled-read synchronization evidence before implementation. Focused validation passed for `MK_renderer_environment_fog_policy_tests`, `MK_renderer_environment_policy_tests`, and `MK_renderer_tests`; DXC compiled the contract to DXIL and Vulkan SPIR-V for vertex/pixel entry points, and `spirv-val --target-env vulkan1.3` passed for both SPIR-V artifacts. `tools/check-shader-toolchain.ps1` reported D3D12 DXIL and Vulkan SPIR-V ready, with Metal tools missing as diagnostic-only on this Windows host. This is height-fog policy and shader-contract evidence only; it does not add D3D12 readback proof, Vulkan runtime proof, package-visible `environment_fog_status`, renderer/RHI environment execution, volumetric fog, package counters, Vulkan readiness, Metal readiness, or broad `environment_ready`.

**2026-06-04 PR8 Task 9 Evidence:** GREEN added stable height-fog postprocess descriptor constants for scene color at bindings `0/1`, scene depth at `2/3`, and first-stage uniform constants at `4`; `pack_environment_fog_constants` writes the reviewed 256-byte height-fog constant layout; and `RhiPostprocessFrameRendererDesc::postprocess_first_uniform_buffer` lets the renderer-owned postprocess path bind one optional uniform buffer without exposing native handles. `shaders/environment/height_fog.hlsl` now consumes scene color, scene depth, and the uniform row through that binding contract. A focused D3D12 WARP-safe readback test renders two known depth regions, runs the height-fog postprocess shader with packed constants, and proves fog changes near and far pixels differently while recording expected descriptor bind/write, draw, copy/readback, and buffer-write counters. Official context was re-checked for Microsoft D3D12 resource barriers (`https://learn.microsoft.com/en-us/windows/win32/api/d3d12/nf-d3d12-id3d12graphicscommandlist-resourcebarrier`), D3D12 root signatures (`https://learn.microsoft.com/en-us/windows/win32/direct3d12/root-signatures-overview`), D3D12 resource binding flow (`https://learn.microsoft.com/en-us/windows/win32/direct3d12/resource-binding-flow-of-control`), Microsoft HLSL semantics (`https://learn.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-semantics`), and Context7 `/khronosgroup/vulkan-docs` depth attachment write to fragment shader sampled-read synchronization. Focused validation passed for `MK_renderer_environment_fog_policy_tests`, `MK_renderer_tests`, and `MK_d3d12_rhi_tests`. This is a selected D3D12 height-fog execution/readback proof only; it does not add Vulkan runtime proof, package-visible `environment_fog_status`, physical-sky Vulkan/package proof, package counters, shader package artifacts, Metal readiness, broad GPU optimization, backend parity, or broad `environment_ready`.

**2026-06-04 PR10 Task 9/16 Evidence:** GREEN adds selected D3D12 package-visible height-fog evidence for `sample_desktop_runtime_game`. The package target now emits `shaders/sample_desktop_runtime_game_environment_fog.ps.dxil` from the reviewed `height_fog_ps_main` shader, `Win32DesktopPresentation` owns the fog constant buffer privately, `evaluate_win32_desktop_presentation_environment_fog` fail-closes missing D3D12 selection, depth input, postprocess execution, constant-buffer readiness, and pass-count evidence, and installed validation accepts `--require-environment-fog-evidence` only when `environment_fog_status=ready`, `environment_fog_ready=1`, `environment_fog_selected_backend=d3d12`, `environment_fog_depth_input_ready=1`, `environment_fog_constant_buffer_ready=1`, `environment_fog_constants_binding=4`, `environment_fog_constants_byte_size=256`, `environment_fog_postprocess_passes_ok=1`, `postprocess_policy_fog_effect=1`, and `postprocess_d3d12_execution_passes_ok=1` are present. Source-tree smoke and `tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game -RequireD3d12Shaders` with the fog smoke args passed on 2026-06-04. This is selected D3D12 height-fog package evidence only; it does not claim height-fog Vulkan execution proof, physical-sky package proof, volumetric fog, cloud/rain readiness, Metal readiness, backend parity, broad optimization, or broad `environment_ready`.

## Task 10: Volumetric Fog

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/volumetric_fog_policy.hpp`
- Create: `engine/renderer/src/volumetric_fog_policy.cpp`
- Create: `shaders/environment/volumetric_fog.hlsl`
- Create: `tests/unit/renderer_volumetric_fog_policy_tests.cpp`

- [x] **Step 1: Add RED volumetric fog policy tests**

Require froxel grid dimensions, slice count, range, density, albedo, anisotropy, temporal reprojection toggle, history weight, local fog volume rows, and quality tier diagnostics.

- [x] **Step 2: Implement value planning**

Add CPU-side policy rows first. The first PR for this task may stop at value planning and validation if GPU execution is too large for one review.

- [ ] **Step 3: Add execution proof**

Promote to ready only after D3D12 readback or package evidence proves volumetric fog output. Vulkan remains strict-gated; Metal remains Apple-host-gated.

**2026-06-04 PR11 Task 10 Evidence:** GREEN adds backend-neutral `VolumetricFogPolicyDesc`, `VolumetricFogFroxelGridDesc`, `VolumetricFogTemporalDesc`, `VolumetricFogLocalVolumeDesc`, `VolumetricFogPolicyPlan`, `plan_volumetric_fog_policy`, `has_volumetric_fog_diagnostic`, stable scene-depth bindings at `2/3`, stable constant binding `5`, froxel grid rows, quality/temporal rows, depth-input rows, local fog volume rows, and fail-closed diagnostics for unsupported quality tiers, invalid froxel grids, invalid range/density/albedo/anisotropy/history-weight values, invalid local volumes, missing scene depth, missing shader-contract evidence, missing execution evidence, unsupported froxel allocation, unsupported backend execution, and native-handle claims. `shaders/environment/volumetric_fog.hlsl` and `tests/shaders/environment_volumetric_fog.hlsl` add a reviewed compute-shader contract source for density, albedo, anisotropy, froxel slice range, temporal history weight, and scene-depth binding metadata. Official context was re-checked for Unreal Volumetric Fog global/local controls, view distance and grid-size/performance controls, and temporal reprojection (`https://dev.epicgames.com/documentation/en-us/unreal-engine/volumetric-fog-in-unreal-engine`), Unity HDRP Volumetric Lighting quality/reprojection guidance (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4016.0/manual/Volumetric-Lighting.html`), and Godot Volumetric Fog/FogVolume density, albedo, anisotropy, length, froxel volume size/depth, temporal reprojection, and local volume guidance (`https://docs.godotengine.org/en/stable/tutorials/3d/volumetric_fog.html`). Focused validation passed for `MK_renderer_volumetric_fog_policy_tests`, `tools/check-tidy.ps1 -Files engine/renderer/src/volumetric_fog_policy.cpp,tests/unit/renderer_volumetric_fog_policy_tests.cpp`, `tools/check-format.ps1`, DXC DXIL compute compilation, DXC Vulkan SPIR-V compute compilation, and `spirv-val`. This is volumetric-fog value planning and shader-contract evidence only; it does not allocate froxel volumes, execute renderer/RHI backends, add D3D12 readback/package evidence, prove Vulkan runtime execution, prove Metal readiness, claim cloud/rain readiness, claim backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 11: Cloud Layer

**Files:**
- Create: `engine/environment/include/mirakana/environment/cloud_layer.hpp`
- Create: `engine/environment/src/cloud_layer.cpp`
- Create: `engine/renderer/include/mirakana/renderer/cloud_layer_policy.hpp`
- Create: `engine/renderer/src/cloud_layer_policy.cpp`
- Create: `shaders/environment/cloud_layer.hlsl`
- Create: `tests/unit/environment_cloud_tests.cpp`
- Create: `tests/unit/renderer_cloud_layer_policy_tests.cpp`

- [x] **Step 1: Add RED cloud layer tests**

Require 2D cloud coverage, opacity, altitude, wind velocity, flow-map asset reference, sky tint response, time-of-day response, and IBL contribution mode diagnostics.

- [x] **Step 2: Implement cheap cloud layer**

This is the low-cost weather sky path. It should be usable without volumetric clouds and should be the default for lower quality tiers.

- [ ] **Step 3: Package evidence**

Add package status fields only when the selected sample proves the cloud layer path through a render/readback or strict smoke lane.

**2026-06-04 PR12 Task 11 Evidence:** GREEN adds `MK_environment` cloud-layer value validation through `EnvironmentCloudLayerDesc`, `EnvironmentCloudLayerMode`, `EnvironmentCloudIblContributionMode`, `EnvironmentCloudLayerValidationResult`, `validate_environment_cloud_layer`, `is_valid_environment_cloud_layer`, and `has_environment_cloud_layer_diagnostic` for cheap 2D equirectangular sky clouds. `MK_renderer` adds `CloudLayerPolicyDesc`, `CloudLayerPolicyPlan`, texture/visual/IBL/shader-contract/quality rows, stable cloud-map binding `6`, flow-map binding `7`, sampler binding `6`, constants binding `6`, and fail-closed diagnostics for invalid environment rows, unsupported quality tiers, missing shader/package/execution evidence, unsupported texture upload, backend execution, native handles, and volumetric cloud claims. `shaders/environment/cloud_layer.hlsl` and `tests/shaders/environment_cloud_layer.hlsl` add a reviewed LatLong cloud-map + flow-map HLSL contract for the low-cost path. Official context was re-checked for Unity HDRP Cloud Layer as a 2D LatLong texture animated with a flowmap (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4016.0/manual/create-simple-clouds-cloud-layer`), Unreal Volumetric Cloud as a separate ray-marched 3D volume material path with quality/performance implications (`https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine?lang=en-US`), and Context7 `/godotengine/godot-docs` ProceduralSkyMaterial `sky_cover` guidance for sky-layer overlays. Focused validation passed for `MK_environment_cloud_tests`, `MK_renderer_cloud_layer_policy_tests`, `tools/check-tidy.ps1 -Files engine/environment/src/cloud_layer.cpp,engine/renderer/src/cloud_layer_policy.cpp,tests/unit/environment_cloud_tests.cpp,tests/unit/renderer_cloud_layer_policy_tests.cpp`, `tools/check-format.ps1`, DXC DXIL vertex/pixel compilation, DXC Vulkan SPIR-V vertex/pixel compilation, and `spirv-val`. This is cloud-layer value planning and shader-contract evidence only; it does not upload textures, execute renderer/RHI backends, add D3D12 readback/package evidence, prove Vulkan runtime execution, prove Metal readiness, claim volumetric clouds, claim rain readiness, claim backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 12: Weather And Precipitation

**Files:**
- Create: `engine/environment/include/mirakana/environment/weather.hpp`
- Create: `engine/environment/src/weather.cpp`
- Create: `engine/renderer/include/mirakana/renderer/precipitation_policy.hpp`
- Create: `engine/renderer/src/precipitation_policy.cpp`
- Create: `shaders/environment/precipitation.hlsl`
- Create: `tests/shaders/environment_precipitation.hlsl`
- Create: `tests/unit/environment_weather_tests.cpp`
- Create: `tests/unit/renderer_precipitation_policy_tests.cpp`

- [x] **Step 1: Add RED weather contract tests**

Require deterministic plans for `clear`, `cloudy`, `rain`, `storm`, `snow`, `foggy`, `dust`, and `ash`. Reject invalid intensity, non-finite wind, unsupported precipitation kind, missing occlusion policy when rain reaches scene geometry, and backend/native handle claims.

- [x] **Step 2: Add precipitation planning**

Add value rows for rain, snow, sleet, hail, ash, and dust:

```cpp
struct EnvironmentPrecipitationParticleRow;
struct EnvironmentSurfaceWetnessRow;
struct EnvironmentPrecipitationOcclusionRow;
struct EnvironmentPrecipitationPlan;

[[nodiscard]] EnvironmentPrecipitationPlan
plan_environment_precipitation(const EnvironmentPrecipitationPlanDesc& desc);
```

- [x] **Step 3: Add rain occlusion and wetness policy**

Plan camera-near precipitation, surface wetness, splash/ripple intent, and roof/indoor occlusion as explicit rows. Do not mutate materials or scene geometry in this phase.

- [x] **Step 4: Add audio handoff rows**

Add value-only rows for rain loop, indoor muffling, thunder delay, and storm intensity handoff to `MK_audio`. Do not play audio in `MK_environment`.

- [ ] **Step 5: Evidence**

Promote `environment_precipitation_status=ready` only after package-visible counters prove selected rain or snow rows and zero native handle leakage.

**2026-06-04 PR13 Task 12 Evidence:** GREEN adds `MK_environment` weather/precipitation value planning through `EnvironmentPrecipitationPlanDesc`, `EnvironmentWeatherRow`, `EnvironmentPrecipitationParticleRow`, `EnvironmentSurfaceWetnessRow`, `EnvironmentPrecipitationOcclusionRow`, `EnvironmentPrecipitationAudioHandoffRow`, `EnvironmentPrecipitationPlan`, `plan_environment_precipitation`, `has_environment_precipitation_diagnostic`, and `has_environment_precipitation_audio_cue`. The plan emits deterministic rows for `clear`, `cloudy`, `rain`, `storm`, `snow`, `foggy`, `dust`, and `ash`, including rain/storm wetness rows, camera-near particle intent rows, scene-geometry occlusion rows, and value-only audio handoff rows for rain loops, indoor muffling, thunder delay, storm intensity, snow, dust, and ash without invoking audio playback. `MK_renderer` adds `PrecipitationPolicyDesc`, `PrecipitationPolicyPlan`, shader/wetness/audio/quality rows, stable particle texture binding `8`, scene-depth texture binding `9`, sampler binding `8`, constants binding `7`, and fail-closed diagnostics for invalid environment plans, unsupported quality tiers, missing shader/package/execution evidence, particle-buffer upload, backend execution, native handles, material mutation, and audio playback. `shaders/environment/precipitation.hlsl` and `tests/shaders/environment_precipitation.hlsl` add a reviewed HLSL vertex/pixel shader contract for camera-near precipitation sprites and scene-depth occlusion. Official context was re-checked for Unity Visual Effect Graph particle contexts as spawn/initialize/update/output stages (`https://docs.unity3d.com/Packages/com.unity.visualeffectgraph%4017.0/manual/Contexts.html`), Unreal Niagara systems/emitters/modules/parameters and render groups (`https://dev.epicgames.com/documentation/en-us/unreal-engine/overview-of-niagara-effects-for-unreal-engine`), Unity HDRP Decal Projector bounds/material projection behavior for wet-surface intent (`https://docs.unity3d.com/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/decal-projector-reference.html`), and Context7 plus Godot official docs for `GPUParticles3D` / `ParticleProcessMaterial` particle material, shader, collision, and physics properties (`https://docs.godotengine.org/en/stable/classes/class_gpuparticles3d.html`, `https://docs.godotengine.org/en/stable/classes/class_particleprocessmaterial.html`). Focused validation passed for `MK_environment_weather_tests`, `MK_renderer_precipitation_policy_tests`, `tools/check-tidy.ps1 -Files engine/environment/src/weather.cpp,engine/renderer/src/precipitation_policy.cpp,tests/unit/environment_weather_tests.cpp,tests/unit/renderer_precipitation_policy_tests.cpp`, `tools/check-format.ps1`, `tools/check-shader-toolchain.ps1` diagnostic readiness, DXC DXIL vertex/pixel compilation, DXC Vulkan SPIR-V vertex/pixel compilation, and `spirv-val`. This is weather/precipitation value planning and shader-contract evidence only; it does not upload particle buffers, mutate materials, play audio, execute renderer/RHI/audio backends, add D3D12 readback/package evidence, prove Vulkan runtime execution, prove Metal readiness, claim precipitation ready counters, claim backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 13: Volumetric Clouds And Storm Lighting

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/volumetric_cloud_policy.hpp`
- Create: `engine/renderer/src/volumetric_cloud_policy.cpp`
- Create: `shaders/environment/volumetric_clouds.hlsl`
- Create: `tests/shaders/environment_volumetric_clouds.hlsl`
- Create: `tests/unit/renderer_volumetric_cloud_tests.cpp`

- [x] **Step 1: Re-check official cloud sources**

Re-check Unreal Volumetric Clouds and Unity HDRP clouds before selecting property names and quality budgets.

- [x] **Step 2: Add RED cloud policy tests**

Require weather map, coverage, density, shape noise reference, erosion noise reference, altitude range, wind, lighting source count, raymarch primary steps, raymarch light steps, temporal reprojection, and cloud shadow diagnostics.

- [x] **Step 3: Implement policy rows**

Add policy rows that support at most two atmospheric directional lights, matching the sun/moon structure. Reject extra atmospheric lights deterministically.

- [x] **Step 4: Add storm rows**

Plan lightning flash intensity, lightning direction, thunder delay, cloud darkening, precipitation boost, wind gusts, and exposure response as value rows. Renderer and audio execution remain separate adapter work within this task's evidence boundary.

**2026-06-04 PR14 Task 13 Evidence:** GREEN adds backend-neutral `VolumetricCloudPolicyDesc`, `VolumetricCloudPolicyPlan`, weather-map/shape-noise/erosion-noise rows, layer rows, lighting rows, raymarch rows, temporal rows, shadow rows, shader-contract rows, quality rows, storm rows, `plan_volumetric_cloud_policy`, `has_volumetric_cloud_diagnostic`, stable weather-map binding `10`, shape-noise binding `11`, erosion-noise binding `12`, sampler binding `10`, and constants binding `8`. The plan supports at most two atmospheric directional light rows, fails closed for invalid first-party asset references, coverage/density/altitude/wind/raymarch/temporal/shadow/storm values, missing shader-contract evidence, missing execution evidence for ready promotion, unsupported volume-texture upload, backend execution, native handles, audio playback, and precipitation execution. `shaders/environment/volumetric_clouds.hlsl` and `tests/shaders/environment_volumetric_clouds.hlsl` add a reviewed HLSL vertex/pixel shader contract for weather-map, shape-noise, erosion-noise, altitude shaping, bounded raymarch, and storm-lighting response. Official context was re-checked for Unreal Volumetric Clouds (`https://dev.epicgames.com/documentation/unreal-engine/volumetric-cloud-component-in-unreal-engine?lang=en-US`), Unity HDRP Volumetric Clouds (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/Override-Volumetric-Clouds.html`), Godot volumetric fog/cloud-adjacent volume controls through Context7, and Microsoft HLSL semantics/resource binding. Focused validation passed for `MK_renderer_volumetric_cloud_tests`, `tools/check-tidy.ps1 -Files engine/renderer/src/volumetric_cloud_policy.cpp,tests/unit/renderer_volumetric_cloud_tests.cpp`, `tools/check-format.ps1`, `tools/check-shader-toolchain.ps1` diagnostic readiness, DXC DXIL vertex/pixel compilation, DXC Vulkan SPIR-V vertex/pixel compilation, and `spirv-val`. This is volumetric-cloud value planning and shader-contract evidence only; it does not upload volume textures, execute renderer/RHI/audio/precipitation backends, add D3D12 readback/package evidence, prove Vulkan runtime execution, prove Metal readiness, claim volumetric-cloud ready counters, claim backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 14: Time Of Day And Weather Blending

**Files:**
- Create: `engine/environment/include/mirakana/environment/time_of_day.hpp`
- Create: `engine/environment/src/time_of_day.cpp`
- Create: `tests/unit/environment_time_of_day_tests.cpp`

- [x] **Step 1: Add RED time-of-day tests**

Require deterministic sun direction, moon direction, normalized day time, profile blend weights, exposure intent, weather transition rows, and stable replay hash.

- [x] **Step 2: Implement value planning**

Add:

```cpp
struct EnvironmentTimeOfDayPlanDesc;
struct EnvironmentTimeOfDayPlan;

[[nodiscard]] EnvironmentTimeOfDayPlan
plan_environment_time_of_day(const EnvironmentTimeOfDayPlanDesc& desc);
```

- [x] **Step 3: Keep gameplay ownership clear**

The planner returns environment values. It does not tick game state, mutate scenes, or own a scheduler.

**2026-06-04 PR15 Task 14 Evidence:** GREEN adds backend-neutral `EnvironmentTimeOfDayPlanDesc`, `EnvironmentWeatherBlendLayerDesc`, `EnvironmentWeatherTransitionDesc`, `EnvironmentExposureIntentDesc`, `EnvironmentTimeOfDayPlan`, celestial sun/moon rows, profile blend rows, weather transition rows, exposure intent rows, deterministic `replay_hash`, `plan_environment_time_of_day`, and `has_environment_time_of_day_diagnostic`. The planner maps normalized day time to deterministic sun/moon atmospheric light directions, keeps sun/moon as value rows with atmosphere light indices `0/1`, normalizes profile blend weights, computes linear or smoothstep weather transition weights, records fixed/auto exposure EV100 intent, and fails closed for invalid day time, moon phase, profile rows, blend weights, transition duration/elapsed values, unsupported blend modes, invalid exposure limits/adaptation, backend execution, native-handle claims, and profile mutation. Official context was re-checked for Unreal Sky Atmosphere time-of-day and two atmospheric directional lights (`https://dev.epicgames.com/documentation/en-us/unreal-engine/sky-atmosphere?application_version=4.27`), Unreal Directional Light atmosphere/cloud sun/moon indices (`https://dev.epicgames.com/documentation/unreal-engine/directional-lights-in-unreal-engine?lang=en-US`), Unity HDRP Volume/Profile blending (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/understand-volumes.html`), Unity HDRP Physically Based Sky (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/create-a-physically-based-sky.html`), Unity HDRP Exposure EV100/adaptation (`https://docs.unity.cn/Packages/com.unity.render-pipelines.high-definition%4017.0/manual/reference-override-exposure.html`), and Context7 `/godotengine/godot-docs` Environment sky / ambient light / sky bake guidance. Focused validation passed for `MK_environment_time_of_day_tests`, `tools/check-tidy.ps1 -Files engine/environment/src/time_of_day.cpp,tests/unit/environment_time_of_day_tests.cpp`, and `tools/check-format.ps1`. This is time-of-day and weather-blending value planning only; it does not tick game state, mutate scenes or profiles, execute renderer/RHI/audio backends, add package-visible time/weather counters, prove D3D12/Vulkan/Metal runtime execution, claim weather-blending readiness, claim backend parity, claim broad optimization, or claim broad `environment_ready`.

## Task 15: Editor Authoring Surface

**Files:**
- Modify: `editor/core/include/mirakana/editor_core/environment_authoring.hpp`
- Create: `editor/core/src/environment_authoring.cpp`
- Modify: `editor/src/editor_app.cpp`
- Create: `tests/unit/editor_environment_tests.cpp`

- [ ] **Step 1: Add editor-core model tests**

Require deterministic inspector rows for sky, sun, moon, atmosphere, fog, cloud layer, volumetric clouds, precipitation, weather presets, and quality tier.

- [ ] **Step 2: Add authoring document**

Add an editor-core document over `GameEngine.EnvironmentProfile.v1` text IO using `ITextStore`, undo/redo, dirty tracking, validation diagnostics, and package registration review rows.

- [ ] **Step 3: Add visible editor panel**

Add a visible first-party retained `MK_editor` panel only after editor-core model tests pass. The editor panel must use `mirakana::ui` / `MK_ui_renderer` contracts and must not expose native renderer/RHI handles, reintroduce Dear ImGui or other UI middleware, or execute package scripts.

## Task 16: Package And Validation Evidence

**Files:**
- Modify: `tools/package-desktop-runtime.ps1`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/run-validation-recipe.ps1`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`

- [ ] **Step 1: Add smoke flags only for implemented phases**

Add flags such as:

```text
--require-environment-profile
--require-environment-physical-sky
--require-environment-fog
--require-environment-cloud-layer
--require-environment-precipitation
--require-environment-volumetric-clouds
```

Do not add a flag before its corresponding implementation and tests exist.

- [ ] **Step 2: Validate installed output**

Installed validation must reject missing status rows, mismatched selected backend evidence, inferred Vulkan readiness, inferred Metal readiness, and broad environment-ready claims without narrow counters.

- [ ] **Step 3: Compose manifest**

When agent-surface rows change, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: composed manifest and AI integration checks pass.

## Task 17: Final Documentation Closeout

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/rhi.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-26-environment-system-v1.md`

- [ ] **Step 1: Update current-truth docs**

Document only the phases that have actually landed. Keep host-gated and planned environment features separate from ready features.

- [ ] **Step 2: Register active plan state**

If this plan becomes active, update the plan registry and manifest fragments in the same change. If it remains proposed, do not make it the `currentActivePlan`.

- [ ] **Step 3: Run final validation**

For C++/runtime/rendering/package changes, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS, or a concrete missing-host/toolchain blocker is recorded.

## Validation Matrix

| Slice | Required evidence before ready claim |
| --- | --- |
| Environment profile | Unit tests, text IO round trip, package row validation. |
| Scene binding | Scene v2 tests, runtime scene tests, no `LightComponent` pollution. |
| Render packet | `MK_scene_renderer` tests, deterministic rows, no GPU execution. |
| Physical sky | Policy tests, shader contract validation, selected D3D12 readback proof, Vulkan/package proof before runtime ready. |
| IBL | Cubemap policy tests, HDR dependency/legal checks if EXR/KTX is selected. |
| Height fog | Depth-aware readback/package evidence, not only enum support. |
| Volumetric fog | Froxel policy tests, D3D12 readback or package evidence. |
| Cloud layer | Cheap cloud layer tests, package evidence for selected sample. |
| Rain/snow/storm | Precipitation, wetness, occlusion, and audio handoff value tests. |
| Volumetric clouds | Raymarch quality budget tests, D3D12 proof, Vulkan strict gate. |
| Editor | Editor-core retained model tests before visible first-party `MK_editor` panel claims. |
| Package | Installed validation counters for each selected feature. |

## Recommended PR Slices

1. PR 1: `MK_environment` value contract, validation, and tests.
2. PR 2: Environment text IO, asset kind, and package rows.
3. PR 3: Scene/runtime scene environment binding.
4. PR 4: Environment render packet and renderer policy planning.
5. PR 5: Physical sky policy and shader contract.
6. PR 6: Sky lighting and IBL policy.
7. PR 7: Height fog policy and shader contract.
8. PR 8: Height fog D3D12 execution/readback proof and postprocess constant binding.
9. PR 9: Physical sky D3D12 execution/readback proof.
10. PR 10: Height-fog D3D12 package-visible evidence.
11. PR 11: Height-fog Vulkan host-gated proof or volumetric fog policy, split further if review or host gates require it.
12. PR 12: Cloud layer.
13. PR 13: Weather and precipitation, including rain, snow, wetness, occlusion, and audio handoff rows.
14. PR 14: Volumetric clouds and storm lighting.
15. PR 15: Time-of-day and weather blending.
16. PR 16: Editor environment authoring.
17. PR 17: Package validation recipes, manifest, docs, and final closeout.

## Completion Definition

The Environment System v1 plan is complete only when:

- environment profiles are first-class package assets;
- scene and runtime packages can reference environment profiles;
- physical sky, environment lighting, fog, clouds, rain/snow/storm, and time-of-day each have narrow ready counters only after evidence exists;
- D3D12 has primary host/package proof for selected runtime samples;
- Vulkan has strict host/toolchain/runtime gated proof for selected features;
- Metal is either Apple-host proven or explicitly host-gated;
- editor authoring exists through reviewed editor-core models before visible UI claims;
- docs, plan registry, manifest fragments, composed manifest, static checks, and validation recipes match the implemented evidence;
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records an exact local host/toolchain blocker.
