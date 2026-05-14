// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/shadow_map.hpp"
#include "mirakana/renderer/sprite_batch.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"
#include "mirakana/scene/render_packet.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct SceneMaterialColor {
    AssetId material;
    Color color;
};

struct SceneMeshGpuBinding {
    AssetId mesh;
    MeshGpuBinding binding;
};

struct SceneMaterialGpuBinding {
    AssetId material;
    MaterialGpuBinding binding;
};

struct SceneMorphMeshGpuBinding {
    AssetId morph_mesh;
    MorphMeshGpuBinding binding;
};

class SceneMaterialPalette {
  public:
    bool try_add(const MaterialDefinition& material);
    bool try_add_instance(const MaterialDefinition& parent, const MaterialInstanceDefinition& instance);
    void add(const MaterialDefinition& material);
    void add_instance(const MaterialDefinition& parent, const MaterialInstanceDefinition& instance);

    [[nodiscard]] const Color* find(AssetId material) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;

  private:
    std::vector<SceneMaterialColor> colors_;
};

class SceneGpuBindingPalette {
  public:
    bool try_add_mesh(AssetId mesh, MeshGpuBinding binding);
    bool try_add_material(AssetId material, MaterialGpuBinding binding);
    void add_mesh(AssetId mesh, MeshGpuBinding binding);
    void add_material(AssetId material, MaterialGpuBinding binding);

    [[nodiscard]] const MeshGpuBinding* find_mesh(AssetId mesh) const noexcept;
    [[nodiscard]] const MaterialGpuBinding* find_material(AssetId material) const noexcept;
    [[nodiscard]] std::size_t mesh_count() const noexcept;
    [[nodiscard]] std::size_t material_count() const noexcept;

  private:
    std::vector<SceneMeshGpuBinding> meshes_;
    std::vector<SceneMaterialGpuBinding> materials_;
};

struct SceneSkinnedMeshGpuBinding {
    AssetId mesh{};
    SkinnedMeshGpuBinding binding;
};

/// Retains skinned mesh GPU resources separately from `SceneGpuBindingPalette` so static mesh bindings are never
/// overloaded with skinning metadata.
class SceneSkinnedGpuBindingPalette {
  public:
    bool try_add_skinned_mesh(AssetId mesh, SkinnedMeshGpuBinding binding);
    void add_skinned_mesh(AssetId mesh, SkinnedMeshGpuBinding binding);

    [[nodiscard]] const SkinnedMeshGpuBinding* find_skinned_mesh(AssetId mesh) const noexcept;
    [[nodiscard]] std::size_t skinned_mesh_count() const noexcept;
    [[nodiscard]] std::span<const SceneSkinnedMeshGpuBinding> skinned_entries() const noexcept;

  private:
    std::vector<SceneSkinnedMeshGpuBinding> skinned_meshes_;
};

/// Retains morph mesh GPU resources separately from static/skinned mesh bindings so callers can explicitly pair a
/// base mesh draw with a selected morph payload.
class SceneMorphGpuBindingPalette {
  public:
    bool try_add_morph_mesh(AssetId morph_mesh, MorphMeshGpuBinding binding);
    void add_morph_mesh(AssetId morph_mesh, MorphMeshGpuBinding binding);

    [[nodiscard]] const MorphMeshGpuBinding* find_morph_mesh(AssetId morph_mesh) const noexcept;
    [[nodiscard]] std::size_t morph_mesh_count() const noexcept;
    [[nodiscard]] std::span<const SceneMorphMeshGpuBinding> morph_entries() const noexcept;

  private:
    std::vector<SceneMorphMeshGpuBinding> morph_meshes_;
};

struct SceneShadowMapDesc {
    /// Per-cascade tile resolution; atlas width is `extent.width * directional_cascade_count`.
    rhi::Extent2D extent;
    rhi::Format depth_format{rhi::Format::depth24_stencil8};
    std::uint32_t directional_cascade_count{1};
};

struct SceneShadowLightSpaceDesc {
    float minimum_focus_radius{1.0F};
    float depth_padding{1.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
    /// Aspect ratio used when fitting cascades to a perspective camera (`width / height`).
    float viewport_aspect{16.0F / 9.0F};
    /// Split blend weight for `compute_practical_shadow_cascade_distances` when using multi-cascade shadows.
    float cascade_split_lambda{0.75F};
};

struct SceneRenderSubmitResult {
    std::size_t meshes_submitted{0};
    std::size_t sprites_submitted{0};
    std::size_t cameras_available{0};
    std::size_t lights_available{0};
    std::size_t material_colors_resolved{0};
    std::size_t mesh_gpu_bindings_resolved{0};
    std::size_t material_gpu_bindings_resolved{0};
    std::size_t skinned_mesh_gpu_bindings_resolved{0};
    bool has_primary_camera{false};
};

enum class SceneMeshDrawPlanDiagnosticCode : std::uint8_t {
    invalid_mesh_asset,
    invalid_material_asset,
};

struct SceneMeshDrawPlanDiagnostic {
    SceneMeshDrawPlanDiagnosticCode code{SceneMeshDrawPlanDiagnosticCode::invalid_mesh_asset};
    std::size_t mesh_index{0};
    AssetId asset;
    std::string message;
};

struct SceneMeshDrawPlan {
    std::size_t mesh_count{0};
    std::size_t draw_count{0};
    std::size_t unique_mesh_count{0};
    std::size_t unique_material_count{0};
    std::vector<SceneMeshDrawPlanDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSceneRenderLoadFailure {
    AssetId asset;
    std::string diagnostic;
};

struct RuntimeSceneRenderLoadResult {
    std::optional<Scene> scene;
    SceneMaterialPalette material_palette;
    std::vector<RuntimeSceneRenderLoadFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSceneRenderInstance {
    std::optional<Scene> scene;
    SceneMaterialPalette material_palette;
    SceneRenderPacket render_packet;
    std::vector<RuntimeSceneRenderLoadFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSceneRenderAnimationApplyResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t sampled_track_count{0};
    std::size_t applied_sample_count{0};
    std::vector<runtime_scene::RuntimeSceneAnimationTransformBindingDiagnostic> binding_diagnostics;
};

struct RuntimeSceneRenderSpriteAnimationApplyResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t sampled_frame_count{0};
    std::size_t applied_frame_count{0};
    std::size_t selected_frame_index{0};
};

struct RuntimeMorphMeshCpuAnimationSampleResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t sampled_track_count{0};
    std::size_t applied_weight_count{0};
    std::vector<float> target_weights;
    std::vector<Vec3> morphed_positions;
};

struct SceneCameraMatrices {
    Mat4 view_from_world;
    Mat4 clip_from_view;
    Mat4 clip_from_world;
};

inline constexpr std::size_t scene_pbr_frame_uniform_packed_bytes = 256;

struct ScenePbrGpuSubmitContext {
    rhi::IRhiDevice* device{nullptr};
    rhi::BufferHandle scene_frame_uniform{};
    SceneCameraMatrices camera{};
    Vec3 camera_world_position{};
    std::array<float, 3> ambient_rgb{0.04F, 0.045F, 0.05F};
    float viewport_aspect{16.0F / 9.0F};
};

struct ScenePbrFrameGpuPackInput {
    const SceneRenderPacket* packet{nullptr};
    SceneCameraMatrices camera{};
    Mat4 world_from_node{Mat4::identity()};
    Vec3 camera_world_position{};
    std::array<float, 3> ambient_rgb{0.04F, 0.045F, 0.05F};
    float viewport_aspect{16.0F / 9.0F};
};

void pack_scene_pbr_frame_gpu(std::span<std::uint8_t> dst, const ScenePbrFrameGpuPackInput& input) noexcept;

struct SceneRenderSubmitDesc {
    Color fallback_mesh_color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    const SceneMaterialPalette* material_palette{nullptr};
    const SceneGpuBindingPalette* gpu_bindings{nullptr};
    const SceneSkinnedGpuBindingPalette* skinned_gpu_bindings{nullptr};
    const ScenePbrGpuSubmitContext* pbr_gpu{nullptr};
};

struct SceneLightCommand {
    LightType type{LightType::unknown};
    Vec3 position;
    Vec3 direction{.x = 0.0F, .y = 0.0F, .z = -1.0F};
    Color color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    float intensity{1.0F};
    float range{0.0F};
    float inner_cone_radians{0.0F};
    float outer_cone_radians{0.0F};
    bool casts_shadows{false};
};

[[nodiscard]] Color color_from_material(const MaterialDefinition& material) noexcept;
[[nodiscard]] Color resolve_scene_mesh_color(const SceneRenderMesh& mesh, const SceneRenderSubmitDesc& desc) noexcept;
[[nodiscard]] SceneCameraMatrices make_scene_camera_matrices(const SceneRenderCamera& camera, float aspect_ratio);
[[nodiscard]] SceneLightCommand make_scene_light_command(const SceneRenderLight& light) noexcept;
[[nodiscard]] ShadowMapPlan build_scene_shadow_map_plan(const SceneRenderPacket& packet, SceneShadowMapDesc desc);
[[nodiscard]] DirectionalShadowLightSpacePlan
build_scene_directional_shadow_light_space_plan(const SceneRenderPacket& packet, const ShadowMapPlan& shadow_map,
                                                SceneShadowLightSpaceDesc desc);
[[nodiscard]] MeshCommand make_scene_mesh_command(const SceneRenderMesh& mesh, Color color) noexcept;
[[nodiscard]] MeshCommand make_scene_mesh_command(const SceneRenderMesh& mesh,
                                                  const SceneRenderSubmitDesc& desc) noexcept;
[[nodiscard]] SceneMeshDrawPlan plan_scene_mesh_draws(const SceneRenderPacket& packet);
[[nodiscard]] SpriteCommand make_scene_sprite_command(const SceneRenderSprite& sprite) noexcept;
[[nodiscard]] SpriteBatchPlan plan_scene_sprite_batches(const SceneRenderPacket& packet);
[[nodiscard]] RuntimeSceneRenderLoadResult load_runtime_scene_render_data(const runtime::RuntimeAssetPackage& package,
                                                                          AssetId scene);
[[nodiscard]] RuntimeSceneRenderInstance
instantiate_runtime_scene_render_data(const runtime::RuntimeAssetPackage& package, AssetId scene);
[[nodiscard]] RuntimeSceneRenderAnimationApplyResult sample_and_apply_runtime_scene_render_animation_float_clip(
    RuntimeSceneRenderInstance& instance, const AnimationFloatClipSourceDocument& clip,
    const AnimationTransformBindingSourceDocument& binding_source, float time_seconds);
[[nodiscard]] RuntimeSceneRenderAnimationApplyResult sample_and_apply_runtime_scene_render_animation_pose_3d(
    RuntimeSceneRenderInstance& instance, const AnimationSkeleton3dDesc& skeleton,
    const std::vector<AnimationJointTrack3dDesc>& tracks, float time_seconds);
[[nodiscard]] RuntimeSceneRenderSpriteAnimationApplyResult sample_and_apply_runtime_scene_render_sprite_animation(
    RuntimeSceneRenderInstance& instance, const runtime::RuntimeSpriteAnimationPayload& animation, float time_seconds);
[[nodiscard]] RuntimeMorphMeshCpuAnimationSampleResult
sample_runtime_morph_mesh_cpu_animation_float_clip(const runtime::RuntimeMorphMeshCpuPayload& morph,
                                                   const AnimationFloatClipSourceDocument& clip,
                                                   std::string_view target_prefix, float time_seconds);
[[nodiscard]] SceneRenderSubmitResult submit_scene_render_packet(IRenderer& renderer, const SceneRenderPacket& packet,
                                                                 SceneRenderSubmitDesc desc = {});

} // namespace mirakana
