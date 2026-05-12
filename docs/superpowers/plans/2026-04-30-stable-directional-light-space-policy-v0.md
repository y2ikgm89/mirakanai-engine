# Stable Directional Light-Space Policy v0 Implementation Plan (2026-04-30)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [x]`) syntax for tracking.

**Goal:** Add a backend-neutral, deterministic directional shadow light-space policy that produces a texel-snapped orthographic light view/projection plan for the existing shadow-map foundation.

**Architecture:** Keep the policy in `MK_renderer` as first-party value data and diagnostics. Adapt `SceneRenderPacket` to the policy in `MK_scene_renderer` by deriving a conservative mesh-center bounds estimate and the selected directional light direction, without adding RHI/backend/editor dependencies.

**Tech Stack:** C++23, existing `MK_math` `Vec3`/`Mat4`, `MK_renderer`, `MK_scene_renderer`, CTest through the existing CMake targets.

---

## Context

Current shadow support has `ShadowMapDesc`, `ShadowMapPlan`, `ShadowReceiverDesc`, `ShadowReceiverPlan`, fixed sampled-depth 3x3 PCF metadata, RHI readback proof, and package-visible sample smoke. It explicitly does not claim cascades, atlases, hardware comparison samplers, stable light-space policy, Metal presentation, editor authoring, or production shadow quality.

This slice only adds the stable light-space planning policy. It does not change native D3D12/Vulkan/Metal code, shader bytecode, package smoke requirements, or public game APIs outside first-party `mirakana::` renderer/scene-renderer value types.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/shadow_map.hpp`
- Modify: `engine/renderer/src/shadow_map.cpp`
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/scene_renderer_tests.cpp`
- Modify: `docs/superpowers/plans/README.md`
- Later sync after implementation: `docs/roadmap.md`, `docs/architecture.md`, `docs/specs/2026-04-27-engine-essential-gap-analysis.md`, `engine/agent/manifest.json`, `.agents/skills/rendering-change/SKILL.md`, `.codex/agents/rendering-auditor.toml`, `.claude/skills/rendering-change/SKILL.md` if present, `.claude/agents/rendering-auditor.md` or `.toml` if present, and any static checks that mention shadow non-goals.

## Task 1: Renderer Policy Tests

**Files:**
- Modify: `tests/unit/renderer_rhi_tests.cpp`

- [x] **Step 1: Write failing tests for stable light-space planning**

Add tests before the existing postprocess renderer tests:

```cpp
MK_TEST("stable directional shadow light-space policy builds a texel-snapped orthographic plan") {
    const auto shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        mirakana::ShadowMapLightDesc{mirakana::ShadowMapLightType::directional, true, 0},
        mirakana::rhi::Extent2D{1024, 1024},
        mirakana::rhi::Format::depth24_stencil8,
        3,
        3,
    });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto plan = mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
        .shadow_map = &shadow_plan,
        .light_direction = mirakana::Vec3{0.0F, -1.0F, 0.0F},
        .focus_center = mirakana::Vec3{1.13F, 2.0F, -3.27F},
        .focus_radius = 8.0F,
        .depth_radius = 16.0F,
        .texel_snap = mirakana::ShadowLightSpaceTexelSnap::enabled,
    });

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.light_direction == (mirakana::Vec3{0.0F, -1.0F, 0.0F}));
    MK_REQUIRE(plan.focus_radius == 8.0F);
    MK_REQUIRE(plan.depth_radius == 16.0F);
    MK_REQUIRE(plan.ortho_width == 16.0F);
    MK_REQUIRE(plan.ortho_height == 16.0F);
    MK_REQUIRE(plan.texel_world_size == 16.0F / 1024.0F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.x - 1.125F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.y - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.snapped_focus_center.z + 3.265625F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.view_from_world.at(0, 3) + 3.265625F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.view_from_world.at(1, 3) - 1.125F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.view_from_world.at(2, 3) - 2.0F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.clip_from_world.at(0, 0) + 0.125F) < 0.0001F);
    MK_REQUIRE(std::abs(plan.clip_from_world.at(1, 2) + 0.125F) < 0.0001F);
}

MK_TEST("stable directional shadow light-space policy reports deterministic diagnostics") {
    const auto shadow_plan = mirakana::build_shadow_map_plan(mirakana::ShadowMapDesc{
        mirakana::ShadowMapLightDesc{mirakana::ShadowMapLightType::directional, true, 0},
        mirakana::rhi::Extent2D{512, 512},
        mirakana::rhi::Format::depth24_stencil8,
        1,
        1,
    });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto null_shadow = mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{});
    MK_REQUIRE(!null_shadow.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        null_shadow, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_shadow_map_plan));

    const auto invalid_direction =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{0.0F, 0.0F, 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_direction.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_direction, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_light_direction));

    const auto invalid_center =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{0.0F, -1.0F, 0.0F},
            .focus_center = mirakana::Vec3{std::numeric_limits<float>::infinity(), 0.0F, 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_center.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_center, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_center));

    const auto invalid_radius =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{0.0F, -1.0F, 0.0F},
            .focus_radius = 0.0F,
            .depth_radius = 1.0F,
        });
    MK_REQUIRE(!invalid_radius.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_radius, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_radius));

    const auto invalid_depth =
        mirakana::build_directional_shadow_light_space_plan(mirakana::DirectionalShadowLightSpaceDesc{
            .shadow_map = &shadow_plan,
            .light_direction = mirakana::Vec3{0.0F, -1.0F, 0.0F},
            .focus_radius = 1.0F,
            .depth_radius = -1.0F,
        });
    MK_REQUIRE(!invalid_depth.succeeded());
    MK_REQUIRE(mirakana::has_directional_shadow_light_space_diagnostic(
        invalid_depth, mirakana::DirectionalShadowLightSpaceDiagnosticCode::invalid_depth_radius));
}
```

- [x] **Step 2: Run test to verify it fails**

Run: `cmake --build --preset dev --target MK_renderer_tests`

Expected: compile failure because `DirectionalShadowLightSpaceDesc`, `ShadowLightSpaceTexelSnap`, `build_directional_shadow_light_space_plan`, and the diagnostic helper do not exist.

## Task 2: Renderer Policy Implementation

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/shadow_map.hpp`
- Modify: `engine/renderer/src/shadow_map.cpp`

- [x] **Step 1: Add the public value API**

Add to `shadow_map.hpp`:

```cpp
enum class DirectionalShadowLightSpaceDiagnosticCode {
    none = 0,
    invalid_shadow_map_plan,
    invalid_light_direction,
    invalid_focus_center,
    invalid_focus_radius,
    invalid_depth_radius,
};

enum class ShadowLightSpaceTexelSnap { disabled = 0, enabled };

struct DirectionalShadowLightSpaceDesc {
    const ShadowMapPlan* shadow_map{nullptr};
    Vec3 light_direction{0.0F, -1.0F, 0.0F};
    Vec3 focus_center;
    float focus_radius{0.0F};
    float depth_radius{0.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
};

struct DirectionalShadowLightSpaceDiagnostic {
    DirectionalShadowLightSpaceDiagnosticCode code{DirectionalShadowLightSpaceDiagnosticCode::none};
    std::string message;
};

struct DirectionalShadowLightSpacePlan {
    Vec3 light_direction{0.0F, -1.0F, 0.0F};
    Vec3 right{1.0F, 0.0F, 0.0F};
    Vec3 up{0.0F, 0.0F, 1.0F};
    Vec3 focus_center;
    Vec3 snapped_focus_center;
    float focus_radius{0.0F};
    float depth_radius{0.0F};
    float ortho_width{0.0F};
    float ortho_height{0.0F};
    float near_z{0.0F};
    float far_z{0.0F};
    float texel_world_size{0.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
    Mat4 view_from_world;
    Mat4 clip_from_view;
    Mat4 clip_from_world;
    std::vector<DirectionalShadowLightSpaceDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] DirectionalShadowLightSpacePlan
build_directional_shadow_light_space_plan(const DirectionalShadowLightSpaceDesc& desc);
[[nodiscard]] bool has_directional_shadow_light_space_diagnostic(
    const DirectionalShadowLightSpacePlan& plan, DirectionalShadowLightSpaceDiagnosticCode code) noexcept;
```

- [x] **Step 2: Implement validation and deterministic matrix planning**

In `shadow_map.cpp`, add helpers for finite `Vec3`, normalization, deterministic light basis, texel snapping, view matrix, and orthographic projection. Reuse `is_valid_shadow_map_plan_for_receiver` for the shadow-map validation so policy inputs are consistent with receiver planning.

The policy must:

- reject null or unsuccessful shadow plans
- reject zero/non-finite light directions
- reject non-finite centers
- reject non-finite or non-positive focus/depth radii
- normalize `light_direction`
- choose world `Y` as the basis reference unless nearly parallel, then choose world `X`
- compute `right = normalize(cross(reference, light_direction))`
- compute `up = cross(light_direction, right)`
- compute `texel_world_size = (focus_radius * 2) / shadow_map.depth_texture.extent.width`
- snap focus center in light-space x/y to nearest texel when enabled
- emit `view_from_world`, `clip_from_view`, and `clip_from_world`

- [x] **Step 3: Run renderer tests to verify pass**

Run: `cmake --build --preset dev --target MK_renderer_tests; ctest --preset dev -R MK_renderer_tests --output-on-failure`

Expected: `MK_renderer_tests` passes.

## Task 3: Scene Adapter Tests

**Files:**
- Modify: `tests/unit/scene_renderer_tests.cpp`

- [x] **Step 1: Write failing scene adapter tests**

Add after the existing scene shadow-map plan tests:

```cpp
MK_TEST("scene renderer builds a stable directional shadow light-space policy from mesh bounds") {
    mirakana::Scene scene("shadow-light-space");
    const auto light_node = scene.create_node("Sun");
    const auto left_mesh = scene.create_node("LeftCaster");
    const auto right_mesh = scene.create_node("RightCaster");

    mirakana::SceneNodeComponents light_components;
    light_components.light = mirakana::LightComponent{
        mirakana::LightType::directional, mirakana::Vec3{1.0F, 1.0F, 1.0F}, 2.0F, 100.0F, 0.0F, 0.0F, true,
    };
    scene.set_components(light_node, light_components);

    scene.find_node(left_mesh)->transform.position = mirakana::Vec3{-3.0F, 0.0F, 1.0F};
    scene.find_node(right_mesh)->transform.position = mirakana::Vec3{5.0F, 0.0F, -1.0F};

    mirakana::SceneNodeComponents mesh_components;
    mesh_components.mesh_renderer = mirakana::MeshRendererComponent{
        mirakana::AssetId::from_name("meshes/caster"),
        mirakana::AssetId::from_name("materials/caster"),
        true,
    };
    scene.set_components(left_mesh, mesh_components);
    scene.set_components(right_mesh, mesh_components);

    const auto packet = mirakana::build_scene_render_packet(scene);
    const auto shadow_plan = mirakana::build_scene_shadow_map_plan(packet, mirakana::SceneShadowMapDesc{
                                                                         mirakana::rhi::Extent2D{512, 512},
                                                                         mirakana::rhi::Format::depth24_stencil8,
                                                                     });
    MK_REQUIRE(shadow_plan.succeeded());

    const auto light_space =
        mirakana::build_scene_directional_shadow_light_space_plan(packet, shadow_plan, mirakana::SceneShadowLightSpaceDesc{});

    MK_REQUIRE(light_space.succeeded());
    MK_REQUIRE(light_space.focus_center == (mirakana::Vec3{1.0F, 0.0F, 0.0F}));
    MK_REQUIRE(light_space.focus_radius >= 5.0F);
    MK_REQUIRE(light_space.depth_radius >= light_space.focus_radius);
    MK_REQUIRE(light_space.texel_world_size > 0.0F);
}
```

- [x] **Step 2: Run scene-renderer test to verify it fails**

Run: `cmake --build --preset dev --target MK_scene_renderer_tests`

Expected: compile failure because `SceneShadowLightSpaceDesc` and `build_scene_directional_shadow_light_space_plan` do not exist.

## Task 4: Scene Adapter Implementation

**Files:**
- Modify: `engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp`
- Modify: `engine/scene_renderer/src/scene_renderer.cpp`

- [x] **Step 1: Add scene adapter API**

Add to `scene_renderer.hpp`:

```cpp
struct SceneShadowLightSpaceDesc {
    float minimum_focus_radius{1.0F};
    float depth_padding{1.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
};

[[nodiscard]] DirectionalShadowLightSpacePlan
build_scene_directional_shadow_light_space_plan(const SceneRenderPacket& packet, const ShadowMapPlan& shadow_map,
                                                SceneShadowLightSpaceDesc desc);
```

- [x] **Step 2: Implement scene mesh bounds derivation**

In `scene_renderer.cpp`:

- use the same selected shadow light index from `ShadowMapPlan::selected_light_index`
- derive light direction from the selected `SceneRenderLight`
- compute mesh centers from each visible mesh `world_from_node` translation
- compute center as min/max midpoint
- compute focus radius from the farthest mesh center distance to center plus a conservative unit extent
- use `minimum_focus_radius` and `depth_padding` to keep dimensions non-zero
- forward to `build_directional_shadow_light_space_plan`

- [x] **Step 3: Run scene-renderer tests**

Run: `cmake --build --preset dev --target MK_scene_renderer_tests; ctest --preset dev -R MK_scene_renderer_tests --output-on-failure`

Expected: `MK_scene_renderer_tests` passes.

## Task 5: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/rendering-change/SKILL.md`
- Modify: `.codex/agents/rendering-auditor.toml`
- Modify if present: `.claude/skills/rendering-change/SKILL.md`
- Modify if present: `.claude/agents/rendering-auditor.toml`

- [x] **Step 1: Update docs and guidance**

Record that Stable Directional Light-Space Policy v0 is implemented as a backend-neutral planning policy with deterministic texel-snapped light-space matrices, scene-packet adapter, and tests. Keep non-goals explicit: no cascades, no atlases, no hardware comparison samplers, no Metal shadow presentation, no editor authoring, no production shadow quality claim.

- [x] **Step 2: Update manifest**

Update `MK_renderer`, `MK_scene_renderer`, rendering readiness, and next-target text so generated-game guidance can distinguish stable light-space policy from cascades/atlases and backend-native shadow quality.

- [x] **Step 3: Update plan registry**

Move this plan from active to completed with focused validation evidence after verification.

## Task 6: Validation

**Files:** None unless failures require fixes.

- [x] **Step 1: Run focused renderer validation**

Run:

```powershell
cmake --build --preset dev --target MK_renderer_tests MK_scene_renderer_tests
ctest --preset dev -R "MK_renderer_tests|MK_scene_renderer_tests" --output-on-failure
```

Expected: both focused test targets pass.

- [x] **Step 2: Run required boundary/toolchain/static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
```

Expected: checks pass or report only known host-gated diagnostics.

- [x] **Step 3: Run default validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: default validation passes with known Metal/Apple/signing/device/tidy host gates only.

## Validation Evidence

- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`
- PASS: `ctest --preset dev -R "MK_renderer_tests|MK_scene_renderer_tests" --output-on-failure`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1` with known diagnostic-only `metal` / `metallib` missing blockers
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` with 22/22 tests passed and known Metal/Apple/signing/device/tidy host gates

## Done When

- [x] `MK_renderer` exposes `DirectionalShadowLightSpaceDesc` / `DirectionalShadowLightSpacePlan` and deterministic texel-snapped directional light-space matrix planning.
- [x] `MK_scene_renderer` adapts `SceneRenderPacket` plus `ShadowMapPlan` into the stable light-space policy without making `MK_scene` depend on renderer/RHI/backend handles.
- [x] Renderer and scene-renderer tests prove valid planning, snapping, invalid diagnostics, and scene mesh-bounds derivation.
- [x] Docs, roadmap, gap analysis, manifest, Codex/Claude guidance, and static checks are synchronized.
- [x] Focused validation, API boundary, shader toolchain diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have fresh evidence.
