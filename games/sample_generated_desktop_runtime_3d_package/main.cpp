// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ai/behavior_tree.hpp"
#include "mirakana/ai/perception.hpp"
#include "mirakana/animation/skeleton.hpp"
#include "mirakana/animation/state_machine.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/audio/audio_mixer.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/navigation/local_avoidance.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_crowd.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_navmesh.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/physics/physics3d.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/production_vfx_profiling.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/renderer/renderer_quality_matrix.hpp"
#include "mirakana/runtime/addressable_content_streaming.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/entity_scale_culling.hpp"
#include "mirakana/runtime/gameplay_interaction.hpp"
#include "mirakana/runtime/gameplay_runtime_scheduler.hpp"
#include "mirakana/runtime/genre_rpg_systems.hpp"
#include "mirakana/runtime/genre_sandbox_world.hpp"
#include "mirakana/runtime/genre_simulation_management.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/physics_collision_runtime.hpp"
#include "mirakana/runtime/production_network_replication.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime/session_services.hpp"
#include "mirakana/runtime/world_entity_model.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/runtime_scene/runtime_scene.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <filesystem>
#include <iostream>
#include <limits>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeOptions {
    bool smoke{false};
    bool show_help{false};
    bool throttle{true};
    bool require_d3d12_scene_shaders{false};
    bool require_vulkan_scene_shaders{false};
    bool require_d3d12_renderer{false};
    bool require_vulkan_renderer{false};
    bool require_scene_gpu_bindings{false};
    bool require_postprocess{false};
    bool require_postprocess_depth_input{false};
    bool require_directional_shadow{false};
    bool require_directional_shadow_filtering{false};
    bool require_shadow_morph_composition{false};
    bool require_renderer_quality_gates{false};
    bool require_renderer_quality_matrix{false};
    bool require_rendering_vfx_profiling{false};
    bool require_playable_3d_slice{false};
    bool require_visible_3d_production_proof{false};
    bool require_vulkan_visible_3d_production_proof{false};
    bool require_native_ui_overlay{false};
    bool require_native_ui_textured_sprite_atlas{false};
    bool require_native_ui_text_glyph_atlas{false};
    bool require_primary_camera_controller{false};
    bool require_transform_animation{false};
    bool require_morph_package{false};
    bool require_compute_morph{false};
    bool require_compute_morph_normal_tangent{false};
    bool require_compute_morph_skin{false};
    bool require_compute_morph_async_telemetry{false};
    bool require_quaternion_animation{false};
    bool require_package_streaming_safe_point{false};
    bool require_package_upload_staging{false};
    bool require_gameplay_systems{false};
    bool require_scene_collision_package{false};
    bool require_entity_scale_culling{false};
    bool require_runtime_profile_resume{false};
    bool require_runtime_menu_hud{false};
    bool require_audio_gameplay_mixer{false};
    bool require_audio_production{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1"};
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/generated_3d_package_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneMorphVertexShaderPath{"shaders/generated_3d_package_scene_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphShaderPath{
    "shaders/generated_3d_package_scene_compute_morph.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_tangent_frame.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_tangent_frame.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_skinned.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_skinned.cs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/generated_3d_package_scene.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverFragmentShaderPath{
    "shaders/generated_3d_package_shadow_receiver.ps.dxil"};
constexpr std::string_view kRuntimeShiftedShadowReceiverFragmentShaderPath{
    "shaders/generated_3d_package_shadow_receiver_shifted.ps.dxil"};
constexpr std::string_view kRuntimeShadowVertexShaderPath{"shaders/generated_3d_package_shadow.vs.dxil"};
constexpr std::string_view kRuntimeShadowFragmentShaderPath{"shaders/generated_3d_package_shadow.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/generated_3d_package_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanMorphVertexShaderPath{"shaders/generated_3d_package_scene_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphShaderPath{
    "shaders/generated_3d_package_scene_compute_morph.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_tangent_frame.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_tangent_frame.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_skinned.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedShaderPath{
    "shaders/generated_3d_package_scene_compute_morph_skinned.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/generated_3d_package_scene.ps.spv"};
constexpr std::string_view kRuntimeShadowReceiverVulkanFragmentShaderPath{
    "shaders/generated_3d_package_shadow_receiver.ps.spv"};
constexpr std::string_view kRuntimeShiftedShadowReceiverVulkanFragmentShaderPath{
    "shaders/generated_3d_package_shadow_receiver_shifted.ps.spv"};
constexpr std::string_view kRuntimeShadowVulkanVertexShaderPath{"shaders/generated_3d_package_shadow.vs.spv"};
constexpr std::string_view kRuntimeShadowVulkanFragmentShaderPath{"shaders/generated_3d_package_shadow.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{"shaders/generated_3d_package_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{"shaders/generated_3d_package_postprocess.ps.dxil"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{"shaders/generated_3d_package_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{
    "shaders/generated_3d_package_postprocess.ps.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVertexShaderPath{"shaders/generated_3d_package_ui_overlay.vs.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayFragmentShaderPath{"shaders/generated_3d_package_ui_overlay.ps.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanVertexShaderPath{
    "shaders/generated_3d_package_ui_overlay.vs.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanFragmentShaderPath{
    "shaders/generated_3d_package_ui_overlay.ps.spv"};
constexpr std::uint32_t kRuntimeSceneTangentSpaceStrideBytes{48};
constexpr std::uint64_t kPackageStreamingResidentBudgetBytes{67108864};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/scenes/packaged-3d-scene");
}

[[nodiscard]] mirakana::AssetId packaged_base_color_texture_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/textures/base-color");
}

[[nodiscard]] mirakana::AssetId packaged_material_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/materials/lit");
}

[[nodiscard]] mirakana::AssetId packaged_animation_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/animations/packaged-mesh-bob");
}

[[nodiscard]] mirakana::AssetId packaged_mesh_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/meshes/triangle");
}

[[nodiscard]] mirakana::AssetId packaged_skinned_mesh_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/meshes/skinned-triangle");
}

[[nodiscard]] mirakana::AssetId packaged_morph_mesh_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/morphs/packaged-mesh");
}

[[nodiscard]] mirakana::AssetId packaged_morph_animation_asset_id() {
    return asset_id_from_game_asset_key(
        "sample-generated-desktop-runtime-3d-package/animations/packaged-mesh-morph-weights");
}

[[nodiscard]] mirakana::AssetId packaged_quaternion_animation_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/animations/packaged-pose");
}

[[nodiscard]] mirakana::AssetId packaged_ui_atlas_metadata_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/ui/hud-atlas");
}

[[nodiscard]] mirakana::AssetId packaged_ui_text_glyph_atlas_metadata_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/ui/hud-text-glyph-atlas");
}

[[nodiscard]] mirakana::AssetId packaged_collision_scene_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/physics/collision");
}

[[nodiscard]] mirakana::AssetId packaged_audio_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/audio/gameplay-systems");
}

constexpr const char* kHudAtlasProofResourceId{"hud.texture_atlas_proof"};
constexpr const char* kHudAtlasProofAssetUri{"runtime/assets/3d/base_color.texture.geasset"};
constexpr const char* kHudTextGlyphAtlasProofId{"hud.text_glyph_atlas_proof"};

[[nodiscard]] std::string_view
package_streaming_status_name(mirakana::runtime::RuntimePackageStreamingExecutionStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::invalid_descriptor:
        return "invalid_descriptor";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::validation_preflight_required:
        return "validation_preflight_required";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::package_load_failed:
        return "package_load_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::residency_hint_failed:
        return "residency_hint_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::over_budget_intent:
        return "over_budget_intent";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::safe_point_replacement_failed:
        return "safe_point_replacement_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_mount_failed:
        return "resident_mount_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_unmount_failed:
        return "resident_unmount_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_replace_failed:
        return "resident_replace_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_catalog_refresh_failed:
        return "resident_catalog_refresh_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::resident_eviction_plan_failed:
        return "resident_eviction_plan_failed";
    case mirakana::runtime::RuntimePackageStreamingExecutionStatus::committed:
        return "committed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view package_upload_staging_status_name(
    bool requested, const mirakana::runtime_rhi::RuntimePackageUploadStagingEvidence& evidence) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return evidence.ready ? "ready" : "blocked";
}

[[nodiscard]] mirakana::runtime_rhi::RuntimePackageUploadStagingEvidence
run_package_upload_staging_evidence(mirakana::SdlDesktopPresentation& presentation) {
    if (auto* device = presentation.scene_pbr_frame_rhi_device(); device != nullptr) {
        return mirakana::runtime_rhi::execute_runtime_package_upload_staging_evidence(*device);
    }

    mirakana::runtime_rhi::RuntimePackageUploadStagingEvidence evidence;
    evidence.diagnostics.push_back(mirakana::runtime_rhi::RuntimePackageUploadStagingEvidenceDiagnostic{
        .code = "rhi-device-unavailable",
        .message = "runtime package upload staging evidence requires a native RHI device",
    });
    return evidence;
}

[[nodiscard]] mirakana::AnimationSkeleton3dDesc packaged_quaternion_animation_skeleton() {
    return mirakana::AnimationSkeleton3dDesc{
        {mirakana::AnimationSkeleton3dJointDesc{.name = "PackagedMesh", .parent_index = mirakana::animation_no_parent}},
    };
}

[[nodiscard]] std::vector<mirakana::AnimationJointTrack3dDesc>
make_quaternion_animation_tracks(const mirakana::runtime::RuntimeAnimationQuaternionClipPayload& payload) {
    std::vector<mirakana::AnimationJointTrack3dByteSource> sources;
    sources.reserve(payload.clip.tracks.size());
    for (const auto& track : payload.clip.tracks) {
        sources.push_back(mirakana::AnimationJointTrack3dByteSource{
            .joint_index = track.joint_index,
            .target = track.target,
            .translation_time_seconds_bytes = track.translation_time_seconds_bytes,
            .translation_xyz_bytes = track.translation_xyz_bytes,
            .rotation_time_seconds_bytes = track.rotation_time_seconds_bytes,
            .rotation_xyzw_bytes = track.rotation_xyzw_bytes,
            .scale_time_seconds_bytes = track.scale_time_seconds_bytes,
            .scale_xyz_bytes = track.scale_xyz_bytes,
        });
    }
    return mirakana::make_animation_joint_tracks_3d_from_f32_bytes(sources);
}

[[nodiscard]] mirakana::AnimationTransformBindingSourceDocument packaged_animation_bindings() {
    mirakana::AnimationTransformBindingSourceDocument document;
    document.bindings.push_back(mirakana::AnimationTransformBindingSourceRow{
        .target = "gltf/node/0/translation/x",
        .node_name = "PackagedMesh",
        .component = mirakana::AnimationTransformBindingComponent::translation_x,
    });
    return document;
}

enum class UiAtlasMetadataStatus : std::uint8_t {
    not_requested,
    missing,
    malformed,
    invalid_reference,
    unsupported,
    ready,
};

struct UiAtlasMetadataRuntimeState {
    UiAtlasMetadataStatus status{UiAtlasMetadataStatus::not_requested};
    mirakana::UiRendererImagePalette palette;
    mirakana::AssetId atlas_page;
    std::size_t pages{0};
    std::size_t bindings{0};
    std::vector<std::string> diagnostics;
};

struct UiGlyphAtlasMetadataRuntimeState {
    UiAtlasMetadataStatus status{UiAtlasMetadataStatus::not_requested};
    mirakana::UiRendererGlyphAtlasPalette glyph_atlas;
    mirakana::AssetId atlas_page;
    std::size_t pages{0};
    std::size_t glyphs{0};
    std::vector<std::string> diagnostics;
};

[[nodiscard]] std::string_view ui_atlas_metadata_status_name(UiAtlasMetadataStatus status) noexcept {
    switch (status) {
    case UiAtlasMetadataStatus::not_requested:
        return "not_requested";
    case UiAtlasMetadataStatus::missing:
        return "missing";
    case UiAtlasMetadataStatus::malformed:
        return "malformed";
    case UiAtlasMetadataStatus::invalid_reference:
        return "invalid_reference";
    case UiAtlasMetadataStatus::unsupported:
        return "unsupported";
    case UiAtlasMetadataStatus::ready:
        return "ready";
    }
    return "unknown";
}

[[nodiscard]] UiAtlasMetadataStatus
classify_ui_atlas_metadata_failure(mirakana::AssetId metadata_asset,
                                   const mirakana::UiRendererImagePaletteBuildFailure& failure) {
    if (failure.asset != metadata_asset) {
        return UiAtlasMetadataStatus::invalid_reference;
    }
    if (failure.diagnostic.find("source image decoding") != std::string::npos ||
        failure.diagnostic.find("production atlas packing") != std::string::npos) {
        return UiAtlasMetadataStatus::unsupported;
    }
    if (failure.diagnostic.find("missing") != std::string::npos) {
        return UiAtlasMetadataStatus::missing;
    }
    return UiAtlasMetadataStatus::malformed;
}

[[nodiscard]] UiAtlasMetadataStatus
classify_ui_atlas_metadata_failure(mirakana::AssetId metadata_asset,
                                   const mirakana::UiRendererGlyphAtlasPaletteBuildFailure& failure) {
    if (failure.asset != metadata_asset) {
        return UiAtlasMetadataStatus::invalid_reference;
    }
    if (failure.diagnostic.find("font rasterization") != std::string::npos ||
        failure.diagnostic.find("glyph atlas generation") != std::string::npos) {
        return UiAtlasMetadataStatus::unsupported;
    }
    if (failure.diagnostic.find("missing") != std::string::npos) {
        return UiAtlasMetadataStatus::missing;
    }
    return UiAtlasMetadataStatus::malformed;
}

[[nodiscard]] UiAtlasMetadataRuntimeState
load_required_ui_atlas_metadata(const mirakana::runtime::RuntimeAssetPackage* package) {
    UiAtlasMetadataRuntimeState state;
    const auto metadata_asset = packaged_ui_atlas_metadata_asset_id();
    if (package == nullptr) {
        state.status = UiAtlasMetadataStatus::missing;
        state.diagnostics.push_back("ui atlas metadata requires a loaded runtime package");
        return state;
    }

    auto result = mirakana::build_ui_renderer_image_palette_from_runtime_ui_atlas(*package, metadata_asset);
    if (!result.succeeded()) {
        state.status = UiAtlasMetadataStatus::malformed;
        for (const auto& failure : result.failures) {
            const auto classified = classify_ui_atlas_metadata_failure(metadata_asset, failure);
            if (state.status == UiAtlasMetadataStatus::malformed || classified == UiAtlasMetadataStatus::unsupported ||
                classified == UiAtlasMetadataStatus::invalid_reference) {
                state.status = classified;
            }
            state.diagnostics.push_back(failure.diagnostic);
        }
        return state;
    }

    state.status = UiAtlasMetadataStatus::ready;
    state.pages = result.atlas_page_assets.size();
    state.bindings = result.palette.count();
    state.atlas_page = result.atlas_page_assets.empty() ? mirakana::AssetId{} : result.atlas_page_assets.front();
    state.palette = std::move(result.palette);
    if (state.pages == 0 || state.bindings == 0 || state.atlas_page.value == 0) {
        state.status = UiAtlasMetadataStatus::malformed;
        state.diagnostics.push_back("ui atlas metadata did not produce any atlas page or image binding");
    }
    return state;
}

[[nodiscard]] UiGlyphAtlasMetadataRuntimeState
load_required_ui_text_glyph_atlas_metadata(const mirakana::runtime::RuntimeAssetPackage* package) {
    UiGlyphAtlasMetadataRuntimeState state;
    const auto metadata_asset = packaged_ui_text_glyph_atlas_metadata_asset_id();
    if (package == nullptr) {
        state.status = UiAtlasMetadataStatus::missing;
        state.diagnostics.push_back("ui text glyph atlas metadata requires a loaded runtime package");
        return state;
    }

    auto result = mirakana::build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas(*package, metadata_asset);
    if (!result.succeeded()) {
        state.status = UiAtlasMetadataStatus::malformed;
        for (const auto& failure : result.failures) {
            const auto classified = classify_ui_atlas_metadata_failure(metadata_asset, failure);
            if (state.status == UiAtlasMetadataStatus::malformed || classified == UiAtlasMetadataStatus::unsupported ||
                classified == UiAtlasMetadataStatus::invalid_reference) {
                state.status = classified;
            }
            state.diagnostics.push_back(failure.diagnostic);
        }
        return state;
    }

    state.status = UiAtlasMetadataStatus::ready;
    state.pages = result.atlas_page_assets.size();
    state.glyphs = result.palette.count();
    state.atlas_page = result.atlas_page_assets.empty() ? mirakana::AssetId{} : result.atlas_page_assets.front();
    state.glyph_atlas = std::move(result.palette);
    if (state.pages == 0 || state.glyphs == 0 || state.atlas_page.value == 0) {
        state.status = UiAtlasMetadataStatus::malformed;
        state.diagnostics.push_back("ui text glyph atlas metadata did not produce any atlas page or glyph binding");
    }
    return state;
}

void print_ui_atlas_metadata_diagnostics(std::string_view prefix, const UiAtlasMetadataRuntimeState& state) {
    for (const auto& diagnostic : state.diagnostics) {
        std::cout << prefix << " ui_atlas_metadata_diagnostic=" << ui_atlas_metadata_status_name(state.status) << ": "
                  << diagnostic << '\n';
    }
}

void print_ui_atlas_metadata_diagnostics(std::string_view prefix, const UiGlyphAtlasMetadataRuntimeState& state) {
    for (const auto& diagnostic : state.diagnostics) {
        std::cout << prefix << " ui_atlas_metadata_diagnostic=" << ui_atlas_metadata_status_name(state.status) << ": "
                  << diagnostic << '\n';
    }
}

constexpr mirakana::SceneNodeId kPackagedMeshNode{1};
constexpr mirakana::SceneNodeId kPrimaryCameraNode{3};

constexpr std::uint32_t kRuntimeScenePositionStrideBytes{12};
constexpr std::uint32_t kRuntimeSceneSkinnedStrideBytes{
    mirakana::runtime_rhi::runtime_skinned_mesh_vertex_stride_bytes};
constexpr std::uint32_t kRuntimeSceneSkinnedJointIndicesOffsetBytes{48};
constexpr std::uint32_t kRuntimeSceneSkinnedJointWeightsOffsetBytes{56};

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_compute_morph_skinned_scene_vertex_buffers() {
    return {
        mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 0,
            .stride = kRuntimeScenePositionStrideBytes,
            .input_rate = mirakana::rhi::VertexInputRate::vertex,
        },
        mirakana::rhi::VertexBufferLayoutDesc{
            .binding = 3,
            .stride = kRuntimeSceneSkinnedStrideBytes,
            .input_rate = mirakana::rhi::VertexInputRate::vertex,
        },
    };
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_compute_morph_skinned_scene_vertex_attributes() {
    return {
        mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 3,
            .offset = kRuntimeSceneSkinnedJointIndicesOffsetBytes,
            .format = mirakana::rhi::VertexFormat::uint16x4,
            .semantic = mirakana::rhi::VertexSemantic::joint_indices,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 3,
            .offset = kRuntimeSceneSkinnedJointWeightsOffsetBytes,
            .format = mirakana::rhi::VertexFormat::float32x4,
            .semantic = mirakana::rhi::VertexSemantic::joint_weights,
        },
    };
}

[[nodiscard]] bool activate_compute_morph_skinned_scene(std::optional<mirakana::RuntimeSceneRenderInstance>& scene) {
    if (!scene.has_value() || !scene->scene.has_value()) {
        return false;
    }
    auto* mesh_node = scene->scene->find_node(kPackagedMeshNode);
    if (mesh_node == nullptr || !mesh_node->components.mesh_renderer.has_value()) {
        return false;
    }
    mesh_node->components.mesh_renderer->mesh = packaged_skinned_mesh_asset_id();
    scene->render_packet = mirakana::build_scene_render_packet(*scene->scene);
    return true;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_scene_vertex_buffers() {
    return {mirakana::rhi::VertexBufferLayoutDesc{
        .binding = 0,
        .stride = kRuntimeSceneTangentSpaceStrideBytes,
        .input_rate = mirakana::rhi::VertexInputRate::vertex,
    }};
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_scene_vertex_attributes() {
    return {
        mirakana::rhi::VertexAttributeDesc{
            .location = 0,
            .binding = 0,
            .offset = 0,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::position,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 1,
            .binding = 0,
            .offset = 12,
            .format = mirakana::rhi::VertexFormat::float32x3,
            .semantic = mirakana::rhi::VertexSemantic::normal,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 2,
            .binding = 0,
            .offset = 24,
            .format = mirakana::rhi::VertexFormat::float32x2,
            .semantic = mirakana::rhi::VertexSemantic::texcoord,
        },
        mirakana::rhi::VertexAttributeDesc{
            .location = 3,
            .binding = 0,
            .offset = 32,
            .format = mirakana::rhi::VertexFormat::float32x4,
            .semantic = mirakana::rhi::VertexSemantic::tangent,
        },
    };
}

constexpr mirakana::BehaviorTreeNodeId kGameplaySystemsRootNode{1};
constexpr mirakana::BehaviorTreeNodeId kGameplaySystemsHasTargetNode{2};
constexpr mirakana::BehaviorTreeNodeId kGameplaySystemsNeedsMoveNode{3};
constexpr mirakana::BehaviorTreeNodeId kGameplaySystemsMoveActionNode{4};
constexpr const char* kGameplaySystemsHasTargetKey{"generated3d.has_target"};
constexpr const char* kGameplaySystemsNeedsMoveKey{"generated3d.needs_move"};
constexpr const char* kGameplaySystemsTargetIdKey{"generated3d.target_id"};
constexpr const char* kGameplaySystemsTargetDistanceKey{"generated3d.target_distance"};
constexpr const char* kGameplaySystemsVisibleTargetsKey{"generated3d.visible_targets"};
constexpr const char* kGameplaySystemsAudibleTargetsKey{"generated3d.audible_targets"};
constexpr const char* kGameplaySystemsTargetStateKey{"generated3d.target_state"};

enum class GameplaySystemsStatus : std::uint8_t {
    not_started,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view gameplay_systems_status_name(GameplaySystemsStatus status) noexcept {
    switch (status) {
    case GameplaySystemsStatus::not_started:
        return "not_started";
    case GameplaySystemsStatus::ready:
        return "ready";
    case GameplaySystemsStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

[[nodiscard]] const char* audio_production_status_name(mirakana::AudioProductionReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::AudioProductionReadinessStatus::ready:
        return "ready";
    case mirakana::AudioProductionReadinessStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::AudioProductionReadinessStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

struct AudioGameplayMixerProbeResult {
    bool ready{false};
    std::size_t diagnostics{0U};
    std::size_t buses{0U};
    std::size_t cues{0U};
    std::size_t triggers{0U};
    std::size_t commands{0U};
    std::size_t paused_buses{0U};
    std::size_t faded_buses{0U};
    std::size_t looping_commands{0U};
    std::size_t spatial_commands{0U};
    std::size_t render_commands{0U};
    std::uint32_t render_frames{0U};
    std::size_t render_samples{0U};
    float sample_abs_sum{0.0F};
    std::size_t payload_diagnostics{0U};
};

struct AudioProductionProbeResult {
    mirakana::AudioProductionReadinessStatus status{mirakana::AudioProductionReadinessStatus::invalid_request};
    bool reviewed{false};
    bool production_audio_ready{false};
    bool selected_package_evidence_ready{false};
    std::size_t decoded_source_rows{0U};
    std::size_t streaming_chunk_rows{0U};
    std::size_t format_conversion_policy_rows{0U};
    std::size_t bus_budget_rows{0U};
    std::size_t voice_budget_rows{0U};
    std::size_t dsp_graph_rows{0U};
    std::size_t listener_rows{0U};
    std::size_t spatial_source_rows{0U};
    std::size_t hrtf_host_gate_rows{0U};
    std::size_t device_lifecycle_rows{0U};
    bool device_host_evidence_available{false};
    bool hrtf_host_evidence_available{false};
    std::size_t unsupported_claim_rows{0U};
    bool invoked_codec_decode{false};
    bool invoked_background_streaming{false};
    bool invoked_middleware{false};
    bool invoked_hrtf{false};
    bool invoked_device_callback{false};
    bool invoked_device_io{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool package_evidence_ready{false};
};

[[nodiscard]] AudioGameplayMixerProbeResult
validate_audio_gameplay_mixer_package_evidence(const mirakana::AudioClipSampleData& samples) {
    AudioGameplayMixerProbeResult result;
    result.payload_diagnostics = mirakana::is_valid_audio_clip_sample_data(samples) ? 0U : 1U;

    const auto request = mirakana::AudioGameplayMixRequest{
        .buses =
            {
                mirakana::AudioGameplayBusMixDesc{.name = "music",
                                                  .gain = 0.8F,
                                                  .paused = false,
                                                  .fade_from_gain = 0.5F,
                                                  .fade_to_gain = 1.0F,
                                                  .fade_elapsed_seconds = 0.5F,
                                                  .fade_duration_seconds = 1.0F},
                mirakana::AudioGameplayBusMixDesc{.name = "sfx", .gain = 1.0F, .paused = true},
            },
        .cues =
            {
                mirakana::AudioGameplayCueDesc{.id = "music.theme",
                                               .kind = mirakana::AudioGameplayCueKind::music,
                                               .clip = samples.clip,
                                               .bus = "music",
                                               .gain = 0.5F,
                                               .looping = true},
                mirakana::AudioGameplayCueDesc{.id = "sfx.jump",
                                               .kind = mirakana::AudioGameplayCueKind::sfx,
                                               .clip = samples.clip,
                                               .bus = "sfx",
                                               .gain = 1.0F,
                                               .looping = false,
                                               .spatialized = true,
                                               .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                               .min_distance = 1.0F,
                                               .max_distance = 8.0F},
            },
        .triggers =
            {
                mirakana::AudioGameplayCueTrigger{.cue_id = "music.theme", .start_frame = 0U, .gain_scale = 1.0F},
                mirakana::AudioGameplayCueTrigger{.cue_id = "sfx.jump", .start_frame = 0U, .gain_scale = 0.75F},
            },
    };

    result.buses = request.buses.size();
    result.cues = request.cues.size();
    result.triggers = request.triggers.size();
    for (const auto& bus : request.buses) {
        if (bus.paused) {
            ++result.paused_buses;
        }
        if (bus.fade_duration_seconds > 0.0F) {
            ++result.faded_buses;
        }
    }

    const auto plan = mirakana::plan_gameplay_audio_mix(request);
    result.diagnostics = plan.diagnostics.size();
    result.commands = plan.commands.size();
    for (const auto& command : plan.commands) {
        if (command.voice.looping) {
            ++result.looping_commands;
        }
        if (command.spatialized) {
            ++result.spatial_commands;
        }
    }
    if (!plan.succeeded() || result.payload_diagnostics != 0U) {
        return result;
    }

    try {
        mirakana::AudioMixer mixer;
        for (const auto& bus : plan.buses) {
            mixer.add_bus(bus);
        }
        if (!mixer.register_clip(mirakana::AudioClipDesc{.clip = samples.clip,
                                                         .sample_rate = samples.format.sample_rate,
                                                         .channel_count = samples.format.channel_count,
                                                         .frame_count = samples.frame_count,
                                                         .sample_format = samples.format.sample_format,
                                                         .streaming = false,
                                                         .buffered_frame_count = samples.frame_count})) {
            ++result.payload_diagnostics;
            return result;
        }
        for (const auto& command : plan.commands) {
            (void)mixer.play(command.voice);
        }

        const auto output = mirakana::render_audio_device_stream_interleaved_float(
            mixer,
            mirakana::AudioDeviceStreamRequest{
                .format = samples.format,
                .device_frame = 0U,
                .queued_frames = 0U,
                .target_queued_frames = 2U,
                .max_render_frames = 2U,
            },
            std::span<const mirakana::AudioClipSampleData>{&samples, 1});
        result.render_commands = output.buffer.plan.commands.size();
        result.render_frames = output.buffer.frame_count;
        result.render_samples = output.buffer.interleaved_float_samples.size();
        for (const auto sample : output.buffer.interleaved_float_samples) {
            result.sample_abs_sum += std::abs(sample);
        }
    } catch (const std::exception&) {
        ++result.diagnostics;
        return result;
    }

    result.ready = result.diagnostics == 0U && result.payload_diagnostics == 0U && result.buses == 2U &&
                   result.cues == 2U && result.triggers == 2U && result.commands == 2U && result.paused_buses == 1U &&
                   result.faded_buses == 1U && result.looping_commands == 1U && result.spatial_commands == 1U &&
                   result.render_commands == 2U && result.render_frames == 2U && result.render_samples == 2U &&
                   result.sample_abs_sum > 0.0F;
    return result;
}

[[nodiscard]] AudioProductionProbeResult
validate_audio_production_package_evidence(const mirakana::AudioClipSampleData& samples) {
    AudioProductionProbeResult result;
    const auto reviewed_frames = std::min<std::uint64_t>(samples.frame_count, 4U);
    const auto clip = samples.clip;
    const auto request = mirakana::AudioProductionReviewRequest{
        .decoded_sources =
            {
                mirakana::AudioProductionDecodedSourceEvidenceRow{
                    .clip = clip,
                    .format = samples.format,
                    .frame_count = reviewed_frames,
                    .decoded_byte_count = reviewed_frames * samples.format.channel_count * sizeof(float),
                    .reviewed = true,
                    .source_index = 1U,
                },
            },
        .streaming_chunks =
            {
                mirakana::AudioProductionStreamingChunkEvidenceRow{
                    .chunk =
                        mirakana::AudioStreamingChunkDesc{
                            .clip = clip,
                            .format = samples.format,
                            .start_frame = 0U,
                            .frame_count = reviewed_frames,
                        },
                    .queued_frame_count = reviewed_frames,
                    .reviewed = true,
                    .source_index = 2U,
                },
            },
        .format_conversion_policies =
            {
                mirakana::AudioProductionFormatConversionPolicyRow{
                    .clip = clip,
                    .source_format = samples.format,
                    .device_format = samples.format,
                    .resampling_quality = mirakana::AudioResamplingQuality::linear,
                    .reviewed = true,
                    .source_index = 3U,
                },
            },
        .dsp_graph_rows =
            {
                mirakana::AudioProductionDspGraphRow{
                    .node_id = "sample3d.audio.limiter",
                    .kind = mirakana::AudioProductionDspNodeKind::limiter,
                    .input_count = 1U,
                    .output_count = 1U,
                    .deterministic = true,
                    .reviewed = true,
                    .source_index = 4U,
                },
            },
        .listener =
            mirakana::AudioSpatialListenerDesc{
                .position = mirakana::AudioPoint3{},
                .right = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
            },
        .spatial_voices =
            {
                mirakana::AudioSpatialVoiceDesc{
                    .voice = mirakana::AudioVoiceId{1U},
                    .position = mirakana::AudioPoint3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                    .min_distance = 1.0F,
                    .max_distance = 8.0F,
                    .spatialized = true,
                },
            },
        .device_lifecycle_rows =
            {
                mirakana::AudioProductionDeviceLifecycleRow{
                    .backend_id = "sdl3",
                    .uses_logical_device = true,
                    .uses_audio_stream = true,
                    .uses_queueing = true,
                    .uses_callback = false,
                    .can_pause_resume = true,
                    .can_clear = true,
                    .host_evidence_available = false,
                    .native_handle_exposed = false,
                    .source_index = 5U,
                },
            },
        .unsupported_claim_rows = {},
        .max_voice_budget = 8U,
        .active_voice_count = 1U,
        .max_bus_budget = 4U,
        .active_bus_count = 2U,
        .row_budget = 32U,
        .official_sources_reviewed = true,
        .hrtf_host_evidence_available = false,
        .request_native_device_handles = false,
        .invoked_codec_decode = false,
        .invoked_background_streaming = false,
        .invoked_middleware = false,
        .invoked_hrtf = false,
        .invoked_device_callback = false,
        .invoked_device_io = false,
        .seed = 20260527U,
    };

    const auto plan = mirakana::review_audio_production_readiness(request);
    result.status = plan.status;
    result.reviewed = plan.reviewed;
    result.production_audio_ready = plan.production_audio_ready;
    result.selected_package_evidence_ready = plan.selected_package_evidence_ready;
    result.decoded_source_rows = plan.decoded_source_rows;
    result.streaming_chunk_rows = plan.streaming_chunk_rows;
    result.format_conversion_policy_rows = plan.format_conversion_policy_rows;
    result.bus_budget_rows = plan.bus_budget_rows;
    result.voice_budget_rows = plan.voice_budget_rows;
    result.dsp_graph_rows = plan.dsp_graph_rows;
    result.listener_rows = plan.listener_rows;
    result.spatial_source_rows = plan.spatial_source_rows;
    result.hrtf_host_gate_rows = plan.hrtf_host_gate_rows;
    result.device_lifecycle_rows = plan.device_lifecycle_rows;
    result.device_host_evidence_available = plan.device_host_evidence_available;
    result.hrtf_host_evidence_available = plan.hrtf_host_evidence_available;
    result.unsupported_claim_rows = plan.unsupported_claim_rows;
    result.invoked_codec_decode = plan.invoked_codec_decode;
    result.invoked_background_streaming = plan.invoked_background_streaming;
    result.invoked_middleware = plan.invoked_middleware;
    result.invoked_hrtf = plan.invoked_hrtf;
    result.invoked_device_callback = plan.invoked_device_callback;
    result.invoked_device_io = plan.invoked_device_io;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.package_evidence_ready =
        result.status == mirakana::AudioProductionReadinessStatus::host_evidence_required && result.reviewed &&
        !result.production_audio_ready && result.selected_package_evidence_ready && result.decoded_source_rows == 1U &&
        result.streaming_chunk_rows == 1U && result.format_conversion_policy_rows == 1U &&
        result.bus_budget_rows == 1U && result.voice_budget_rows == 1U && result.dsp_graph_rows == 1U &&
        result.listener_rows == 1U && result.spatial_source_rows == 1U && result.hrtf_host_gate_rows == 1U &&
        result.device_lifecycle_rows == 1U && !result.device_host_evidence_available &&
        !result.hrtf_host_evidence_available && result.unsupported_claim_rows == 0U && !result.invoked_codec_decode &&
        !result.invoked_background_streaming && !result.invoked_middleware && !result.invoked_hrtf &&
        !result.invoked_device_callback && !result.invoked_device_io && result.diagnostics == 2U &&
        result.replay_hash != 0U;
    return result;
}

[[nodiscard]] std::string_view
runtime_gameplay_session_state_name(mirakana::runtime::RuntimeGameplaySessionState state) noexcept {
    switch (state) {
    case mirakana::runtime::RuntimeGameplaySessionState::running:
        return "running";
    case mirakana::runtime::RuntimeGameplaySessionState::won:
        return "won";
    case mirakana::runtime::RuntimeGameplaySessionState::lost:
        return "lost";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_scene_gameplay_session_state_name(mirakana::runtime_scene::RuntimeSceneGameplaySessionState state) noexcept {
    switch (state) {
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running:
        return "running";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::won:
        return "won";
    case mirakana::runtime_scene::RuntimeSceneGameplaySessionState::lost:
        return "lost";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
runtime_session_profile_resume_status_name(mirakana::runtime::RuntimeSessionProfileResumeStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSessionProfileResumeStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

[[nodiscard]] const char*
gameplay_runtime_scheduler_status_name(mirakana::runtime::RuntimeGameplaySchedulerStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::no_steps:
        return "no_steps";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::budget_limited:
        return "budget_limited";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::paused:
        return "paused";
    case mirakana::runtime::RuntimeGameplaySchedulerStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
world_entity_model_status_name(mirakana::runtime::RuntimeWorldEntityLifecycleStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::no_entities:
        return "no_entities";
    case mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
addressable_content_status_name(mirakana::runtime::RuntimeAddressableContentStreamingStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::no_requests:
        return "no_requests";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::budget_limited:
        return "budget_limited";
    case mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* rpg_systems_status_name(mirakana::runtime::RuntimeRpgSystemsStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeRpgSystemsStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeRpgSystemsStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeRpgSystemsStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* sandbox_world_status_name(mirakana::runtime::RuntimeSandboxWorldStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSandboxWorldStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSandboxWorldStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeSandboxWorldStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
simulation_management_status_name(mirakana::runtime::RuntimeSimulationManagementStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeSimulationManagementStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeSimulationManagementStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeSimulationManagementStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
network_replication_status_name(mirakana::runtime::RuntimeNetworkReplicationStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeNetworkReplicationStatus::ready:
        return "ready";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::no_rows:
        return "no_rows";
    case mirakana::runtime::RuntimeNetworkReplicationStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char*
rendering_vfx_profiling_status_name(mirakana::RendererProductionVfxProfilingStatus status) noexcept {
    switch (status) {
    case mirakana::RendererProductionVfxProfilingStatus::ready:
        return "ready";
    case mirakana::RendererProductionVfxProfilingStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::RendererProductionVfxProfilingStatus::no_rows:
        return "no_rows";
    case mirakana::RendererProductionVfxProfilingStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] const char* renderer_quality_matrix_status_name(mirakana::RendererQualityMatrixStatus status) noexcept {
    switch (status) {
    case mirakana::RendererQualityMatrixStatus::ready:
        return "ready";
    case mirakana::RendererQualityMatrixStatus::host_evidence_required:
        return "host_evidence_required";
    case mirakana::RendererQualityMatrixStatus::dependency_evidence_required:
        return "dependency_evidence_required";
    case mirakana::RendererQualityMatrixStatus::unsupported:
        return "unsupported";
    case mirakana::RendererQualityMatrixStatus::no_rows:
        return "no_rows";
    case mirakana::RendererQualityMatrixStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

struct SceneGameplayBindingProbeResult {
    std::size_t source_rows{0U};
    std::size_t binding_rows{0U};
    std::size_t gameplay_systems{0U};
    std::size_t component_rows{0U};
    std::size_t binding_diagnostics{0U};
    std::size_t interaction_rows{0U};
    std::size_t interaction_diagnostics{0U};
    mirakana::runtime_scene::RuntimeSceneGameplaySessionState final_session_state{
        mirakana::runtime_scene::RuntimeSceneGameplaySessionState::running};
    bool ready{false};
};

struct GameplayRuntimeSchedulerProbeResult {
    mirakana::runtime::RuntimeGameplaySchedulerStatus status{
        mirakana::runtime::RuntimeGameplaySchedulerStatus::invalid_request};
    std::size_t available_steps{0U};
    std::size_t planned_steps{0U};
    std::size_t step_rows{0U};
    std::size_t system_rows{0U};
    std::size_t command_rows{0U};
    std::uint64_t consumed_time_us{0U};
    std::uint64_t remaining_time_us{0U};
    bool budget_limited{false};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool ready{false};
};

struct WorldEntityModelProbeResult {
    mirakana::runtime::RuntimeWorldEntityLifecycleStatus status{
        mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request};
    std::size_t entity_rows{0U};
    std::size_t component_rows{0U};
    std::size_t region_ownership_rows{0U};
    std::size_t lifecycle_rows{0U};
    std::size_t persistence_rows{0U};
    std::size_t streaming_region_rows{0U};
    std::size_t spawn_rows{0U};
    std::size_t move_rows{0U};
    std::size_t despawn_rows{0U};
    std::size_t duplicate_entity_diagnostics{0U};
    mirakana::runtime::RuntimeWorldEntityLifecycleStatus bridge_rejection_status{
        mirakana::runtime::RuntimeWorldEntityLifecycleStatus::invalid_request};
    std::size_t bridge_rejection_diagnostics{0U};
    std::size_t bridge_rejection_persistence_rows{0U};
    std::size_t bridge_rejection_streaming_region_rows{0U};
    std::size_t bridge_rejection_streaming_diagnostics_present{0U};
    bool bridge_rejection_fail_closed{false};
    std::size_t diagnostics{0U};
    bool ready{false};
};

struct AddressableContentStreamingProbeResult {
    mirakana::runtime::RuntimeAddressableContentStreamingStatus status{
        mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request};
    mirakana::runtime::RuntimeAddressableContentStreamingStatus budget_rejection_status{
        mirakana::runtime::RuntimeAddressableContentStreamingStatus::invalid_request};
    std::size_t address_rows{0U};
    std::size_t dependency_rows{0U};
    std::size_t load_rows{0U};
    std::size_t release_rows{0U};
    std::size_t refcount_rows{0U};
    std::uint64_t resident_bytes{0U};
    std::uint64_t resident_budget_bytes{0U};
    std::size_t budget_rejection_diagnostics{0U};
    std::size_t diagnostics{0U};
    bool package_io{false};
    bool async_execution{false};
    bool committed{false};
    bool ready{false};
};

struct RpgSystemsProbeResult {
    mirakana::runtime::RuntimeRpgSystemsStatus status{mirakana::runtime::RuntimeRpgSystemsStatus::invalid_request};
    std::size_t party_members{0U};
    std::size_t enemy_members{0U};
    std::size_t stat_rows{0U};
    std::size_t progression_rows{0U};
    std::size_t skill_rows{0U};
    std::size_t blocked_skill_rows{0U};
    std::size_t equipment_rows{0U};
    std::size_t blocked_equipment_rows{0U};
    std::size_t combat_turn_rows{0U};
    std::size_t combat_rounds{0U};
    std::size_t reward_rows{0U};
    std::size_t save_validation_rows{0U};
    std::size_t repairable_save_validation_rows{0U};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_combat_execution{false};
    bool invoked_reward_application{false};
    bool invoked_save_io{false};
    bool ready{false};
};

struct SandboxWorldProbeResult {
    mirakana::runtime::RuntimeSandboxWorldStatus status{mirakana::runtime::RuntimeSandboxWorldStatus::invalid_request};
    std::size_t chunk_rows{0U};
    std::size_t resident_chunk_rows{0U};
    std::size_t existing_cell_rows{0U};
    std::size_t placement_intent_rows{0U};
    std::size_t placement_accepted_rows{0U};
    std::size_t placement_rejected_rows{0U};
    std::size_t destruction_intent_rows{0U};
    std::size_t destruction_accepted_rows{0U};
    std::size_t destruction_rejected_rows{0U};
    std::size_t construction_cost_rows{0U};
    std::size_t mutation_rows{0U};
    std::size_t persistence_rows{0U};
    std::size_t persistence_repairable_rows{0U};
    std::size_t rejected_unsafe_mutation_rows{0U};
    std::size_t diagnostics{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_world_mutation{false};
    bool invoked_persistence_io{false};
    bool invoked_package_io{false};
    bool ready{false};
};

struct SimulationManagementProbeResult {
    mirakana::runtime::RuntimeSimulationManagementStatus status{
        mirakana::runtime::RuntimeSimulationManagementStatus::invalid_request};
    std::uint64_t tick_count{0U};
    std::size_t resource_balance_rows{0U};
    std::size_t job_rows{0U};
    std::size_t job_assignment_rows{0U};
    std::size_t logistics_links{0U};
    std::size_t logistics_transfer_rows{0U};
    std::size_t logistics_scheduled_transfer_rows{0U};
    std::size_t economy_summary_rows{0U};
    std::size_t population_need_rows{0U};
    std::size_t need_deficit_rows{0U};
    std::size_t schedule_rows{0U};
    std::size_t save_review_rows{0U};
    std::size_t save_review_repairable_rows{0U};
    std::size_t dashboard_rows{0U};
    std::uint64_t replay_hash{0U};
    std::size_t diagnostics{0U};
    bool invoked_economy_execution{false};
    bool invoked_save_io{false};
    bool invoked_runtime_ui{false};
    bool invoked_package_io{false};
    bool ready{false};
};

struct NetworkReplicationProbeResult {
    mirakana::runtime::RuntimeNetworkReplicationStatus status{
        mirakana::runtime::RuntimeNetworkReplicationStatus::invalid_request};
    std::size_t object_rows{0U};
    std::size_t input_rows{0U};
    std::size_t snapshot_rows{0U};
    std::size_t rollback_rows{0U};
    std::size_t rejected_unsafe_rows{0U};
    std::uint64_t replay_hash{0U};
    bool requires_transport_host_evidence{false};
    bool has_transport_host_evidence{false};
    bool invoked_network_io{false};
    bool invoked_rollback_execution{false};
    bool invoked_world_mutation{false};
    std::size_t diagnostics{0U};
    bool reviewed{false};
    bool ready{false};
};

struct RenderingVfxProfilingProbeResult {
    mirakana::RendererProductionVfxProfilingStatus status{
        mirakana::RendererProductionVfxProfilingStatus::invalid_request};
    std::size_t feature_rows{0U};
    std::size_t gpu_particle_budget_rows{0U};
    std::size_t postprocess_rows{0U};
    std::size_t backend_timing_rows{0U};
    std::size_t backend_evidence_rows{0U};
    std::size_t backend_evidence_ready{0U};
    std::size_t backend_evidence_host_gated{0U};
    std::size_t cpu_profile_rows{0U};
    std::size_t package_counter_rows{0U};
    std::size_t package_counter_ready{0U};
    std::size_t package_counter_host_gated{0U};
    std::size_t crash_telemetry_handoff_rows{0U};
    std::size_t host_validated_backends{0U};
    std::size_t rejected_unsafe_rows{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_host_evidence_ready{false};
    bool vulkan_strict_host_evidence_ready{false};
    bool metal_host_evidence_ready{false};
    bool requires_metal_host_evidence{false};
    bool has_metal_host_evidence{false};
    bool invoked_gpu_commands{false};
    bool invoked_native_capture{false};
    bool invoked_crash_upload{false};
    std::size_t diagnostics{0U};
    bool reviewed{false};
    bool ready{false};
};

struct RendererQualityMatrixProbeResult {
    mirakana::RendererQualityMatrixStatus status{mirakana::RendererQualityMatrixStatus::invalid_request};
    std::size_t rows{0U};
    std::size_t ready_rows{0U};
    std::size_t host_gated_rows{0U};
    std::size_t dependency_gated_rows{0U};
    std::size_t unsupported_rows{0U};
    std::size_t host_validated_backends{0U};
    std::uint64_t replay_hash{0U};
    bool d3d12_ready{false};
    bool vulkan_strict_ready{false};
    bool metal_ready{false};
    bool requires_metal_host_evidence{false};
    bool has_metal_host_evidence{false};
    bool selected_package_evidence_ready{false};
    bool general_renderer_quality_ready{false};
    bool invoked_gpu_commands{false};
    bool invoked_native_capture{false};
    bool invoked_crash_upload{false};
    std::size_t diagnostics{0U};
    bool reviewed{false};
    bool ready{false};
};

struct InputContextRebindingProbeResult {
    std::size_t requested_layers{0U};
    std::size_t active_contexts{0U};
    bool capture_context_active{false};
    bool gameplay_input_consumed{false};
    std::size_t profile_overlays_applied{0U};
    mirakana::runtime::RuntimeInputRebindingCaptureStatus action_capture_status{
        mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting};
    mirakana::runtime::RuntimeInputRebindingCaptureStatus axis_capture_status{
        mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting};
    bool focus_gameplay_consumed{false};
    bool focus_retained{false};
    std::size_t presentation_rows{0U};
    std::size_t glyph_lookup_keys{0U};
    std::size_t diagnostics{0U};
    bool ready{false};
};

[[nodiscard]] std::size_t count_scene_gameplay_binding_systems(
    std::span<const mirakana::runtime_scene::RuntimeSceneGameplayBindingRow> bindings) {
    std::vector<std::string> systems;
    systems.reserve(bindings.size());
    for (const auto& binding : bindings) {
        if (!std::ranges::contains(systems, binding.gameplay_system_id)) {
            systems.push_back(binding.gameplay_system_id);
        }
    }
    return systems.size();
}

[[nodiscard]] GameplayRuntimeSchedulerProbeResult validate_gameplay_runtime_scheduler_package_evidence() {
    using Command = mirakana::runtime::RuntimeGameplaySchedulerInputCommandDesc;
    using Mode = mirakana::runtime::RuntimeGameplaySchedulerMode;
    using Request = mirakana::runtime::RuntimeGameplaySchedulerRequest;
    using Status = mirakana::runtime::RuntimeGameplaySchedulerStatus;
    using System = mirakana::runtime::RuntimeGameplaySchedulerSystemDesc;

    const auto plan = mirakana::runtime::plan_runtime_gameplay_schedule(Request{
        .scheduler_id = "sample3d.gameplay",
        .next_tick = 42U,
        .tick_delta_us = 16'666U,
        .accumulated_time_us = 49'998U,
        .max_steps_per_frame = 2U,
        .mode = Mode::run,
        .systems =
            std::vector<System>{
                System{.system_id = "physics", .order = 20, .enabled = true, .budget_us = 400U, .source_index = 2U},
                System{.system_id = "input", .order = 10, .enabled = true, .budget_us = 100U, .source_index = 1U},
                System{.system_id = "ai", .order = 30, .enabled = true, .budget_us = 300U, .source_index = 3U},
            },
        .input_commands =
            std::vector<Command>{
                Command{.command_id = "cmd.move", .target_tick = 42U, .payload_hash = 1001U, .source_index = 1U},
                Command{.command_id = "cmd.interact", .target_tick = 43U, .payload_hash = 1002U, .source_index = 2U},
            },
    });

    GameplayRuntimeSchedulerProbeResult result;
    result.status = plan.status;
    result.available_steps = plan.available_step_count;
    result.planned_steps = plan.planned_step_count;
    result.step_rows = plan.steps.size();
    result.system_rows = plan.total_system_rows;
    result.command_rows = plan.total_command_rows;
    result.consumed_time_us = plan.consumed_time_us;
    result.remaining_time_us = plan.remaining_time_us;
    result.budget_limited = plan.status == Status::budget_limited;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.ready = plan.succeeded() && result.budget_limited && result.available_steps == 3U &&
                   result.planned_steps == 2U && result.step_rows == 2U && result.system_rows == 6U &&
                   result.command_rows == 2U && result.consumed_time_us == 33'332U &&
                   result.remaining_time_us == 16'666U && result.diagnostics == 0U && result.replay_hash != 0U;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeWorldEntityId make_rpg_entity(std::string value) {
    return mirakana::runtime::RuntimeWorldEntityId{.value = std::move(value)};
}

[[nodiscard]] RpgSystemsProbeResult validate_rpg_systems_package_evidence(std::string_view sample_id) {
    using Combat = mirakana::runtime::RuntimeRpgCombatLoopRequest;
    using Equipment = mirakana::runtime::RuntimeRpgEquipmentRow;
    using EquipmentStatus = mirakana::runtime::RuntimeRpgEquipmentStatus;
    using Progression = mirakana::runtime::RuntimeRpgProgressionRow;
    using Request = mirakana::runtime::RuntimeRpgSystemsRequest;
    using Reward = mirakana::runtime::RuntimeRpgRewardRow;
    using RewardKind = mirakana::runtime::RuntimeRpgRewardKind;
    using Save = mirakana::runtime::RuntimeRpgSaveValidationRow;
    using SaveStatus = mirakana::runtime::RuntimeRpgSaveValidationStatus;
    using Skill = mirakana::runtime::RuntimeRpgSkillRow;
    using SkillStatus = mirakana::runtime::RuntimeRpgSkillStatus;
    using Stat = mirakana::runtime::RuntimeRpgStatRow;
    using Status = mirakana::runtime::RuntimeRpgSystemsStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_rpg_systems(
            Request{
                .system_id = sample_prefix + ".rpg",
                .world_id = sample_prefix + ".world",
                .world_tick = 42U,
                .party_entity_ids = {make_rpg_entity("party.entity.0"), make_rpg_entity("party.entity.1")},
                .enemy_entity_ids = {make_rpg_entity("opponent.entity.0")},
                .stat_rows =
                    std::vector<Stat>{
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "health",
                             .current_value = 30,
                             .max_value = 30,
                             .source_index = 1U},
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "focus",
                             .current_value = 7,
                             .max_value = 10,
                             .source_index = 2U},
                        Stat{.entity_id = make_rpg_entity("party.entity.0"),
                             .stat_id = "initiative",
                             .current_value = 12,
                             .max_value = 12,
                             .source_index = 3U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "health",
                             .current_value = 24,
                             .max_value = 24,
                             .source_index = 4U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "focus",
                             .current_value = 2,
                             .max_value = 10,
                             .source_index = 5U},
                        Stat{.entity_id = make_rpg_entity("party.entity.1"),
                             .stat_id = "initiative",
                             .current_value = 8,
                             .max_value = 8,
                             .source_index = 6U},
                        Stat{.entity_id = make_rpg_entity("opponent.entity.0"),
                             .stat_id = "health",
                             .current_value = 18,
                             .max_value = 18,
                             .source_index = 7U},
                        Stat{.entity_id = make_rpg_entity("opponent.entity.0"),
                             .stat_id = "initiative",
                             .current_value = 10,
                             .max_value = 10,
                             .source_index = 8U},
                    },
                .progression_rows =
                    std::vector<Progression>{
                        Progression{.entity_id = make_rpg_entity("party.entity.0"),
                                    .track_id = "track.core",
                                    .level = 3U,
                                    .experience = 120U,
                                    .skill_points = 1U,
                                    .source_index = 1U},
                        Progression{.entity_id = make_rpg_entity("party.entity.1"),
                                    .track_id = "track.core",
                                    .level = 1U,
                                    .experience = 25U,
                                    .skill_points = 0U,
                                    .source_index = 2U},
                    },
                .skill_rows =
                    std::vector<Skill>{
                        Skill{.entity_id = make_rpg_entity("party.entity.0"),
                              .skill_id = "skill.primary",
                              .required_level = 2U,
                              .required_stat_id = "focus",
                              .required_stat_value = 6,
                              .status = SkillStatus::invalid,
                              .source_index = 1U},
                        Skill{.entity_id = make_rpg_entity("party.entity.1"),
                              .skill_id = "skill.support",
                              .required_level = 1U,
                              .required_stat_id = "focus",
                              .required_stat_value = 6,
                              .status = SkillStatus::invalid,
                              .source_index = 2U},
                    },
                .equipment_rows =
                    std::vector<Equipment>{
                        Equipment{.entity_id = make_rpg_entity("party.entity.0"),
                                  .slot_id = "slot.primary",
                                  .item_id = "item.primary",
                                  .required_level = 2U,
                                  .required_stat_id = "focus",
                                  .required_stat_value = 5,
                                  .status = EquipmentStatus::invalid,
                                  .source_index = 1U},
                        Equipment{.entity_id = make_rpg_entity("party.entity.1"),
                                  .slot_id = "slot.support",
                                  .item_id = "item.support",
                                  .required_level = 1U,
                                  .required_stat_id = "focus",
                                  .required_stat_value = 5,
                                  .status = EquipmentStatus::invalid,
                                  .source_index = 2U},
                    },
                .combat_request =
                    Combat{.encounter_id = "encounter.package", .initiative_stat_id = "initiative", .max_rounds = 2U},
                .reward_rows =
                    std::vector<Reward>{
                        Reward{.reward_id = "reward.progress",
                               .entity_id = make_rpg_entity("party.entity.0"),
                               .kind = RewardKind::experience,
                               .item_id = "",
                               .quantity = 50U,
                               .source_index = 1U},
                        Reward{.reward_id = "reward.item",
                               .entity_id = make_rpg_entity("party.entity.1"),
                               .kind = RewardKind::item,
                               .item_id = "item.resource",
                               .quantity = 1U,
                               .source_index = 2U},
                    },
                .save_validation_rows =
                    std::vector<Save>{
                        Save{.entity_id = make_rpg_entity("party.entity.0"),
                             .domain = "profile",
                             .key = "state.party.0",
                             .expected_schema_version = 1U,
                             .observed_schema_version = 1U,
                             .status = SaveStatus::rejected,
                             .source_index = 1U},
                        Save{.entity_id = make_rpg_entity("party.entity.1"),
                             .domain = "profile",
                             .key = "state.party.1",
                             .expected_schema_version = 2U,
                             .observed_schema_version = 1U,
                             .status = SaveStatus::rejected,
                             .source_index = 2U},
                    },
            });

    RpgSystemsProbeResult result;
    result.status = plan.status;
    result.party_members = plan.party_member_count;
    result.enemy_members = plan.enemy_member_count;
    result.stat_rows = plan.stat_count;
    result.progression_rows = plan.progression_count;
    result.skill_rows = plan.skill_rows.size();
    result.blocked_skill_rows = plan.blocked_skill_count;
    result.equipment_rows = plan.equipment_rows.size();
    result.blocked_equipment_rows = plan.blocked_equipment_count;
    result.combat_turn_rows = plan.combat_turn_count;
    result.combat_rounds = plan.combat_round_count;
    result.reward_rows = plan.reward_count;
    result.save_validation_rows = plan.save_validation_count;
    result.repairable_save_validation_rows = plan.repairable_save_validation_count;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_combat_execution = plan.invoked_combat_execution;
    result.invoked_reward_application = plan.invoked_reward_application;
    result.invoked_save_io = plan.invoked_save_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.party_members == 2U &&
                   result.enemy_members == 1U && result.stat_rows == 8U && result.progression_rows == 2U &&
                   result.skill_rows == 2U && result.blocked_skill_rows == 1U && result.equipment_rows == 2U &&
                   result.blocked_equipment_rows == 1U && result.combat_turn_rows == 6U && result.combat_rounds == 2U &&
                   result.reward_rows == 2U && result.save_validation_rows == 2U &&
                   result.repairable_save_validation_rows == 1U && result.diagnostics == 0U &&
                   result.replay_hash != 0U && !result.invoked_combat_execution && !result.invoked_reward_application &&
                   !result.invoked_save_io;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeSandboxCellCoord make_sandbox_cell(std::int32_t x, std::int32_t y,
                                                                           std::int32_t z) {
    return mirakana::runtime::RuntimeSandboxCellCoord{.x = x, .y = y, .z = z};
}

[[nodiscard]] SandboxWorldProbeResult validate_sandbox_world_package_evidence(std::string_view sample_id) {
    using Chunk = mirakana::runtime::RuntimeSandboxChunkRow;
    using Cost = mirakana::runtime::RuntimeSandboxConstructionCostRow;
    using Destruction = mirakana::runtime::RuntimeSandboxDestructionIntent;
    using ExistingCell = mirakana::runtime::RuntimeSandboxExistingCellRow;
    using Persistence = mirakana::runtime::RuntimeSandboxPersistenceRow;
    using PersistenceStatus = mirakana::runtime::RuntimeSandboxPersistenceStatus;
    using Placement = mirakana::runtime::RuntimeSandboxPlacementIntent;
    using Request = mirakana::runtime::RuntimeSandboxWorldMutationRequest;
    using Status = mirakana::runtime::RuntimeSandboxWorldStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_sandbox_world_mutation(
            Request{.world_id = sample_prefix + ".sandbox.world",
                    .world_tick = 64U,
                    .chunk_rows =
                        std::vector<Chunk>{
                            Chunk{.chunk_id = "chunk.0",
                                  .region_id = "region.spawn",
                                  .origin_x = 0,
                                  .origin_y = 0,
                                  .origin_z = 0,
                                  .size_x = 8U,
                                  .size_y = 8U,
                                  .size_z = 8U,
                                  .resident = true,
                                  .persistent = true,
                                  .source_index = 1U},
                            Chunk{.chunk_id = "chunk.1",
                                  .region_id = "region.spawn",
                                  .origin_x = 8,
                                  .origin_y = 0,
                                  .origin_z = 0,
                                  .size_x = 8U,
                                  .size_y = 8U,
                                  .size_z = 8U,
                                  .resident = true,
                                  .persistent = true,
                                  .source_index = 2U},
                        },
                    .existing_cell_rows =
                        std::vector<ExistingCell>{
                            ExistingCell{.chunk_id = "chunk.0",
                                         .coord = make_sandbox_cell(1, 0, 1),
                                         .block_id = "block.soil",
                                         .destructible = true,
                                         .protected_cell = false,
                                         .source_index = 1U},
                            ExistingCell{.chunk_id = "chunk.0",
                                         .coord = make_sandbox_cell(2, 0, 1),
                                         .block_id = "block.stone",
                                         .destructible = false,
                                         .protected_cell = true,
                                         .source_index = 2U},
                        },
                    .placement_intents =
                        std::vector<Placement>{
                            Placement{.intent_id = "place.wall",
                                      .chunk_id = "chunk.0",
                                      .coord = make_sandbox_cell(3, 0, 1),
                                      .block_id = "block.wall",
                                      .provided_costs = {Cost{.block_id = "block.wall",
                                                              .item_id = "item.wood",
                                                              .quantity = 3U,
                                                              .source_index = 1U}},
                                      .source_index = 1U},
                            Placement{.intent_id = "place.occupied",
                                      .chunk_id = "chunk.0",
                                      .coord = make_sandbox_cell(1, 0, 1),
                                      .block_id = "block.wall",
                                      .provided_costs = {Cost{.block_id = "block.wall",
                                                              .item_id = "item.wood",
                                                              .quantity = 3U,
                                                              .source_index = 2U}},
                                      .source_index = 2U},
                            Placement{.intent_id = "place.missing_cost",
                                      .chunk_id = "chunk.1",
                                      .coord = make_sandbox_cell(9, 0, 1),
                                      .block_id = "block.door",
                                      .source_index = 3U},
                        },
                    .destruction_intents =
                        std::vector<Destruction>{
                            Destruction{.intent_id = "destroy.soil",
                                        .chunk_id = "chunk.0",
                                        .coord = make_sandbox_cell(1, 0, 1),
                                        .source_index = 1U},
                            Destruction{.intent_id = "destroy.protected",
                                        .chunk_id = "chunk.0",
                                        .coord = make_sandbox_cell(2, 0, 1),
                                        .source_index = 2U},
                        },
                    .construction_cost_rows =
                        std::vector<Cost>{
                            Cost{.block_id = "block.wall", .item_id = "item.wood", .quantity = 3U, .source_index = 1U},
                            Cost{.block_id = "block.door", .item_id = "item.wood", .quantity = 4U, .source_index = 2U},
                        },
                    .persistence_rows = std::vector<Persistence>{
                        Persistence{.chunk_id = "chunk.0",
                                    .key = "persist.chunk.0",
                                    .expected_schema_version = 2U,
                                    .observed_schema_version = 2U,
                                    .status = PersistenceStatus::rejected,
                                    .source_index = 1U},
                        Persistence{.chunk_id = "chunk.1",
                                    .key = "persist.chunk.1",
                                    .expected_schema_version = 3U,
                                    .observed_schema_version = 2U,
                                    .status = PersistenceStatus::rejected,
                                    .source_index = 2U},
                    }});

    SandboxWorldProbeResult result;
    result.status = plan.status;
    result.chunk_rows = plan.chunk_count;
    result.resident_chunk_rows = plan.resident_chunk_count;
    result.existing_cell_rows = plan.existing_cell_rows.size();
    result.placement_intent_rows = plan.placement_intent_count;
    result.placement_accepted_rows = plan.accepted_placement_count;
    result.placement_rejected_rows = plan.rejected_placement_count;
    result.destruction_intent_rows = plan.destruction_intent_count;
    result.destruction_accepted_rows = plan.accepted_destruction_count;
    result.destruction_rejected_rows = plan.rejected_destruction_count;
    result.construction_cost_rows = plan.construction_cost_count;
    result.mutation_rows = plan.mutation_rows.size();
    result.persistence_rows = plan.persistence_row_count;
    result.persistence_repairable_rows = plan.repairable_persistence_row_count;
    result.rejected_unsafe_mutation_rows = plan.rejected_unsafe_mutation_count;
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_world_mutation = plan.invoked_world_mutation;
    result.invoked_persistence_io = plan.invoked_persistence_io;
    result.invoked_package_io = plan.invoked_package_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.chunk_rows == 2U &&
                   result.resident_chunk_rows == 2U && result.existing_cell_rows == 2U &&
                   result.placement_intent_rows == 3U && result.placement_accepted_rows == 1U &&
                   result.placement_rejected_rows == 2U && result.destruction_intent_rows == 2U &&
                   result.destruction_accepted_rows == 1U && result.destruction_rejected_rows == 1U &&
                   result.construction_cost_rows == 2U && result.mutation_rows == 5U && result.persistence_rows == 2U &&
                   result.persistence_repairable_rows == 1U && result.rejected_unsafe_mutation_rows == 3U &&
                   result.diagnostics == 0U && result.replay_hash != 0U && !result.invoked_world_mutation &&
                   !result.invoked_persistence_io && !result.invoked_package_io;
    return result;
}

[[nodiscard]] SimulationManagementProbeResult
validate_simulation_management_package_evidence(std::string_view sample_id) {
    using Economy = mirakana::runtime::RuntimeSimulationEconomySummary;
    using Job = mirakana::runtime::RuntimeSimulationJobRow;
    using JobStatus = mirakana::runtime::RuntimeSimulationJobStatus;
    using Link = mirakana::runtime::RuntimeSimulationLogisticsLink;
    using Need = mirakana::runtime::RuntimeSimulationPopulationNeedRow;
    using NeedStatus = mirakana::runtime::RuntimeSimulationNeedStatus;
    using Request = mirakana::runtime::RuntimeSimulationManagementRequest;
    using Resource = mirakana::runtime::RuntimeSimulationResourceRow;
    using Save = mirakana::runtime::RuntimeSimulationSaveReviewRow;
    using SaveStatus = mirakana::runtime::RuntimeSimulationSaveReviewStatus;
    using Schedule = mirakana::runtime::RuntimeSimulationScheduleRow;
    using Status = mirakana::runtime::RuntimeSimulationManagementStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan = mirakana::runtime::plan_runtime_simulation_management(Request{
        .simulation_id = sample_prefix + ".simulation.management",
        .world_tick = 100U,
        .long_run_tick_count = 240U,
        .resource_rows =
            std::vector<Resource>{
                Resource{
                    .resource_id = "food", .storage_id = "colony", .quantity = 25, .capacity = 100, .source_index = 1U},
                Resource{
                    .resource_id = "meal", .storage_id = "colony", .quantity = 2, .capacity = 30, .source_index = 2U},
                Resource{
                    .resource_id = "ore", .storage_id = "mine", .quantity = 12, .capacity = 40, .source_index = 3U},
                Resource{
                    .resource_id = "ore", .storage_id = "colony", .quantity = 0, .capacity = 40, .source_index = 4U},
            },
        .job_rows =
            std::vector<Job>{
                Job{.job_id = "job.cook",
                    .worker_id = "worker.0",
                    .input_resource_id = "food",
                    .input_storage_id = "colony",
                    .output_resource_id = "meal",
                    .output_storage_id = "colony",
                    .input_quantity = 5,
                    .output_quantity = 3,
                    .duration_ticks = 4U,
                    .status = JobStatus::invalid,
                    .source_index = 1U},
                Job{.job_id = "job.shortage",
                    .worker_id = "worker.1",
                    .input_resource_id = "ore",
                    .input_storage_id = "mine",
                    .output_resource_id = "meal",
                    .output_storage_id = "colony",
                    .input_quantity = 99,
                    .output_quantity = 1,
                    .duration_ticks = 4U,
                    .status = JobStatus::invalid,
                    .source_index = 2U},
            },
        .logistics_links =
            std::vector<Link>{
                Link{.link_id = "route.ore",
                     .resource_id = "ore",
                     .source_storage_id = "mine",
                     .destination_storage_id = "colony",
                     .transfer_quantity = 4,
                     .travel_ticks = 6U,
                     .enabled = true,
                     .source_index = 1U},
                Link{.link_id = "route.ore.back",
                     .resource_id = "ore",
                     .source_storage_id = "colony",
                     .destination_storage_id = "mine",
                     .transfer_quantity = 99,
                     .travel_ticks = 6U,
                     .enabled = true,
                     .source_index = 2U},
            },
        .economy_summaries = std::vector<Economy>{Economy{.summary_id = "economy.food",
                                                          .resource_id = "food",
                                                          .produced_quantity = 6,
                                                          .consumed_quantity = 5,
                                                          .traded_quantity = 0,
                                                          .source_index = 1U}},
        .population_need_rows =
            std::vector<Need>{
                Need{.population_id = "population.colony",
                     .need_id = "need.food",
                     .resource_id = "food",
                     .storage_id = "colony",
                     .required_quantity = 20,
                     .available_quantity = 0,
                     .status = NeedStatus::invalid,
                     .source_index = 1U},
                Need{.population_id = "population.colony",
                     .need_id = "need.comfort",
                     .resource_id = "meal",
                     .storage_id = "colony",
                     .required_quantity = 5,
                     .available_quantity = 0,
                     .status = NeedStatus::invalid,
                     .source_index = 2U},
            },
        .schedule_rows =
            std::vector<Schedule>{
                Schedule{.schedule_id = "schedule.job.cook",
                         .target_id = "job.cook",
                         .start_tick = 90U,
                         .end_tick = 200U,
                         .enabled = true,
                         .source_index = 1U},
                Schedule{.schedule_id = "schedule.route.ore",
                         .target_id = "route.ore",
                         .start_tick = 90U,
                         .end_tick = 200U,
                         .enabled = true,
                         .source_index = 2U},
            },
        .save_review_rows =
            std::vector<Save>{
                Save{.domain = "simulation",
                     .key = "state",
                     .expected_schema_version = 2U,
                     .observed_schema_version = 2U,
                     .status = SaveStatus::rejected,
                     .source_index = 1U},
                Save{.domain = "simulation",
                     .key = "balances",
                     .expected_schema_version = 3U,
                     .observed_schema_version = 2U,
                     .status = SaveStatus::rejected,
                     .source_index = 2U},
            },
        .seed = 42U,
    });

    SimulationManagementProbeResult result;
    result.status = plan.status;
    result.tick_count = plan.tick_count;
    result.resource_balance_rows = plan.resource_balance_rows.size();
    result.job_rows = plan.job_rows.size();
    result.job_assignment_rows = plan.job_assignment_count;
    result.logistics_links = plan.logistics_links.size();
    result.logistics_transfer_rows = plan.logistics_transfer_rows.size();
    result.logistics_scheduled_transfer_rows = plan.scheduled_logistics_transfer_count;
    result.economy_summary_rows = plan.economy_summaries.size();
    result.population_need_rows = plan.population_need_rows.size();
    result.need_deficit_rows = plan.need_deficit_count;
    result.schedule_rows = plan.schedule_rows.size();
    result.save_review_rows = plan.save_review_rows.size();
    result.save_review_repairable_rows = plan.repairable_save_review_count;
    result.dashboard_rows = plan.dashboard_rows.size();
    result.diagnostics = plan.diagnostics.size();
    result.replay_hash = plan.replay_hash;
    result.invoked_economy_execution = plan.invoked_economy_execution;
    result.invoked_save_io = plan.invoked_save_io;
    result.invoked_runtime_ui = plan.invoked_runtime_ui;
    result.invoked_package_io = plan.invoked_package_io;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.tick_count == 240U &&
                   result.resource_balance_rows == 4U && result.job_rows == 2U && result.job_assignment_rows == 1U &&
                   result.logistics_links == 2U && result.logistics_transfer_rows == 2U &&
                   result.logistics_scheduled_transfer_rows == 1U && result.economy_summary_rows == 1U &&
                   result.population_need_rows == 2U && result.need_deficit_rows == 1U && result.schedule_rows == 2U &&
                   result.save_review_rows == 2U && result.save_review_repairable_rows == 1U &&
                   result.dashboard_rows == 7U && result.diagnostics == 0U && result.replay_hash != 0U &&
                   !result.invoked_economy_execution && !result.invoked_save_io && !result.invoked_runtime_ui &&
                   !result.invoked_package_io;
    return result;
}

[[nodiscard]] mirakana::runtime::RuntimeNetworkFoundationPlan make_network_replication_foundation_plan() {
    using Authority = mirakana::runtime::RuntimeNetworkReplicationAuthority;
    using Capability = mirakana::runtime::RuntimeNetworkTransportCapabilityKind;
    using Delivery = mirakana::runtime::RuntimeNetworkReplicationDelivery;
    using Role = mirakana::runtime::RuntimeNetworkLocalRole;
    using Topology = mirakana::runtime::RuntimeNetworkSessionTopology;
    using Trust = mirakana::runtime::RuntimeNetworkTrustBoundary;

    return mirakana::runtime::plan_runtime_network_foundation(
        mirakana::runtime::RuntimeNetworkFoundationPolicyDesc{
            .sessions =
                std::vector<mirakana::runtime::RuntimeNetworkSessionDesc>{
                    mirakana::runtime::RuntimeNetworkSessionDesc{
                        .session_id = "arena",
                        .topology = Topology::listen_server,
                        .local_role = Role::host,
                        .trust_boundary = Trust::untrusted_remote_peers,
                        .transports =
                            std::vector<mirakana::runtime::RuntimeNetworkTransportRequirementDesc>{
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::reliable_ordered, .source_index = 1U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::unreliable_unordered, .source_index = 2U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::encrypted_transport, .source_index = 3U},
                                mirakana::runtime::RuntimeNetworkTransportRequirementDesc{
                                    .capability = Capability::authenticated_peer, .source_index = 4U},
                            },
                        .channels =
                            std::vector<mirakana::runtime::RuntimeNetworkReplicationChannelDesc>{
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "state",
                                    .authority = Authority::server,
                                    .delivery = Delivery::state_snapshot,
                                    .tick_rate_hz = 60U,
                                    .source_index = 5U,
                                },
                                mirakana::runtime::RuntimeNetworkReplicationChannelDesc{
                                    .channel_id = "input",
                                    .authority = Authority::client,
                                    .delivery = Delivery::unreliable_unordered,
                                    .tick_rate_hz = 60U,
                                    .source_index = 6U,
                                },
                            },
                        .replay =
                            mirakana::runtime::RuntimeNetworkReplayPrerequisiteDesc{
                                .replay_seed = 42U,
                                .fixed_tick_rate_hz = 60U,
                                .deterministic_simulation = true,
                                .ordered_inputs = true,
                                .source_index = 7U,
                            },
                        .source_index = 8U,
                    },
                },
            .reviewed_transport_capabilities =
                {
                    Capability::reliable_ordered,
                    Capability::unreliable_unordered,
                    Capability::encrypted_transport,
                    Capability::authenticated_peer,
                },
        });
}

[[nodiscard]] NetworkReplicationProbeResult validate_network_replication_package_evidence(std::string_view sample_id) {
    using Mode = mirakana::runtime::RuntimeNetworkReplicationMode;
    using Ownership = mirakana::runtime::RuntimeReplicationOwnership;
    using Request = mirakana::runtime::RuntimeNetworkReplicationRequest;
    using RollbackMode = mirakana::runtime::RuntimeRollbackMode;
    using SnapshotKind = mirakana::runtime::RuntimeReplicationSnapshotKind;
    using Status = mirakana::runtime::RuntimeNetworkReplicationStatus;

    const auto sample_prefix = std::string{sample_id};
    const auto plan =
        mirakana::runtime::plan_runtime_network_replication(
            Request{.foundation_plan = make_network_replication_foundation_plan(),
                    .session =
                        mirakana::runtime::RuntimeNetworkReplicationSessionDesc{
                            .session_id = "arena",
                            .world_id = sample_prefix + ".network.world",
                            .mode = Mode::authoritative_snapshot,
                            .fixed_tick_rate_hz = 60U,
                            .max_players = 4U,
                            .max_objects = 16U,
                            .source_index = 1U,
                        },
                    .object_rows =
                        std::vector<mirakana::runtime::RuntimeReplicatedObjectRow>{
                            mirakana::runtime::RuntimeReplicatedObjectRow{
                                .object_id = "player.0",
                                .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "entity.player0"},
                                .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
                                .schema_id =
                                    mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
                                .channel_id = "state",
                                .ownership = Ownership::server_owned,
                                .priority = 10U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicatedObjectRow{
                                .object_id = "crate.0",
                                .entity_id = mirakana::runtime::RuntimeWorldEntityId{.value = "entity.crate0"},
                                .region_id = mirakana::runtime::RuntimeWorldRegionId{.value = "region.arena"},
                                .schema_id =
                                    mirakana::runtime::RuntimeWorldComponentSchemaId{.value = "schema.transform"},
                                .channel_id = "state",
                                .ownership = Ownership::server_owned,
                                .priority = 8U,
                                .source_index = 2U,
                            },
                        },
                    .input_rows =
                        std::vector<mirakana::runtime::RuntimeReplicationInputCommandRow>{
                            mirakana::runtime::RuntimeReplicationInputCommandRow{
                                .player_id = "player.0",
                                .command_id = "move.left",
                                .channel_id = "input",
                                .target_tick = 100U,
                                .sequence = 1U,
                                .payload_hash = 1001U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicationInputCommandRow{
                                .player_id = "player.0",
                                .command_id = "move.right",
                                .channel_id = "input",
                                .target_tick = 101U,
                                .sequence = 2U,
                                .payload_hash = 1002U,
                                .source_index = 2U,
                            },
                        },
                    .snapshot_rows =
                        std::vector<mirakana::runtime::RuntimeReplicationSnapshotRow>{
                            mirakana::runtime::RuntimeReplicationSnapshotRow{
                                .snapshot_id = "snapshot.100",
                                .channel_id = "state",
                                .tick = 100U,
                                .kind = SnapshotKind::full_state,
                                .object_ids = {"player.0", "crate.0"},
                                .state_hash = 9001U,
                                .byte_count = 256U,
                                .source_index = 1U,
                            },
                            mirakana::runtime::RuntimeReplicationSnapshotRow{
                                .snapshot_id = "snapshot.101",
                                .channel_id = "state",
                                .tick = 101U,
                                .kind = SnapshotKind::delta_state,
                                .object_ids = {"player.0", "crate.0"},
                                .state_hash = 9002U,
                                .byte_count = 128U,
                                .source_index = 2U,
                            },
                        },
                    .rollback_policy =
                        mirakana::runtime::RuntimeRollbackPolicyRow{
                            .mode = RollbackMode::input_resimulation,
                            .max_rollback_ticks = 8U,
                            .input_delay_ticks = 2U,
                            .snapshot_history_limit = 16U,
                            .requires_deterministic_simulation = true,
                            .requires_ordered_inputs = true,
                            .requires_transport_host_evidence = true,
                            .source_index = 20U,
                        },
                    .row_budget = 32U,
                    .snapshot_byte_budget = 1024U,
                    .seed = 99U});

    NetworkReplicationProbeResult result;
    result.status = plan.status;
    result.object_rows = plan.replicated_object_count;
    result.input_rows = plan.input_row_count;
    result.snapshot_rows = plan.snapshot_row_count;
    result.rollback_rows = plan.rollback_row_count;
    result.rejected_unsafe_rows = plan.rejected_unsafe_row_count;
    result.replay_hash = plan.replay_hash;
    result.requires_transport_host_evidence = plan.requires_transport_host_evidence;
    result.has_transport_host_evidence = plan.has_transport_host_evidence;
    result.invoked_network_io = plan.invoked_network_io;
    result.invoked_rollback_execution = plan.invoked_rollback_execution;
    result.invoked_world_mutation = plan.invoked_world_mutation;
    result.diagnostics = plan.diagnostics.size();
    result.reviewed = plan.status == Status::host_evidence_required && result.object_rows == 2U &&
                      result.input_rows == 2U && result.snapshot_rows == 2U && result.rollback_rows == 1U &&
                      result.rejected_unsafe_rows == 0U && result.replay_hash != 0U &&
                      result.requires_transport_host_evidence && !result.has_transport_host_evidence &&
                      !result.invoked_network_io && !result.invoked_rollback_execution &&
                      !result.invoked_world_mutation && result.diagnostics == 0U;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.object_rows == 2U &&
                   result.input_rows == 2U && result.snapshot_rows == 2U && result.rollback_rows == 1U &&
                   result.rejected_unsafe_rows == 0U && result.replay_hash != 0U &&
                   result.requires_transport_host_evidence && result.has_transport_host_evidence &&
                   !result.invoked_network_io && !result.invoked_rollback_execution && !result.invoked_world_mutation &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] RenderingVfxProfilingProbeResult validate_rendering_vfx_profiling_package_evidence() {
    using Backend = mirakana::rhi::BackendKind;
    using FeatureKind = mirakana::RendererProductionVfxFeatureKind;
    using Request = mirakana::RendererProductionVfxProfilingRequest;
    using Status = mirakana::RendererProductionVfxProfilingStatus;

    const auto
        plan =
            mirakana::plan_renderer_production_vfx_profiling(
                Request{
                    .required_backends =
                        {
                            Backend::d3d12,
                            Backend::vulkan,
                            Backend::metal,
                        },
                    .feature_rows =
                        {
                            mirakana::RendererProductionVfxFeatureRow{
                                .feature_id = "vfx.particles.spark",
                                .kind = FeatureKind::gpu_particles,
                                .backend = Backend::d3d12,
                                .reviewed = true,
                                .request_broad_performance_claim = false,
                                .request_native_handle_access = false,
                                .source_index = 1U,
                            },
                            mirakana::RendererProductionVfxFeatureRow{
                                .feature_id = "vfx.particles.spark",
                                .kind = FeatureKind::gpu_particles,
                                .backend = Backend::vulkan,
                                .reviewed = true,
                                .request_broad_performance_claim = false,
                                .request_native_handle_access = false,
                                .source_index = 2U,
                            },
                            mirakana::RendererProductionVfxFeatureRow{
                                .feature_id = "vfx.particles.spark",
                                .kind = FeatureKind::gpu_particles,
                                .backend = Backend::metal,
                                .reviewed = true,
                                .request_broad_performance_claim = false,
                                .request_native_handle_access = false,
                                .source_index = 3U,
                            },
                        },
                    .gpu_particle_budget_rows =
                        {
                            mirakana::RendererProductionGpuParticleBudgetRow{
                                .effect_id = "vfx.particles.spark",
                                .backend = Backend::d3d12,
                                .max_particles = 4096U,
                                .max_emitters = 16U,
                                .max_spawn_per_frame = 256U,
                                .simulation_budget_us = 700U,
                                .submission_budget_us = 300U,
                                .requires_compute_simulation = true,
                                .requires_gpu_sort = true,
                                .source_index = 4U,
                            },
                            mirakana::RendererProductionGpuParticleBudgetRow{
                                .effect_id = "vfx.particles.spark",
                                .backend = Backend::vulkan,
                                .max_particles = 4096U,
                                .max_emitters = 16U,
                                .max_spawn_per_frame = 256U,
                                .simulation_budget_us = 700U,
                                .submission_budget_us = 300U,
                                .requires_compute_simulation = true,
                                .requires_gpu_sort = true,
                                .source_index = 5U,
                            },
                            mirakana::RendererProductionGpuParticleBudgetRow{
                                .effect_id = "vfx.particles.spark",
                                .backend = Backend::metal,
                                .max_particles = 4096U,
                                .max_emitters = 16U,
                                .max_spawn_per_frame = 256U,
                                .simulation_budget_us = 700U,
                                .submission_budget_us = 300U,
                                .requires_compute_simulation = true,
                                .requires_gpu_sort = true,
                                .source_index = 6U,
                            },
                        },
                    .postprocess_rows =
                        {
                            mirakana::RendererProductionPostprocessRow{
                                .chain_id = "post.fx",
                                .backend = Backend::d3d12,
                                .pass_count = 3U,
                                .scene_color_available = true,
                                .scene_depth_available = true,
                                .hdr_available = true,
                                .history_available = true,
                                .backend_shader_evidence_ready = true,
                                .source_index = 7U,
                            },
                            mirakana::RendererProductionPostprocessRow{
                                .chain_id = "post.fx",
                                .backend = Backend::vulkan,
                                .pass_count = 3U,
                                .scene_color_available = true,
                                .scene_depth_available = true,
                                .hdr_available = true,
                                .history_available = true,
                                .backend_shader_evidence_ready = true,
                                .source_index = 8U,
                            },
                            mirakana::RendererProductionPostprocessRow{
                                .chain_id = "post.fx",
                                .backend = Backend::metal,
                                .pass_count = 3U,
                                .scene_color_available = true,
                                .scene_depth_available = true,
                                .hdr_available = true,
                                .history_available = true,
                                .backend_shader_evidence_ready = true,
                                .source_index = 9U,
                            },
                        },
                    .backend_timing_rows =
                        {
                            mirakana::RendererProductionBackendTimingRow{
                                .backend = Backend::d3d12,
                                .profile_zone_id = "frame.main",
                                .gpu_timestamp_frequency_hz = 1000000000ULL,
                                .begin_tick = 1000ULL,
                                .end_tick = 1900ULL,
                                .calibrated_cpu_begin_tick = 2000ULL,
                                .calibrated_cpu_end_tick = 2900ULL,
                                .max_clock_deviation_ns = 250ULL,
                                .debug_scope_count = 2U,
                                .debug_marker_count = 1U,
                                .resource_barrier_count = 4U,
                                .layout_transition_count = 3U,
                                .queue_wait_count = 1U,
                                .queue_ownership_transfer_reviewed = true,
                                .shader_validation_count = 2U,
                                .backend_validation_ready = true,
                                .strict_host_recipe_ready = true,
                                .capture_handoff_ready = true,
                                .host_validated = true,
                                .source_index = 10U,
                            },
                            mirakana::RendererProductionBackendTimingRow{
                                .backend = Backend::vulkan,
                                .profile_zone_id = "frame.main",
                                .gpu_timestamp_frequency_hz = 1000000000ULL,
                                .begin_tick = 3000ULL,
                                .end_tick = 3900ULL,
                                .calibrated_cpu_begin_tick = 4000ULL,
                                .calibrated_cpu_end_tick = 4900ULL,
                                .max_clock_deviation_ns = 250ULL,
                                .debug_scope_count = 2U,
                                .debug_marker_count = 1U,
                                .resource_barrier_count = 4U,
                                .layout_transition_count = 3U,
                                .queue_wait_count = 1U,
                                .queue_ownership_transfer_reviewed = true,
                                .shader_validation_count = 2U,
                                .backend_validation_ready = true,
                                .strict_host_recipe_ready = true,
                                .capture_handoff_ready = true,
                                .host_validated = true,
                                .source_index = 11U,
                            },
                            mirakana::RendererProductionBackendTimingRow{
                                .backend = Backend::metal,
                                .profile_zone_id = "frame.main",
                                .gpu_timestamp_frequency_hz = 1000000000ULL,
                                .begin_tick = 5000ULL,
                                .end_tick = 5900ULL,
                                .calibrated_cpu_begin_tick = 6000ULL,
                                .calibrated_cpu_end_tick = 6900ULL,
                                .max_clock_deviation_ns = 250ULL,
                                .debug_scope_count = 2U,
                                .debug_marker_count = 1U,
                                .resource_barrier_count = 4U,
                                .layout_transition_count = 3U,
                                .queue_wait_count = 1U,
                                .queue_ownership_transfer_reviewed = true,
                                .shader_validation_count = 2U,
                                .backend_validation_ready = true,
                                .strict_host_recipe_ready = false,
                                .capture_handoff_ready = false,
                                .host_validated = false,
                                .source_index = 12U,
                            },
                        },
                    .cpu_profile_rows =
                        {
                            mirakana::RendererProductionCpuProfileRow{
                                .backend = Backend::d3d12,
                                .profile_zone_id = "frame.main.cpu",
                                .begin_tick = 2000ULL,
                                .end_tick = 2900ULL,
                                .budget_us = 1200U,
                                .sample_count = 4U,
                                .source_index = 13U,
                            },
                            mirakana::RendererProductionCpuProfileRow{
                                .backend = Backend::vulkan,
                                .profile_zone_id = "frame.main.cpu",
                                .begin_tick = 4000ULL,
                                .end_tick = 4900ULL,
                                .budget_us = 1200U,
                                .sample_count = 4U,
                                .source_index = 14U,
                            },
                            mirakana::RendererProductionCpuProfileRow{
                                .backend = Backend::metal,
                                .profile_zone_id = "frame.main.cpu",
                                .begin_tick = 6000ULL,
                                .end_tick = 6900ULL,
                                .budget_us = 1200U,
                                .sample_count = 4U,
                                .source_index = 15U,
                            },
                        },
                    .package_counter_rows =
                        {
                            mirakana::RendererProductionPackageCounterRow{
                                .backend = Backend::d3d12,
                                .counter_id = "rendering_vfx_profiling.d3d12.backend_ready",
                                .ready = true,
                                .host_gated = false,
                                .source_index = 16U,
                            },
                            mirakana::RendererProductionPackageCounterRow{
                                .backend = Backend::vulkan,
                                .counter_id = "rendering_vfx_profiling.vulkan.backend_ready",
                                .ready = true,
                                .host_gated = false,
                                .source_index = 17U,
                            },
                            mirakana::RendererProductionPackageCounterRow{
                                .backend = Backend::metal,
                                .counter_id = "rendering_vfx_profiling.metal.host_gated",
                                .ready = false,
                                .host_gated = true,
                                .source_index = 18U,
                            },
                        },
                    .crash_telemetry_handoff_rows =
                        {
                            mirakana::RendererProductionCrashTelemetryHandoffRow{
                                .handoff_id = "handoff.primary",
                                .backend = Backend::d3d12,
                                .trace_event_count = 8U,
                                .crash_dump_reviewed = true,
                                .symbolication_ready = true,
                                .telemetry_schema_reviewed = true,
                                .operator_handoff_ready = true,
                                .request_crash_upload = false,
                                .request_native_capture = false,
                                .source_index = 19U,
                            },
                            mirakana::RendererProductionCrashTelemetryHandoffRow{
                                .handoff_id = "handoff.strict_vulkan",
                                .backend = Backend::vulkan,
                                .trace_event_count = 8U,
                                .crash_dump_reviewed = true,
                                .symbolication_ready = true,
                                .telemetry_schema_reviewed = true,
                                .operator_handoff_ready = true,
                                .request_crash_upload = false,
                                .request_native_capture = false,
                                .source_index = 20U,
                            },
                            mirakana::RendererProductionCrashTelemetryHandoffRow{
                                .handoff_id = "handoff.apple",
                                .backend = Backend::metal,
                                .trace_event_count = 8U,
                                .crash_dump_reviewed = true,
                                .symbolication_ready = true,
                                .telemetry_schema_reviewed = true,
                                .operator_handoff_ready = true,
                                .request_crash_upload = false,
                                .request_native_capture = false,
                                .source_index = 21U,
                            },
                        },
                    .row_budget = 32U,
                    .seed = 123U,
                });

    RenderingVfxProfilingProbeResult result;
    result.status = plan.status;
    result.feature_rows = plan.feature_row_count;
    result.gpu_particle_budget_rows = plan.gpu_particle_budget_row_count;
    result.postprocess_rows = plan.postprocess_row_count;
    result.backend_timing_rows = plan.backend_timing_row_count;
    result.backend_evidence_rows = plan.backend_evidence_row_count;
    result.backend_evidence_ready = plan.backend_evidence_ready_count;
    result.backend_evidence_host_gated = plan.backend_evidence_host_gated_count;
    result.cpu_profile_rows = plan.cpu_profile_row_count;
    result.package_counter_rows = plan.package_counter_row_count;
    result.package_counter_ready = plan.package_counter_ready_count;
    result.package_counter_host_gated = plan.package_counter_host_gated_count;
    result.crash_telemetry_handoff_rows = plan.crash_telemetry_handoff_row_count;
    result.host_validated_backends = plan.host_validated_backend_count;
    result.rejected_unsafe_rows = plan.rejected_unsafe_row_count;
    result.replay_hash = plan.replay_hash;
    result.d3d12_host_evidence_ready = plan.d3d12_host_evidence_ready;
    result.vulkan_strict_host_evidence_ready = plan.vulkan_strict_host_evidence_ready;
    result.metal_host_evidence_ready = plan.metal_host_evidence_ready;
    result.requires_metal_host_evidence = plan.requires_metal_host_evidence;
    result.has_metal_host_evidence = plan.has_metal_host_evidence;
    result.invoked_gpu_commands = plan.invoked_gpu_commands;
    result.invoked_native_capture = plan.invoked_native_capture;
    result.invoked_crash_upload = plan.invoked_crash_upload;
    result.diagnostics = plan.diagnostics.size();
    result.reviewed = plan.status == Status::host_evidence_required && result.feature_rows == 3U &&
                      result.gpu_particle_budget_rows == 3U && result.postprocess_rows == 3U &&
                      result.backend_timing_rows == 3U && result.backend_evidence_rows == 3U &&
                      result.backend_evidence_ready == 2U && result.backend_evidence_host_gated == 1U &&
                      result.cpu_profile_rows == 3U && result.package_counter_rows == 3U &&
                      result.package_counter_ready == 2U && result.package_counter_host_gated == 1U &&
                      result.crash_telemetry_handoff_rows == 3U && result.host_validated_backends == 2U &&
                      result.rejected_unsafe_rows == 0U && result.replay_hash != 0U &&
                      result.d3d12_host_evidence_ready && result.vulkan_strict_host_evidence_ready &&
                      !result.metal_host_evidence_ready && result.requires_metal_host_evidence &&
                      !result.has_metal_host_evidence && !result.invoked_gpu_commands &&
                      !result.invoked_native_capture && !result.invoked_crash_upload && result.diagnostics == 0U;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.feature_rows == 3U &&
                   result.gpu_particle_budget_rows == 3U && result.postprocess_rows == 3U &&
                   result.backend_timing_rows == 3U && result.backend_evidence_rows == 3U &&
                   result.backend_evidence_ready == 3U && result.backend_evidence_host_gated == 0U &&
                   result.cpu_profile_rows == 3U && result.package_counter_rows == 3U &&
                   result.package_counter_ready == 3U && result.package_counter_host_gated == 0U &&
                   result.crash_telemetry_handoff_rows == 3U && result.host_validated_backends == 3U &&
                   result.rejected_unsafe_rows == 0U && result.replay_hash != 0U && result.d3d12_host_evidence_ready &&
                   result.vulkan_strict_host_evidence_ready && result.metal_host_evidence_ready &&
                   result.requires_metal_host_evidence && result.has_metal_host_evidence &&
                   !result.invoked_gpu_commands && !result.invoked_native_capture && !result.invoked_crash_upload &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::string renderer_quality_matrix_feature_id(mirakana::RendererQualityFeatureKind feature) {
    switch (feature) {
    case mirakana::RendererQualityFeatureKind::materials:
        return "materials.lit";
    case mirakana::RendererQualityFeatureKind::lighting_shadows:
        return "lighting.directional_shadow";
    case mirakana::RendererQualityFeatureKind::postprocess:
        return "postprocess.depth_aware";
    case mirakana::RendererQualityFeatureKind::sprite_ui:
        return "sprite_ui.atlas_overlay";
    case mirakana::RendererQualityFeatureKind::scene_scale:
        return "scene_scale.visibility_budget";
    case mirakana::RendererQualityFeatureKind::gpu_memory_residency:
        return "gpu_memory.residency_budget";
    case mirakana::RendererQualityFeatureKind::profiling_capture:
        return "profiling.capture_handoff";
    }
    return "unknown";
}

[[nodiscard]] mirakana::RendererQualityMatrixRow
make_renderer_quality_matrix_ready_row(mirakana::RendererQualityFeatureKind feature, mirakana::rhi::BackendKind backend,
                                       std::uint32_t source_index) {
    const auto is_d3d12 = backend == mirakana::rhi::BackendKind::d3d12;
    const auto is_vulkan = backend == mirakana::rhi::BackendKind::vulkan;
    const auto is_metal = backend == mirakana::rhi::BackendKind::metal;
    const auto feature_id = renderer_quality_matrix_feature_id(feature);
    return mirakana::RendererQualityMatrixRow{
        .feature_id = feature_id,
        .feature = feature,
        .backend = backend,
        .proof = mirakana::RendererQualityProofKind::selected_package,
        .status = mirakana::RendererQualityRowStatus::ready,
        .evidence_categories =
            {
                mirakana::RendererQualityEvidenceCategory::synchronization,
                mirakana::RendererQualityEvidenceCategory::shader_tool_validation,
                mirakana::RendererQualityEvidenceCategory::memory_residency,
                mirakana::RendererQualityEvidenceCategory::render_pass_frame_graph,
                mirakana::RendererQualityEvidenceCategory::profiling,
                mirakana::RendererQualityEvidenceCategory::package_evidence,
            },
        .dependency_gate_id = {},
        .unsupported_claim_id = {},
        .notes = "backend-local package renderer quality evidence",
        .reviewed = true,
        .backend_local_evidence = true,
        .d3d12_resource_state_barrier_evidence = is_d3d12,
        .d3d12_fence_evidence = is_d3d12,
        .vulkan_synchronization2_evidence = is_vulkan,
        .vulkan_layout_transition_evidence = is_vulkan,
        .vulkan_validation_layer_evidence = is_vulkan,
        .vulkan_spirv_validation_evidence = is_vulkan,
        .metal_resource_synchronization_evidence = is_metal,
        .metal_feature_set_evidence = is_metal,
        .shader_tool_validation_evidence = true,
        .package_counter_ids = {std::string{"renderer_quality_matrix."} + feature_id},
        .timing_budget_us = 1000U,
        .gpu_memory_evidence = true,
        .backend_parity_evidence = true,
        .host_validated = true,
        .host_gate_required = false,
        .request_native_handle_access = false,
        .request_capture_execution = false,
        .request_crash_upload_execution = false,
        .request_inferred_backend_parity = false,
        .request_subjective_visual_quality_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::RendererQualityMatrixRow
make_renderer_quality_matrix_metal_host_gated_row(mirakana::RendererQualityFeatureKind feature,
                                                  std::uint32_t source_index) {
    return mirakana::RendererQualityMatrixRow{
        .feature_id = renderer_quality_matrix_feature_id(feature),
        .feature = feature,
        .backend = mirakana::rhi::BackendKind::metal,
        .proof = mirakana::RendererQualityProofKind::host_gate,
        .status = mirakana::RendererQualityRowStatus::host_gated,
        .evidence_categories =
            {
                mirakana::RendererQualityEvidenceCategory::host_gate,
            },
        .dependency_gate_id = {},
        .unsupported_claim_id = {},
        .notes = "Metal quality row requires Apple host evidence",
        .reviewed = true,
        .backend_local_evidence = true,
        .d3d12_resource_state_barrier_evidence = false,
        .d3d12_fence_evidence = false,
        .vulkan_synchronization2_evidence = false,
        .vulkan_layout_transition_evidence = false,
        .vulkan_validation_layer_evidence = false,
        .vulkan_spirv_validation_evidence = false,
        .metal_resource_synchronization_evidence = false,
        .metal_feature_set_evidence = false,
        .shader_tool_validation_evidence = false,
        .package_counter_ids = {},
        .timing_budget_us = 0U,
        .gpu_memory_evidence = false,
        .backend_parity_evidence = false,
        .host_validated = false,
        .host_gate_required = true,
        .request_native_handle_access = false,
        .request_capture_execution = false,
        .request_crash_upload_execution = false,
        .request_inferred_backend_parity = false,
        .request_subjective_visual_quality_claim = false,
        .source_index = source_index,
    };
}

[[nodiscard]] RendererQualityMatrixProbeResult validate_renderer_quality_matrix_package_evidence() {
    constexpr mirakana::RendererQualityFeatureKind required_features[] = {
        mirakana::RendererQualityFeatureKind::materials,
        mirakana::RendererQualityFeatureKind::lighting_shadows,
        mirakana::RendererQualityFeatureKind::postprocess,
        mirakana::RendererQualityFeatureKind::sprite_ui,
        mirakana::RendererQualityFeatureKind::scene_scale,
        mirakana::RendererQualityFeatureKind::gpu_memory_residency,
        mirakana::RendererQualityFeatureKind::profiling_capture,
    };

    std::vector<mirakana::RendererQualityMatrixRow> rows;
    rows.reserve(21U);
    std::uint32_t source_index{1U};
    for (const auto feature : required_features) {
        rows.push_back(
            make_renderer_quality_matrix_ready_row(feature, mirakana::rhi::BackendKind::d3d12, source_index++));
        rows.push_back(
            make_renderer_quality_matrix_ready_row(feature, mirakana::rhi::BackendKind::vulkan, source_index++));
        rows.push_back(make_renderer_quality_matrix_metal_host_gated_row(feature, source_index++));
    }

    const auto plan = mirakana::plan_renderer_quality_matrix(mirakana::RendererQualityMatrixRequest{
        .required_backends =
            {
                mirakana::rhi::BackendKind::d3d12,
                mirakana::rhi::BackendKind::vulkan,
                mirakana::rhi::BackendKind::metal,
            },
        .rows = std::move(rows),
        .row_budget = 64U,
        .seed = 456U,
    });

    RendererQualityMatrixProbeResult result;
    result.status = plan.status;
    result.rows = plan.row_count;
    result.ready_rows = plan.ready_row_count;
    result.host_gated_rows = plan.host_gated_row_count;
    result.dependency_gated_rows = plan.dependency_gated_row_count;
    result.unsupported_rows = plan.unsupported_row_count;
    result.host_validated_backends = plan.host_validated_backend_count;
    result.replay_hash = plan.replay_hash;
    result.d3d12_ready = plan.d3d12_quality_matrix_ready;
    result.vulkan_strict_ready = plan.vulkan_strict_quality_matrix_ready;
    result.metal_ready = plan.metal_quality_matrix_ready;
    result.requires_metal_host_evidence = plan.requires_metal_host_evidence;
    result.has_metal_host_evidence = plan.has_metal_host_evidence;
    result.selected_package_evidence_ready = plan.selected_package_quality_evidence_ready;
    result.general_renderer_quality_ready = plan.general_renderer_quality_ready;
    result.invoked_gpu_commands = plan.invoked_gpu_commands;
    result.invoked_native_capture = plan.invoked_native_capture;
    result.invoked_crash_upload = plan.invoked_crash_upload;
    result.diagnostics = plan.diagnostics.size();
    result.reviewed = plan.status == mirakana::RendererQualityMatrixStatus::host_evidence_required &&
                      result.rows == 21U && result.ready_rows == 14U && result.host_gated_rows == 7U &&
                      result.dependency_gated_rows == 0U && result.unsupported_rows == 0U &&
                      result.host_validated_backends == 2U && result.replay_hash != 0U && result.d3d12_ready &&
                      result.vulkan_strict_ready && !result.metal_ready && result.requires_metal_host_evidence &&
                      !result.has_metal_host_evidence && result.selected_package_evidence_ready &&
                      !result.general_renderer_quality_ready && !result.invoked_gpu_commands &&
                      !result.invoked_native_capture && !result.invoked_crash_upload && result.diagnostics == 0U;
    result.ready = plan.status == mirakana::RendererQualityMatrixStatus::ready && plan.succeeded() &&
                   result.rows == 21U && result.ready_rows == 21U && result.host_gated_rows == 0U &&
                   result.dependency_gated_rows == 0U && result.unsupported_rows == 0U &&
                   result.host_validated_backends == 3U && result.replay_hash != 0U && result.d3d12_ready &&
                   result.vulkan_strict_ready && result.metal_ready && result.requires_metal_host_evidence &&
                   result.has_metal_host_evidence && result.selected_package_evidence_ready &&
                   result.general_renderer_quality_ready && !result.invoked_gpu_commands &&
                   !result.invoked_native_capture && !result.invoked_crash_upload && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::size_t count_world_entity_diagnostics(const mirakana::runtime::RuntimeWorldEntityLifecyclePlan& plan,
                                                         mirakana::runtime::RuntimeWorldEntityDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] WorldEntityModelProbeResult validate_world_entity_model_package_evidence() {
    using Action = mirakana::runtime::RuntimeWorldEntityLifecycleAction;
    using Code = mirakana::runtime::RuntimeWorldEntityDiagnosticCode;
    using Component = mirakana::runtime::RuntimeWorldComponentRow;
    using Entity = mirakana::runtime::RuntimeWorldEntityRow;
    using EntityId = mirakana::runtime::RuntimeWorldEntityId;
    using PersistEntity = mirakana::runtime::RuntimeSimulationPersistentEntityRow;
    using PersistPlan = mirakana::runtime::RuntimeSimulationPersistencePlan;
    using PersistStatus = mirakana::runtime::RuntimeSimulationPersistenceStatus;
    using Region = mirakana::runtime::RuntimeWorldRegionRow;
    using RegionAction = mirakana::runtime::RuntimeWorldRegionStreamingActionKind;
    using RegionCode = mirakana::runtime::RuntimeWorldRegionStreamingDiagnosticCode;
    using RegionId = mirakana::runtime::RuntimeWorldRegionId;
    using StreamingDiagnostic = mirakana::runtime::RuntimeWorldRegionStreamingDiagnostic;
    using StreamingPlan = mirakana::runtime::RuntimeWorldRegionStreamingPlan;
    using StreamingRow = mirakana::runtime::RuntimeWorldRegionStreamingPlanRow;
    using StreamingStatus = mirakana::runtime::RuntimeWorldRegionStreamingPlanStatus;
    using Request = mirakana::runtime::RuntimeWorldEntityLifecycleRequest;
    using Schema = mirakana::runtime::RuntimeWorldComponentSchemaRow;
    using SchemaId = mirakana::runtime::RuntimeWorldComponentSchemaId;
    using Status = mirakana::runtime::RuntimeWorldEntityLifecycleStatus;

    const auto
        plan =
            mirakana::runtime::plan_runtime_world_entity_lifecycle(
                Request{
                    .world_id = "sample3d.world",
                    .regions =
                        std::vector<Region>{
                            Region{.region_id = RegionId{.value = "field"}, .source_index = 1U},
                            Region{.region_id = RegionId{.value = "town"}, .source_index = 2U},
                        },
                    .entities =
                        std::vector<Entity>{
                            Entity{.entity_id = EntityId{.value = "player"},
                                   .region_id = RegionId{.value = "field"},
                                   .archetype_id = "hero",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 1U},
                            Entity{.entity_id = EntityId{.value = "npc.vendor"},
                                   .region_id = RegionId{.value = "town"},
                                   .archetype_id = "vendor",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 2U},
                            Entity{.entity_id = EntityId{.value = "crate"},
                                   .region_id = RegionId{.value = "town"},
                                   .archetype_id = "prop",
                                   .active = true,
                                   .generation = 1U,
                                   .source_index = 3U},
                        },
                    .component_schemas =
                        std::vector<Schema>{
                            Schema{.schema_id = SchemaId{.value = "physics"}, .version = 1U, .source_index = 2U},
                            Schema{.schema_id = SchemaId{.value = "transform"}, .version = 1U, .source_index = 1U},
                        },
                    .components =
                        std::vector<Component>{
                            Component{.entity_id = EntityId{.value = "player"},
                                      .schema_id = SchemaId{.value = "transform"},
                                      .state_hash = "hash.player.transform",
                                      .source_index = 1U},
                            Component{.entity_id = EntityId{.value = "player"},
                                      .schema_id = SchemaId{.value = "physics"},
                                      .state_hash = "hash.player.physics",
                                      .source_index = 2U},
                            Component{.entity_id = EntityId{.value = "npc.vendor"},
                                      .schema_id = SchemaId{.value = "transform"},
                                      .state_hash = "hash.vendor.transform",
                                      .source_index = 3U},
                            Component{.entity_id = EntityId{.value = "crate"},
                                      .schema_id = SchemaId{.value = "physics"},
                                      .state_hash = "hash.crate.physics",
                                      .source_index = 4U},
                        },
                    .lifecycle_intents =
                        {
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{
                                .action = Action::spawn_entity,
                                .entity_id = EntityId{.value = "pickup.gem"},
                                .target_region_id = RegionId{.value = "town"},
                                .archetype_id = "pickup",
                                .source_index = 1U},
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{
                                .action = Action::move_entity_region,
                                .entity_id = EntityId{.value = "player"},
                                .target_region_id = RegionId{.value = "town"},
                                .source_index = 2U},
                            mirakana::runtime::RuntimeWorldEntityLifecycleIntent{.action = Action::despawn_entity,
                                                                                 .entity_id =
                                                                                     EntityId{.value = "npc.vendor"},
                                                                                 .source_index = 3U},
                        },
                    .persistence_bridge =
                        mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                            .required = true,
                            .plan =
                                PersistPlan{
                                    .status = PersistStatus::ready,
                                    .world_id = "sample3d.world",
                                    .world_tick = 240U,
                                    .entity_rows =
                                        {
                                            PersistEntity{.entity_id = "player",
                                                          .entity_type = "hero",
                                                          .region_id = "field",
                                                          .state_hash = "hash.player"},
                                        },
                                },
                        },
                    .streaming_bridge =
                        mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                            .required = true,
                            .plan =
                                StreamingPlan{
                                    .status = StreamingStatus::planned,
                                    .rows =
                                        {
                                            StreamingRow{.action = RegionAction::load_region,
                                                         .region_id = "town",
                                                         .estimated_resident_bytes = 8192U,
                                                         .estimated_asset_records = 12U},
                                        },
                                },
                        },
                });

    const auto duplicate_plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(Request{
        .world_id = "sample3d.world",
        .regions = std::vector<Region>{Region{.region_id = RegionId{.value = "field"}, .source_index = 1U}},
        .entities =
            std::vector<Entity>{
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero",
                       .active = true,
                       .generation = 1U,
                       .source_index = 1U},
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero.copy",
                       .active = true,
                       .generation = 1U,
                       .source_index = 2U},
            },
    });
    const auto bridge_rejection_plan = mirakana::runtime::plan_runtime_world_entity_lifecycle(Request{
        .world_id = "sample3d.world",
        .regions = std::vector<Region>{Region{.region_id = RegionId{.value = "field"}, .source_index = 1U}},
        .entities =
            std::vector<Entity>{
                Entity{.entity_id = EntityId{.value = "player"},
                       .region_id = RegionId{.value = "field"},
                       .archetype_id = "hero",
                       .active = true,
                       .generation = 1U,
                       .source_index = 1U},
            },
        .persistence_bridge =
            mirakana::runtime::RuntimeWorldEntityPersistenceBridgeDesc{
                .required = true,
                .plan =
                    PersistPlan{
                        .status = PersistStatus::ready,
                        .world_id = "other_world",
                        .world_tick = 240U,
                        .entity_rows =
                            {
                                PersistEntity{.entity_id = "ghost",
                                              .entity_type = "npc",
                                              .region_id = "missing_region",
                                              .state_hash = "hash.ghost"},
                            },
                    },
            },
        .streaming_bridge =
            mirakana::runtime::RuntimeWorldEntityStreamingBridgeDesc{
                .required = true,
                .plan =
                    StreamingPlan{
                        .status = StreamingStatus::planned,
                        .diagnostics =
                            {
                                StreamingDiagnostic{.code = RegionCode::missing_desired_region,
                                                    .region_id = "missing_region",
                                                    .message = "synthetic bridge diagnostic"},
                            },
                        .rows =
                            {
                                StreamingRow{.action = RegionAction::load_region,
                                             .region_id = "missing_region",
                                             .estimated_resident_bytes = 8192U,
                                             .estimated_asset_records = 12U},
                            },
                    },
            },
    });

    WorldEntityModelProbeResult result;
    result.status = plan.status;
    result.entity_rows = plan.entity_rows.size();
    result.component_rows = plan.component_rows.size();
    result.region_ownership_rows = plan.region_ownership_rows.size();
    result.lifecycle_rows = plan.lifecycle_rows.size();
    result.persistence_rows = plan.persistence_rows.size();
    result.streaming_region_rows = plan.streaming_region_rows.size();
    result.spawn_rows = plan.spawn_count;
    result.move_rows = plan.move_count;
    result.despawn_rows = plan.despawn_count;
    result.duplicate_entity_diagnostics = count_world_entity_diagnostics(duplicate_plan, Code::duplicate_entity_id);
    result.bridge_rejection_status = bridge_rejection_plan.status;
    result.bridge_rejection_diagnostics = bridge_rejection_plan.diagnostics.size();
    result.bridge_rejection_persistence_rows = bridge_rejection_plan.persistence_rows.size();
    result.bridge_rejection_streaming_region_rows = bridge_rejection_plan.streaming_region_rows.size();
    result.bridge_rejection_streaming_diagnostics_present =
        count_world_entity_diagnostics(bridge_rejection_plan, Code::streaming_bridge_diagnostics_present);
    result.bridge_rejection_fail_closed =
        bridge_rejection_plan.status == Status::invalid_request && !bridge_rejection_plan.succeeded() &&
        result.bridge_rejection_persistence_rows == 0U && result.bridge_rejection_streaming_region_rows == 0U;
    result.diagnostics = plan.diagnostics.size();
    result.ready = plan.status == Status::ready && plan.succeeded() && result.entity_rows == 3U &&
                   result.component_rows == 4U && result.region_ownership_rows == 3U && result.lifecycle_rows == 3U &&
                   result.persistence_rows == 1U && result.streaming_region_rows == 1U && result.spawn_rows == 1U &&
                   result.move_rows == 1U && result.despawn_rows == 1U && result.duplicate_entity_diagnostics == 1U &&
                   result.bridge_rejection_status == Status::invalid_request &&
                   result.bridge_rejection_diagnostics == 5U &&
                   result.bridge_rejection_streaming_diagnostics_present == 1U && result.bridge_rejection_fail_closed &&
                   result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::size_t
count_runtime_addressable_diagnostics(const mirakana::runtime::RuntimeAddressableContentStreamingPlan& plan,
                                      mirakana::runtime::RuntimeAddressableContentDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] AddressableContentStreamingProbeResult
validate_addressable_content_streaming_package_evidence(const mirakana::runtime::RuntimeAssetPackage& runtime_package) {
    using AddressId = mirakana::runtime::RuntimeAddressableAssetId;
    using AddressRow = mirakana::runtime::RuntimeAddressableAssetRow;
    using Budget = mirakana::runtime::RuntimeAddressableResidentBudget;
    using Code = mirakana::runtime::RuntimeAddressableContentDiagnosticCode;
    using LoadRequest = mirakana::runtime::RuntimeAddressableLoadRequest;
    using ReleaseRequest = mirakana::runtime::RuntimeAddressableReleaseRequest;
    using Request = mirakana::runtime::RuntimeAddressableContentStreamingRequest;
    using ResidentRow = mirakana::runtime::RuntimeAddressableResidentAssetRow;
    using Status = mirakana::runtime::RuntimeAddressableContentStreamingStatus;

    const std::vector<AddressRow> address_rows{
        AddressRow{
            .address_id = AddressId{.value = "scene/packaged"}, .asset = packaged_scene_asset_id(), .source_index = 1U},
        AddressRow{.address_id = AddressId{.value = "texture/base_color"},
                   .asset = packaged_base_color_texture_asset_id(),
                   .source_index = 2U},
        AddressRow{.address_id = AddressId{.value = "material/lit"},
                   .asset = packaged_material_asset_id(),
                   .source_index = 3U},
        AddressRow{
            .address_id = AddressId{.value = "mesh/triangle"}, .asset = packaged_mesh_asset_id(), .source_index = 4U},
        AddressRow{.address_id = AddressId{.value = "ui/hud"},
                   .asset = packaged_ui_atlas_metadata_asset_id(),
                   .source_index = 5U},
        AddressRow{.address_id = AddressId{.value = "ui/hud_text"},
                   .asset = packaged_ui_text_glyph_atlas_metadata_asset_id(),
                   .source_index = 6U},
    };
    const Request request{
        .stream_id = "sample3d.addressable_content",
        .package = runtime_package,
        .addressable_assets = address_rows,
        .resident_assets =
            {
                ResidentRow{.address_id = AddressId{.value = "material/lit"}, .ref_count = 1U, .source_index = 1U},
            },
        .load_requests =
            {
                LoadRequest{.address_id = AddressId{.value = "scene/packaged"},
                            .include_dependencies = true,
                            .source_index = 1U},
            },
        .release_requests =
            {
                ReleaseRequest{.address_id = AddressId{.value = "material/lit"},
                               .release_count = 1U,
                               .include_dependencies = false,
                               .source_index = 1U},
            },
        .budget = Budget{.max_resident_bytes = kPackageStreamingResidentBudgetBytes},
    };
    auto budget_request = request;
    budget_request.budget = Budget{.max_resident_bytes = 1U};

    const auto plan = mirakana::runtime::plan_runtime_addressable_content_streaming(request);
    const auto budget_plan = mirakana::runtime::plan_runtime_addressable_content_streaming(budget_request);

    AddressableContentStreamingProbeResult result;
    result.status = plan.status;
    result.budget_rejection_status = budget_plan.status;
    result.address_rows = plan.address_rows.size();
    result.dependency_rows = plan.dependency_rows.size();
    result.load_rows = plan.load_rows.size();
    result.release_rows = plan.release_rows.size();
    result.refcount_rows = plan.load_rows.size() + plan.release_rows.size();
    result.resident_bytes = plan.final_resident_bytes;
    result.resident_budget_bytes = plan.resident_budget_bytes;
    result.budget_rejection_diagnostics =
        count_runtime_addressable_diagnostics(budget_plan, Code::resident_budget_exceeded);
    result.diagnostics = plan.diagnostics.size();
    result.package_io = plan.invoked_package_io || budget_plan.invoked_package_io;
    result.async_execution = plan.invoked_async_execution || budget_plan.invoked_async_execution;
    result.committed = plan.committed || budget_plan.committed;
    result.ready = plan.status == Status::ready && plan.succeeded() && result.address_rows == 6U &&
                   result.dependency_rows == 5U && result.load_rows == 4U && result.release_rows == 1U &&
                   result.refcount_rows == 5U && result.resident_bytes > 0U && result.resident_budget_bytes > 0U &&
                   budget_plan.status == Status::budget_limited && result.budget_rejection_diagnostics == 1U &&
                   !result.package_io && !result.async_execution && !result.committed && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] std::string_view
runtime_input_rebinding_capture_status_name(mirakana::runtime::RuntimeInputRebindingCaptureStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::waiting:
        return "waiting";
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::captured:
        return "captured";
    case mirakana::runtime::RuntimeInputRebindingCaptureStatus::blocked:
        return "blocked";
    }
    return "unknown";
}

[[nodiscard]] mirakana::runtime::RuntimeInputActionTrigger
make_input_rebinding_gamepad_button_trigger(mirakana::GamepadId gamepad_id, mirakana::GamepadButton button) {
    return mirakana::runtime::RuntimeInputActionTrigger{
        .kind = mirakana::runtime::RuntimeInputActionTriggerKind::gamepad_button,
        .key = mirakana::Key::unknown,
        .pointer_id = mirakana::PointerId{0},
        .gamepad_id = gamepad_id,
        .gamepad_button = button,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeInputAxisSource
make_input_rebinding_gamepad_axis_source(mirakana::GamepadId gamepad_id, mirakana::GamepadAxis axis, float scale,
                                         float deadzone) {
    return mirakana::runtime::RuntimeInputAxisSource{
        .kind = mirakana::runtime::RuntimeInputAxisSourceKind::gamepad_axis,
        .negative_key = mirakana::Key::unknown,
        .positive_key = mirakana::Key::unknown,
        .gamepad_id = gamepad_id,
        .gamepad_axis = axis,
        .scale = scale,
        .deadzone = deadzone,
    };
}

[[nodiscard]] std::size_t
count_input_rebinding_glyph_lookup_keys(const mirakana::runtime::RuntimeInputRebindingPresentationModel& model) {
    std::size_t count = 0U;
    const auto count_tokens = [&count](
                                  std::span<const mirakana::runtime::RuntimeInputRebindingPresentationToken> tokens) {
        count += static_cast<std::size_t>(std::count_if(
            tokens.begin(), tokens.end(), [](const mirakana::runtime::RuntimeInputRebindingPresentationToken& token) {
                return !token.glyph_lookup_key.empty();
            }));
    };
    for (const auto& row : model.rows) {
        count_tokens(row.base_tokens);
        count_tokens(row.profile_tokens);
    }
    return count;
}

[[nodiscard]] InputContextRebindingProbeResult validate_sample_input_context_rebinding() {
    mirakana::runtime::RuntimeInputContextStackRequest context_request;
    context_request.layers = {
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "rebinding_confirm",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::rebinding,
            .active = true,
            .blocks_lower_priority = true,
            .consumes_gameplay_input = true,
        },
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "hud",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::overlay,
            .active = true,
            .blocks_lower_priority = false,
            .consumes_gameplay_input = false,
        },
        mirakana::runtime::RuntimeInputContextLayerDesc{
            .context = "gameplay",
            .kind = mirakana::runtime::RuntimeInputContextLayerKind::gameplay,
            .active = true,
            .blocks_lower_priority = false,
            .consumes_gameplay_input = false,
        },
    };
    const auto context_plan = mirakana::runtime::plan_runtime_input_context_stack(context_request);

    mirakana::runtime::RuntimeInputActionMap base;
    base.bind_key_in_context("gameplay", "confirm", mirakana::Key::space);
    base.bind_pointer_in_context("gameplay", "confirm", mirakana::primary_pointer_id);
    base.bind_key_axis_in_context("gameplay", "move_x", mirakana::Key::left, mirakana::Key::right);

    mirakana::runtime::RuntimeInputRebindingProfile profile;
    profile.profile_id = "player_one";
    profile.action_overrides.push_back(mirakana::runtime::RuntimeInputRebindingActionOverride{
        .context = "gameplay",
        .action = "confirm",
        .triggers = {make_input_rebinding_gamepad_button_trigger(mirakana::GamepadId{1},
                                                                 mirakana::GamepadButton::south)},
    });
    profile.axis_overrides.push_back(mirakana::runtime::RuntimeInputRebindingAxisOverride{
        .context = "gameplay",
        .action = "move_x",
        .sources = {make_input_rebinding_gamepad_axis_source(mirakana::GamepadId{1}, mirakana::GamepadAxis::left_x,
                                                             1.0F, 0.25F)},
    });

    const auto applied = mirakana::runtime::apply_runtime_input_rebinding_profile(base, profile);

    mirakana::VirtualInput keyboard;
    keyboard.press(mirakana::Key::up);
    mirakana::runtime::RuntimeInputRebindingCaptureRequest action_request;
    action_request.context = "gameplay";
    action_request.action = "confirm";
    action_request.state.keyboard = &keyboard;
    const auto action_capture =
        mirakana::runtime::capture_runtime_input_rebinding_action(base, profile, action_request);

    mirakana::VirtualGamepadInput gamepad;
    gamepad.set_axis(mirakana::GamepadId{1}, mirakana::GamepadAxis::right_y, 0.9F);
    mirakana::runtime::RuntimeInputRebindingAxisCaptureRequest axis_request;
    axis_request.context = "gameplay";
    axis_request.action = "move_x";
    axis_request.state.gamepad = &gamepad;
    const auto axis_capture = mirakana::runtime::capture_runtime_input_rebinding_axis(base, profile, axis_request);

    mirakana::VirtualInput focus_keyboard;
    mirakana::runtime::RuntimeInputRebindingFocusCaptureRequest focus_request;
    focus_request.capture.context = "gameplay";
    focus_request.capture.action = "confirm";
    focus_request.capture.state.keyboard = &focus_keyboard;
    focus_request.capture_id = "rebinding_confirm";
    focus_request.focused_id = "rebinding_confirm";
    focus_request.modal_layer_id = "rebinding_confirm";
    focus_request.armed = true;
    focus_request.consume_gameplay_input = true;
    const auto focus_capture =
        mirakana::runtime::capture_runtime_input_rebinding_action_with_focus(base, profile, focus_request);

    const auto presentation = mirakana::runtime::make_runtime_input_rebinding_presentation(base, profile);

    InputContextRebindingProbeResult result{
        .requested_layers = context_request.layers.size(),
        .active_contexts = context_plan.stack.active_contexts.size(),
        .capture_context_active = context_plan.capture_context_active,
        .gameplay_input_consumed = context_plan.gameplay_input_consumed,
        .profile_overlays_applied = applied.action_overrides_applied + applied.axis_overrides_applied,
        .action_capture_status = action_capture.status,
        .axis_capture_status = axis_capture.status,
        .focus_gameplay_consumed = focus_capture.gameplay_input_consumed,
        .focus_retained = focus_capture.focus_retained,
        .presentation_rows = presentation.rows.size(),
        .glyph_lookup_keys = count_input_rebinding_glyph_lookup_keys(presentation),
        .diagnostics = context_plan.diagnostics.size() + applied.diagnostics.size() +
                       action_capture.diagnostics.size() + axis_capture.diagnostics.size() +
                       focus_capture.diagnostics.size() + presentation.diagnostics.size(),
    };
    result.ready = context_plan.succeeded() && applied.succeeded() && action_capture.captured() &&
                   axis_capture.captured() && focus_capture.waiting() && presentation.ready() &&
                   result.requested_layers == 3U && result.active_contexts == 1U && result.capture_context_active &&
                   result.gameplay_input_consumed && result.profile_overlays_applied == 2U &&
                   result.focus_gameplay_consumed && result.focus_retained && result.presentation_rows == 2U &&
                   result.glyph_lookup_keys == 5U && result.diagnostics == 0U;
    return result;
}

[[nodiscard]] SceneGameplayBindingProbeResult
validate_generated_3d_scene_gameplay_bindings(const mirakana::Scene& scene) {
    using namespace mirakana::runtime_scene;

    RuntimeSceneInstance instance{
        .scene_asset = packaged_scene_asset_id(),
        .handle = mirakana::runtime::RuntimeAssetHandle{},
        .scene = scene,
        .references = {},
    };
    const std::vector<RuntimeSceneGameplayBindingSourceRow> source_rows{
        {
            .binding_id = "mesh.actor",
            .gameplay_system_id = "generated_3d_controller",
            .slot_id = "actor",
            .node_name = "PackagedMesh",
            .required_component = RuntimeSceneGameplayBindingComponentKind::mesh_renderer,
        },
        {
            .binding_id = "light.key",
            .gameplay_system_id = "generated_3d_feedback",
            .slot_id = "light",
            .node_name = "KeyLight",
            .required_component = RuntimeSceneGameplayBindingComponentKind::light,
        },
        {
            .binding_id = "camera.primary",
            .gameplay_system_id = "generated_3d_camera",
            .slot_id = "camera",
            .node_name = "PrimaryCamera",
            .required_component = RuntimeSceneGameplayBindingComponentKind::camera,
        },
    };
    const auto bindings = resolve_runtime_scene_gameplay_bindings(instance, source_rows);
    const std::vector<RuntimeSceneGameplayInteractionSourceRow> interactions{
        {
            .action_id = "mesh.light.trigger",
            .kind = RuntimeSceneGameplayInteractionKind::trigger,
            .source_binding_id = "mesh.actor",
            .target_binding_id = "light.key",
        },
        {
            .action_id = "camera.objective",
            .kind = RuntimeSceneGameplayInteractionKind::objective_progress,
            .source_binding_id = "camera.primary",
            .objective_id = "inspect_mesh",
            .amount = 1,
        },
        {
            .action_id = "camera.win",
            .kind = RuntimeSceneGameplayInteractionKind::win,
            .source_binding_id = "camera.primary",
        },
    };
    const auto plan = plan_runtime_scene_gameplay_interactions(
        bindings.bindings, interactions,
        RuntimeSceneGameplayInteractionPlanRequest{.session_state = RuntimeSceneGameplaySessionState::running});

    SceneGameplayBindingProbeResult result{
        .source_rows = source_rows.size(),
        .binding_rows = bindings.bindings.size(),
        .gameplay_systems = count_scene_gameplay_binding_systems(bindings.bindings),
        .component_rows = static_cast<std::size_t>(
            std::count_if(bindings.bindings.begin(), bindings.bindings.end(),
                          [](const RuntimeSceneGameplayBindingRow& binding) {
                              return binding.required_component != RuntimeSceneGameplayBindingComponentKind::none;
                          })),
        .binding_diagnostics = bindings.diagnostics.size(),
        .interaction_rows = plan.rows.size(),
        .interaction_diagnostics = plan.diagnostics.size(),
        .final_session_state = plan.final_session_state,
    };
    result.ready = bindings.succeeded() && plan.succeeded() && result.source_rows == 3U && result.binding_rows == 3U &&
                   result.gameplay_systems == 3U && result.component_rows == 3U && result.binding_diagnostics == 0U &&
                   result.interaction_rows == 3U && result.interaction_diagnostics == 0U &&
                   result.final_session_state == RuntimeSceneGameplaySessionState::won;
    return result;
}

[[nodiscard]] bool gameplay_systems_near(float value, float expected, float epsilon = 0.001F) noexcept {
    return std::abs(value - expected) <= epsilon;
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_status_name(mirakana::NavigationGridAgentPathStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationGridAgentPathStatus::ready:
        return "ready";
    case mirakana::NavigationGridAgentPathStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationGridAgentPathStatus::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathStatus::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationGridAgentPathStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationGridAgentPathStatus::no_path:
        return "no_path";
    case mirakana::NavigationGridAgentPathStatus::invalid_source_path:
        return "invalid_source_path";
    case mirakana::NavigationGridAgentPathStatus::agent_path_invalid:
        return "agent_path_invalid";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_grid_agent_path_diagnostic_name(mirakana::NavigationGridAgentPathDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationGridAgentPathDiagnostic::none:
        return "none";
    case mirakana::NavigationGridAgentPathDiagnostic::invalid_mapping:
        return "invalid_mapping";
    case mirakana::NavigationGridAgentPathDiagnostic::unsupported_adjacency:
        return "unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::path_invalid_endpoint:
        return "path_invalid_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_blocked_endpoint:
        return "path_blocked_endpoint";
    case mirakana::NavigationGridAgentPathDiagnostic::path_not_found:
        return "path_not_found";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_invalid_source_path:
        return "smoothing_invalid_source_path";
    case mirakana::NavigationGridAgentPathDiagnostic::smoothing_unsupported_adjacency:
        return "smoothing_unsupported_adjacency";
    case mirakana::NavigationGridAgentPathDiagnostic::point_mapping_failed:
        return "point_mapping_failed";
    case mirakana::NavigationGridAgentPathDiagnostic::agent_path_rejected:
        return "agent_path_rejected";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_navmesh_path_status_name(mirakana::NavigationNavmeshPathStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationNavmeshPathStatus::success:
        return "success";
    case mirakana::NavigationNavmeshPathStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationNavmeshPathStatus::invalid_navmesh:
        return "invalid_navmesh";
    case mirakana::NavigationNavmeshPathStatus::invalid_endpoint:
        return "invalid_endpoint";
    case mirakana::NavigationNavmeshPathStatus::blocked_endpoint:
        return "blocked_endpoint";
    case mirakana::NavigationNavmeshPathStatus::no_path:
        return "no_path";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_navmesh_path_diagnostic_name(mirakana::NavigationNavmeshPathDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationNavmeshPathDiagnostic::none:
        return "none";
    case mirakana::NavigationNavmeshPathDiagnostic::empty_navmesh:
        return "empty_navmesh";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_polygon_id:
        return "invalid_polygon_id";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_polygon_id:
        return "duplicate_polygon_id";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_scene_ref:
        return "invalid_scene_ref";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_scene_ref:
        return "duplicate_scene_ref";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_polygon_center:
        return "invalid_polygon_center";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_traversal_cost:
        return "invalid_traversal_cost";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_portal_endpoint:
        return "invalid_portal_endpoint";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_portal_cost:
        return "invalid_portal_cost";
    case mirakana::NavigationNavmeshPathDiagnostic::invalid_obstacle:
        return "invalid_obstacle";
    case mirakana::NavigationNavmeshPathDiagnostic::duplicate_obstacle_id:
        return "duplicate_obstacle_id";
    case mirakana::NavigationNavmeshPathDiagnostic::blocked_start:
        return "blocked_start";
    case mirakana::NavigationNavmeshPathDiagnostic::blocked_goal:
        return "blocked_goal";
    case mirakana::NavigationNavmeshPathDiagnostic::cost_overflow:
        return "cost_overflow";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_navmesh_readiness_status_name(mirakana::NavigationNavmeshReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationNavmeshReadinessStatus::ready:
        return "ready";
    case mirakana::NavigationNavmeshReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::NavigationNavmeshReadinessStatus::invalid_result:
        return "invalid_result";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_navmesh_readiness_diagnostic_name(mirakana::NavigationNavmeshReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationNavmeshReadinessDiagnostic::none:
        return "none";
    case mirakana::NavigationNavmeshReadinessDiagnostic::invalid_path_result:
        return "invalid_path_result";
    case mirakana::NavigationNavmeshReadinessDiagnostic::missing_scene_refs:
        return "missing_scene_refs";
    case mirakana::NavigationNavmeshReadinessDiagnostic::scene_ref_count_mismatch:
        return "scene_ref_count_mismatch";
    case mirakana::NavigationNavmeshReadinessDiagnostic::missing_dynamic_obstacle_route:
        return "missing_dynamic_obstacle_route";
    case mirakana::NavigationNavmeshReadinessDiagnostic::insufficient_polygon_path:
        return "insufficient_polygon_path";
    case mirakana::NavigationNavmeshReadinessDiagnostic::insufficient_visited_polygons:
        return "insufficient_visited_polygons";
    case mirakana::NavigationNavmeshReadinessDiagnostic::total_cost_exceeded:
        return "total_cost_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view navigation_crowd_plan_status_name(mirakana::NavigationCrowdPlanStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationCrowdPlanStatus::success:
        return "success";
    case mirakana::NavigationCrowdPlanStatus::invalid_request:
        return "invalid_request";
    case mirakana::NavigationCrowdPlanStatus::invalid_agent:
        return "invalid_agent";
    case mirakana::NavigationCrowdPlanStatus::duplicate_agent:
        return "duplicate_agent";
    case mirakana::NavigationCrowdPlanStatus::agent_budget_exceeded:
        return "agent_budget_exceeded";
    case mirakana::NavigationCrowdPlanStatus::route_failed:
        return "route_failed";
    case mirakana::NavigationCrowdPlanStatus::avoidance_failed:
        return "avoidance_failed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_crowd_plan_diagnostic_name(mirakana::NavigationCrowdPlanDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationCrowdPlanDiagnostic::none:
        return "none";
    case mirakana::NavigationCrowdPlanDiagnostic::empty_agents:
        return "empty_agents";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_id:
        return "invalid_agent_id";
    case mirakana::NavigationCrowdPlanDiagnostic::duplicate_agent_id:
        return "duplicate_agent_id";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_position:
        return "invalid_agent_position";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_velocity:
        return "invalid_agent_velocity";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_radius:
        return "invalid_agent_radius";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_speed:
        return "invalid_agent_speed";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_goal:
        return "invalid_agent_goal";
    case mirakana::NavigationCrowdPlanDiagnostic::invalid_agent_state:
        return "invalid_agent_state";
    case mirakana::NavigationCrowdPlanDiagnostic::agent_budget_exceeded:
        return "agent_budget_exceeded";
    case mirakana::NavigationCrowdPlanDiagnostic::route_failed:
        return "route_failed";
    case mirakana::NavigationCrowdPlanDiagnostic::avoidance_failed:
        return "avoidance_failed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_crowd_readiness_status_name(mirakana::NavigationCrowdReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationCrowdReadinessStatus::ready:
        return "ready";
    case mirakana::NavigationCrowdReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::NavigationCrowdReadinessStatus::invalid_result:
        return "invalid_result";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_crowd_readiness_diagnostic_name(mirakana::NavigationCrowdReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationCrowdReadinessDiagnostic::none:
        return "none";
    case mirakana::NavigationCrowdReadinessDiagnostic::invalid_crowd_result:
        return "invalid_crowd_result";
    case mirakana::NavigationCrowdReadinessDiagnostic::missing_rows:
        return "missing_rows";
    case mirakana::NavigationCrowdReadinessDiagnostic::source_order_mismatch:
        return "source_order_mismatch";
    case mirakana::NavigationCrowdReadinessDiagnostic::missing_route_success:
        return "missing_route_success";
    case mirakana::NavigationCrowdReadinessDiagnostic::missing_avoidance_success:
        return "missing_avoidance_success";
    case mirakana::NavigationCrowdReadinessDiagnostic::missing_applied_neighbors:
        return "missing_applied_neighbors";
    case mirakana::NavigationCrowdReadinessDiagnostic::missing_dynamic_obstacles:
        return "missing_dynamic_obstacles";
    case mirakana::NavigationCrowdReadinessDiagnostic::insufficient_planned_agents:
        return "insufficient_planned_agents";
    case mirakana::NavigationCrowdReadinessDiagnostic::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_local_avoidance_status_name(mirakana::NavigationLocalAvoidanceStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationLocalAvoidanceStatus::success:
        return "success";
    case mirakana::NavigationLocalAvoidanceStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
navigation_local_avoidance_diagnostic_name(mirakana::NavigationLocalAvoidanceDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::NavigationLocalAvoidanceDiagnostic::none:
        return "none";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_agent_id:
        return "invalid_agent_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_position:
        return "invalid_position";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_desired_velocity:
        return "invalid_desired_velocity";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_radius:
        return "invalid_radius";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_max_speed:
        return "invalid_max_speed";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_id:
        return "invalid_neighbor_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_position:
        return "invalid_neighbor_position";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_velocity:
        return "invalid_neighbor_velocity";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_radius:
        return "invalid_neighbor_radius";
    case mirakana::NavigationLocalAvoidanceDiagnostic::duplicate_neighbor_id:
        return "duplicate_neighbor_id";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_separation_weight:
        return "invalid_separation_weight";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_prediction_time_seconds:
        return "invalid_prediction_time_seconds";
    case mirakana::NavigationLocalAvoidanceDiagnostic::invalid_epsilon:
        return "invalid_epsilon";
    case mirakana::NavigationLocalAvoidanceDiagnostic::calculation_overflow:
        return "calculation_overflow";
    }
    return "unknown";
}

[[nodiscard]] std::string_view navigation_agent_status_name(mirakana::NavigationAgentStatus status) noexcept {
    switch (status) {
    case mirakana::NavigationAgentStatus::idle:
        return "idle";
    case mirakana::NavigationAgentStatus::moving:
        return "moving";
    case mirakana::NavigationAgentStatus::reached_destination:
        return "reached_destination";
    case mirakana::NavigationAgentStatus::cancelled:
        return "cancelled";
    case mirakana::NavigationAgentStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_character_dynamic_policy_status_name(mirakana::PhysicsCharacterDynamicPolicy3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::moved:
        return "moved";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained:
        return "constrained";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::stepped:
        return "stepped";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsCharacterDynamicPolicy3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view physics_character_dynamic_policy_diagnostic_name(
    mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::invalid_request:
        return "invalid_request";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::step_blocked:
        return "step_blocked";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_advanced_controller_status_name(mirakana::PhysicsAdvancedController3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsAdvancedController3DStatus::moved:
        return "moved";
    case mirakana::PhysicsAdvancedController3DStatus::constrained:
        return "constrained";
    case mirakana::PhysicsAdvancedController3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_advanced_controller_diagnostic_name(mirakana::PhysicsAdvancedController3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsAdvancedController3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsAdvancedController3DDiagnostic::invalid_movement:
        return "invalid_movement";
    case mirakana::PhysicsAdvancedController3DDiagnostic::missing_controller_body:
        return "missing_controller_body";
    case mirakana::PhysicsAdvancedController3DDiagnostic::invalid_moving_platform:
        return "invalid_moving_platform";
    case mirakana::PhysicsAdvancedController3DDiagnostic::invalid_constraints:
        return "invalid_constraints";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_character_dynamics_readiness_status_name(mirakana::PhysicsCharacterDynamics3DReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsCharacterDynamics3DReadinessStatus::ready:
        return "ready";
    case mirakana::PhysicsCharacterDynamics3DReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::PhysicsCharacterDynamics3DReadinessStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view physics_character_dynamics_readiness_diagnostic_name(
    mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::none:
        return "none";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::invalid_controller:
        return "invalid_controller";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_dynamic_push:
        return "missing_dynamic_push";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_step_up:
        return "missing_step_up";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_walkable_slope:
        return "missing_walkable_slope";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_ground_probe:
        return "missing_ground_probe";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_moving_platform:
        return "missing_moving_platform";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::missing_constraint:
        return "missing_constraint";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::replay_signature_unchanged:
        return "replay_signature_unchanged";
    case mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::movement_row_budget_exceeded:
        return "movement_row_budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view physics_constraint_status_name(mirakana::PhysicsConstraint3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsConstraint3DStatus::solved:
        return "solved";
    case mirakana::PhysicsConstraint3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_constraint_diagnostic_name(mirakana::PhysicsConstraint3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsConstraint3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsConstraint3DDiagnostic::invalid_config:
        return "invalid_config";
    case mirakana::PhysicsConstraint3DDiagnostic::invalid_constraint:
        return "invalid_constraint";
    case mirakana::PhysicsConstraint3DDiagnostic::missing_body:
        return "missing_body";
    case mirakana::PhysicsConstraint3DDiagnostic::static_pair:
        return "static_pair";
    case mirakana::PhysicsConstraint3DDiagnostic::disabled_constraint:
        return "disabled_constraint";
    case mirakana::PhysicsConstraint3DDiagnostic::invalid_axis:
        return "invalid_axis";
    case mirakana::PhysicsConstraint3DDiagnostic::invalid_limits:
        return "invalid_limits";
    case mirakana::PhysicsConstraint3DDiagnostic::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_kinematic_motion_status_name(mirakana::PhysicsKinematicMotion3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsKinematicMotion3DStatus::moved:
        return "moved";
    case mirakana::PhysicsKinematicMotion3DStatus::constrained:
        return "constrained";
    case mirakana::PhysicsKinematicMotion3DStatus::blocked:
        return "blocked";
    case mirakana::PhysicsKinematicMotion3DStatus::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsKinematicMotion3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_kinematic_motion_diagnostic_name(mirakana::PhysicsKinematicMotion3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsKinematicMotion3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsKinematicMotion3DDiagnostic::invalid_request:
        return "invalid_request";
    case mirakana::PhysicsKinematicMotion3DDiagnostic::initial_overlap:
        return "initial_overlap";
    case mirakana::PhysicsKinematicMotion3DDiagnostic::iteration_limit:
        return "iteration_limit";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_simple_vehicle_status_name(mirakana::PhysicsSimpleVehicle3DStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsSimpleVehicle3DStatus::grounded:
        return "grounded";
    case mirakana::PhysicsSimpleVehicle3DStatus::airborne:
        return "airborne";
    case mirakana::PhysicsSimpleVehicle3DStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_simple_vehicle_diagnostic_name(mirakana::PhysicsSimpleVehicle3DDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsSimpleVehicle3DDiagnostic::none:
        return "none";
    case mirakana::PhysicsSimpleVehicle3DDiagnostic::invalid_request:
        return "invalid_request";
    case mirakana::PhysicsSimpleVehicle3DDiagnostic::invalid_motion:
        return "invalid_motion";
    }
    return "unknown";
}

[[nodiscard]] std::string_view behavior_tree_status_name(mirakana::BehaviorTreeStatus status) noexcept {
    switch (status) {
    case mirakana::BehaviorTreeStatus::success:
        return "success";
    case mirakana::BehaviorTreeStatus::failure:
        return "failure";
    case mirakana::BehaviorTreeStatus::running:
        return "running";
    case mirakana::BehaviorTreeStatus::invalid_tree:
        return "invalid_tree";
    case mirakana::BehaviorTreeStatus::missing_leaf_result:
        return "missing_leaf_result";
    case mirakana::BehaviorTreeStatus::invalid_leaf_result:
        return "invalid_leaf_result";
    }
    return "unknown";
}

[[nodiscard]] std::string_view ai_perception_status_name(mirakana::AiPerceptionStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionStatus::ready:
        return "ready";
    case mirakana::AiPerceptionStatus::invalid_agent:
        return "invalid_agent";
    case mirakana::AiPerceptionStatus::invalid_target:
        return "invalid_target";
    case mirakana::AiPerceptionStatus::duplicate_target_id:
        return "duplicate_target_id";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
ai_perception_blackboard_status_name(mirakana::AiPerceptionBlackboardStatus status) noexcept {
    switch (status) {
    case mirakana::AiPerceptionBlackboardStatus::ready:
        return "ready";
    case mirakana::AiPerceptionBlackboardStatus::invalid_snapshot:
        return "invalid_snapshot";
    case mirakana::AiPerceptionBlackboardStatus::invalid_key:
        return "invalid_key";
    case mirakana::AiPerceptionBlackboardStatus::blackboard_write_failed:
        return "blackboard_write_failed";
    }
    return "unknown";
}

[[nodiscard]] std::string_view audio_device_stream_status_name(mirakana::AudioDeviceStreamStatus status) noexcept {
    switch (status) {
    case mirakana::AudioDeviceStreamStatus::ready:
        return "ready";
    case mirakana::AudioDeviceStreamStatus::no_work:
        return "no_work";
    case mirakana::AudioDeviceStreamStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
audio_device_stream_diagnostic_name(mirakana::AudioDeviceStreamDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::AudioDeviceStreamDiagnostic::none:
        return "none";
    case mirakana::AudioDeviceStreamDiagnostic::invalid_format:
        return "invalid_format";
    case mirakana::AudioDeviceStreamDiagnostic::invalid_queue_target:
        return "invalid_queue_target";
    case mirakana::AudioDeviceStreamDiagnostic::invalid_render_budget:
        return "invalid_render_budget";
    case mirakana::AudioDeviceStreamDiagnostic::invalid_resampling_quality:
        return "invalid_resampling_quality";
    case mirakana::AudioDeviceStreamDiagnostic::device_frame_overflow:
        return "device_frame_overflow";
    }
    return "unknown";
}

class GeneratedGameplaySystemsProbe final {
  public:
    explicit GeneratedGameplaySystemsProbe(std::optional<mirakana::AudioClipSampleData> audio_samples = std::nullopt)
        : physics_(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}}),
          animation_(mirakana::AnimationStateMachineDesc{
              .states =
                  {
                      mirakana::AnimationStateDesc{
                          .name = "idle",
                          .clip =
                              mirakana::AnimationClipDesc{.name = "idle", .duration_seconds = 1.0F, .looping = true}},
                      mirakana::AnimationStateDesc{
                          .name = "walk",
                          .clip =
                              mirakana::AnimationClipDesc{.name = "walk", .duration_seconds = 1.0F, .looping = true}},
                  },
              .initial_state = "idle",
              .transitions =
                  {
                      mirakana::AnimationTransitionDesc{
                          .from_state = "idle", .to_state = "walk", .trigger = "move", .blend_seconds = 0.25F},
                  },
          }),
          audio_samples_(std::move(audio_samples)) {}

    void start() {
        floor_body_ = physics_.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 5.0F, .y = 1.0F, .z = 5.0F},
        });
        actor_body_ = physics_.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 1.5F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = -1.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 0.5F},
        });

        build_authored_collision_probe();
        build_navigation_agent();
        build_navigation_navmesh_probe();
        build_navigation_crowd_probe();
        build_physics_movement_policy_probe();
        build_physics_advanced_controller_probe();
        build_physics_constraints_probe();
        build_physics_kinematic_motion_probe();
        render_audio_stream_probe();
        build_gameplay_interaction_probe();
        build_runtime_profile_resume_probe();
        build_runtime_menu_hud_probe();
        (void)animation_.trigger("move");
        started_ = true;
    }

    void tick() {
        if (!started_) {
            return;
        }

        physics_.apply_force(actor_body_, mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F});
        physics_.step(0.25F);
        physics_.resolve_contacts();
        if (const auto* actor = physics_.find_body(actor_body_); actor != nullptr) {
            final_actor_position_ = actor->position;
        }
        ++physics_ticks_;

        animation_.update(0.25F);
        final_animation_sample_ = animation_.sample();
        update_navigation_local_avoidance_probe();
        update_ai_navigation_composition();
        ++ticks_;
    }

    [[nodiscard]] bool passed(std::uint32_t expected_ticks) const {
        return started_ && ticks_ == expected_ticks && physics_ticks_ == expected_ticks &&
               floor_body_ != mirakana::null_physics_body_3d && actor_body_ != mirakana::null_physics_body_3d &&
               authored_collision_.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success &&
               authored_collision_.bodies.size() == 2U && controller_result_.grounded &&
               navigation_plan_status_ == mirakana::NavigationGridAgentPathStatus::ready &&
               navigation_plan_diagnostic_ == mirakana::NavigationGridAgentPathDiagnostic::none &&
               navigation_path_point_count_ == 2U && gameplay_systems_near(navigation_goal_.x, 1.5F) &&
               gameplay_systems_near(navigation_goal_.y, 0.5F) &&
               navigation_navmesh_result_.status == mirakana::NavigationNavmeshPathStatus::success &&
               navigation_navmesh_result_.diagnostic == mirakana::NavigationNavmeshPathDiagnostic::none &&
               navigation_navmesh_result_.polygon_path.size() == 3U &&
               navigation_navmesh_result_.dynamic_obstacle_count == 1U &&
               navigation_navmesh_readiness_.status == mirakana::NavigationNavmeshReadinessStatus::ready &&
               navigation_navmesh_readiness_.diagnostic == mirakana::NavigationNavmeshReadinessDiagnostic::none &&
               navigation_navmesh_readiness_.diagnostics.empty() &&
               navigation_navmesh_readiness_.scene_ref_rows == 3U &&
               navigation_navmesh_readiness_.point_path_rows == 3U &&
               navigation_navmesh_readiness_.visited_polygon_count == 3U &&
               navigation_navmesh_readiness_.total_cost == 5U &&
               navigation_crowd_result_.status == mirakana::NavigationCrowdPlanStatus::success &&
               navigation_crowd_result_.diagnostic == mirakana::NavigationCrowdPlanDiagnostic::none &&
               navigation_crowd_result_.rows.size() == 2U && navigation_crowd_source_order_ready() &&
               navigation_crowd_result_.planned_agent_count == 2U &&
               navigation_crowd_result_.route_success_count == 2U &&
               navigation_crowd_result_.avoidance_success_count == 2U &&
               navigation_crowd_result_.applied_neighbor_count == 2U &&
               navigation_crowd_result_.dynamic_obstacle_count == 2U &&
               navigation_crowd_readiness_.status == mirakana::NavigationCrowdReadinessStatus::ready &&
               navigation_crowd_readiness_.diagnostic == mirakana::NavigationCrowdReadinessDiagnostic::none &&
               navigation_crowd_readiness_.diagnostics.empty() && navigation_crowd_readiness_.row_count == 2U &&
               navigation_crowd_readiness_.planned_agent_count == 2U &&
               navigation_crowd_readiness_.route_success_count == 2U &&
               navigation_crowd_readiness_.avoidance_success_count == 2U &&
               navigation_crowd_readiness_.applied_neighbor_count == 2U &&
               navigation_crowd_readiness_.dynamic_obstacle_count == 2U &&
               navigation_crowd_readiness_.source_order_ready &&
               local_avoidance_status_ == mirakana::NavigationLocalAvoidanceStatus::success &&
               local_avoidance_diagnostic_ == mirakana::NavigationLocalAvoidanceDiagnostic::none &&
               local_avoidance_steps_ == expected_ticks && local_avoidance_applied_neighbor_count_ == expected_ticks &&
               physics_policy_result_.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained &&
               physics_policy_result_.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none &&
               physics_policy_result_.rows.size() == 3U && physics_policy_dynamic_push_count_ == 1U &&
               physics_policy_solid_contact_count_ == 1U && physics_policy_trigger_overlap_count_ == 1U &&
               advanced_controller_result_.status == mirakana::PhysicsAdvancedController3DStatus::moved &&
               advanced_controller_result_.diagnostic == mirakana::PhysicsAdvancedController3DDiagnostic::none &&
               advanced_controller_result_.movement.rows.size() == 5U &&
               advanced_controller_result_.moving_platform_rows.size() == 1U &&
               advanced_controller_platform_applied_count_ == 1U &&
               advanced_controller_result_.constraints.status == mirakana::PhysicsJoint3DStatus::solved &&
               advanced_controller_result_.constraints.rows.size() == 1U &&
               advanced_controller_result_.replay_before.body_count == 6U &&
               advanced_controller_result_.replay_after.body_count == 6U &&
               advanced_controller_result_.replay_after.value != advanced_controller_result_.replay_before.value &&
               gameplay_systems_near(advanced_controller_result_.position.x, 3.25F) &&
               character_dynamics_readiness_.status == mirakana::PhysicsCharacterDynamics3DReadinessStatus::ready &&
               character_dynamics_readiness_.diagnostic ==
                   mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::none &&
               character_dynamics_readiness_.diagnostics.empty() && character_dynamics_readiness_.movement_rows == 5U &&
               character_dynamics_readiness_.dynamic_push_rows == 1U &&
               character_dynamics_readiness_.step_up_rows == 1U &&
               character_dynamics_readiness_.walkable_slope_rows == 1U &&
               character_dynamics_readiness_.ground_probe_rows == 1U &&
               character_dynamics_readiness_.moving_platform_rows == 1U &&
               character_dynamics_readiness_.applied_moving_platform_rows == 1U &&
               character_dynamics_readiness_.constraint_rows == 1U && character_dynamics_readiness_.replay_changed &&
               physics_constraints_result_.status == mirakana::PhysicsConstraint3DStatus::solved &&
               physics_constraints_result_.diagnostic == mirakana::PhysicsConstraint3DDiagnostic::none &&
               physics_constraints_result_.rows.size() == 2U && physics_constraints_fixed_row_count() == 1U &&
               physics_constraints_linear_axis_row_count() == 1U &&
               physics_constraints_axis_limit_clamped_count() == 1U &&
               kinematic_motion_result_.status == mirakana::PhysicsKinematicMotion3DStatus::constrained &&
               kinematic_motion_result_.diagnostic == mirakana::PhysicsKinematicMotion3DDiagnostic::none &&
               kinematic_motion_result_.rows.size() == 2U && kinematic_motion_result_.grounded &&
               simple_vehicle_result_.status == mirakana::PhysicsSimpleVehicle3DStatus::grounded &&
               simple_vehicle_result_.diagnostic == mirakana::PhysicsSimpleVehicle3DDiagnostic::none &&
               simple_vehicle_result_.wheel_rows.size() == 4U && simple_vehicle_result_.grounded_wheel_count == 4U &&
               simple_vehicle_result_.wheel_hit_count == 4U &&
               gameplay_systems_near(simple_vehicle_result_.position.x, 1.0F) &&
               navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination &&
               gameplay_systems_near(navigation_agent_.position.x, 1.5F) &&
               gameplay_systems_near(navigation_agent_.position.y, 0.5F) &&
               last_perception_status_ == mirakana::AiPerceptionStatus::ready && last_perception_has_primary_target_ &&
               last_perception_primary_target_id_ == 1U &&
               gameplay_systems_near(last_perception_primary_target_distance_, 0.0F) &&
               last_perception_visible_count_ == 1U && last_perception_audible_count_ == 0U &&
               last_blackboard_status_ == mirakana::AiPerceptionBlackboardStatus::ready && blackboard_has_target_ &&
               blackboard_needs_move_ && blackboard_target_id_ == 1U && blackboard_visible_count_ == 1U &&
               last_tree_result_.status == mirakana::BehaviorTreeStatus::success &&
               last_tree_result_.visited_nodes.size() == 4U && last_perception_target_count_ == 1U &&
               audio_voice_started_ && audio_stream_status_ == mirakana::AudioDeviceStreamStatus::ready &&
               audio_stream_diagnostic_ == mirakana::AudioDeviceStreamDiagnostic::none &&
               audio_render_command_count_ == 1U && audio_stream_frames_ == 2U && audio_stream_sample_count_ == 2U &&
               gameplay_systems_near(audio_stream_first_sample_, 0.3F) &&
               gameplay_systems_near(audio_stream_second_sample_, 0.4F) &&
               gameplay_systems_near(audio_stream_sample_abs_sum_, 0.7F) && interaction_ready() &&
               interaction_plan_.rows.size() == 10U && interaction_plan_.feedback_rows.size() == 10U &&
               interaction_plan_.state.session_state == mirakana::runtime::RuntimeGameplaySessionState::running &&
               runtime_profile_resume_ready() && runtime_menu_hud_ready() &&
               runtime_menu_hud_diagnostic_count() == 0U && runtime_menu_hud_display_row_count() == 6U &&
               runtime_menu_hud_command_row_count() == 2U && runtime_menu_hud_dialogue_row_count() == 1U &&
               runtime_menu_hud_input_binding_prompt_row_count() == 1U && final_animation_sample_.to_state == "walk" &&
               !final_animation_sample_.blending && final_actor_position_.x > 0.0F;
    }

    [[nodiscard]] GameplaySystemsStatus status(std::uint32_t expected_ticks) const {
        if (!started_) {
            return GameplaySystemsStatus::not_started;
        }
        return passed(expected_ticks) ? GameplaySystemsStatus::ready : GameplaySystemsStatus::diagnostics;
    }

    [[nodiscard]] std::size_t diagnostics_count(std::uint32_t expected_ticks) const {
        const bool checks[] = {
            ticks_ == expected_ticks,
            physics_ticks_ == expected_ticks,
            floor_body_ != mirakana::null_physics_body_3d && actor_body_ != mirakana::null_physics_body_3d,
            authored_collision_.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success,
            authored_collision_.bodies.size() == 2U,
            controller_result_.grounded,
            navigation_plan_status_ == mirakana::NavigationGridAgentPathStatus::ready,
            navigation_plan_diagnostic_ == mirakana::NavigationGridAgentPathDiagnostic::none,
            navigation_path_point_count_ == 2U,
            gameplay_systems_near(navigation_goal_.x, 1.5F) && gameplay_systems_near(navigation_goal_.y, 0.5F),
            navigation_navmesh_result_.status == mirakana::NavigationNavmeshPathStatus::success,
            navigation_navmesh_result_.diagnostic == mirakana::NavigationNavmeshPathDiagnostic::none,
            navigation_navmesh_result_.polygon_path.size() == 3U,
            navigation_navmesh_result_.dynamic_obstacle_count == 1U,
            navigation_navmesh_readiness_.status == mirakana::NavigationNavmeshReadinessStatus::ready,
            navigation_navmesh_readiness_.diagnostic == mirakana::NavigationNavmeshReadinessDiagnostic::none,
            navigation_navmesh_readiness_.diagnostics.empty(),
            navigation_navmesh_readiness_.scene_ref_rows == 3U,
            navigation_navmesh_readiness_.point_path_rows == 3U,
            navigation_navmesh_readiness_.visited_polygon_count == 3U,
            navigation_navmesh_readiness_.total_cost == 5U,
            navigation_crowd_result_.status == mirakana::NavigationCrowdPlanStatus::success,
            navigation_crowd_result_.diagnostic == mirakana::NavigationCrowdPlanDiagnostic::none,
            navigation_crowd_result_.rows.size() == 2U,
            navigation_crowd_source_order_ready(),
            navigation_crowd_result_.planned_agent_count == 2U,
            navigation_crowd_result_.route_success_count == 2U,
            navigation_crowd_result_.avoidance_success_count == 2U,
            navigation_crowd_result_.applied_neighbor_count == 2U,
            navigation_crowd_result_.dynamic_obstacle_count == 2U,
            navigation_crowd_readiness_.status == mirakana::NavigationCrowdReadinessStatus::ready,
            navigation_crowd_readiness_.diagnostic == mirakana::NavigationCrowdReadinessDiagnostic::none,
            navigation_crowd_readiness_.diagnostics.empty(),
            navigation_crowd_readiness_.row_count == 2U,
            navigation_crowd_readiness_.planned_agent_count == 2U,
            navigation_crowd_readiness_.route_success_count == 2U,
            navigation_crowd_readiness_.avoidance_success_count == 2U,
            navigation_crowd_readiness_.applied_neighbor_count == 2U,
            navigation_crowd_readiness_.dynamic_obstacle_count == 2U,
            navigation_crowd_readiness_.source_order_ready,
            local_avoidance_status_ == mirakana::NavigationLocalAvoidanceStatus::success,
            local_avoidance_diagnostic_ == mirakana::NavigationLocalAvoidanceDiagnostic::none,
            local_avoidance_steps_ == expected_ticks,
            local_avoidance_applied_neighbor_count_ == expected_ticks,
            physics_policy_result_.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained,
            physics_policy_result_.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none,
            physics_policy_result_.rows.size() == 3U,
            physics_policy_dynamic_push_count_ == 1U && physics_policy_solid_contact_count_ == 1U &&
                physics_policy_trigger_overlap_count_ == 1U,
            advanced_controller_result_.status == mirakana::PhysicsAdvancedController3DStatus::moved,
            advanced_controller_result_.diagnostic == mirakana::PhysicsAdvancedController3DDiagnostic::none,
            advanced_controller_result_.movement.rows.size() == 5U,
            advanced_controller_result_.moving_platform_rows.size() == 1U,
            advanced_controller_platform_applied_count_ == 1U,
            advanced_controller_result_.constraints.status == mirakana::PhysicsJoint3DStatus::solved,
            advanced_controller_result_.constraints.rows.size() == 1U,
            advanced_controller_result_.replay_before.body_count == 6U,
            advanced_controller_result_.replay_after.body_count == 6U,
            advanced_controller_result_.replay_after.value != advanced_controller_result_.replay_before.value,
            gameplay_systems_near(advanced_controller_result_.position.x, 3.25F),
            character_dynamics_readiness_.status == mirakana::PhysicsCharacterDynamics3DReadinessStatus::ready,
            character_dynamics_readiness_.diagnostic == mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic::none,
            character_dynamics_readiness_.diagnostics.empty(),
            character_dynamics_readiness_.movement_rows == 5U,
            character_dynamics_readiness_.dynamic_push_rows == 1U,
            character_dynamics_readiness_.step_up_rows == 1U,
            character_dynamics_readiness_.walkable_slope_rows == 1U,
            character_dynamics_readiness_.ground_probe_rows == 1U,
            character_dynamics_readiness_.moving_platform_rows == 1U,
            character_dynamics_readiness_.applied_moving_platform_rows == 1U,
            character_dynamics_readiness_.constraint_rows == 1U,
            character_dynamics_readiness_.replay_changed,
            physics_constraints_result_.status == mirakana::PhysicsConstraint3DStatus::solved,
            physics_constraints_result_.diagnostic == mirakana::PhysicsConstraint3DDiagnostic::none,
            physics_constraints_result_.rows.size() == 2U,
            physics_constraints_fixed_row_count() == 1U,
            physics_constraints_linear_axis_row_count() == 1U,
            physics_constraints_axis_limit_clamped_count() == 1U,
            kinematic_motion_result_.status == mirakana::PhysicsKinematicMotion3DStatus::constrained,
            kinematic_motion_result_.diagnostic == mirakana::PhysicsKinematicMotion3DDiagnostic::none,
            kinematic_motion_result_.rows.size() == 2U,
            kinematic_motion_result_.grounded,
            simple_vehicle_result_.status == mirakana::PhysicsSimpleVehicle3DStatus::grounded,
            simple_vehicle_result_.diagnostic == mirakana::PhysicsSimpleVehicle3DDiagnostic::none,
            simple_vehicle_result_.wheel_rows.size() == 4U,
            simple_vehicle_result_.grounded_wheel_count == 4U,
            simple_vehicle_result_.wheel_hit_count == 4U,
            gameplay_systems_near(simple_vehicle_result_.position.x, 1.0F),
            navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination,
            gameplay_systems_near(navigation_agent_.position.x, 1.5F) &&
                gameplay_systems_near(navigation_agent_.position.y, 0.5F),
            last_perception_status_ == mirakana::AiPerceptionStatus::ready,
            last_perception_has_primary_target_,
            last_perception_primary_target_id_ == 1U,
            gameplay_systems_near(last_perception_primary_target_distance_, 0.0F),
            last_perception_visible_count_ == 1U && last_perception_audible_count_ == 0U,
            last_blackboard_status_ == mirakana::AiPerceptionBlackboardStatus::ready,
            blackboard_has_target_ && blackboard_needs_move_ && blackboard_target_id_ == 1U &&
                blackboard_visible_count_ == 1U,
            last_tree_result_.status == mirakana::BehaviorTreeStatus::success,
            last_tree_result_.visited_nodes.size() == 4U,
            last_perception_target_count_ == 1U,
            audio_voice_started_,
            audio_stream_status_ == mirakana::AudioDeviceStreamStatus::ready,
            audio_stream_diagnostic_ == mirakana::AudioDeviceStreamDiagnostic::none,
            audio_render_command_count_ == 1U,
            audio_stream_frames_ == 2U,
            audio_stream_sample_count_ == 2U,
            gameplay_systems_near(audio_stream_first_sample_, 0.3F),
            gameplay_systems_near(audio_stream_second_sample_, 0.4F),
            gameplay_systems_near(audio_stream_sample_abs_sum_, 0.7F),
            interaction_ready(),
            interaction_plan_.rows.size() == 10U,
            interaction_plan_.feedback_rows.size() == 10U,
            interaction_plan_.state.session_state == mirakana::runtime::RuntimeGameplaySessionState::running,
            runtime_profile_resume_ready(),
            runtime_menu_hud_ready(),
            runtime_menu_hud_diagnostic_count() == 0U,
            runtime_menu_hud_display_row_count() == 6U,
            runtime_menu_hud_command_row_count() == 2U,
            runtime_menu_hud_dialogue_row_count() == 1U,
            runtime_menu_hud_input_binding_prompt_row_count() == 1U,
            final_animation_sample_.to_state == "walk" && !final_animation_sample_.blending,
            final_actor_position_.x > 0.0F,
        };
        std::size_t failures = 0;
        for (const bool check : checks) {
            if (!check) {
                ++failures;
            }
        }
        return failures;
    }

    [[nodiscard]] std::uint32_t ticks() const noexcept {
        return ticks_;
    }

    [[nodiscard]] std::uint32_t physics_ticks() const noexcept {
        return physics_ticks_;
    }

    [[nodiscard]] std::size_t authored_collision_body_count() const noexcept {
        return authored_collision_.bodies.size();
    }

    [[nodiscard]] bool controller_grounded() const noexcept {
        return controller_result_.grounded;
    }

    [[nodiscard]] std::size_t navigation_path_point_count() const noexcept {
        return navigation_path_point_count_;
    }

    [[nodiscard]] bool navigation_reached_destination() const noexcept {
        return navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus navigation_plan_status() const noexcept {
        return navigation_plan_status_;
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic() const noexcept {
        return navigation_plan_diagnostic_;
    }

    [[nodiscard]] mirakana::NavigationAgentStatus navigation_agent_status() const noexcept {
        return navigation_agent_.status;
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathStatus navigation_navmesh_status() const noexcept {
        return navigation_navmesh_result_.status;
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathDiagnostic navigation_navmesh_diagnostic() const noexcept {
        return navigation_navmesh_result_.diagnostic;
    }

    [[nodiscard]] std::size_t navigation_navmesh_polygon_count() const noexcept {
        return navigation_navmesh_result_.polygon_path.size();
    }

    [[nodiscard]] std::size_t navigation_navmesh_dynamic_obstacle_count() const noexcept {
        return navigation_navmesh_result_.dynamic_obstacle_count;
    }

    [[nodiscard]] std::uint32_t navigation_navmesh_total_cost() const noexcept {
        return navigation_navmesh_result_.total_cost;
    }

    [[nodiscard]] mirakana::NavigationNavmeshReadinessStatus navigation_navmesh_readiness_status() const noexcept {
        return navigation_navmesh_readiness_.status;
    }

    [[nodiscard]] mirakana::NavigationNavmeshReadinessDiagnostic
    navigation_navmesh_readiness_diagnostic() const noexcept {
        return navigation_navmesh_readiness_.diagnostic;
    }

    [[nodiscard]] std::size_t navigation_navmesh_readiness_diagnostics_count() const noexcept {
        return navigation_navmesh_readiness_.diagnostics.size();
    }

    [[nodiscard]] std::size_t navigation_navmesh_scene_ref_count() const noexcept {
        return navigation_navmesh_readiness_.scene_ref_rows;
    }

    [[nodiscard]] std::size_t navigation_navmesh_point_path_count() const noexcept {
        return navigation_navmesh_readiness_.point_path_rows;
    }

    [[nodiscard]] std::size_t navigation_navmesh_visited_polygon_count() const noexcept {
        return navigation_navmesh_readiness_.visited_polygon_count;
    }

    [[nodiscard]] mirakana::NavigationCrowdPlanStatus navigation_crowd_status() const noexcept {
        return navigation_crowd_result_.status;
    }

    [[nodiscard]] mirakana::NavigationCrowdPlanDiagnostic navigation_crowd_diagnostic() const noexcept {
        return navigation_crowd_result_.diagnostic;
    }

    [[nodiscard]] std::size_t navigation_crowd_row_count() const noexcept {
        return navigation_crowd_result_.rows.size();
    }

    [[nodiscard]] bool navigation_crowd_source_order_ready() const noexcept {
        return navigation_crowd_result_.rows.size() == 2U && navigation_crowd_result_.rows[0].agent_id == 10U &&
               navigation_crowd_result_.rows[0].source_index == 1U &&
               navigation_crowd_result_.rows[1].agent_id == 20U && navigation_crowd_result_.rows[1].source_index == 0U;
    }

    [[nodiscard]] std::size_t navigation_crowd_agent_count() const noexcept {
        return navigation_crowd_result_.planned_agent_count;
    }

    [[nodiscard]] std::size_t navigation_crowd_route_success_count() const noexcept {
        return navigation_crowd_result_.route_success_count;
    }

    [[nodiscard]] std::size_t navigation_crowd_avoidance_success_count() const noexcept {
        return navigation_crowd_result_.avoidance_success_count;
    }

    [[nodiscard]] std::size_t navigation_crowd_applied_neighbor_count() const noexcept {
        return navigation_crowd_result_.applied_neighbor_count;
    }

    [[nodiscard]] std::size_t navigation_crowd_dynamic_obstacle_count() const noexcept {
        return navigation_crowd_result_.dynamic_obstacle_count;
    }

    [[nodiscard]] mirakana::NavigationCrowdReadinessStatus navigation_crowd_readiness_status() const noexcept {
        return navigation_crowd_readiness_.status;
    }

    [[nodiscard]] mirakana::NavigationCrowdReadinessDiagnostic navigation_crowd_readiness_diagnostic() const noexcept {
        return navigation_crowd_readiness_.diagnostic;
    }

    [[nodiscard]] std::size_t navigation_crowd_readiness_diagnostics_count() const noexcept {
        return navigation_crowd_readiness_.diagnostics.size();
    }

    [[nodiscard]] std::size_t navigation_crowd_readiness_row_count() const noexcept {
        return navigation_crowd_readiness_.row_count;
    }

    [[nodiscard]] bool navigation_crowd_readiness_source_order_ready() const noexcept {
        return navigation_crowd_readiness_.source_order_ready;
    }

    [[nodiscard]] std::size_t navigation_crowd_readiness_applied_neighbor_count() const noexcept {
        return navigation_crowd_readiness_.applied_neighbor_count;
    }

    [[nodiscard]] std::size_t navigation_crowd_readiness_dynamic_obstacle_count() const noexcept {
        return navigation_crowd_readiness_.dynamic_obstacle_count;
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceStatus local_avoidance_status() const noexcept {
        return local_avoidance_status_;
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceDiagnostic local_avoidance_diagnostic() const noexcept {
        return local_avoidance_diagnostic_;
    }

    [[nodiscard]] std::uint32_t local_avoidance_steps() const noexcept {
        return local_avoidance_steps_;
    }

    [[nodiscard]] std::size_t local_avoidance_applied_neighbor_count() const noexcept {
        return local_avoidance_applied_neighbor_count_;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DStatus physics_policy_status() const noexcept {
        return physics_policy_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic physics_policy_diagnostic() const noexcept {
        return physics_policy_result_.diagnostic;
    }

    [[nodiscard]] std::size_t physics_policy_row_count() const noexcept {
        return physics_policy_result_.rows.size();
    }

    [[nodiscard]] std::size_t physics_policy_dynamic_push_count() const noexcept {
        return physics_policy_dynamic_push_count_;
    }

    [[nodiscard]] std::size_t physics_policy_solid_contact_count() const noexcept {
        return physics_policy_solid_contact_count_;
    }

    [[nodiscard]] std::size_t physics_policy_trigger_overlap_count() const noexcept {
        return physics_policy_trigger_overlap_count_;
    }

    [[nodiscard]] mirakana::PhysicsAdvancedController3DStatus advanced_controller_status() const noexcept {
        return advanced_controller_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsAdvancedController3DDiagnostic advanced_controller_diagnostic() const noexcept {
        return advanced_controller_result_.diagnostic;
    }

    [[nodiscard]] std::size_t advanced_controller_movement_row_count() const noexcept {
        return advanced_controller_result_.movement.rows.size();
    }

    [[nodiscard]] std::size_t advanced_controller_moving_platform_row_count() const noexcept {
        return advanced_controller_result_.moving_platform_rows.size();
    }

    [[nodiscard]] std::size_t advanced_controller_platform_applied_count() const noexcept {
        return advanced_controller_platform_applied_count_;
    }

    [[nodiscard]] std::size_t advanced_controller_constraint_row_count() const noexcept {
        return advanced_controller_result_.constraints.rows.size();
    }

    [[nodiscard]] bool advanced_controller_replay_changed() const noexcept {
        return advanced_controller_result_.replay_after.value != advanced_controller_result_.replay_before.value;
    }

    [[nodiscard]] std::uint64_t advanced_controller_replay_before_body_count() const noexcept {
        return advanced_controller_result_.replay_before.body_count;
    }

    [[nodiscard]] std::uint64_t advanced_controller_replay_after_body_count() const noexcept {
        return advanced_controller_result_.replay_after.body_count;
    }

    [[nodiscard]] float advanced_controller_final_x() const noexcept {
        return advanced_controller_result_.position.x;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamics3DReadinessStatus character_dynamics_status() const noexcept {
        return character_dynamics_readiness_.status;
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic
    character_dynamics_diagnostic() const noexcept {
        return character_dynamics_readiness_.diagnostic;
    }

    [[nodiscard]] std::size_t character_dynamics_movement_rows() const noexcept {
        return character_dynamics_readiness_.movement_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_dynamic_pushes() const noexcept {
        return character_dynamics_readiness_.dynamic_push_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_step_ups() const noexcept {
        return character_dynamics_readiness_.step_up_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_walkable_slope_rows() const noexcept {
        return character_dynamics_readiness_.walkable_slope_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_ground_probes() const noexcept {
        return character_dynamics_readiness_.ground_probe_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_moving_platform_rows() const noexcept {
        return character_dynamics_readiness_.moving_platform_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_applied_platform_rows() const noexcept {
        return character_dynamics_readiness_.applied_moving_platform_rows;
    }

    [[nodiscard]] std::size_t character_dynamics_constraint_rows() const noexcept {
        return character_dynamics_readiness_.constraint_rows;
    }

    [[nodiscard]] bool character_dynamics_replay_changed() const noexcept {
        return character_dynamics_readiness_.replay_changed;
    }

    [[nodiscard]] std::size_t character_dynamics_diagnostics_count() const noexcept {
        return character_dynamics_readiness_.diagnostics.size();
    }

    [[nodiscard]] mirakana::PhysicsConstraint3DStatus physics_constraints_status() const noexcept {
        return physics_constraints_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsConstraint3DDiagnostic physics_constraints_diagnostic() const noexcept {
        return physics_constraints_result_.diagnostic;
    }

    [[nodiscard]] std::size_t physics_constraints_row_count() const noexcept {
        return physics_constraints_result_.rows.size();
    }

    [[nodiscard]] std::size_t physics_constraints_fixed_row_count() const noexcept {
        std::size_t count = 0;
        for (const auto& row : physics_constraints_result_.rows) {
            if (row.kind == mirakana::PhysicsConstraint3DKind::fixed) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t physics_constraints_linear_axis_row_count() const noexcept {
        std::size_t count = 0;
        for (const auto& row : physics_constraints_result_.rows) {
            if (row.kind == mirakana::PhysicsConstraint3DKind::linear_axis) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] std::size_t physics_constraints_axis_limit_clamped_count() const noexcept {
        std::size_t count = 0;
        for (const auto& row : physics_constraints_result_.rows) {
            if (row.axis_limit_clamped) {
                ++count;
            }
        }
        return count;
    }

    [[nodiscard]] mirakana::PhysicsKinematicMotion3DStatus kinematic_motion_status() const noexcept {
        return kinematic_motion_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsKinematicMotion3DDiagnostic kinematic_motion_diagnostic() const noexcept {
        return kinematic_motion_result_.diagnostic;
    }

    [[nodiscard]] std::size_t kinematic_motion_row_count() const noexcept {
        return kinematic_motion_result_.rows.size();
    }

    [[nodiscard]] bool kinematic_motion_grounded() const noexcept {
        return kinematic_motion_result_.grounded;
    }

    [[nodiscard]] mirakana::PhysicsSimpleVehicle3DStatus vehicle_status() const noexcept {
        return simple_vehicle_result_.status;
    }

    [[nodiscard]] mirakana::PhysicsSimpleVehicle3DDiagnostic vehicle_diagnostic() const noexcept {
        return simple_vehicle_result_.diagnostic;
    }

    [[nodiscard]] std::size_t vehicle_wheel_row_count() const noexcept {
        return simple_vehicle_result_.wheel_rows.size();
    }

    [[nodiscard]] std::size_t vehicle_grounded_wheel_count() const noexcept {
        return simple_vehicle_result_.grounded_wheel_count;
    }

    [[nodiscard]] std::size_t vehicle_wheel_probe_hit_count() const noexcept {
        return simple_vehicle_result_.wheel_hit_count;
    }

    [[nodiscard]] float vehicle_final_x() const noexcept {
        return simple_vehicle_result_.position.x;
    }

    [[nodiscard]] float navigation_goal_x() const noexcept {
        return navigation_goal_.x;
    }

    [[nodiscard]] float navigation_goal_y() const noexcept {
        return navigation_goal_.y;
    }

    [[nodiscard]] float navigation_final_x() const noexcept {
        return navigation_agent_.position.x;
    }

    [[nodiscard]] float navigation_final_y() const noexcept {
        return navigation_agent_.position.y;
    }

    [[nodiscard]] mirakana::AiPerceptionStatus perception_status() const noexcept {
        return last_perception_status_;
    }

    [[nodiscard]] std::size_t perception_target_count() const noexcept {
        return last_perception_target_count_;
    }

    [[nodiscard]] bool perception_has_primary_target() const noexcept {
        return last_perception_has_primary_target_;
    }

    [[nodiscard]] mirakana::AiPerceptionEntityId perception_primary_target_id() const noexcept {
        return last_perception_primary_target_id_;
    }

    [[nodiscard]] float perception_primary_target_distance() const noexcept {
        return last_perception_primary_target_distance_;
    }

    [[nodiscard]] std::size_t perception_visible_count() const noexcept {
        return last_perception_visible_count_;
    }

    [[nodiscard]] std::size_t perception_audible_count() const noexcept {
        return last_perception_audible_count_;
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus blackboard_status() const noexcept {
        return last_blackboard_status_;
    }

    [[nodiscard]] bool blackboard_has_target() const noexcept {
        return blackboard_has_target_;
    }

    [[nodiscard]] bool blackboard_needs_move() const noexcept {
        return blackboard_needs_move_;
    }

    [[nodiscard]] mirakana::AiPerceptionEntityId blackboard_target_id() const noexcept {
        return blackboard_target_id_;
    }

    [[nodiscard]] std::size_t blackboard_visible_count() const noexcept {
        return blackboard_visible_count_;
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus behavior_status() const noexcept {
        return last_tree_result_.status;
    }

    [[nodiscard]] std::size_t behavior_visited_node_count() const noexcept {
        return last_tree_result_.visited_nodes.size();
    }

    [[nodiscard]] bool audio_voice_started() const noexcept {
        return audio_voice_started_;
    }

    [[nodiscard]] mirakana::AudioDeviceStreamStatus audio_stream_status() const noexcept {
        return audio_stream_status_;
    }

    [[nodiscard]] mirakana::AudioDeviceStreamDiagnostic audio_stream_diagnostic() const noexcept {
        return audio_stream_diagnostic_;
    }

    [[nodiscard]] std::size_t audio_render_command_count() const noexcept {
        return audio_render_command_count_;
    }

    [[nodiscard]] std::uint32_t audio_stream_frames() const noexcept {
        return audio_stream_frames_;
    }

    [[nodiscard]] std::size_t audio_stream_sample_count() const noexcept {
        return audio_stream_sample_count_;
    }

    [[nodiscard]] float audio_stream_first_sample() const noexcept {
        return audio_stream_first_sample_;
    }

    [[nodiscard]] float audio_stream_second_sample() const noexcept {
        return audio_stream_second_sample_;
    }

    [[nodiscard]] float audio_stream_sample_abs_sum() const noexcept {
        return audio_stream_sample_abs_sum_;
    }

    [[nodiscard]] const AudioGameplayMixerProbeResult& audio_gameplay_mixer_probe() const noexcept {
        return audio_gameplay_mixer_;
    }

    [[nodiscard]] bool interaction_ready() const noexcept {
        return interaction_plan_.succeeded && interaction_plan_.diagnostics.empty();
    }

    [[nodiscard]] std::size_t interaction_diagnostic_count() const noexcept {
        return interaction_plan_.diagnostics.size();
    }

    [[nodiscard]] std::size_t interaction_row_count() const noexcept {
        return interaction_plan_.rows.size();
    }

    [[nodiscard]] std::size_t interaction_feedback_row_count() const noexcept {
        return interaction_plan_.feedback_rows.size();
    }

    [[nodiscard]] mirakana::runtime::RuntimeGameplaySessionState interaction_final_session_state() const noexcept {
        return interaction_plan_.state.session_state;
    }

    [[nodiscard]] bool runtime_profile_resume_ready() const noexcept {
        return runtime_profile_resume_plan_.ready();
    }

    [[nodiscard]] mirakana::runtime::RuntimeSessionProfileResumeStatus runtime_profile_resume_status() const noexcept {
        return runtime_profile_resume_plan_.status;
    }

    [[nodiscard]] std::size_t runtime_profile_resume_diagnostic_count() const noexcept {
        return runtime_profile_resume_plan_.diagnostics.size();
    }

    [[nodiscard]] std::size_t runtime_profile_resume_loaded_document_count() const noexcept {
        return runtime_profile_resume_plan_.loaded_document_rows;
    }

    [[nodiscard]] std::size_t runtime_profile_resume_defaulted_document_count() const noexcept {
        return runtime_profile_resume_plan_.defaulted_document_rows;
    }

    [[nodiscard]] std::uint32_t runtime_profile_resume_save_schema_version() const noexcept {
        return runtime_profile_resume_plan_.save_schema_version;
    }

    [[nodiscard]] std::uint32_t runtime_profile_resume_settings_schema_version() const noexcept {
        return runtime_profile_resume_plan_.settings_schema_version;
    }

    [[nodiscard]] bool runtime_menu_hud_ready() const noexcept {
        return runtime_menu_hud_plan_.succeeded();
    }

    [[nodiscard]] std::size_t runtime_menu_hud_diagnostic_count() const noexcept {
        return runtime_menu_hud_plan_.diagnostics.size();
    }

    [[nodiscard]] std::size_t runtime_menu_hud_display_row_count() const noexcept {
        return runtime_menu_hud_plan_.display_rows.size();
    }

    [[nodiscard]] std::size_t runtime_menu_hud_command_row_count() const noexcept {
        return runtime_menu_hud_plan_.command_rows.size();
    }

    [[nodiscard]] std::size_t runtime_menu_hud_dialogue_row_count() const noexcept {
        return runtime_menu_hud_display_row_count(mirakana::ui::RuntimeMenuHudRowKind::dialogue_box);
    }

    [[nodiscard]] std::size_t runtime_menu_hud_input_binding_prompt_row_count() const noexcept {
        return runtime_menu_hud_display_row_count(mirakana::ui::RuntimeMenuHudRowKind::input_binding_prompt);
    }

    [[nodiscard]] std::string_view animation_state() const noexcept {
        return final_animation_sample_.to_state;
    }

    [[nodiscard]] float final_actor_x() const noexcept {
        return final_actor_position_.x;
    }

  private:
    [[nodiscard]] std::size_t
    runtime_menu_hud_display_row_count(mirakana::ui::RuntimeMenuHudRowKind kind) const noexcept {
        return static_cast<std::size_t>(
            std::count_if(runtime_menu_hud_plan_.display_rows.begin(), runtime_menu_hud_plan_.display_rows.end(),
                          [kind](const mirakana::ui::RuntimeMenuHudDisplayRow& row) { return row.kind == kind; }));
    }

    void build_gameplay_interaction_probe() {
        using namespace mirakana::runtime;

        const RuntimeGameplayInteractionState state{
            .session_state = RuntimeGameplaySessionState::running,
            .entities =
                std::vector<RuntimeGameplayEntityState>{
                    RuntimeGameplayEntityState{.id = "player", .health = 8, .max_health = 10, .active = true},
                    RuntimeGameplayEntityState{.id = "enemy", .health = 5, .max_health = 5, .active = true},
                },
            .pickups =
                std::vector<RuntimeGameplayPickupState>{
                    RuntimeGameplayPickupState{
                        .id = "pickup.relic", .item_id = "relic", .quantity = 1U, .available = true},
                },
            .objectives =
                std::vector<RuntimeGameplayObjectiveState>{
                    RuntimeGameplayObjectiveState{
                        .id = "objective.extract", .progress = 1U, .target = 3U, .completed = false},
                },
        };
        const std::vector<RuntimeGameplayInteractionEvent> events{
            RuntimeGameplayInteractionEvent{
                .id = "event.trigger",
                .kind = RuntimeGameplayInteractionKind::trigger,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.trigger",
                .amount = 0,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.damage",
                .kind = RuntimeGameplayInteractionKind::damage,
                .source_entity_id = "player",
                .target_entity_id = "enemy",
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.damage",
                .amount = 3,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.heal",
                .kind = RuntimeGameplayInteractionKind::heal,
                .source_entity_id = "player",
                .target_entity_id = "player",
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.heal",
                .amount = 2,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.pickup",
                .kind = RuntimeGameplayInteractionKind::pickup,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = "pickup.relic",
                .objective_id = {},
                .feedback_id = "feedback.pickup",
                .amount = 1,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.objective",
                .kind = RuntimeGameplayInteractionKind::objective_progress,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = "objective.extract",
                .feedback_id = "feedback.objective",
                .amount = 2,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.feedback",
                .kind = RuntimeGameplayInteractionKind::feedback,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.prompt",
                .amount = 0,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.win",
                .kind = RuntimeGameplayInteractionKind::win,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.win",
                .amount = 0,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.restart",
                .kind = RuntimeGameplayInteractionKind::restart,
                .source_entity_id = {},
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.restart",
                .amount = 0,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.loss",
                .kind = RuntimeGameplayInteractionKind::loss,
                .source_entity_id = "player",
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.loss",
                .amount = 0,
            },
            RuntimeGameplayInteractionEvent{
                .id = "event.restart_after_loss",
                .kind = RuntimeGameplayInteractionKind::restart,
                .source_entity_id = {},
                .target_entity_id = {},
                .pickup_id = {},
                .objective_id = {},
                .feedback_id = "feedback.restart_after_loss",
                .amount = 0,
            },
        };

        interaction_plan_ =
            plan_runtime_gameplay_interactions(state, std::span<const RuntimeGameplayInteractionEvent>{events});
    }

    void build_runtime_profile_resume_probe() {
        mirakana::MemoryFileSystem fs;
        mirakana::runtime::RuntimeSessionProfileDocuments documents;
        documents.save_data.schema_version = 3;
        documents.save_data.set_value("save.slot", "slot_1");
        documents.save_data.set_value("progression.checkpoint", "scene.generated_3d.ready");
        documents.save_data.set_value("package.id", "runtime/sample_generated_desktop_runtime_3d_package.geindex");
        documents.settings.schema_version = 2;
        documents.settings.set_value("settings.profile", "desktop_3d");
        documents.input_rebinding_profile.profile_id = "slot_1";

        const auto profile = mirakana::runtime::RuntimeSessionProfilePathRequest{
            .game_id = "sample_generated_desktop_runtime_3d_package", .profile_id = "slot_1", .root_path = "profiles"};
        const auto write = mirakana::runtime::write_runtime_session_profile_documents(
            fs,
            mirakana::runtime::RuntimeSessionProfileDocumentWriteRequest{.profile = profile, .documents = documents});
        const auto loaded = mirakana::runtime::load_runtime_session_profile_documents(
            fs, mirakana::runtime::RuntimeSessionProfileDocumentLoadRequest{.profile = profile, .defaults = documents});
        runtime_profile_resume_plan_ = mirakana::runtime::plan_runtime_session_profile_resume(
            mirakana::runtime::RuntimeSessionProfileResumeRequest{
                .documents = loaded,
                .expected_save_slot = "slot_1",
                .expected_progression_checkpoint = "scene.generated_3d.ready",
                .expected_package_id = "runtime/sample_generated_desktop_runtime_3d_package.geindex",
                .expected_profile_id = "slot_1"});
        if (!write.succeeded()) {
            runtime_profile_resume_plan_.status = mirakana::runtime::RuntimeSessionProfileResumeStatus::blocked;
        }
    }

    void build_runtime_menu_hud_probe() {
        runtime_menu_hud_plan_ = mirakana::ui::plan_runtime_menu_hud({
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "hud.prompt",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::prompt,
                .label = "Move through the generated scene",
            },
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "dialogue.intro",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::dialogue_box,
                .label = "Generated runtime scene ready",
            },
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "bindings.jump",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::input_binding_prompt,
                .label = "Space: jump",
            },
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "menu.pause",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::command,
                .label = "Pause",
                .command_id = "game.pause",
                .command_intent = mirakana::ui::RuntimeMenuHudCommandIntent::pause_game,
                .command_target = mirakana::ui::RuntimeMenuHudCommandTarget::game_session,
            },
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "menu.restart",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::command,
                .label = "Restart",
                .command_id = "game.restart",
                .command_intent = mirakana::ui::RuntimeMenuHudCommandIntent::restart_session,
                .command_target = mirakana::ui::RuntimeMenuHudCommandTarget::game_session,
            },
            mirakana::ui::RuntimeMenuHudRowDesc{
                .id = "hud.meshes",
                .kind = mirakana::ui::RuntimeMenuHudRowKind::counter,
                .label = "Scene meshes",
            },
        });
    }

    void build_authored_collision_probe() {
        mirakana::PhysicsAuthoredCollisionScene3DDesc scene;
        scene.world_config = mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}};
        scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
            .name = "floor",
            .body =
                mirakana::PhysicsBody3DDesc{
                    .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .mass = 0.0F,
                    .linear_damping = 0.0F,
                    .dynamic = false,
                    .half_extents = mirakana::Vec3{.x = 5.0F, .y = 0.5F, .z = 5.0F},
                },
        });
        scene.bodies.push_back(mirakana::PhysicsAuthoredCollisionBody3DDesc{
            .name = "wall",
            .body =
                mirakana::PhysicsBody3DDesc{
                    .position = mirakana::Vec3{.x = 3.0F, .y = 1.0F, .z = 0.0F},
                    .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                    .mass = 0.0F,
                    .linear_damping = 0.0F,
                    .dynamic = false,
                    .half_extents = mirakana::Vec3{.x = 0.5F, .y = 1.0F, .z = 2.0F},
                },
        });

        authored_collision_ = mirakana::build_physics_world_3d_from_authored_collision_scene(scene);
        if (authored_collision_.status != mirakana::PhysicsAuthoredCollision3DBuildStatus::success) {
            return;
        }

        mirakana::PhysicsCharacterController3DDesc request;
        request.position = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 0.0F};
        request.displacement = mirakana::Vec3{.x = 0.0F, .y = -3.0F, .z = 0.0F};
        request.radius = 0.5F;
        request.half_height = 0.5F;
        request.skin_width = 0.05F;
        controller_result_ = mirakana::move_physics_character_controller_3d(authored_collision_.world, request);
    }

    void build_navigation_agent() {
        mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 1});
        const auto plan =
            mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                                .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                                .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                                .mapping =
                                                                    mirakana::NavigationGridPointMapping{
                                                                        .origin = mirakana::NavigationPoint2{},
                                                                        .cell_size = 1.0F,
                                                                        .use_cell_centers = true,
                                                                    },
                                                                .adjacency = mirakana::NavigationAdjacency::cardinal4,
                                                                .smooth_path = true,
                                                            });
        navigation_plan_status_ = plan.status;
        navigation_plan_diagnostic_ = plan.diagnostic;
        navigation_path_point_count_ = plan.planned_grid_point_count;
        navigation_agent_ = plan.agent_state;
        if (!navigation_agent_.path.empty()) {
            navigation_goal_ = navigation_agent_.path.back();
        }
    }

    void build_navigation_navmesh_probe() {
        const std::vector<mirakana::NavigationNavmeshPolygon> polygons{
            mirakana::NavigationNavmeshPolygon{.id = 1U,
                                               .scene_ref = "scene/navmesh/start",
                                               .center = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 2U,
                                               .scene_ref = "scene/navmesh/blocked-direct",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 3U,
                                               .scene_ref = "scene/navmesh/goal",
                                               .center = mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 4U,
                                               .scene_ref = "scene/navmesh/bypass",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 1.0F}},
        };
        const std::vector<mirakana::NavigationNavmeshPortal> portals{
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 2U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 2U, .to = 3U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 4U, .cost = 2U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 4U, .to = 3U, .cost = 1U, .bidirectional = true},
        };
        const std::vector<mirakana::NavigationNavmeshDynamicObstacle> obstacles{
            mirakana::NavigationNavmeshDynamicObstacle{
                .id = 10U, .blocked_polygon = 2U, .scene_ref = "scene/obstacles/generated-crate", .enabled = true},
        };

        navigation_navmesh_result_ = mirakana::plan_navigation_navmesh_path(mirakana::NavigationNavmeshPathRequest{
            .polygons = std::span<const mirakana::NavigationNavmeshPolygon>{polygons},
            .portals = std::span<const mirakana::NavigationNavmeshPortal>{portals},
            .dynamic_obstacles = std::span<const mirakana::NavigationNavmeshDynamicObstacle>{obstacles},
            .start = 1U,
            .goal = 3U,
        });

        mirakana::NavigationNavmeshReadinessConfig readiness_config;
        readiness_config.require_scene_refs = true;
        readiness_config.require_dynamic_obstacle_route = true;
        readiness_config.min_polygon_path_rows = 3U;
        readiness_config.min_visited_polygons = 3U;
        readiness_config.max_total_cost = 5U;
        navigation_navmesh_readiness_ =
            mirakana::evaluate_navigation_navmesh_readiness(navigation_navmesh_result_, readiness_config);
    }

    void build_navigation_crowd_probe() {
        const std::vector<mirakana::NavigationNavmeshPolygon> polygons{
            mirakana::NavigationNavmeshPolygon{.id = 1U,
                                               .scene_ref = "scene/navmesh/start",
                                               .center = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 2U,
                                               .scene_ref = "scene/navmesh/blocked-direct",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 3U,
                                               .scene_ref = "scene/navmesh/goal",
                                               .center = mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F}},
            mirakana::NavigationNavmeshPolygon{.id = 4U,
                                               .scene_ref = "scene/navmesh/bypass",
                                               .center = mirakana::NavigationPoint2{.x = 1.0F, .y = 1.0F}},
        };
        const std::vector<mirakana::NavigationNavmeshPortal> portals{
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 2U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 2U, .to = 3U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 1U, .to = 4U, .cost = 1U, .bidirectional = true},
            mirakana::NavigationNavmeshPortal{.from = 4U, .to = 3U, .cost = 1U, .bidirectional = true},
        };
        const std::vector<mirakana::NavigationNavmeshDynamicObstacle> obstacles{
            mirakana::NavigationNavmeshDynamicObstacle{
                .id = 20U, .blocked_polygon = 2U, .scene_ref = "scene/obstacles/generated-crate", .enabled = true},
        };
        const auto config =
            mirakana::NavigationAgentConfig{.max_speed = 2.0F, .slowing_radius = 1.0F, .arrival_radius = 0.01F};
        const std::vector<mirakana::NavigationCrowdAgentDesc> agents{
            mirakana::NavigationCrowdAgentDesc{
                .id = 20U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .start_polygon = 1U,
                .goal_polygon = 3U,
                .radius = 0.5F,
                .config = config,
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            },
            mirakana::NavigationCrowdAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.25F, .y = 0.0F},
                .start_polygon = 1U,
                .goal_polygon = 3U,
                .radius = 0.5F,
                .config = config,
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            },
        };

        navigation_crowd_result_ = mirakana::plan_navigation_navmesh_crowd(mirakana::NavigationCrowdPlanRequest{
            .polygons = std::span<const mirakana::NavigationNavmeshPolygon>{polygons},
            .portals = std::span<const mirakana::NavigationNavmeshPortal>{portals},
            .dynamic_obstacles = std::span<const mirakana::NavigationNavmeshDynamicObstacle>{obstacles},
            .agents = std::span<const mirakana::NavigationCrowdAgentDesc>{agents},
            .max_agents = 4U,
            .separation_weight = 2.0F,
            .prediction_time_seconds = 0.0F,
        });

        mirakana::NavigationCrowdReadinessConfig readiness_config;
        readiness_config.require_source_order = true;
        readiness_config.require_route_success = true;
        readiness_config.require_avoidance_success = true;
        readiness_config.require_applied_neighbors = true;
        readiness_config.require_dynamic_obstacles = true;
        readiness_config.min_planned_agents = 2U;
        readiness_config.max_rows = 2U;
        navigation_crowd_readiness_ =
            mirakana::evaluate_navigation_crowd_readiness(navigation_crowd_result_, readiness_config);
    }

    void build_physics_movement_policy_probe() {
        constexpr std::uint32_t character_layer = 1U << 0U;
        constexpr std::uint32_t trigger_layer = 1U << 1U;
        constexpr std::uint32_t dynamic_layer = 1U << 2U;
        constexpr std::uint32_t solid_layer = 1U << 3U;

        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 1.0F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = trigger_layer,
            .collision_mask = character_layer,
            .trigger = true,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 2.5F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = dynamic_layer,
            .collision_mask = character_layer,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .collision_layer = solid_layer,
            .collision_mask = character_layer,
        });

        mirakana::PhysicsCharacterDynamicPolicy3DDesc request;
        request.position = mirakana::Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
        request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 0.0F};
        request.radius = 0.5F;
        request.half_height = 0.5F;
        request.character_layer = character_layer;
        request.collision_mask = trigger_layer | dynamic_layer | solid_layer;
        request.include_triggers = true;
        request.skin_width = 0.05F;
        request.dynamic_push_distance = 0.25F;
        physics_policy_result_ = mirakana::evaluate_physics_character_dynamic_policy_3d(world, request);
        physics_policy_dynamic_push_count_ = 0U;
        physics_policy_solid_contact_count_ = 0U;
        physics_policy_trigger_overlap_count_ = 0U;
        for (const auto& row : physics_policy_result_.rows) {
            if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::dynamic_push) {
                ++physics_policy_dynamic_push_count_;
            } else if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::solid_contact) {
                ++physics_policy_solid_contact_count_;
            } else if (row.kind == mirakana::PhysicsCharacterDynamicPolicy3DRowKind::trigger_overlap) {
                ++physics_policy_trigger_overlap_count_;
            }
        }
    }

    void build_physics_advanced_controller_probe() {
        constexpr std::uint32_t character_layer = 1U << 0U;
        constexpr std::uint32_t platform_layer = 1U << 1U;
        constexpr std::uint32_t trigger_layer = 1U << 2U;
        constexpr std::uint32_t dynamic_layer = 1U << 3U;
        constexpr std::uint32_t solid_layer = 1U << 4U;

        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        const auto platform = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 1.5F, .y = -0.5F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 4.0F, .y = 0.5F, .z = 4.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = platform_layer,
            .collision_mask = character_layer,
            .half_height = 0.5F,
            .trigger = false,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 1.2F, .y = 1.05F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 1.0F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = trigger_layer,
            .collision_mask = character_layer,
            .half_height = 0.5F,
            .trigger = true,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 2.2F, .y = 1.05F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.5F, .z = 0.5F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = dynamic_layer,
            .collision_mask = character_layer,
            .half_height = 0.5F,
            .trigger = false,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 2.3F, .y = 0.25F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.25F, .z = 1.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = solid_layer,
            .collision_mask = character_layer,
            .half_height = 0.5F,
            .trigger = false,
        });
        const auto controller_proxy = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = 1U,
            .collision_mask = 0xFFFF'FFFFU,
            .half_height = 0.5F,
            .trigger = false,
        });
        const auto follower = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.9F, .y = 1.05F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = 1U,
            .collision_mask = 0xFFFF'FFFFU,
            .half_height = 0.5F,
            .trigger = false,
        });

        mirakana::PhysicsAdvancedController3DDesc request;
        request.movement.position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F};
        request.movement.displacement = mirakana::Vec3{.x = 3.0F, .y = 0.0F, .z = 0.0F};
        request.movement.radius = 0.5F;
        request.movement.half_height = 0.5F;
        request.movement.character_layer = character_layer;
        request.movement.collision_mask = platform_layer | trigger_layer | dynamic_layer | solid_layer;
        request.movement.include_triggers = true;
        request.movement.skin_width = 0.02F;
        request.movement.step_height = 0.65F;
        request.movement.ground_probe_distance = 0.2F;
        request.movement.grounded_normal_y = 0.70F;
        request.movement.max_slope_normal_y = 0.70F;
        request.movement.dynamic_push_distance = 0.25F;
        request.moving_platforms.push_back(mirakana::PhysicsMovingPlatform3DDesc{
            .body = platform,
            .displacement = mirakana::Vec3{.x = 0.25F, .y = 0.0F, .z = 0.0F},
        });
        request.controller_body = controller_proxy;
        request.constraints.config.iterations = 4U;
        request.constraints.distance_joints.push_back(mirakana::PhysicsDistanceJoint3DDesc{
            .first = controller_proxy,
            .second = follower,
            .rest_distance = 1.0F,
        });

        advanced_controller_result_ = mirakana::plan_physics_advanced_controller_3d(world, request);
        advanced_controller_platform_applied_count_ = 0U;
        for (const auto& row : advanced_controller_result_.moving_platform_rows) {
            if (row.applied) {
                ++advanced_controller_platform_applied_count_;
            }
        }
        mirakana::PhysicsCharacterDynamics3DReadinessConfig readiness_config;
        readiness_config.require_dynamic_push = true;
        readiness_config.require_step_up = true;
        readiness_config.require_walkable_slope = true;
        readiness_config.require_ground_probe = true;
        readiness_config.require_moving_platform = true;
        readiness_config.require_constraint = true;
        readiness_config.require_replay_change = true;
        readiness_config.max_movement_rows = 8U;
        character_dynamics_readiness_ =
            mirakana::evaluate_physics_character_dynamics_readiness_3d(advanced_controller_result_, readiness_config);
    }

    void build_physics_constraints_probe() {
        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        const auto fixed_first = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
        });
        const auto fixed_second = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
        });
        const auto linear_axis_first = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 10.0F, .y = 0.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
        });
        const auto linear_axis_second = world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 14.0F, .y = 3.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 1.0F,
            .linear_damping = 0.0F,
            .dynamic = true,
            .half_extents = mirakana::Vec3{.x = 0.1F, .y = 0.1F, .z = 0.1F},
            .collision_enabled = false,
        });

        physics_constraints_result_ = mirakana::solve_physics_constraints_3d(
            world, mirakana::PhysicsConstraintSolve3DDesc{
                       .config =
                           mirakana::PhysicsConstraintSolve3DConfig{
                               .iterations = 1U,
                               .tolerance = 0.0001F,
                               .max_rows = std::numeric_limits<std::size_t>::max(),
                           },
                       .distance_joints = {},
                       .fixed_constraints =
                           std::vector<mirakana::PhysicsFixedConstraint3DDesc>{
                               mirakana::PhysicsFixedConstraint3DDesc{
                                   .first = fixed_first,
                                   .second = fixed_second,
                                   .target_offset = mirakana::Vec3{.x = 2.0F, .y = 0.0F, .z = 0.0F},
                               },
                           },
                       .linear_axis_constraints =
                           std::vector<mirakana::PhysicsLinearAxisConstraint3DDesc>{
                               mirakana::PhysicsLinearAxisConstraint3DDesc{
                                   .first = linear_axis_first,
                                   .second = linear_axis_second,
                                   .axis = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                   .min_axis_distance = 0.0F,
                                   .max_axis_distance = 2.0F,
                               },
                           },
                   });
    }

    void build_physics_kinematic_motion_probe() {
        constexpr std::uint32_t vehicle_layer = 1U << 0U;
        constexpr std::uint32_t floor_layer = 1U << 1U;
        constexpr std::uint32_t wall_layer = 1U << 2U;

        mirakana::PhysicsWorld3D world(mirakana::PhysicsWorld3DConfig{mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F}});
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 0.0F, .y = -0.5F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 8.0F, .y = 0.5F, .z = 8.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = floor_layer,
            .collision_mask = vehicle_layer,
        });
        (void)world.create_body(mirakana::PhysicsBody3DDesc{
            .position = mirakana::Vec3{.x = 4.0F, .y = 1.0F, .z = 0.0F},
            .velocity = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .mass = 0.0F,
            .linear_damping = 0.0F,
            .dynamic = false,
            .half_extents = mirakana::Vec3{.x = 0.25F, .y = 1.0F, .z = 8.0F},
            .collision_enabled = true,
            .shape = mirakana::PhysicsShape3DKind::aabb,
            .radius = 0.5F,
            .collision_layer = wall_layer,
            .collision_mask = vehicle_layer,
        });

        mirakana::PhysicsKinematicMotion3DDesc request;
        request.position = mirakana::Vec3{.x = 0.0F, .y = 1.05F, .z = 0.0F};
        request.displacement = mirakana::Vec3{.x = 5.0F, .y = 0.0F, .z = 1.0F};
        request.shape = mirakana::PhysicsShape3DDesc::capsule(0.5F, 0.5F);
        request.filter.collision_mask = floor_layer | wall_layer;
        request.filter.include_triggers = false;
        request.skin_width = 0.02F;
        request.ground_probe_distance = 0.2F;
        kinematic_motion_result_ = mirakana::plan_physics_kinematic_motion_3d(world, request);

        mirakana::PhysicsSimpleVehicle3DDesc vehicle_request;
        vehicle_request.motion.position = mirakana::Vec3{.x = 0.0F, .y = 0.45F, .z = 0.0F};
        vehicle_request.motion.displacement = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
        vehicle_request.motion.shape =
            mirakana::PhysicsShape3DDesc::aabb(mirakana::Vec3{.x = 0.8F, .y = 0.25F, .z = 0.5F});
        vehicle_request.motion.filter.collision_mask = floor_layer;
        vehicle_request.motion.filter.include_triggers = false;
        vehicle_request.motion.skin_width = 0.02F;
        vehicle_request.motion.ground_probe_distance = 0.3F;
        vehicle_request.wheel_filter.collision_mask = floor_layer;
        vehicle_request.wheel_filter.include_triggers = false;
        vehicle_request.grounded_normal_y = 0.70F;
        vehicle_request.wheels = std::vector<mirakana::PhysicsSimpleVehicle3DWheelDesc>{
            mirakana::PhysicsSimpleVehicle3DWheelDesc{
                .local_offset = mirakana::Vec3{.x = -0.55F, .y = 0.0F, .z = -0.35F},
                .radius = 0.2F,
                .ground_probe_distance = 0.5F,
            },
            mirakana::PhysicsSimpleVehicle3DWheelDesc{
                .local_offset = mirakana::Vec3{.x = 0.55F, .y = 0.0F, .z = -0.35F},
                .radius = 0.2F,
                .ground_probe_distance = 0.5F,
            },
            mirakana::PhysicsSimpleVehicle3DWheelDesc{
                .local_offset = mirakana::Vec3{.x = -0.55F, .y = 0.0F, .z = 0.35F},
                .radius = 0.2F,
                .ground_probe_distance = 0.5F,
            },
            mirakana::PhysicsSimpleVehicle3DWheelDesc{
                .local_offset = mirakana::Vec3{.x = 0.55F, .y = 0.0F, .z = 0.35F},
                .radius = 0.2F,
                .ground_probe_distance = 0.5F,
            },
        };
        simple_vehicle_result_ = mirakana::plan_physics_simple_vehicle_3d(world, vehicle_request);
    }

    void render_audio_stream_probe() {
        mirakana::AudioMixer mixer;
        const mirakana::AudioClipSampleData fallback_samples{
            .clip = packaged_audio_asset_id(),
            .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                  .channel_count = 1,
                                                  .sample_format = mirakana::AudioSampleFormat::float32},
            .frame_count = 4,
            .interleaved_float_samples = {0.1F, 0.2F, 0.3F, 0.4F},
        };
        const mirakana::AudioClipSampleData& clip_samples =
            audio_samples_.has_value() ? *audio_samples_ : fallback_samples;
        const auto clip = clip_samples.clip;
        mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                               .sample_rate = clip_samples.format.sample_rate,
                                               .channel_count = clip_samples.format.channel_count,
                                               .frame_count = clip_samples.frame_count,
                                               .sample_format = clip_samples.format.sample_format,
                                               .streaming = false,
                                               .buffered_frame_count = clip_samples.frame_count});
        const auto voice =
            mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});
        audio_voice_started_ = voice != mirakana::null_audio_voice;

        const std::vector<mirakana::AudioClipSampleData> samples{
            clip_samples,
        };
        audio_gameplay_mixer_ = validate_audio_gameplay_mixer_package_evidence(samples[0]);
        const auto output = mirakana::render_audio_device_stream_interleaved_float(mixer,
                                                                                   mirakana::AudioDeviceStreamRequest{
                                                                                       .format = clip_samples.format,
                                                                                       .device_frame = 0,
                                                                                       .queued_frames = 2,
                                                                                       .target_queued_frames = 4,
                                                                                       .max_render_frames = 2,
                                                                                   },
                                                                                   samples);
        audio_stream_status_ = output.plan.status;
        audio_stream_diagnostic_ = output.plan.diagnostic;
        audio_render_command_count_ = output.buffer.plan.commands.size();
        audio_stream_frames_ = output.buffer.frame_count;
        audio_stream_sample_count_ = output.buffer.interleaved_float_samples.size();
        if (!output.buffer.interleaved_float_samples.empty()) {
            audio_stream_first_sample_ = output.buffer.interleaved_float_samples.front();
        }
        if (output.buffer.interleaved_float_samples.size() > 1U) {
            audio_stream_second_sample_ = output.buffer.interleaved_float_samples[1U];
        }
        for (const auto sample : output.buffer.interleaved_float_samples) {
            audio_stream_sample_abs_sum_ += std::abs(sample);
        }
    }

    void update_navigation_local_avoidance_probe() {
        if (navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
            navigation_agent_.path.empty()) {
            return;
        }

        const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors{
            mirakana::NavigationLocalAvoidanceNeighborDesc{
                .id = 200U,
                .position = mirakana::NavigationPoint2{.x = navigation_agent_.position.x + 0.25F,
                                                       .y = navigation_agent_.position.y},
                .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .radius = 0.35F,
            },
        };
        const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 100U,
                    .position = navigation_agent_.position,
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.35F,
                    .max_speed = 2.0F,
                },
            .neighbors = std::span<const mirakana::NavigationLocalAvoidanceNeighborDesc>{neighbors},
            .separation_weight = 1.5F,
            .prediction_time_seconds = 1.0F,
            .epsilon = 0.0001F,
        });
        local_avoidance_status_ = result.status;
        local_avoidance_diagnostic_ = result.diagnostic;
        if (result.status == mirakana::NavigationLocalAvoidanceStatus::success) {
            ++local_avoidance_steps_;
            local_avoidance_applied_neighbor_count_ += result.applied_neighbor_count;
        }
    }

    void update_ai_navigation_composition() {
        if (navigation_plan_status_ != mirakana::NavigationGridAgentPathStatus::ready ||
            navigation_agent_.path.empty()) {
            return;
        }

        const std::vector<mirakana::AiPerceptionTarget2D> route_targets{
            mirakana::AiPerceptionTarget2D{
                .id = 1U,
                .position = mirakana::AiPerceptionPoint2{.x = navigation_agent_.path.back().x,
                                                         .y = navigation_agent_.path.back().y},
                .radius = 0.0F,
                .sight_enabled = true,
                .hearing_enabled = false,
                .sound_radius = 0.0F,
            },
        };
        const auto perception = mirakana::build_ai_perception_snapshot_2d(mirakana::AiPerceptionRequest2D{
            .agent = mirakana::AiPerceptionAgent2D{.id = 100U,
                                                   .position =
                                                       mirakana::AiPerceptionPoint2{.x = navigation_agent_.position.x,
                                                                                    .y = navigation_agent_.position.y},
                                                   .forward = mirakana::AiPerceptionPoint2{.x = 1.0F, .y = 0.0F},
                                                   .sight_range = 4.0F,
                                                   .field_of_view_radians = 6.28318530718F,
                                                   .hearing_range = 0.0F},
            .targets = std::span<const mirakana::AiPerceptionTarget2D>{route_targets},
        });
        last_perception_status_ = perception.status;
        last_perception_target_count_ = perception.targets.size();
        last_perception_has_primary_target_ = perception.has_primary_target;
        if (perception.has_primary_target) {
            last_perception_primary_target_id_ = perception.primary_target.id;
            last_perception_primary_target_distance_ = perception.primary_target.distance;
        }
        last_perception_visible_count_ = perception.visible_count;
        last_perception_audible_count_ = perception.audible_count;

        mirakana::BehaviorTreeBlackboard blackboard;
        const auto blackboard_result =
            mirakana::write_ai_perception_blackboard(perception,
                                                     mirakana::AiPerceptionBlackboardKeys{
                                                         .has_target_key = kGameplaySystemsHasTargetKey,
                                                         .target_id_key = kGameplaySystemsTargetIdKey,
                                                         .target_distance_key = kGameplaySystemsTargetDistanceKey,
                                                         .visible_count_key = kGameplaySystemsVisibleTargetsKey,
                                                         .audible_count_key = kGameplaySystemsAudibleTargetsKey,
                                                         .target_state_key = kGameplaySystemsTargetStateKey,
                                                     },
                                                     blackboard);
        last_blackboard_status_ = blackboard_result.status;
        if (blackboard_result.status != mirakana::AiPerceptionBlackboardStatus::ready ||
            !blackboard.set(kGameplaySystemsNeedsMoveKey, mirakana::make_behavior_tree_blackboard_bool(true))) {
            return;
        }

        if (const auto* value = blackboard.find(kGameplaySystemsHasTargetKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_has_target_ = value->bool_value;
        }
        if (const auto* value = blackboard.find(kGameplaySystemsNeedsMoveKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::boolean) {
            blackboard_needs_move_ = value->bool_value;
        }
        if (const auto* value = blackboard.find(kGameplaySystemsTargetIdKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::signed_integer &&
            value->integer_value >= 0) {
            blackboard_target_id_ = static_cast<mirakana::AiPerceptionEntityId>(value->integer_value);
        }
        if (const auto* value = blackboard.find(kGameplaySystemsVisibleTargetsKey);
            value != nullptr && value->kind == mirakana::BehaviorTreeBlackboardValueKind::signed_integer &&
            value->integer_value >= 0) {
            blackboard_visible_count_ = static_cast<std::size_t>(value->integer_value);
        }

        const std::vector<mirakana::BehaviorTreeBlackboardCondition> conditions{
            mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplaySystemsHasTargetNode,
                                                      .key = kGameplaySystemsHasTargetKey,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
            mirakana::BehaviorTreeBlackboardCondition{.node_id = kGameplaySystemsNeedsMoveNode,
                                                      .key = kGameplaySystemsNeedsMoveKey,
                                                      .comparison = mirakana::BehaviorTreeBlackboardComparison::equal,
                                                      .expected = mirakana::make_behavior_tree_blackboard_bool(true)},
        };
        const auto supporting_systems_ready =
            authored_collision_.status == mirakana::PhysicsAuthoredCollision3DBuildStatus::success &&
            controller_result_.grounded && audio_stream_status_ == mirakana::AudioDeviceStreamStatus::ready;
        const std::vector<mirakana::BehaviorTreeLeafResult> leaf_results{
            mirakana::BehaviorTreeLeafResult{.node_id = kGameplaySystemsMoveActionNode,
                                             .status = supporting_systems_ready
                                                           ? mirakana::BehaviorTreeStatus::success
                                                           : mirakana::BehaviorTreeStatus::failure},
        };

        last_tree_result_ = mirakana::evaluate_behavior_tree(
            mirakana::BehaviorTreeDesc{
                .root_id = kGameplaySystemsRootNode,
                .nodes =
                    {
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplaySystemsRootNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::sequence,
                                                       .children = {kGameplaySystemsHasTargetNode,
                                                                    kGameplaySystemsNeedsMoveNode,
                                                                    kGameplaySystemsMoveActionNode}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplaySystemsHasTargetNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::condition,
                                                       .children = {}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplaySystemsNeedsMoveNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::condition,
                                                       .children = {}},
                        mirakana::BehaviorTreeNodeDesc{.id = kGameplaySystemsMoveActionNode,
                                                       .kind = mirakana::BehaviorTreeNodeKind::action,
                                                       .children = {}},
                    },
            },
            mirakana::BehaviorTreeEvaluationContext{
                .leaf_results = std::span<const mirakana::BehaviorTreeLeafResult>{leaf_results},
                .blackboard_entries = blackboard.entries(),
                .blackboard_conditions = std::span<const mirakana::BehaviorTreeBlackboardCondition>{conditions},
            });

        if (last_tree_result_.status != mirakana::BehaviorTreeStatus::success ||
            navigation_agent_.status == mirakana::NavigationAgentStatus::reached_destination) {
            return;
        }

        const auto update = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
            .state = navigation_agent_,
            .config =
                mirakana::NavigationAgentConfig{.max_speed = 8.0F, .slowing_radius = 1.0F, .arrival_radius = 0.001F},
            .delta_seconds = 1.0F,
        });
        navigation_agent_ = update.state;
    }

    mirakana::PhysicsWorld3D physics_;
    mirakana::AnimationStateMachine animation_;
    std::optional<mirakana::AudioClipSampleData> audio_samples_;
    mirakana::PhysicsAuthoredCollisionScene3DBuildResult authored_collision_;
    mirakana::PhysicsCharacterController3DResult controller_result_;
    mirakana::PhysicsCharacterDynamicPolicy3DResult physics_policy_result_;
    mirakana::PhysicsAdvancedController3DResult advanced_controller_result_;
    mirakana::PhysicsCharacterDynamics3DReadinessReport character_dynamics_readiness_;
    mirakana::PhysicsConstraintSolve3DResult physics_constraints_result_;
    mirakana::PhysicsKinematicMotion3DResult kinematic_motion_result_;
    mirakana::PhysicsSimpleVehicle3DResult simple_vehicle_result_;
    mirakana::runtime::RuntimeGameplayInteractionPlan interaction_plan_;
    mirakana::runtime::RuntimeSessionProfileResumePlan runtime_profile_resume_plan_;
    mirakana::ui::RuntimeMenuHudPlan runtime_menu_hud_plan_;
    AudioGameplayMixerProbeResult audio_gameplay_mixer_;
    mirakana::NavigationNavmeshPathResult navigation_navmesh_result_;
    mirakana::NavigationNavmeshReadinessReport navigation_navmesh_readiness_;
    mirakana::NavigationCrowdPlanResult navigation_crowd_result_;
    mirakana::NavigationCrowdReadinessReport navigation_crowd_readiness_;
    mirakana::NavigationAgentState navigation_agent_;
    mirakana::BehaviorTreeTickResult last_tree_result_;
    mirakana::PhysicsBody3DId floor_body_{mirakana::null_physics_body_3d};
    mirakana::PhysicsBody3DId actor_body_{mirakana::null_physics_body_3d};
    mirakana::Vec3 final_actor_position_{.x = 0.0F, .y = 0.0F, .z = 0.0F};
    mirakana::AnimationStateMachineSample final_animation_sample_;
    mirakana::NavigationGridAgentPathStatus navigation_plan_status_{
        mirakana::NavigationGridAgentPathStatus::invalid_request};
    mirakana::NavigationGridAgentPathDiagnostic navigation_plan_diagnostic_{
        mirakana::NavigationGridAgentPathDiagnostic::none};
    mirakana::NavigationLocalAvoidanceStatus local_avoidance_status_{
        mirakana::NavigationLocalAvoidanceStatus::invalid_request};
    mirakana::NavigationLocalAvoidanceDiagnostic local_avoidance_diagnostic_{
        mirakana::NavigationLocalAvoidanceDiagnostic::none};
    mirakana::NavigationPoint2 navigation_goal_{};
    mirakana::AiPerceptionStatus last_perception_status_{mirakana::AiPerceptionStatus::invalid_agent};
    mirakana::AiPerceptionBlackboardStatus last_blackboard_status_{
        mirakana::AiPerceptionBlackboardStatus::invalid_snapshot};
    mirakana::AiPerceptionEntityId last_perception_primary_target_id_{0U};
    float last_perception_primary_target_distance_{0.0F};
    std::size_t last_perception_visible_count_{0U};
    std::size_t last_perception_audible_count_{0U};
    bool last_perception_has_primary_target_{false};
    bool blackboard_has_target_{false};
    bool blackboard_needs_move_{false};
    mirakana::AiPerceptionEntityId blackboard_target_id_{0U};
    std::size_t blackboard_visible_count_{0U};
    mirakana::AudioDeviceStreamStatus audio_stream_status_{mirakana::AudioDeviceStreamStatus::invalid_request};
    mirakana::AudioDeviceStreamDiagnostic audio_stream_diagnostic_{mirakana::AudioDeviceStreamDiagnostic::none};
    std::size_t audio_render_command_count_{0U};
    float audio_stream_first_sample_{0.0F};
    float audio_stream_second_sample_{0.0F};
    float audio_stream_sample_abs_sum_{0.0F};
    std::size_t local_avoidance_applied_neighbor_count_{0U};
    std::size_t physics_policy_dynamic_push_count_{0U};
    std::size_t physics_policy_solid_contact_count_{0U};
    std::size_t physics_policy_trigger_overlap_count_{0U};
    std::size_t advanced_controller_platform_applied_count_{0U};
    std::size_t navigation_path_point_count_{0U};
    std::size_t last_perception_target_count_{0U};
    std::uint32_t audio_stream_frames_{0U};
    std::size_t audio_stream_sample_count_{0U};
    std::uint32_t local_avoidance_steps_{0U};
    std::uint32_t ticks_{0U};
    std::uint32_t physics_ticks_{0U};
    bool audio_voice_started_{false};
    bool started_{false};
};

class GeneratedDesktopRuntime3DPackageGame final : public mirakana::GameApp {
  public:
    GeneratedDesktopRuntime3DPackageGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                                         std::optional<mirakana::RuntimeSceneRenderInstance> scene,
                                         std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip,
                                         mirakana::AnimationTransformBindingSourceDocument animation_bindings,
                                         std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> morph_payload,
                                         std::optional<mirakana::AnimationFloatClipSourceDocument> morph_animation_clip,
                                         std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks,
                                         std::optional<mirakana::AudioClipSampleData> audio_samples,
                                         bool textured_ui_atlas_mode, mirakana::UiRendererImagePalette image_palette,
                                         bool text_glyph_ui_atlas_mode,
                                         mirakana::UiRendererGlyphAtlasPalette glyph_atlas)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          animation_clip_(std::move(animation_clip)), animation_bindings_(std::move(animation_bindings)),
          morph_payload_(std::move(morph_payload)), morph_animation_clip_(std::move(morph_animation_clip)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)),
          image_palette_(std::move(image_palette)), glyph_atlas_(std::move(glyph_atlas)),
          gameplay_systems_(std::move(audio_samples)), textured_ui_atlas_mode_(textured_ui_atlas_mode),
          text_glyph_ui_atlas_mode_(text_glyph_ui_atlas_mode) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{.r = 0.025F, .g = 0.035F, .b = 0.045F, .a = 1.0F});
        input_.press(mirakana::Key::right);
        gameplay_systems_.start();
        if (scene_.has_value() && scene_->scene.has_value()) {
            scene_gameplay_bindings_ = validate_generated_3d_scene_gameplay_bindings(*scene_->scene);
        }
        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        gameplay_systems_.tick();
        renderer_.begin_frame();

        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        if (scene_.has_value()) {
            std::optional<mirakana::SceneRenderPacket> rebuilt_packet;
            const auto* render_packet = &scene_->render_packet;
            if (scene_->scene.has_value()) {
                if (auto* camera = scene_->scene->find_node(kPrimaryCameraNode);
                    camera != nullptr && camera->components.camera.has_value() && camera->components.camera->primary) {
                    camera->transform.position.x += axis.x;
                    final_camera_x_ = camera->transform.position.x;
                    ++camera_controller_ticks_;
                    primary_camera_seen_ = true;
                }
                if (animation_clip_.has_value()) {
                    const auto animation_result = mirakana::sample_and_apply_runtime_scene_render_animation_float_clip(
                        *scene_, *animation_clip_, animation_bindings_, 1.0F);
                    transform_animation_samples_ += animation_result.sampled_track_count;
                    transform_animation_applied_ += animation_result.applied_sample_count;
                    if (animation_result.succeeded) {
                        ++transform_animation_ticks_;
                        transform_animation_seen_ = true;
                        if (const auto* animated_mesh = scene_->scene->find_node(kPackagedMeshNode);
                            animated_mesh != nullptr) {
                            final_mesh_x_ = animated_mesh->transform.position.x;
                        }
                    } else {
                        ++transform_animation_failures_;
                    }
                }
                if (morph_payload_.has_value() && morph_animation_clip_.has_value()) {
                    const auto morph_result = mirakana::sample_runtime_morph_mesh_cpu_animation_float_clip(
                        *morph_payload_, *morph_animation_clip_, "gltf/node/0/weights/", 1.0F);
                    morph_package_samples_ += morph_result.sampled_track_count;
                    morph_package_weights_ += morph_result.applied_weight_count;
                    morph_package_vertices_ += morph_result.morphed_positions.size();
                    if (morph_result.succeeded && !morph_result.morphed_positions.empty()) {
                        const auto vertex_count = static_cast<std::uint64_t>(morph_result.morphed_positions.size());
                        if (morph_package_vertices_per_sample_ == 0U) {
                            morph_package_vertices_per_sample_ = vertex_count;
                        }
                        if (morph_package_vertices_per_sample_ == vertex_count) {
                            ++morph_package_ticks_;
                            morph_package_seen_ = true;
                            morph_first_position_x_ = morph_result.morphed_positions.front().x;
                        } else {
                            ++morph_package_failures_;
                        }
                    } else {
                        ++morph_package_failures_;
                    }
                }
                if (!quaternion_animation_tracks_.empty()) {
                    const auto apply_result = mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d(
                        *scene_, packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                    quaternion_animation_tracks_sampled_ += apply_result.sampled_track_count;
                    if (apply_result.succeeded) {
                        const auto pose = mirakana::sample_animation_local_pose_3d(
                            packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                        const auto* animated_mesh = scene_->scene->find_node(kPackagedMeshNode);
                        quaternion_animation_scene_applied_ +=
                            static_cast<std::uint32_t>(apply_result.applied_sample_count);
                        if (pose.joints.size() == 1U && animated_mesh != nullptr) {
                            ++quaternion_animation_ticks_;
                            quaternion_animation_seen_ = true;
                            final_quaternion_z_ = pose.joints[0].rotation.z;
                            final_quaternion_w_ = pose.joints[0].rotation.w;
                            final_quaternion_scene_rotation_z_ = animated_mesh->transform.rotation_radians.z;
                        } else {
                            ++quaternion_animation_failures_;
                        }
                    } else {
                        ++quaternion_animation_failures_;
                    }
                }
                rebuilt_packet = mirakana::build_scene_render_packet(*scene_->scene);
                render_packet = &*rebuilt_packet;
            }

            const auto mesh_plan = mirakana::plan_scene_mesh_draws(*render_packet);
            scene_mesh_plan_ok_ = scene_mesh_plan_ok_ && mesh_plan.succeeded();
            scene_mesh_plan_meshes_ += mesh_plan.mesh_count;
            scene_mesh_plan_draws_ += mesh_plan.draw_count;
            scene_mesh_plan_unique_meshes_ += mesh_plan.unique_mesh_count;
            scene_mesh_plan_unique_materials_ += mesh_plan.unique_material_count;
            scene_mesh_plan_diagnostics_ += mesh_plan.diagnostics.size();
            const auto scene_submit = mirakana::submit_scene_render_packet(
                renderer_, *render_packet,
                mirakana::SceneRenderSubmitDesc{
                    .fallback_mesh_color = mirakana::Color{.r = 0.35F, .g = 0.75F, .b = 0.45F, .a = 1.0F},
                    .material_palette = &scene_->material_palette,
                });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
            primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;
        } else {
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{
                .transform = transform_, .color = mirakana::Color{.r = 0.35F, .g = 0.75F, .b = 0.45F, .a = 1.0F}});
        }

        update_hud_text();
        const auto layout =
            mirakana::ui::solve_layout(hud_, mirakana::ui::ElementId{"hud.root"},
                                       mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 320.0F, .height = 180.0F});
        const auto submission = mirakana::ui::build_renderer_submission(hud_, layout);
        mirakana::UiRenderSubmitDesc ui_submit_desc;
        ui_submit_desc.theme = &theme_;
        if (textured_ui_atlas_mode_) {
            ui_submit_desc.image_palette = &image_palette_;
        }
        const auto hud_submit = mirakana::submit_ui_renderer_submission(renderer_, submission, ui_submit_desc);
        hud_boxes_submitted_ += hud_submit.boxes_submitted;
        hud_images_submitted_ += hud_submit.image_sprites_submitted;
        if (text_glyph_ui_atlas_mode_) {
            const auto glyph_submit = submit_hud_text_glyph_proof();
            hud_text_glyphs_available_ += glyph_submit.text_glyphs_available;
            hud_text_glyphs_resolved_ += glyph_submit.text_glyphs_resolved;
            hud_text_glyphs_missing_ += glyph_submit.text_glyphs_missing;
            hud_text_glyph_sprites_submitted_ += glyph_submit.text_glyph_sprites_submitted;
        }

        renderer_.end_frame();
        ++frames_;

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    [[nodiscard]] std::uint32_t frames() const noexcept {
        return frames_;
    }

    [[nodiscard]] std::size_t scene_meshes_submitted() const noexcept {
        return scene_meshes_submitted_;
    }

    [[nodiscard]] std::size_t scene_materials_resolved() const noexcept {
        return scene_materials_resolved_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_meshes() const noexcept {
        return scene_mesh_plan_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_draws() const noexcept {
        return scene_mesh_plan_draws_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_meshes() const noexcept {
        return scene_mesh_plan_unique_meshes_;
    }

    [[nodiscard]] std::uint64_t scene_mesh_plan_unique_materials() const noexcept {
        return scene_mesh_plan_unique_materials_;
    }

    [[nodiscard]] std::size_t scene_mesh_plan_diagnostics() const noexcept {
        return scene_mesh_plan_diagnostics_;
    }

    [[nodiscard]] bool scene_mesh_plan_succeeded() const noexcept {
        return scene_mesh_plan_ok_;
    }

    [[nodiscard]] bool primary_camera_controller_passed(std::uint32_t expected_frames) const noexcept {
        return primary_camera_seen_ && camera_controller_ticks_ == expected_frames &&
               final_camera_x_ == static_cast<float>(expected_frames);
    }

    [[nodiscard]] std::uint32_t camera_controller_ticks() const noexcept {
        return camera_controller_ticks_;
    }

    [[nodiscard]] float final_camera_x() const noexcept {
        return final_camera_x_;
    }

    [[nodiscard]] bool transform_animation_passed(std::uint32_t expected_frames) const noexcept {
        return transform_animation_seen_ && transform_animation_ticks_ == expected_frames &&
               transform_animation_samples_ == static_cast<std::uint64_t>(expected_frames) &&
               transform_animation_applied_ == static_cast<std::uint64_t>(expected_frames) &&
               transform_animation_failures_ == 0 && final_mesh_x_ == 0.5F;
    }

    [[nodiscard]] std::uint32_t transform_animation_ticks() const noexcept {
        return transform_animation_ticks_;
    }

    [[nodiscard]] std::uint64_t transform_animation_samples() const noexcept {
        return transform_animation_samples_;
    }

    [[nodiscard]] std::uint64_t transform_animation_applied() const noexcept {
        return transform_animation_applied_;
    }

    [[nodiscard]] std::uint32_t transform_animation_failures() const noexcept {
        return transform_animation_failures_;
    }

    [[nodiscard]] float final_mesh_x() const noexcept {
        return final_mesh_x_;
    }

    [[nodiscard]] bool morph_package_passed(std::uint32_t expected_frames) const noexcept {
        return morph_package_seen_ && morph_package_ticks_ == expected_frames &&
               morph_package_samples_ == static_cast<std::uint64_t>(expected_frames) &&
               morph_package_weights_ == static_cast<std::uint64_t>(expected_frames) &&
               morph_package_vertices_per_sample_ > 0U &&
               morph_package_vertices_ ==
                   static_cast<std::uint64_t>(expected_frames) * morph_package_vertices_per_sample_ &&
               morph_package_failures_ == 0 && morph_first_position_x_ == 0.0F;
    }

    [[nodiscard]] std::uint32_t morph_package_ticks() const noexcept {
        return morph_package_ticks_;
    }

    [[nodiscard]] std::uint64_t morph_package_samples() const noexcept {
        return morph_package_samples_;
    }

    [[nodiscard]] std::uint64_t morph_package_weights() const noexcept {
        return morph_package_weights_;
    }

    [[nodiscard]] std::uint64_t morph_package_vertices() const noexcept {
        return morph_package_vertices_;
    }

    [[nodiscard]] std::uint32_t morph_package_failures() const noexcept {
        return morph_package_failures_;
    }

    [[nodiscard]] float morph_first_position_x() const noexcept {
        return morph_first_position_x_;
    }

    [[nodiscard]] bool quaternion_animation_passed(std::uint32_t expected_frames) const noexcept {
        return quaternion_animation_seen_ && quaternion_animation_ticks_ == expected_frames &&
               quaternion_animation_tracks_sampled_ ==
                   static_cast<std::uint64_t>(expected_frames) * quaternion_animation_tracks_.size() &&
               quaternion_animation_scene_applied_ == expected_frames && quaternion_animation_failures_ == 0 &&
               std::abs(final_quaternion_z_ - 1.0F) < 0.0001F && std::abs(final_quaternion_w_) < 0.0001F &&
               std::abs(final_quaternion_scene_rotation_z_ - 3.14159265F) < 0.0001F;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_ticks() const noexcept {
        return quaternion_animation_ticks_;
    }

    [[nodiscard]] std::uint64_t quaternion_animation_tracks_sampled() const noexcept {
        return quaternion_animation_tracks_sampled_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_failures() const noexcept {
        return quaternion_animation_failures_;
    }

    [[nodiscard]] std::uint32_t quaternion_animation_scene_applied() const noexcept {
        return quaternion_animation_scene_applied_;
    }

    [[nodiscard]] float final_quaternion_z() const noexcept {
        return final_quaternion_z_;
    }

    [[nodiscard]] float final_quaternion_w() const noexcept {
        return final_quaternion_w_;
    }

    [[nodiscard]] float final_quaternion_scene_rotation_z() const noexcept {
        return final_quaternion_scene_rotation_z_;
    }

    [[nodiscard]] bool gameplay_systems_passed(std::uint32_t expected_frames) const {
        return gameplay_systems_.passed(expected_frames);
    }

    [[nodiscard]] bool hud_passed(std::uint32_t expected_frames) const noexcept {
        const bool image_rows_ok = !textured_ui_atlas_mode_ || hud_images_submitted_ == expected_frames;
        const bool text_glyph_rows_ok =
            !text_glyph_ui_atlas_mode_ ||
            (hud_text_glyph_sprites_submitted_ == expected_frames && hud_text_glyphs_available_ == expected_frames &&
             hud_text_glyphs_resolved_ == expected_frames && hud_text_glyphs_missing_ == 0U);
        return ui_ok_ && ui_text_updates_ok_ && hud_boxes_submitted_ == expected_frames && image_rows_ok &&
               text_glyph_rows_ok;
    }

    [[nodiscard]] std::size_t hud_boxes_submitted() const noexcept {
        return hud_boxes_submitted_;
    }

    [[nodiscard]] std::size_t hud_images_submitted() const noexcept {
        return hud_images_submitted_;
    }

    [[nodiscard]] std::size_t hud_text_glyph_sprites_submitted() const noexcept {
        return hud_text_glyph_sprites_submitted_;
    }

    [[nodiscard]] std::size_t hud_text_glyphs_available() const noexcept {
        return hud_text_glyphs_available_;
    }

    [[nodiscard]] std::size_t hud_text_glyphs_resolved() const noexcept {
        return hud_text_glyphs_resolved_;
    }

    [[nodiscard]] std::size_t hud_text_glyphs_missing() const noexcept {
        return hud_text_glyphs_missing_;
    }

    [[nodiscard]] GameplaySystemsStatus gameplay_systems_status(std::uint32_t expected_frames) const {
        return gameplay_systems_.status(expected_frames);
    }

    [[nodiscard]] std::size_t gameplay_systems_diagnostics_count(std::uint32_t expected_frames) const {
        return gameplay_systems_.diagnostics_count(expected_frames);
    }

    [[nodiscard]] std::uint32_t gameplay_systems_ticks() const noexcept {
        return gameplay_systems_.ticks();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_physics_ticks() const noexcept {
        return gameplay_systems_.physics_ticks();
    }

    [[nodiscard]] std::size_t gameplay_systems_authored_collision_bodies() const noexcept {
        return gameplay_systems_.authored_collision_body_count();
    }

    [[nodiscard]] bool gameplay_systems_controller_grounded() const noexcept {
        return gameplay_systems_.controller_grounded();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_path_points() const noexcept {
        return gameplay_systems_.navigation_path_point_count();
    }

    [[nodiscard]] bool gameplay_systems_navigation_reached_destination() const noexcept {
        return gameplay_systems_.navigation_reached_destination();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathStatus gameplay_systems_navigation_plan_status() const noexcept {
        return gameplay_systems_.navigation_plan_status();
    }

    [[nodiscard]] mirakana::NavigationGridAgentPathDiagnostic
    gameplay_systems_navigation_plan_diagnostic() const noexcept {
        return gameplay_systems_.navigation_plan_diagnostic();
    }

    [[nodiscard]] mirakana::NavigationAgentStatus gameplay_systems_navigation_agent_status() const noexcept {
        return gameplay_systems_.navigation_agent_status();
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathStatus gameplay_systems_navigation_navmesh_status() const noexcept {
        return gameplay_systems_.navigation_navmesh_status();
    }

    [[nodiscard]] mirakana::NavigationNavmeshPathDiagnostic
    gameplay_systems_navigation_navmesh_diagnostic() const noexcept {
        return gameplay_systems_.navigation_navmesh_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_polygons() const noexcept {
        return gameplay_systems_.navigation_navmesh_polygon_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_dynamic_obstacles() const noexcept {
        return gameplay_systems_.navigation_navmesh_dynamic_obstacle_count();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_navigation_navmesh_total_cost() const noexcept {
        return gameplay_systems_.navigation_navmesh_total_cost();
    }

    [[nodiscard]] mirakana::NavigationNavmeshReadinessStatus
    gameplay_systems_navigation_navmesh_readiness_status() const noexcept {
        return gameplay_systems_.navigation_navmesh_readiness_status();
    }

    [[nodiscard]] mirakana::NavigationNavmeshReadinessDiagnostic
    gameplay_systems_navigation_navmesh_readiness_diagnostic() const noexcept {
        return gameplay_systems_.navigation_navmesh_readiness_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_readiness_diagnostics() const noexcept {
        return gameplay_systems_.navigation_navmesh_readiness_diagnostics_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_scene_refs() const noexcept {
        return gameplay_systems_.navigation_navmesh_scene_ref_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_points() const noexcept {
        return gameplay_systems_.navigation_navmesh_point_path_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_navmesh_visited_polygons() const noexcept {
        return gameplay_systems_.navigation_navmesh_visited_polygon_count();
    }

    [[nodiscard]] mirakana::NavigationCrowdPlanStatus gameplay_systems_navigation_crowd_status() const noexcept {
        return gameplay_systems_.navigation_crowd_status();
    }

    [[nodiscard]] mirakana::NavigationCrowdPlanDiagnostic
    gameplay_systems_navigation_crowd_diagnostic() const noexcept {
        return gameplay_systems_.navigation_crowd_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_rows() const noexcept {
        return gameplay_systems_.navigation_crowd_row_count();
    }

    [[nodiscard]] bool gameplay_systems_navigation_crowd_source_order_ready() const noexcept {
        return gameplay_systems_.navigation_crowd_source_order_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_agents() const noexcept {
        return gameplay_systems_.navigation_crowd_agent_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_route_successes() const noexcept {
        return gameplay_systems_.navigation_crowd_route_success_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_avoidance_successes() const noexcept {
        return gameplay_systems_.navigation_crowd_avoidance_success_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_applied_neighbors() const noexcept {
        return gameplay_systems_.navigation_crowd_applied_neighbor_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_dynamic_obstacles() const noexcept {
        return gameplay_systems_.navigation_crowd_dynamic_obstacle_count();
    }

    [[nodiscard]] mirakana::NavigationCrowdReadinessStatus
    gameplay_systems_navigation_crowd_readiness_status() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_status();
    }

    [[nodiscard]] mirakana::NavigationCrowdReadinessDiagnostic
    gameplay_systems_navigation_crowd_readiness_diagnostic() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_readiness_diagnostics() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_diagnostics_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_readiness_rows() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_row_count();
    }

    [[nodiscard]] bool gameplay_systems_navigation_crowd_readiness_source_order_ready() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_source_order_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_readiness_applied_neighbors() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_applied_neighbor_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_navigation_crowd_readiness_dynamic_obstacles() const noexcept {
        return gameplay_systems_.navigation_crowd_readiness_dynamic_obstacle_count();
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceStatus gameplay_systems_local_avoidance_status() const noexcept {
        return gameplay_systems_.local_avoidance_status();
    }

    [[nodiscard]] mirakana::NavigationLocalAvoidanceDiagnostic
    gameplay_systems_local_avoidance_diagnostic() const noexcept {
        return gameplay_systems_.local_avoidance_diagnostic();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_local_avoidance_steps() const noexcept {
        return gameplay_systems_.local_avoidance_steps();
    }

    [[nodiscard]] std::size_t gameplay_systems_local_avoidance_applied_neighbors() const noexcept {
        return gameplay_systems_.local_avoidance_applied_neighbor_count();
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DStatus
    gameplay_systems_physics_policy_status() const noexcept {
        return gameplay_systems_.physics_policy_status();
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic
    gameplay_systems_physics_policy_diagnostic() const noexcept {
        return gameplay_systems_.physics_policy_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_policy_rows() const noexcept {
        return gameplay_systems_.physics_policy_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_policy_dynamic_pushes() const noexcept {
        return gameplay_systems_.physics_policy_dynamic_push_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_policy_solid_contacts() const noexcept {
        return gameplay_systems_.physics_policy_solid_contact_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_policy_trigger_overlaps() const noexcept {
        return gameplay_systems_.physics_policy_trigger_overlap_count();
    }

    [[nodiscard]] mirakana::PhysicsAdvancedController3DStatus
    gameplay_systems_advanced_controller_status() const noexcept {
        return gameplay_systems_.advanced_controller_status();
    }

    [[nodiscard]] mirakana::PhysicsAdvancedController3DDiagnostic
    gameplay_systems_advanced_controller_diagnostic() const noexcept {
        return gameplay_systems_.advanced_controller_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_advanced_controller_movement_rows() const noexcept {
        return gameplay_systems_.advanced_controller_movement_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_advanced_controller_platform_rows() const noexcept {
        return gameplay_systems_.advanced_controller_moving_platform_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_advanced_controller_platform_applied() const noexcept {
        return gameplay_systems_.advanced_controller_platform_applied_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_advanced_controller_constraint_rows() const noexcept {
        return gameplay_systems_.advanced_controller_constraint_row_count();
    }

    [[nodiscard]] bool gameplay_systems_advanced_controller_replay_changed() const noexcept {
        return gameplay_systems_.advanced_controller_replay_changed();
    }

    [[nodiscard]] std::uint64_t gameplay_systems_advanced_controller_replay_before_bodies() const noexcept {
        return gameplay_systems_.advanced_controller_replay_before_body_count();
    }

    [[nodiscard]] std::uint64_t gameplay_systems_advanced_controller_replay_after_bodies() const noexcept {
        return gameplay_systems_.advanced_controller_replay_after_body_count();
    }

    [[nodiscard]] float gameplay_systems_advanced_controller_final_x() const noexcept {
        return gameplay_systems_.advanced_controller_final_x();
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamics3DReadinessStatus
    gameplay_systems_character_dynamics_status() const noexcept {
        return gameplay_systems_.character_dynamics_status();
    }

    [[nodiscard]] mirakana::PhysicsCharacterDynamics3DReadinessDiagnostic
    gameplay_systems_character_dynamics_diagnostic() const noexcept {
        return gameplay_systems_.character_dynamics_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_movement_rows() const noexcept {
        return gameplay_systems_.character_dynamics_movement_rows();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_dynamic_pushes() const noexcept {
        return gameplay_systems_.character_dynamics_dynamic_pushes();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_step_ups() const noexcept {
        return gameplay_systems_.character_dynamics_step_ups();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_walkable_slope_rows() const noexcept {
        return gameplay_systems_.character_dynamics_walkable_slope_rows();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_ground_probes() const noexcept {
        return gameplay_systems_.character_dynamics_ground_probes();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_moving_platform_rows() const noexcept {
        return gameplay_systems_.character_dynamics_moving_platform_rows();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_applied_platform_rows() const noexcept {
        return gameplay_systems_.character_dynamics_applied_platform_rows();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_constraint_rows() const noexcept {
        return gameplay_systems_.character_dynamics_constraint_rows();
    }

    [[nodiscard]] bool gameplay_systems_character_dynamics_replay_changed() const noexcept {
        return gameplay_systems_.character_dynamics_replay_changed();
    }

    [[nodiscard]] std::size_t gameplay_systems_character_dynamics_diagnostics() const noexcept {
        return gameplay_systems_.character_dynamics_diagnostics_count();
    }

    [[nodiscard]] mirakana::PhysicsConstraint3DStatus gameplay_systems_physics_constraints_status() const noexcept {
        return gameplay_systems_.physics_constraints_status();
    }

    [[nodiscard]] mirakana::PhysicsConstraint3DDiagnostic
    gameplay_systems_physics_constraints_diagnostic() const noexcept {
        return gameplay_systems_.physics_constraints_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_constraints_rows() const noexcept {
        return gameplay_systems_.physics_constraints_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_constraints_fixed_rows() const noexcept {
        return gameplay_systems_.physics_constraints_fixed_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_constraints_linear_axis_rows() const noexcept {
        return gameplay_systems_.physics_constraints_linear_axis_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_physics_constraints_axis_limit_clamped() const noexcept {
        return gameplay_systems_.physics_constraints_axis_limit_clamped_count();
    }

    [[nodiscard]] mirakana::PhysicsKinematicMotion3DStatus gameplay_systems_kinematic_motion_status() const noexcept {
        return gameplay_systems_.kinematic_motion_status();
    }

    [[nodiscard]] mirakana::PhysicsKinematicMotion3DDiagnostic
    gameplay_systems_kinematic_motion_diagnostic() const noexcept {
        return gameplay_systems_.kinematic_motion_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_kinematic_motion_rows() const noexcept {
        return gameplay_systems_.kinematic_motion_row_count();
    }

    [[nodiscard]] bool gameplay_systems_kinematic_motion_grounded() const noexcept {
        return gameplay_systems_.kinematic_motion_grounded();
    }

    [[nodiscard]] mirakana::PhysicsSimpleVehicle3DStatus gameplay_systems_vehicle_status() const noexcept {
        return gameplay_systems_.vehicle_status();
    }

    [[nodiscard]] mirakana::PhysicsSimpleVehicle3DDiagnostic gameplay_systems_vehicle_diagnostic() const noexcept {
        return gameplay_systems_.vehicle_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_vehicle_wheel_rows() const noexcept {
        return gameplay_systems_.vehicle_wheel_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_vehicle_grounded_wheels() const noexcept {
        return gameplay_systems_.vehicle_grounded_wheel_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_vehicle_wheel_probe_hits() const noexcept {
        return gameplay_systems_.vehicle_wheel_probe_hit_count();
    }

    [[nodiscard]] float gameplay_systems_vehicle_final_x() const noexcept {
        return gameplay_systems_.vehicle_final_x();
    }

    [[nodiscard]] float gameplay_systems_navigation_goal_x() const noexcept {
        return gameplay_systems_.navigation_goal_x();
    }

    [[nodiscard]] float gameplay_systems_navigation_goal_y() const noexcept {
        return gameplay_systems_.navigation_goal_y();
    }

    [[nodiscard]] float gameplay_systems_navigation_final_x() const noexcept {
        return gameplay_systems_.navigation_final_x();
    }

    [[nodiscard]] float gameplay_systems_navigation_final_y() const noexcept {
        return gameplay_systems_.navigation_final_y();
    }

    [[nodiscard]] mirakana::AiPerceptionStatus gameplay_systems_perception_status() const noexcept {
        return gameplay_systems_.perception_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_targets() const noexcept {
        return gameplay_systems_.perception_target_count();
    }

    [[nodiscard]] bool gameplay_systems_perception_has_primary_target() const noexcept {
        return gameplay_systems_.perception_has_primary_target();
    }

    [[nodiscard]] mirakana::AiPerceptionEntityId gameplay_systems_perception_primary_target_id() const noexcept {
        return gameplay_systems_.perception_primary_target_id();
    }

    [[nodiscard]] float gameplay_systems_perception_primary_target_distance() const noexcept {
        return gameplay_systems_.perception_primary_target_distance();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_visible_count() const noexcept {
        return gameplay_systems_.perception_visible_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_perception_audible_count() const noexcept {
        return gameplay_systems_.perception_audible_count();
    }

    [[nodiscard]] mirakana::AiPerceptionBlackboardStatus gameplay_systems_blackboard_status() const noexcept {
        return gameplay_systems_.blackboard_status();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_has_target() const noexcept {
        return gameplay_systems_.blackboard_has_target();
    }

    [[nodiscard]] bool gameplay_systems_blackboard_needs_move() const noexcept {
        return gameplay_systems_.blackboard_needs_move();
    }

    [[nodiscard]] mirakana::AiPerceptionEntityId gameplay_systems_blackboard_target_id() const noexcept {
        return gameplay_systems_.blackboard_target_id();
    }

    [[nodiscard]] std::size_t gameplay_systems_blackboard_visible_count() const noexcept {
        return gameplay_systems_.blackboard_visible_count();
    }

    [[nodiscard]] mirakana::BehaviorTreeStatus gameplay_systems_behavior_status() const noexcept {
        return gameplay_systems_.behavior_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_behavior_nodes() const noexcept {
        return gameplay_systems_.behavior_visited_node_count();
    }

    [[nodiscard]] bool gameplay_systems_audio_voice_started() const noexcept {
        return gameplay_systems_.audio_voice_started();
    }

    [[nodiscard]] mirakana::AudioDeviceStreamStatus gameplay_systems_audio_status() const noexcept {
        return gameplay_systems_.audio_stream_status();
    }

    [[nodiscard]] mirakana::AudioDeviceStreamDiagnostic gameplay_systems_audio_diagnostic() const noexcept {
        return gameplay_systems_.audio_stream_diagnostic();
    }

    [[nodiscard]] std::size_t gameplay_systems_audio_commands() const noexcept {
        return gameplay_systems_.audio_render_command_count();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_audio_frames() const noexcept {
        return gameplay_systems_.audio_stream_frames();
    }

    [[nodiscard]] std::size_t gameplay_systems_audio_samples() const noexcept {
        return gameplay_systems_.audio_stream_sample_count();
    }

    [[nodiscard]] float gameplay_systems_audio_first_sample() const noexcept {
        return gameplay_systems_.audio_stream_first_sample();
    }

    [[nodiscard]] float gameplay_systems_audio_second_sample() const noexcept {
        return gameplay_systems_.audio_stream_second_sample();
    }

    [[nodiscard]] float gameplay_systems_audio_abs_sum() const noexcept {
        return gameplay_systems_.audio_stream_sample_abs_sum();
    }

    [[nodiscard]] const AudioGameplayMixerProbeResult& gameplay_systems_audio_gameplay_mixer_probe() const noexcept {
        return gameplay_systems_.audio_gameplay_mixer_probe();
    }

    [[nodiscard]] bool gameplay_systems_interaction_ready() const noexcept {
        return gameplay_systems_.interaction_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_diagnostics() const noexcept {
        return gameplay_systems_.interaction_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_rows() const noexcept {
        return gameplay_systems_.interaction_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_interaction_feedback_rows() const noexcept {
        return gameplay_systems_.interaction_feedback_row_count();
    }

    [[nodiscard]] mirakana::runtime::RuntimeGameplaySessionState
    gameplay_systems_interaction_final_session_state() const noexcept {
        return gameplay_systems_.interaction_final_session_state();
    }

    [[nodiscard]] bool gameplay_systems_scene_binding_ready() const noexcept {
        return scene_gameplay_bindings_.ready;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_source_rows() const noexcept {
        return scene_gameplay_bindings_.source_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_rows() const noexcept {
        return scene_gameplay_bindings_.binding_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_systems() const noexcept {
        return scene_gameplay_bindings_.gameplay_systems;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_component_rows() const noexcept {
        return scene_gameplay_bindings_.component_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_binding_diagnostics() const noexcept {
        return scene_gameplay_bindings_.binding_diagnostics;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_interaction_rows() const noexcept {
        return scene_gameplay_bindings_.interaction_rows;
    }

    [[nodiscard]] std::size_t gameplay_systems_scene_interaction_diagnostics() const noexcept {
        return scene_gameplay_bindings_.interaction_diagnostics;
    }

    [[nodiscard]] mirakana::runtime_scene::RuntimeSceneGameplaySessionState
    gameplay_systems_scene_interaction_final_session_state() const noexcept {
        return scene_gameplay_bindings_.final_session_state;
    }

    [[nodiscard]] bool gameplay_systems_runtime_profile_resume_ready() const noexcept {
        return gameplay_systems_.runtime_profile_resume_ready();
    }

    [[nodiscard]] mirakana::runtime::RuntimeSessionProfileResumeStatus
    gameplay_systems_runtime_profile_resume_status() const noexcept {
        return gameplay_systems_.runtime_profile_resume_status();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_diagnostics() const noexcept {
        return gameplay_systems_.runtime_profile_resume_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_loaded_documents() const noexcept {
        return gameplay_systems_.runtime_profile_resume_loaded_document_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_profile_resume_defaulted_documents() const noexcept {
        return gameplay_systems_.runtime_profile_resume_defaulted_document_count();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_runtime_profile_resume_save_schema_version() const noexcept {
        return gameplay_systems_.runtime_profile_resume_save_schema_version();
    }

    [[nodiscard]] std::uint32_t gameplay_systems_runtime_profile_resume_settings_schema_version() const noexcept {
        return gameplay_systems_.runtime_profile_resume_settings_schema_version();
    }

    [[nodiscard]] bool gameplay_systems_runtime_menu_hud_ready() const noexcept {
        return gameplay_systems_.runtime_menu_hud_ready();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_diagnostics() const noexcept {
        return gameplay_systems_.runtime_menu_hud_diagnostic_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_display_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_display_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_command_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_command_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_dialogue_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_dialogue_row_count();
    }

    [[nodiscard]] std::size_t gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() const noexcept {
        return gameplay_systems_.runtime_menu_hud_input_binding_prompt_row_count();
    }

    [[nodiscard]] std::string_view gameplay_systems_animation_state() const noexcept {
        return gameplay_systems_.animation_state();
    }

    [[nodiscard]] float gameplay_systems_final_actor_x() const noexcept {
        return gameplay_systems_.final_actor_x();
    }

  private:
    [[nodiscard]] bool build_hud() {
        mirakana::ui::ElementDesc root;
        root.id = mirakana::ui::ElementId{"hud.root"};
        root.role = mirakana::ui::SemanticRole::root;
        root.style.layout = mirakana::ui::LayoutMode::column;
        root.style.padding = mirakana::ui::EdgeInsets{.top = 8.0F, .right = 8.0F, .bottom = 8.0F, .left = 8.0F};
        root.style.gap = 4.0F;
        if (!hud_.try_add_element(root)) {
            return false;
        }

        mirakana::ui::ElementDesc status;
        status.id = mirakana::ui::ElementId{"hud.status"};
        status.parent = root.id;
        status.role = mirakana::ui::SemanticRole::label;
        status.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 160.0F, .height = 24.0F};
        status.text = mirakana::ui::TextContent{
            .label = "3D Meshes 0", .localization_key = "hud.status", .font_family = "engine-default"};
        status.style.background_token = "hud.panel";
        status.accessibility_label = "Generated 3D diagnostics";
        if (!hud_.try_add_element(status)) {
            return false;
        }

        if (textured_ui_atlas_mode_) {
            mirakana::ui::ElementDesc atlas_image;
            atlas_image.id = mirakana::ui::ElementId{kHudAtlasProofResourceId};
            atlas_image.parent = root.id;
            atlas_image.role = mirakana::ui::SemanticRole::image;
            atlas_image.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 32.0F};
            atlas_image.image.resource_id = kHudAtlasProofResourceId;
            atlas_image.image.asset_uri = kHudAtlasProofAssetUri;
            atlas_image.accessibility_label = "Texture atlas proof";
            if (!hud_.try_add_element(atlas_image)) {
                return false;
            }
        }

        return true;
    }

    void update_hud_text() {
        const auto text = std::string{"3D Meshes "} + std::to_string(scene_meshes_submitted_);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.status"},
                          mirakana::ui::TextContent{
                              .label = text, .localization_key = "hud.status", .font_family = "engine-default"});
    }

    [[nodiscard]] mirakana::UiRenderSubmitResult submit_hud_text_glyph_proof() {
        mirakana::ui::RendererSubmission submission;
        mirakana::ui::RendererTextRun text;
        text.id = mirakana::ui::ElementId{kHudTextGlyphAtlasProofId};
        text.bounds = mirakana::ui::Rect{.x = 12.0F, .y = 48.0F, .width = 16.0F, .height = 16.0F};
        text.text.label = "A";
        text.text.font_family = "engine-default";
        text.text.wrap = mirakana::ui::TextWrapMode::clip;
        text.foreground_token = "hud.text";
        submission.text_runs.push_back(std::move(text));

        mirakana::UiRenderSubmitDesc desc;
        desc.theme = &theme_;
        desc.glyph_atlas = &glyph_atlas_;
        desc.text_layout_policy = mirakana::ui::MonospaceTextLayoutPolicy{
            .glyph_advance = 8.0F, .whitespace_advance = 4.0F, .line_height = 10.0F};
        return mirakana::submit_ui_renderer_submission(renderer_, submission, desc);
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    bool throttle_{true};
    std::optional<mirakana::RuntimeSceneRenderInstance> scene_;
    std::optional<mirakana::AnimationFloatClipSourceDocument> animation_clip_;
    mirakana::AnimationTransformBindingSourceDocument animation_bindings_;
    std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> morph_payload_;
    std::optional<mirakana::AnimationFloatClipSourceDocument> morph_animation_clip_;
    std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks_;
    mirakana::UiRendererImagePalette image_palette_;
    mirakana::UiRendererGlyphAtlasPalette glyph_atlas_;
    GeneratedGameplaySystemsProbe gameplay_systems_;
    SceneGameplayBindingProbeResult scene_gameplay_bindings_;
    std::uint32_t frames_{0};
    std::size_t scene_meshes_submitted_{0};
    std::size_t scene_materials_resolved_{0};
    std::uint64_t scene_mesh_plan_meshes_{0};
    std::uint64_t scene_mesh_plan_draws_{0};
    std::uint64_t scene_mesh_plan_unique_meshes_{0};
    std::uint64_t scene_mesh_plan_unique_materials_{0};
    std::size_t scene_mesh_plan_diagnostics_{0};
    std::size_t hud_boxes_submitted_{0};
    std::size_t hud_images_submitted_{0};
    std::size_t hud_text_glyphs_available_{0};
    std::size_t hud_text_glyphs_resolved_{0};
    std::size_t hud_text_glyphs_missing_{0};
    std::size_t hud_text_glyph_sprites_submitted_{0};
    std::uint32_t camera_controller_ticks_{0};
    float final_camera_x_{0.0F};
    std::uint32_t transform_animation_ticks_{0};
    std::uint64_t transform_animation_samples_{0};
    std::uint64_t transform_animation_applied_{0};
    std::uint32_t transform_animation_failures_{0};
    float final_mesh_x_{0.0F};
    std::uint32_t morph_package_ticks_{0};
    std::uint64_t morph_package_samples_{0};
    std::uint64_t morph_package_weights_{0};
    std::uint64_t morph_package_vertices_{0};
    std::uint64_t morph_package_vertices_per_sample_{0};
    std::uint32_t morph_package_failures_{0};
    float morph_first_position_x_{0.0F};
    std::uint32_t quaternion_animation_ticks_{0};
    std::uint64_t quaternion_animation_tracks_sampled_{0};
    std::uint32_t quaternion_animation_failures_{0};
    std::uint32_t quaternion_animation_scene_applied_{0};
    float final_quaternion_z_{0.0F};
    float final_quaternion_w_{1.0F};
    float final_quaternion_scene_rotation_z_{0.0F};
    bool primary_camera_seen_{false};
    bool transform_animation_seen_{false};
    bool morph_package_seen_{false};
    bool quaternion_animation_seen_{false};
    bool scene_mesh_plan_ok_{true};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool textured_ui_atlas_mode_{false};
    bool text_glyph_ui_atlas_mode_{false};
};

[[nodiscard]] bool parse_positive_uint32(std::string_view text, std::uint32_t& value) noexcept {
    std::uint32_t parsed{};
    const char* begin = text.data();
    const char* end = text.data() + text.size();
    const auto result = std::from_chars(begin, end, parsed);
    if (result.ec != std::errc{} || result.ptr != end || parsed == 0) {
        return false;
    }
    value = parsed;
    return true;
}

void print_usage() {
    std::cout << "sample_generated_desktop_runtime_3d_package [--smoke] [--max-frames N] [--video-driver NAME] "
                 "[--require-config PATH] [--require-scene-package PATH] [--require-d3d12-scene-shaders] "
                 "[--require-vulkan-scene-shaders] [--require-d3d12-renderer] [--require-vulkan-renderer] "
                 "[--require-scene-gpu-bindings] [--require-postprocess] [--require-postprocess-depth-input] "
                 "[--require-directional-shadow] [--require-directional-shadow-filtering] "
                 "[--require-shadow-morph-composition] "
                 "[--require-renderer-quality-gates] [--require-renderer-quality-matrix] "
                 "[--require-rendering-vfx-profiling] "
                 "[--require-playable-3d-slice] [--require-visible-3d-production-proof] "
                 "[--require-vulkan-visible-3d-production-proof] "
                 "[--require-native-ui-overlay] "
                 "[--require-native-ui-textured-sprite-atlas] "
                 "[--require-native-ui-text-glyph-atlas] "
                 "[--require-primary-camera-controller] "
                 "[--require-transform-animation] [--require-morph-package] [--require-compute-morph] "
                 "[--require-compute-morph-skin] [--require-compute-morph-async-telemetry] "
                 "[--require-quaternion-animation] [--require-package-streaming-safe-point] "
                 "[--require-package-upload-staging] "
                 "[--require-gameplay-systems] [--require-scene-collision-package] "
                 "[--require-entity-scale-culling] [--require-runtime-profile-resume] "
                 "[--require-runtime-menu-hud] [--require-audio-gameplay-mixer] "
                 "[--require-audio-production]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeOptions& options) {
    for (int index = 1; index < argc; ++index) {
        const std::string_view arg{argv[index]};
        if (arg == "--help" || arg == "-h") {
            options.show_help = true;
            return true;
        }
        if (arg == "--smoke") {
            options.smoke = true;
            continue;
        }
        if (arg == "--require-d3d12-scene-shaders") {
            options.require_d3d12_scene_shaders = true;
            continue;
        }
        if (arg == "--require-vulkan-scene-shaders") {
            options.require_vulkan_scene_shaders = true;
            continue;
        }
        if (arg == "--require-d3d12-renderer") {
            options.require_d3d12_renderer = true;
            continue;
        }
        if (arg == "--require-vulkan-renderer") {
            options.require_vulkan_renderer = true;
            continue;
        }
        if (arg == "--require-scene-gpu-bindings") {
            options.require_scene_gpu_bindings = true;
            continue;
        }
        if (arg == "--require-postprocess") {
            options.require_postprocess = true;
            continue;
        }
        if (arg == "--require-postprocess-depth-input") {
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            continue;
        }
        if (arg == "--require-rendering-vfx-profiling") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_renderer_quality_gates = true;
            options.require_renderer_quality_matrix = true;
            options.require_rendering_vfx_profiling = true;
            continue;
        }
        if (arg == "--require-directional-shadow") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            continue;
        }
        if (arg == "--require-directional-shadow-filtering") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            continue;
        }
        if (arg == "--require-shadow-morph-composition") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_shadow_morph_composition = true;
            options.require_renderer_quality_gates = true;
            options.require_morph_package = true;
            continue;
        }
        if (arg == "--require-renderer-quality-gates") {
            options.require_renderer_quality_gates = true;
            continue;
        }
        if (arg == "--require-renderer-quality-matrix") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_renderer_quality_gates = true;
            options.require_renderer_quality_matrix = true;
            continue;
        }
        if (arg == "--require-playable-3d-slice") {
            options.require_playable_3d_slice = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_renderer_quality_gates = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-visible-3d-production-proof") {
            options.require_visible_3d_production_proof = true;
            options.require_d3d12_scene_shaders = true;
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_renderer_quality_gates = true;
            options.require_playable_3d_slice = true;
            options.require_native_ui_overlay = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_compute_morph = true;
            options.require_compute_morph_skin = true;
            options.require_compute_morph_async_telemetry = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--require-vulkan-visible-3d-production-proof") {
            options.require_vulkan_visible_3d_production_proof = true;
            options.require_vulkan_scene_shaders = true;
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_renderer_quality_gates = true;
            options.require_playable_3d_slice = true;
            options.require_native_ui_overlay = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_compute_morph = true;
            options.require_compute_morph_skin = true;
            options.require_compute_morph_async_telemetry = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--require-native-ui-overlay") {
            options.require_native_ui_overlay = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_renderer_quality_gates = true;
            options.require_playable_3d_slice = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-native-ui-textured-sprite-atlas") {
            options.require_native_ui_textured_sprite_atlas = true;
            options.require_native_ui_overlay = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_renderer_quality_gates = true;
            options.require_playable_3d_slice = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-native-ui-text-glyph-atlas") {
            options.require_native_ui_text_glyph_atlas = true;
            options.require_native_ui_overlay = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_renderer_quality_gates = true;
            options.require_playable_3d_slice = true;
            options.require_primary_camera_controller = true;
            options.require_transform_animation = true;
            options.require_morph_package = true;
            options.require_quaternion_animation = true;
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-primary-camera-controller") {
            options.require_primary_camera_controller = true;
            continue;
        }
        if (arg == "--require-transform-animation") {
            options.require_transform_animation = true;
            continue;
        }
        if (arg == "--require-morph-package") {
            options.require_morph_package = true;
            continue;
        }
        if (arg == "--require-compute-morph") {
            options.require_compute_morph = true;
            continue;
        }
        if (arg == "--require-compute-morph-normal-tangent") {
            options.require_compute_morph = true;
            options.require_compute_morph_normal_tangent = true;
            continue;
        }
        if (arg == "--require-compute-morph-skin") {
            options.require_compute_morph = true;
            options.require_compute_morph_skin = true;
            continue;
        }
        if (arg == "--require-compute-morph-async-telemetry") {
            options.require_compute_morph = true;
            options.require_compute_morph_async_telemetry = true;
            continue;
        }
        if (arg == "--require-quaternion-animation") {
            options.require_quaternion_animation = true;
            continue;
        }
        if (arg == "--require-package-streaming-safe-point") {
            options.require_package_streaming_safe_point = true;
            continue;
        }
        if (arg == "--require-package-upload-staging") {
            options.require_package_upload_staging = true;
            continue;
        }
        if (arg == "--require-gameplay-systems") {
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--require-scene-collision-package") {
            options.require_scene_collision_package = true;
            options.require_package_streaming_safe_point = true;
            options.require_gameplay_systems = true;
            continue;
        }
        if (arg == "--require-entity-scale-culling") {
            options.require_entity_scale_culling = true;
            continue;
        }
        if (arg == "--require-runtime-profile-resume") {
            options.require_runtime_profile_resume = true;
            continue;
        }
        if (arg == "--require-runtime-menu-hud") {
            options.require_runtime_menu_hud = true;
            continue;
        }
        if (arg == "--require-audio-gameplay-mixer") {
            options.require_audio_gameplay_mixer = true;
            continue;
        }
        if (arg == "--require-audio-production") {
            options.require_audio_production = true;
            continue;
        }
        if (arg == "--max-frames") {
            if (index + 1 >= argc || !parse_positive_uint32(argv[index + 1], options.max_frames)) {
                std::cerr << "--max-frames requires a positive integer\n";
                return false;
            }
            ++index;
            continue;
        }
        if (arg == "--video-driver") {
            if (index + 1 >= argc) {
                std::cerr << "--video-driver requires a driver name\n";
                return false;
            }
            options.video_driver_hint = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-config") {
            if (index + 1 >= argc) {
                std::cerr << "--require-config requires a relative path\n";
                return false;
            }
            options.required_config_path = argv[index + 1];
            ++index;
            continue;
        }
        if (arg == "--require-scene-package") {
            if (index + 1 >= argc) {
                std::cerr << "--require-scene-package requires a relative path\n";
                return false;
            }
            options.required_scene_package_path = argv[index + 1];
            ++index;
            continue;
        }

        std::cerr << "unknown argument: " << arg << '\n';
        return false;
    }

    if (options.require_d3d12_renderer && options.require_vulkan_renderer) {
        std::cerr << "--require-d3d12-renderer and --require-vulkan-renderer are mutually exclusive\n";
        return false;
    }
    if (options.require_shadow_morph_composition &&
        (options.require_playable_3d_slice || options.require_compute_morph)) {
        std::cerr << "--require-shadow-morph-composition is a selected graphics morph smoke and cannot be combined "
                     "with --require-playable-3d-slice or compute morph flags\n";
        return false;
    }
    if (options.require_directional_shadow && !options.require_shadow_morph_composition &&
        (options.require_playable_3d_slice || options.require_morph_package || options.require_compute_morph)) {
        std::cerr << "--require-directional-shadow is a selected renderer smoke and cannot be combined with "
                     "--require-playable-3d-slice, --require-morph-package, or compute morph flags\n";
        return false;
    }
    if (options.require_native_ui_overlay && !options.require_vulkan_renderer) {
        options.require_d3d12_renderer = true;
        options.require_d3d12_scene_shaders = true;
    }
    if (options.require_native_ui_overlay && options.require_vulkan_renderer) {
        options.require_vulkan_scene_shaders = true;
    }

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 2;
        }
        if (options.video_driver_hint.empty()) {
            options.video_driver_hint = "dummy";
        }
        options.throttle = false;
    }
    return true;
}

[[nodiscard]] mirakana::SdlDesktopPresentationQualityGateDesc
make_renderer_quality_gate_desc(const DesktopRuntimeOptions& options) noexcept {
    mirakana::SdlDesktopPresentationQualityGateDesc desc;
    if (options.require_renderer_quality_gates) {
        desc.require_scene_gpu_bindings = true;
        desc.require_postprocess = true;
        desc.require_postprocess_depth_input = options.require_postprocess_depth_input;
        desc.require_directional_shadow = options.require_directional_shadow;
        desc.require_directional_shadow_filtering = options.require_directional_shadow_filtering;
        desc.expected_frames = options.max_frames;
    }
    return desc;
}

struct EntityScaleCullingProbeResult {
    mirakana::runtime::RuntimeEntityScaleCullingPlanStatus status{
        mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::invalid_request};
    std::size_t rows{0U};
    std::size_t visible_rows{0U};
    std::size_t culled_rows{0U};
    std::size_t lod_rows{0U};
    std::size_t priority_update_rows{0U};
    std::size_t normal_update_rows{0U};
    std::size_t background_update_rows{0U};
    std::uint64_t projected_draw_cost{0U};
    std::uint64_t projected_update_cost{0U};
    std::size_t budget_protected_rows{0U};
    std::size_t diagnostics{0U};
    std::size_t budget_diagnostics{0U};
    bool ready{false};
};

[[nodiscard]] std::string_view
entity_scale_culling_status_name(mirakana::runtime::RuntimeEntityScaleCullingPlanStatus status) noexcept {
    switch (status) {
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::planned:
        return "planned";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::no_entities:
        return "no_entities";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::invalid_request:
        return "invalid_request";
    case mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::budget_exceeded:
        return "budget_exceeded";
    }
    return "unknown";
}

[[nodiscard]] EntityScaleCullingProbeResult validate_entity_scale_culling_package_evidence() {
    using BoundsKind = mirakana::runtime::RuntimeEntityScaleCullingBoundsKind;
    using Code = mirakana::runtime::RuntimeEntityScaleCullingDiagnosticCode;
    using DrawIntent = mirakana::runtime::RuntimeEntityScaleCullingDrawIntentKind;
    using Entity = mirakana::runtime::RuntimeEntityScaleCullingEntityDesc;
    using LodBand = mirakana::runtime::RuntimeEntityScaleCullingLodBandDesc;
    using UpdateBucket = mirakana::runtime::RuntimeEntityScaleCullingUpdateBucket;

    const auto empty_2d = mirakana::runtime::RuntimeEntityScaleCullingBounds2D{};
    const auto empty_3d = mirakana::runtime::RuntimeEntityScaleCullingBounds3D{};

    auto hero = Entity{
        .entity_id = "hero",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = -1.0F,
                .min_y = -1.0F,
                .max_x = 1.0F,
                .max_y = 1.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::priority,
        .enabled = true,
        .source_index = 2U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 3U,
                        .max_view_distance = 4.0F,
                        .draw_cost = 99U,
                        .update_cost = 99U,
                        .update_interval_frames = 9U,
                        .draw_intent = DrawIntent::custom},
                LodBand{.lod_index = 0U,
                        .max_view_distance = 4.0F,
                        .draw_cost = 2U,
                        .update_cost = 1U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::sprite_2d},
                LodBand{.lod_index = 1U,
                        .max_view_distance = 12.0F,
                        .draw_cost = 4U,
                        .update_cost = 2U,
                        .update_interval_frames = 3U,
                        .draw_intent = DrawIntent::sprite_2d},
            },
        .budget_protected = true,
    };
    auto mesh = Entity{
        .entity_id = "mesh",
        .bounds_kind = BoundsKind::aabb_3d,
        .bounds_2d = empty_2d,
        .bounds_3d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                .min_x = 9.0F,
                .min_y = -1.0F,
                .min_z = -1.0F,
                .max_x = 11.0F,
                .max_y = 1.0F,
                .max_z = 1.0F,
            },
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::normal,
        .enabled = true,
        .source_index = 1U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 1U,
                        .max_view_distance = 20.0F,
                        .draw_cost = 5U,
                        .update_cost = 2U,
                        .update_interval_frames = 4U,
                        .draw_intent = DrawIntent::mesh_3d},
                LodBand{.lod_index = 0U,
                        .max_view_distance = 5.0F,
                        .draw_cost = 9U,
                        .update_cost = 4U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::mesh_3d},
            },
        .budget_protected = false,
    };
    auto disabled = Entity{
        .entity_id = "sleeping",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = 0.0F,
                .min_y = 0.0F,
                .max_x = 1.0F,
                .max_y = 1.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x1U,
        .update_bucket = UpdateBucket::background,
        .enabled = false,
        .source_index = 3U,
        .lod_bands =
            std::vector<LodBand>{
                LodBand{.lod_index = 0U,
                        .max_view_distance = 10.0F,
                        .draw_cost = 20U,
                        .update_cost = 20U,
                        .update_interval_frames = 1U,
                        .draw_intent = DrawIntent::sprite_2d},
            },
        .budget_protected = false,
    };
    const auto hidden = Entity{
        .entity_id = "hidden",
        .bounds_kind = BoundsKind::aabb_2d,
        .bounds_2d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds2D{
                .min_x = 2.0F,
                .min_y = 2.0F,
                .max_x = 3.0F,
                .max_y = 3.0F,
            },
        .bounds_3d = empty_3d,
        .layer_mask = 0x4U,
        .update_bucket = UpdateBucket::normal,
        .enabled = true,
        .source_index = 4U,
        .lod_bands = {},
        .budget_protected = false,
    };
    const auto view = mirakana::runtime::RuntimeEntityScaleCullingViewDesc{
        .bounds_kind = BoundsKind::aabb_3d,
        .bounds_2d = empty_2d,
        .bounds_3d =
            mirakana::runtime::RuntimeEntityScaleCullingBounds3D{
                .min_x = -16.0F,
                .min_y = -16.0F,
                .min_z = -8.0F,
                .max_x = 16.0F,
                .max_y = 16.0F,
                .max_z = 8.0F,
            },
        .layer_mask = 0x1U,
        .max_visible_entities = 8U,
        .max_projected_draw_cost = 32U,
        .max_projected_update_cost = 16U,
    };

    EntityScaleCullingProbeResult result;
    const auto plan = mirakana::runtime::plan_runtime_entity_scale_culling(
        mirakana::runtime::RuntimeEntityScaleCullingRequest{.entities = {mesh, disabled, hero, hidden}, .view = view});
    result.status = plan.status;
    result.rows = plan.rows.size();
    result.projected_draw_cost = plan.projected_draw_cost;
    result.projected_update_cost = plan.projected_update_cost;
    result.diagnostics = plan.diagnostics.size();
    for (const auto& row : plan.rows) {
        if (row.visible) {
            ++result.visible_rows;
        } else {
            ++result.culled_rows;
        }
        if (row.projected_draw_cost > 0U || row.projected_update_cost > 0U) {
            ++result.lod_rows;
        }
        if (row.budget_protected) {
            ++result.budget_protected_rows;
        }
        switch (row.update_bucket) {
        case UpdateBucket::background:
            ++result.background_update_rows;
            break;
        case UpdateBucket::normal:
            ++result.normal_update_rows;
            break;
        case UpdateBucket::priority:
            ++result.priority_update_rows;
            break;
        }
    }

    hero.lod_bands = {
        LodBand{.lod_index = 0U,
                .max_view_distance = 4.0F,
                .draw_cost = 9U,
                .update_cost = 5U,
                .update_interval_frames = 1U,
                .draw_intent = DrawIntent::sprite_2d},
    };
    mesh.lod_bands = {
        LodBand{.lod_index = 0U,
                .max_view_distance = 20.0F,
                .draw_cost = 4U,
                .update_cost = 2U,
                .update_interval_frames = 2U,
                .draw_intent = DrawIntent::mesh_3d},
    };
    auto budget_view = view;
    budget_view.max_projected_draw_cost = 10U;
    budget_view.max_projected_update_cost = 6U;
    const auto budget_plan = mirakana::runtime::plan_runtime_entity_scale_culling(
        mirakana::runtime::RuntimeEntityScaleCullingRequest{.entities = {mesh, hero}, .view = budget_view});
    for (const auto& diagnostic : budget_plan.diagnostics) {
        if (diagnostic.code == Code::draw_budget_exceeded || diagnostic.code == Code::update_budget_exceeded) {
            ++result.budget_diagnostics;
        }
    }

    result.ready =
        plan.succeeded() && result.status == mirakana::runtime::RuntimeEntityScaleCullingPlanStatus::planned &&
        result.rows == 4U && result.visible_rows == 2U && result.culled_rows == 2U && result.lod_rows == 2U &&
        result.priority_update_rows == 1U && result.normal_update_rows == 2U && result.background_update_rows == 1U &&
        result.projected_draw_cost == 7U && result.projected_update_cost == 3U && result.budget_protected_rows == 1U &&
        result.diagnostics == 0U && result.budget_diagnostics == 2U;
    return result;
}

enum class Playable3dSliceStatus : std::uint8_t {
    not_requested,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view playable_3d_slice_status_name(Playable3dSliceStatus status) noexcept {
    switch (status) {
    case Playable3dSliceStatus::not_requested:
        return "not_requested";
    case Playable3dSliceStatus::ready:
        return "ready";
    case Playable3dSliceStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

struct Playable3dSliceReport {
    Playable3dSliceStatus status{Playable3dSliceStatus::not_requested};
    bool ready{false};
    std::size_t diagnostics_count{0};
    std::uint32_t expected_frames{0};
    bool frames_ok{false};
    bool game_frames_ok{false};
    bool scene_mesh_plan_ready{false};
    bool camera_controller_ready{false};
    bool animation_ready{false};
    bool morph_ready{false};
    bool quaternion_ready{false};
    bool package_streaming_ready{false};
    bool scene_gpu_ready{false};
    bool postprocess_ready{false};
    bool renderer_quality_ready{false};
    bool compute_morph_selected{false};
    bool compute_morph_ready{false};
    bool compute_morph_normal_tangent_selected{false};
    bool compute_morph_normal_tangent_ready{false};
    bool compute_morph_skin_selected{false};
    bool compute_morph_skin_ready{false};
    bool compute_morph_async_telemetry_selected{false};
    bool compute_morph_async_telemetry_ready{false};
};

enum class Visible3dProductionProofStatus : std::uint8_t {
    not_requested,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view visible_3d_production_proof_status_name(Visible3dProductionProofStatus status) noexcept {
    switch (status) {
    case Visible3dProductionProofStatus::not_requested:
        return "not_requested";
    case Visible3dProductionProofStatus::ready:
        return "ready";
    case Visible3dProductionProofStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

struct Visible3dProductionProofReport {
    Visible3dProductionProofStatus status{Visible3dProductionProofStatus::not_requested};
    bool ready{false};
    std::size_t diagnostics_count{0};
    std::uint32_t expected_frames{0};
    std::uint64_t presented_frames{0};
    bool d3d12_selected{false};
    bool vulkan_selected{false};
    bool null_fallback_used{false};
    bool scene_gpu_ready{false};
    bool postprocess_ready{false};
    bool renderer_quality_ready{false};
    bool playable_ready{false};
    bool ui_overlay_ready{false};
};

enum class CollisionPackageStatus : std::uint8_t {
    not_requested,
    ready,
    diagnostics,
};

[[nodiscard]] std::string_view collision_package_status_name(CollisionPackageStatus status) noexcept {
    switch (status) {
    case CollisionPackageStatus::not_requested:
        return "not_requested";
    case CollisionPackageStatus::ready:
        return "ready";
    case CollisionPackageStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

[[nodiscard]] std::string_view
physics_collision_query_readiness_status_name(mirakana::PhysicsCollisionQuery3DReadinessStatus status) noexcept {
    switch (status) {
    case mirakana::PhysicsCollisionQuery3DReadinessStatus::ready:
        return "ready";
    case mirakana::PhysicsCollisionQuery3DReadinessStatus::diagnostics:
        return "diagnostics";
    case mirakana::PhysicsCollisionQuery3DReadinessStatus::invalid_request:
        return "invalid_request";
    }
    return "unknown";
}

[[nodiscard]] std::string_view physics_collision_query_readiness_diagnostic_name(
    mirakana::PhysicsCollisionQuery3DReadinessDiagnostic diagnostic) noexcept {
    switch (diagnostic) {
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::none:
        return "none";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::invalid_raycast_batch:
        return "invalid_raycast_batch";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::invalid_shape_sweep_batch:
        return "invalid_shape_sweep_batch";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::missing_raycast_hit:
        return "missing_raycast_hit";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::missing_shape_sweep_hit:
        return "missing_shape_sweep_hit";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::missing_no_hit:
        return "missing_no_hit";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::missing_invalid_request:
        return "missing_invalid_request";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::missing_budget_rejection:
        return "missing_budget_rejection";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::source_order_mismatch:
        return "source_order_mismatch";
    case mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::row_budget_exceeded:
        return "row_budget_exceeded";
    }
    return "unknown";
}

struct CollisionPackageReport {
    CollisionPackageStatus status{CollisionPackageStatus::not_requested};
    bool ready{false};
    std::size_t diagnostics_count{0};
    std::size_t body_count{0};
    std::size_t trigger_count{0};
    std::size_t contact_count{0};
    std::size_t trigger_overlap_count{0};
    bool world_ready{false};
    bool query_batch_ready{false};
    bool query_batch_source_order_ready{false};
    mirakana::PhysicsCollisionQuery3DReadinessStatus query_readiness_status{
        mirakana::PhysicsCollisionQuery3DReadinessStatus::invalid_request};
    mirakana::PhysicsCollisionQuery3DReadinessDiagnostic query_readiness_diagnostic{
        mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::none};
    std::size_t query_readiness_diagnostics{0};
    std::size_t query_batch_rows{0};
    std::size_t query_batch_hits{0};
    std::size_t query_batch_no_hits{0};
    std::size_t query_batch_invalid_requests{0};
    std::size_t query_batch_budget_rejections{0};
};

[[nodiscard]] bool
package_streaming_smoke_ready(const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result,
                              std::size_t expected_records) noexcept {
    return package_streaming_result.succeeded() && package_streaming_result.estimated_resident_bytes > 0 &&
           package_streaming_result.replacement.committed_record_count > 0 &&
           package_streaming_result.replacement.committed_record_count == expected_records &&
           package_streaming_result.required_preload_asset_count == 1 &&
           package_streaming_result.resident_resource_kind_count == 11 &&
           package_streaming_result.resident_package_count == 1 && package_streaming_result.diagnostics.empty();
}

[[nodiscard]] CollisionPackageReport
evaluate_scene_collision_package(const DesktopRuntimeOptions& options,
                                 const mirakana::runtime::RuntimeAssetPackage* runtime_package) {
    CollisionPackageReport report;
    if (!options.require_scene_collision_package) {
        return report;
    }

    report.status = CollisionPackageStatus::diagnostics;
    if (runtime_package == nullptr) {
        report.diagnostics_count = 1;
        return report;
    }

    const auto* record = runtime_package->find(packaged_collision_scene_asset_id());
    if (record == nullptr) {
        report.diagnostics_count = 1;
        return report;
    }
    if (record->kind != mirakana::AssetKind::physics_collision_scene) {
        report.diagnostics_count = 1;
        return report;
    }

    const auto payload = mirakana::runtime::runtime_physics_collision_scene_3d_payload(*record);
    if (!payload.succeeded()) {
        report.diagnostics_count = 1;
        return report;
    }

    report.body_count = payload.payload.bodies.size();
    report.trigger_count = static_cast<std::size_t>(
        std::ranges::count_if(payload.payload.bodies, [](const auto& body) { return body.body.trigger; }));

    const auto build = mirakana::runtime::build_physics_world_3d_from_runtime_collision_scene(payload.payload);
    if (build.status != mirakana::PhysicsAuthoredCollision3DBuildStatus::success ||
        build.diagnostic != mirakana::PhysicsAuthoredCollision3DDiagnostic::none) {
        report.diagnostics_count = 1;
        return report;
    }

    report.contact_count = build.world.contacts().size();
    report.trigger_overlap_count = build.world.trigger_overlaps().size();
    report.world_ready = build.bodies.size() == 3U && build.world.bodies().size() == 3U && report.body_count == 3U &&
                         report.trigger_count == 1U && report.contact_count >= 1U && report.trigger_overlap_count >= 1U;

    constexpr std::uint32_t floor_layer = 1U << 0U;
    constexpr std::uint32_t trigger_layer = 1U << 1U;
    constexpr std::uint32_t probe_layer = 1U << 2U;
    const auto raycast_batch = build.world
                                   .raycast_batch(
                                       mirakana::PhysicsRaycastBatch3DDesc{
                                           .queries =
                                               {
                                                   mirakana::PhysicsRaycast3DDesc{
                                                       .origin = mirakana::Vec3{.x = -2.0F, .y = 0.25F, .z = 0.0F},
                                                       .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                       .max_distance = 4.0F,
                                                       .collision_mask = probe_layer,
                                                   },
                                                   mirakana::PhysicsRaycast3DDesc{
                                                       .origin = mirakana::Vec3{.x = -2.0F, .y = 2.0F, .z = 0.0F},
                                                       .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                       .max_distance = 4.0F,
                                                       .collision_mask = floor_layer,
                                                   },
                                                   mirakana::PhysicsRaycast3DDesc{
                                                       .origin = mirakana::Vec3{.x = -2.0F, .y = 0.25F, .z = 0.0F},
                                                       .direction = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
                                                       .max_distance = 4.0F,
                                                       .collision_mask = trigger_layer,
                                                   },
                                               },
                                           .max_queries = 3U,
                                       });
    const auto sweep_batch = build.world
                                 .shape_sweep_batch(
                                     mirakana::PhysicsShapeSweepBatch3DDesc{
                                         .queries =
                                             {
                                                 mirakana::PhysicsShapeSweep3DDesc{
                                                     .origin = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F},
                                                     .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                     .max_distance = 4.0F,
                                                     .shape = mirakana::PhysicsShape3DKind::aabb,
                                                     .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 0.25F},
                                                     .radius = 0.25F,
                                                     .half_height = 0.25F,
                                                     .collision_mask = trigger_layer,
                                                     .ignored_body = mirakana::null_physics_body_3d,
                                                     .include_triggers = true,
                                                 },
                                                 mirakana::PhysicsShapeSweep3DDesc{
                                                     .origin = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F},
                                                     .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                     .max_distance = 4.0F,
                                                     .shape = mirakana::PhysicsShape3DKind::aabb,
                                                     .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 0.25F},
                                                     .radius = 0.25F,
                                                     .half_height = 0.25F,
                                                     .collision_mask = trigger_layer,
                                                     .ignored_body = mirakana::null_physics_body_3d,
                                                     .include_triggers = false,
                                                 },
                                                 mirakana::PhysicsShapeSweep3DDesc{
                                                     .origin = mirakana::Vec3{.x = 0.0F, .y = 0.25F, .z = 0.0F},
                                                     .direction = mirakana::Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F},
                                                     .max_distance = 4.0F,
                                                     .shape = mirakana::PhysicsShape3DKind::capsule,
                                                     .half_extents = mirakana::Vec3{.x = 0.25F, .y = 0.25F, .z = 0.25F},
                                                     .radius = 0.25F,
                                                     .half_height = 0.0F,
                                                     .collision_mask = trigger_layer,
                                                     .ignored_body = mirakana::null_physics_body_3d,
                                                     .include_triggers = true,
                                                 },
                                             },
                                         .max_queries = 3U,
                                     });
    const auto raycast_over_budget = build.world.raycast_batch(mirakana::PhysicsRaycastBatch3DDesc{
        .queries = {mirakana::PhysicsRaycast3DDesc{}, mirakana::PhysicsRaycast3DDesc{}},
        .max_queries = 1U,
    });
    const auto sweep_over_budget = build.world.shape_sweep_batch(mirakana::PhysicsShapeSweepBatch3DDesc{
        .queries = {mirakana::PhysicsShapeSweep3DDesc{}, mirakana::PhysicsShapeSweep3DDesc{}},
        .max_queries = 1U,
    });
    if (raycast_over_budget.status == mirakana::PhysicsCollisionQueryBatchStatus::invalid_request &&
        raycast_over_budget.diagnostic == mirakana::PhysicsCollisionQueryBatchDiagnostic::query_budget_exceeded &&
        raycast_over_budget.rows.empty()) {
        ++report.query_batch_budget_rejections;
    }
    if (sweep_over_budget.status == mirakana::PhysicsCollisionQueryBatchStatus::invalid_request &&
        sweep_over_budget.diagnostic == mirakana::PhysicsCollisionQueryBatchDiagnostic::query_budget_exceeded &&
        sweep_over_budget.rows.empty()) {
        ++report.query_batch_budget_rejections;
    }

    mirakana::PhysicsCollisionQuery3DReadinessConfig query_config;
    query_config.require_raycast_hit = true;
    query_config.require_shape_sweep_hit = true;
    query_config.require_no_hit = true;
    query_config.require_invalid_request = true;
    query_config.require_budget_rejection = true;
    query_config.require_source_order = true;
    query_config.max_total_rows = 6U;
    const auto query_readiness = mirakana::evaluate_physics_collision_query_readiness_3d(
        raycast_batch, sweep_batch, report.query_batch_budget_rejections, query_config);

    report.query_readiness_status = query_readiness.status;
    report.query_readiness_diagnostic = query_readiness.diagnostic;
    report.query_readiness_diagnostics = query_readiness.diagnostics.size();
    report.query_batch_source_order_ready = query_readiness.source_order_ready;
    report.query_batch_rows = query_readiness.total_rows;
    report.query_batch_hits = query_readiness.hit_rows;
    report.query_batch_no_hits = query_readiness.no_hit_rows;
    report.query_batch_invalid_requests = query_readiness.invalid_request_rows;
    report.query_batch_ready =
        query_readiness.status == mirakana::PhysicsCollisionQuery3DReadinessStatus::ready &&
        query_readiness.diagnostic == mirakana::PhysicsCollisionQuery3DReadinessDiagnostic::none &&
        report.query_batch_rows == 6U && report.query_batch_hits == 2U && report.query_batch_no_hits == 2U &&
        report.query_batch_invalid_requests == 2U && report.query_batch_budget_rejections == 2U;
    report.ready = report.world_ready && report.query_batch_ready;
    report.status = report.ready ? CollisionPackageStatus::ready : CollisionPackageStatus::diagnostics;
    report.diagnostics_count = report.ready ? 0U : 1U;
    return report;
}

void print_collision_package_diagnostics(const CollisionPackageReport& report) {
    std::cerr << "sample_generated_desktop_runtime_3d_package scene_collision_package_diagnostic="
              << "status=" << collision_package_status_name(report.status) << " ready=" << (report.ready ? 1 : 0)
              << " world_ready=" << (report.world_ready ? 1 : 0)
              << " query_batch_ready=" << (report.query_batch_ready ? 1 : 0)
              << " query_batch_source_order_ready=" << (report.query_batch_source_order_ready ? 1 : 0)
              << " query_readiness_status="
              << physics_collision_query_readiness_status_name(report.query_readiness_status)
              << " query_readiness_diagnostic="
              << physics_collision_query_readiness_diagnostic_name(report.query_readiness_diagnostic)
              << " query_readiness_diagnostics=" << report.query_readiness_diagnostics
              << " query_batch_rows=" << report.query_batch_rows << " query_batch_hits=" << report.query_batch_hits
              << " query_batch_no_hits=" << report.query_batch_no_hits
              << " query_batch_invalid_requests=" << report.query_batch_invalid_requests
              << " query_batch_budget_rejections=" << report.query_batch_budget_rejections << '\n';
}

[[nodiscard]] bool scene_mesh_plan_ready(const GeneratedDesktopRuntime3DPackageGame& game,
                                         std::uint32_t expected_frames) noexcept {
    return game.scene_meshes_submitted() == static_cast<std::size_t>(expected_frames) &&
           game.scene_materials_resolved() == static_cast<std::size_t>(expected_frames) &&
           game.scene_mesh_plan_meshes() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_draws() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_unique_meshes() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_unique_materials() == static_cast<std::uint64_t>(expected_frames) &&
           game.scene_mesh_plan_succeeded() && game.scene_mesh_plan_diagnostics() == 0;
}

[[nodiscard]] bool scene_gpu_ready(const mirakana::SdlDesktopPresentationReport& report,
                                   std::uint32_t expected_frames) noexcept {
    const auto& stats = report.scene_gpu_stats;
    return (stats.mesh_bindings > 0 || stats.skinned_mesh_bindings > 0) && stats.material_bindings > 0 &&
           (stats.mesh_bindings_resolved + stats.skinned_mesh_bindings_resolved) ==
               static_cast<std::size_t>(expected_frames) &&
           stats.material_bindings_resolved == static_cast<std::size_t>(expected_frames);
}

[[nodiscard]] std::uint64_t expected_framegraph_barrier_steps(bool directional_shadow_requested,
                                                              bool postprocess_depth_input_requested,
                                                              std::uint32_t expected_frames) noexcept {
    const auto frame_count = static_cast<std::uint64_t>(expected_frames);
    if (directional_shadow_requested) {
        return frame_count == 0 ? 0 : 9 + ((frame_count - 1) * 6);
    }
    return postprocess_depth_input_requested ? (frame_count == 0 ? 0 : 1 + (frame_count * 4)) : frame_count * 2;
}

[[nodiscard]] bool postprocess_ready(const mirakana::SdlDesktopPresentationReport& report,
                                     std::uint32_t expected_frames) noexcept {
    const auto expected_framegraph_passes = report.directional_shadow_requested ? 3U : 2U;
    const auto expected_framegraph_barrier_step_count = expected_framegraph_barrier_steps(
        report.directional_shadow_requested, report.postprocess_depth_input_requested, expected_frames);
    return report.postprocess_status == mirakana::SdlDesktopPresentationPostprocessStatus::ready &&
           report.framegraph_passes == expected_framegraph_passes &&
           report.renderer_stats.framegraph_passes_executed ==
               static_cast<std::uint64_t>(expected_frames) * expected_framegraph_passes &&
           report.renderer_stats.framegraph_render_passes_recorded ==
               static_cast<std::uint64_t>(expected_frames) * expected_framegraph_passes &&
           report.renderer_stats.framegraph_barrier_steps_executed == expected_framegraph_barrier_step_count &&
           report.renderer_stats.postprocess_passes_executed == static_cast<std::uint64_t>(expected_frames);
}

[[nodiscard]] bool compute_morph_ready(const mirakana::SdlDesktopPresentationReport& report,
                                       const DesktopRuntimeOptions& options) noexcept {
    const auto& stats = report.scene_gpu_stats;
    if (!options.require_compute_morph || options.require_compute_morph_skin) {
        return false;
    }
    return stats.compute_morph_mesh_bindings > 0 && stats.compute_morph_mesh_dispatches > 0 &&
           stats.compute_morph_queue_waits > 0 &&
           stats.compute_morph_mesh_bindings_resolved == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_mesh_draws == static_cast<std::size_t>(options.max_frames) &&
           report.renderer_stats.gpu_morph_draws == 0 && report.renderer_stats.morph_descriptor_binds == 0;
}

[[nodiscard]] bool compute_morph_skin_ready(const mirakana::SdlDesktopPresentationReport& report,
                                            const DesktopRuntimeOptions& options) noexcept {
    const auto& stats = report.scene_gpu_stats;
    if (!options.require_compute_morph_skin) {
        return false;
    }
    return stats.compute_morph_skinned_mesh_bindings > 0 && stats.compute_morph_skinned_mesh_dispatches > 0 &&
           stats.compute_morph_skinned_queue_waits > 0 &&
           stats.compute_morph_skinned_mesh_bindings_resolved == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_skinned_mesh_draws == static_cast<std::size_t>(options.max_frames) &&
           stats.compute_morph_output_position_bytes > 0 &&
           report.renderer_stats.gpu_skinning_draws == static_cast<std::uint64_t>(options.max_frames) &&
           report.renderer_stats.skinned_palette_descriptor_binds == static_cast<std::uint64_t>(options.max_frames);
}

[[nodiscard]] bool compute_morph_async_telemetry_ready(const mirakana::SdlDesktopPresentationReport& report) noexcept {
    const auto& stats = report.scene_gpu_stats;
    return stats.compute_morph_async_compute_queue_submits > 0 && stats.compute_morph_async_graphics_queue_waits > 0 &&
           stats.compute_morph_async_graphics_queue_submits > 0 &&
           stats.compute_morph_async_last_compute_submitted_fence_value > 0 &&
           stats.compute_morph_async_last_graphics_queue_wait_fence_value ==
               stats.compute_morph_async_last_compute_submitted_fence_value &&
           stats.compute_morph_async_last_graphics_submitted_fence_value > 0;
}

[[nodiscard]] Playable3dSliceReport
evaluate_playable_3d_slice(const DesktopRuntimeOptions& options, const mirakana::DesktopRunResult& result,
                           const GeneratedDesktopRuntime3DPackageGame& game,
                           const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result,
                           std::size_t expected_package_records,
                           const mirakana::SdlDesktopPresentationReport& presentation_report,
                           const mirakana::SdlDesktopPresentationQualityGateReport& renderer_quality) noexcept {
    Playable3dSliceReport report;
    report.expected_frames = options.max_frames;
    report.frames_ok =
        result.status == mirakana::DesktopRunStatus::completed && result.frames_run == options.max_frames;
    report.game_frames_ok = game.frames() == options.max_frames;
    report.scene_mesh_plan_ready = scene_mesh_plan_ready(game, options.max_frames);
    report.camera_controller_ready = game.primary_camera_controller_passed(options.max_frames);
    report.animation_ready = game.transform_animation_passed(options.max_frames);
    report.morph_ready = game.morph_package_passed(options.max_frames);
    report.quaternion_ready = game.quaternion_animation_passed(options.max_frames);
    report.package_streaming_ready = package_streaming_smoke_ready(package_streaming_result, expected_package_records);
    report.scene_gpu_ready = scene_gpu_ready(presentation_report, options.max_frames);
    report.postprocess_ready = postprocess_ready(presentation_report, options.max_frames);
    report.renderer_quality_ready = renderer_quality.ready;
    report.compute_morph_selected = options.require_compute_morph;
    report.compute_morph_skin_selected = options.require_compute_morph_skin;
    report.compute_morph_normal_tangent_selected =
        options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin;
    report.compute_morph_async_telemetry_selected = options.require_compute_morph_async_telemetry;
    report.compute_morph_skin_ready = compute_morph_skin_ready(presentation_report, options);
    report.compute_morph_ready = options.require_compute_morph_skin ? report.compute_morph_skin_ready
                                                                    : compute_morph_ready(presentation_report, options);
    report.compute_morph_normal_tangent_ready =
        report.compute_morph_normal_tangent_selected && report.compute_morph_ready;
    report.compute_morph_async_telemetry_ready =
        options.require_compute_morph_async_telemetry && compute_morph_async_telemetry_ready(presentation_report);

    if (!options.require_playable_3d_slice) {
        return report;
    }

    const bool required_checks[] = {
        report.frames_ok,       report.game_frames_ok,    report.scene_mesh_plan_ready,  report.camera_controller_ready,
        report.animation_ready, report.morph_ready,       report.quaternion_ready,       report.package_streaming_ready,
        report.scene_gpu_ready, report.postprocess_ready, report.renderer_quality_ready,
    };
    for (const bool check : required_checks) {
        if (!check) {
            ++report.diagnostics_count;
        }
    }
    if (report.compute_morph_selected && !report.compute_morph_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_normal_tangent_selected && !report.compute_morph_normal_tangent_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_skin_selected && !report.compute_morph_skin_ready) {
        ++report.diagnostics_count;
    }
    if (report.compute_morph_async_telemetry_selected && !report.compute_morph_async_telemetry_ready) {
        ++report.diagnostics_count;
    }
    report.ready = report.diagnostics_count == 0;
    report.status = report.ready ? Playable3dSliceStatus::ready : Playable3dSliceStatus::diagnostics;
    return report;
}

[[nodiscard]] Visible3dProductionProofReport
evaluate_visible_3d_production_proof(const DesktopRuntimeOptions& options, const mirakana::DesktopRunResult& result,
                                     const mirakana::SdlDesktopPresentationReport& presentation_report,
                                     const mirakana::SdlDesktopPresentationQualityGateReport& renderer_quality,
                                     const Playable3dSliceReport& playable_3d, bool gameplay_systems_ready) noexcept {
    Visible3dProductionProofReport report;
    report.expected_frames = options.max_frames;
    report.presented_frames = presentation_report.renderer_stats.frames_finished;
    report.d3d12_selected = presentation_report.selected_backend == mirakana::SdlDesktopPresentationBackend::d3d12;
    report.vulkan_selected = presentation_report.selected_backend == mirakana::SdlDesktopPresentationBackend::vulkan;
    report.null_fallback_used = presentation_report.used_null_fallback;
    report.scene_gpu_ready = scene_gpu_ready(presentation_report, options.max_frames);
    report.postprocess_ready = postprocess_ready(presentation_report, options.max_frames);
    report.renderer_quality_ready = renderer_quality.ready && renderer_quality.diagnostics_count == 0;
    report.playable_ready = playable_3d.ready && playable_3d.diagnostics_count == 0;
    report.ui_overlay_ready =
        presentation_report.native_ui_overlay_requested && presentation_report.native_ui_overlay_ready &&
        presentation_report.native_ui_overlay_status == mirakana::SdlDesktopPresentationNativeUiOverlayStatus::ready &&
        presentation_report.native_ui_overlay_sprites_submitted >= static_cast<std::uint64_t>(options.max_frames) &&
        presentation_report.native_ui_overlay_draws == static_cast<std::uint64_t>(options.max_frames);

    if (!options.require_visible_3d_production_proof && !options.require_vulkan_visible_3d_production_proof) {
        return report;
    }

    const bool backend_selected =
        options.require_visible_3d_production_proof ? report.d3d12_selected : report.vulkan_selected;

    const bool frame_run_exact =
        result.status == mirakana::DesktopRunStatus::completed && result.frames_run == options.max_frames;
    const bool frame_present_exact = report.presented_frames == static_cast<std::uint64_t>(options.max_frames);
    const bool presentation_diagnostics_clean = presentation_report.diagnostics_count == 0 &&
                                                presentation_report.scene_gpu_diagnostics_count == 0 &&
                                                presentation_report.postprocess_diagnostics_count == 0 &&
                                                presentation_report.native_ui_overlay_diagnostics_count == 0 &&
                                                presentation_report.native_ui_texture_overlay_diagnostics_count == 0;
    const bool required_checks[] = {
        frame_run_exact,
        frame_present_exact,
        backend_selected,
        !report.null_fallback_used,
        presentation_diagnostics_clean,
        report.scene_gpu_ready,
        report.postprocess_ready,
        report.renderer_quality_ready,
        report.playable_ready,
        gameplay_systems_ready,
        report.ui_overlay_ready,
    };
    for (const bool check : required_checks) {
        if (!check) {
            ++report.diagnostics_count;
        }
    }
    report.ready = report.diagnostics_count == 0;
    report.status = report.ready ? Visible3dProductionProofStatus::ready : Visible3dProductionProofStatus::diagnostics;
    return report;
}

void print_package_failures(const std::vector<mirakana::runtime::RuntimeAssetPackageLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime package failure asset=" << failure.asset.value << " path=" << failure.path << ": "
                  << failure.diagnostic << '\n';
    }
}

void print_scene_failures(const std::vector<mirakana::RuntimeSceneRenderLoadFailure>& failures) {
    for (const auto& failure : failures) {
        std::cerr << "runtime scene failure asset=" << failure.asset.value << ": " << failure.diagnostic << '\n';
    }
}

[[nodiscard]] std::optional<mirakana::AudioClipSampleData>
make_audio_samples(const mirakana::runtime::RuntimeAudioPayload& payload) {
    if (payload.sample_format != mirakana::AudioSourceSampleFormat::float32 || payload.channel_count == 0 ||
        payload.samples.size() != payload.source_bytes || payload.samples.size() % sizeof(float) != 0) {
        return std::nullopt;
    }

    std::vector<float> samples(payload.samples.size() / sizeof(float));
    if (!samples.empty()) {
        std::memcpy(samples.data(), payload.samples.data(), payload.samples.size());
    }

    return mirakana::AudioClipSampleData{
        .clip = payload.asset,
        .format = mirakana::AudioDeviceFormat{.sample_rate = payload.sample_rate,
                                              .channel_count = payload.channel_count,
                                              .sample_format = mirakana::AudioSampleFormat::float32},
        .frame_count = payload.frame_count,
        .interleaved_float_samples = std::move(samples),
    };
}

[[nodiscard]] std::filesystem::path executable_directory(const char* executable_path) {
    try {
        if (executable_path != nullptr && !std::string_view{executable_path}.empty()) {
            const auto absolute_path = std::filesystem::absolute(std::filesystem::path{executable_path});
            if (absolute_path.has_parent_path()) {
                return absolute_path.parent_path();
            }
        }
        return std::filesystem::current_path();
    } catch (const std::exception&) {
        return std::filesystem::path{"."};
    }
}

[[nodiscard]] bool verify_required_config(const char* executable_path, std::string_view config_path) {
    if (config_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(config_path)) {
            std::cerr << "required config was not found: " << config_path << '\n';
            return false;
        }

        const auto config_text = filesystem.read_text(config_path);
        if (config_text.empty()) {
            std::cerr << "required config is empty: " << config_path << '\n';
            return false;
        }
        if (!config_text.starts_with(kExpectedConfigFormat)) {
            std::cerr << "required config has unexpected format: " << config_path << '\n';
            return false;
        }
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required config '" << config_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] bool
load_required_scene_package(const char* executable_path, std::string_view package_path,
                            std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                            std::optional<mirakana::RuntimeSceneRenderInstance>& scene,
                            std::optional<mirakana::AnimationFloatClipSourceDocument>& animation_clip,
                            std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload>& morph_payload,
                            std::optional<mirakana::AnimationFloatClipSourceDocument>& morph_animation_clip,
                            std::vector<mirakana::AnimationJointTrack3dDesc>& quaternion_animation_tracks,
                            std::optional<mirakana::AudioClipSampleData>& audio_samples, bool require_audio_samples) {
    if (package_path.empty()) {
        return true;
    }

    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        if (!filesystem.exists(package_path)) {
            std::cerr << "required scene package was not found: " << package_path << '\n';
            return false;
        }

        auto package_result =
            mirakana::runtime::load_runtime_asset_package(filesystem, mirakana::runtime::RuntimeAssetPackageDesc{
                                                                          .index_path = std::string{package_path},
                                                                          .content_root = {},
                                                                      });
        if (!package_result.succeeded()) {
            print_package_failures(package_result.failures);
            return false;
        }

        auto instance =
            mirakana::instantiate_runtime_scene_render_data(package_result.package, packaged_scene_asset_id());
        if (!instance.succeeded()) {
            print_scene_failures(instance.failures);
            return false;
        }
        if (instance.render_packet.meshes.empty()) {
            std::cerr << "runtime scene package did not produce renderable meshes: " << package_path << '\n';
            return false;
        }
        if (instance.material_palette.count() == 0) {
            std::cerr << "runtime scene package did not resolve scene materials: " << package_path << '\n';
            return false;
        }
        const auto* audio_record = package_result.package.find(packaged_audio_asset_id());
        if (audio_record == nullptr) {
            if (require_audio_samples) {
                std::cerr << "runtime scene package did not include packaged gameplay audio payload: " << package_path
                          << '\n';
                return false;
            }
        } else {
            const auto audio_payload = mirakana::runtime::runtime_audio_payload(*audio_record);
            if (!audio_payload.succeeded()) {
                if (require_audio_samples) {
                    std::cerr << "runtime scene package gameplay audio payload is invalid: " << audio_payload.diagnostic
                              << '\n';
                    return false;
                }
            } else {
                auto samples = make_audio_samples(audio_payload.payload);
                if (!samples.has_value()) {
                    if (require_audio_samples) {
                        std::cerr
                            << "runtime scene package gameplay audio payload is not a supported float32 sample clip\n";
                        return false;
                    }
                } else {
                    audio_samples = std::move(samples);
                }
            }
        }
        const auto* animation_record = package_result.package.find(packaged_animation_asset_id());
        if (animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged transform animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto animation_payload = mirakana::runtime::runtime_animation_float_clip_payload(*animation_record);
        if (!animation_payload.succeeded()) {
            std::cerr << "runtime scene package transform animation clip is invalid: " << animation_payload.diagnostic
                      << '\n';
            return false;
        }
        const auto* morph_record = package_result.package.find(packaged_morph_mesh_asset_id());
        if (morph_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged morph mesh CPU payload: " << package_path
                      << '\n';
            return false;
        }
        const auto morph_result = mirakana::runtime::runtime_morph_mesh_cpu_payload(*morph_record);
        if (!morph_result.succeeded()) {
            std::cerr << "runtime scene package morph mesh CPU payload is invalid: " << morph_result.diagnostic << '\n';
            return false;
        }
        const auto* morph_animation_record = package_result.package.find(packaged_morph_animation_asset_id());
        if (morph_animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged morph weight animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto morph_animation_result =
            mirakana::runtime::runtime_animation_float_clip_payload(*morph_animation_record);
        if (!morph_animation_result.succeeded()) {
            std::cerr << "runtime scene package morph weight animation clip is invalid: "
                      << morph_animation_result.diagnostic << '\n';
            return false;
        }
        const auto* quaternion_animation_record = package_result.package.find(packaged_quaternion_animation_asset_id());
        if (quaternion_animation_record == nullptr) {
            std::cerr << "runtime scene package did not include packaged quaternion animation clip: " << package_path
                      << '\n';
            return false;
        }
        const auto quaternion_payload =
            mirakana::runtime::runtime_animation_quaternion_clip_payload(*quaternion_animation_record);
        if (!quaternion_payload.succeeded()) {
            std::cerr << "runtime scene package quaternion animation clip is invalid: " << quaternion_payload.diagnostic
                      << '\n';
            return false;
        }
        auto decoded_quaternion_tracks = make_quaternion_animation_tracks(quaternion_payload.payload);
        if (!mirakana::is_valid_animation_joint_tracks_3d(packaged_quaternion_animation_skeleton(),
                                                          decoded_quaternion_tracks)) {
            std::cerr << "runtime scene package quaternion animation clip does not match its smoke skeleton: "
                      << package_path << '\n';
            return false;
        }

        animation_clip = animation_payload.payload.clip;
        morph_payload = morph_result.payload;
        morph_animation_clip = morph_animation_result.payload.clip;
        quaternion_animation_tracks = std::move(decoded_quaternion_tracks);
        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required scene package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
}

[[nodiscard]] mirakana::runtime::RuntimePackageStreamingExecutionResult
execute_package_streaming_safe_point_smoke(const mirakana::runtime::RuntimeAssetPackage& package,
                                           std::string_view package_path) {
    mirakana::runtime::RuntimeAssetPackageStore store;
    mirakana::runtime::RuntimeResourceCatalogV2 catalog;
    mirakana::runtime::RuntimeAssetPackageLoadResult loaded_package{
        .package = package,
        .failures = {},
    };

    return mirakana::runtime::execute_selected_runtime_package_streaming_safe_point(
        store, catalog,
        mirakana::runtime::RuntimePackageStreamingExecutionDesc{
            .target_id = "packaged-3d-residency-budget",
            .package_index_path = std::string{package_path},
            .content_root = {},
            .runtime_scene_validation_target_id = "packaged-3d-scene",
            .mode = mirakana::runtime::RuntimePackageStreamingExecutionMode::host_gated_safe_point,
            .resident_budget_bytes = kPackageStreamingResidentBudgetBytes,
            .safe_point_required = true,
            .runtime_scene_validation_succeeded = true,
            .required_preload_assets = {packaged_scene_asset_id()},
            .resident_resource_kinds =
                {
                    mirakana::AssetKind::texture,
                    mirakana::AssetKind::mesh,
                    mirakana::AssetKind::skinned_mesh,
                    mirakana::AssetKind::morph_mesh_cpu,
                    mirakana::AssetKind::material,
                    mirakana::AssetKind::animation_float_clip,
                    mirakana::AssetKind::animation_quaternion_clip,
                    mirakana::AssetKind::ui_atlas,
                    mirakana::AssetKind::scene,
                    mirakana::AssetKind::physics_collision_scene,
                    mirakana::AssetKind::audio,
                },
            .max_resident_packages = 1,
        },
        std::move(loaded_package));
}

void print_package_streaming_diagnostics(
    const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result) {
    for (const auto& diagnostic : package_streaming_result.diagnostics) {
        std::cout << "sample_generated_desktop_runtime_3d_package package_streaming_diagnostic=" << diagnostic.code
                  << ": " << diagnostic.message << '\n';
    }
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult load_packaged_d3d12_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_scene_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneFragmentShaderPath},
        .vertex_entry_point = "vs_morph",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_scene_compute_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphShaderPath},
        .vertex_entry_point = "vs_compute_morph",
        .fragment_entry_point = "cs_compute_morph_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_scene_compute_morph_tangent_frame_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphTangentFrameVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphTangentFrameShaderPath},
        .vertex_entry_point = "vs_compute_morph_tangent_frame",
        .fragment_entry_point = "cs_compute_morph_tangent_frame",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_scene_compute_morph_skinned_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneComputeMorphSkinnedVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneComputeMorphSkinnedShaderPath},
        .vertex_entry_point = "vs_compute_morph_skinned",
        .fragment_entry_point = "cs_compute_morph_skinned_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_morph",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphShaderPath},
        .vertex_entry_point = "vs_compute_morph",
        .fragment_entry_point = "cs_compute_morph_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_tangent_frame_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath},
        .vertex_entry_point = "vs_compute_morph_tangent_frame",
        .fragment_entry_point = "cs_compute_morph_tangent_frame",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_scene_compute_morph_skinned_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanComputeMorphSkinnedShaderPath},
        .vertex_entry_point = "vs_compute_morph_skinned",
        .fragment_entry_point = "cs_compute_morph_skinned_position",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shifted_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVertexShaderPath},
        .fragment_path = std::string{kRuntimeShiftedShadowReceiverFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_postprocess_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimePostprocessVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimePostprocessVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_postprocess",
        .fragment_entry_point = "ps_postprocess",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowReceiverVulkanFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shifted_shadow_receiver_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShiftedShadowReceiverVulkanFragmentShaderPath},
        .fragment_entry_point = "ps_shadow_receiver",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_shadow_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeShadowVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeShadowVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_shadow",
        .fragment_entry_point = "ps_shadow",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_d3d12_native_ui_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeNativeUiOverlayVertexShaderPath},
        .fragment_path = std::string{kRuntimeNativeUiOverlayFragmentShaderPath},
        .vertex_entry_point = "vs_native_ui_overlay",
        .fragment_entry_point = "ps_native_ui_overlay",
    });
}

[[nodiscard]] mirakana::DesktopShaderBytecodeLoadResult
load_packaged_vulkan_native_ui_overlay_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeNativeUiOverlayVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeNativeUiOverlayVulkanFragmentShaderPath},
        .vertex_entry_point = "vs_native_ui_overlay",
        .fragment_entry_point = "ps_native_ui_overlay",
    });
}

[[nodiscard]] mirakana::SdlDesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::SdlDesktopPresentationShaderBytecode{
        .entry_point = bytecode.entry_point,
        .bytecode = std::span<const std::uint8_t>{bytecode.bytecode.data(), bytecode.bytecode.size()},
    };
}

[[nodiscard]] std::string_view status_name(mirakana::DesktopRunStatus status) noexcept {
    switch (status) {
    case mirakana::DesktopRunStatus::completed:
        return "completed";
    case mirakana::DesktopRunStatus::stopped_by_app:
        return "stopped_by_app";
    case mirakana::DesktopRunStatus::window_closed:
        return "window_closed";
    case mirakana::DesktopRunStatus::lifecycle_quit:
        return "lifecycle_quit";
    }
    return "unknown";
}

void print_presentation_report(std::string_view prefix, const mirakana::SdlDesktopGameHost& host) {
    const auto report = host.presentation_report();
    std::cout << prefix << " presentation_report=requested="
              << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::sdl_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::sdl_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.message << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeOptions options;
    if (!parse_args(argc, argv, options)) {
        print_usage();
        return 2;
    }
    if (options.show_help) {
        print_usage();
        return 0;
    }
    if (!verify_required_config(argc > 0 ? argv[0] : nullptr, options.required_config_path)) {
        return 4;
    }

    std::optional<mirakana::runtime::RuntimeAssetPackage> runtime_package;
    std::optional<mirakana::RuntimeSceneRenderInstance> packaged_scene;
    std::optional<mirakana::AnimationFloatClipSourceDocument> packaged_animation_clip;
    std::optional<mirakana::runtime::RuntimeMorphMeshCpuPayload> packaged_morph_payload;
    std::optional<mirakana::AnimationFloatClipSourceDocument> packaged_morph_animation_clip;
    std::vector<mirakana::AnimationJointTrack3dDesc> packaged_quaternion_animation_tracks;
    std::optional<mirakana::AudioClipSampleData> packaged_audio_samples;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene, packaged_animation_clip, packaged_morph_payload,
                                     packaged_morph_animation_clip, packaged_quaternion_animation_tracks,
                                     packaged_audio_samples,
                                     options.require_audio_gameplay_mixer || options.require_audio_production)) {
        return 4;
    }
    if (options.require_scene_gpu_bindings && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-scene-gpu-bindings requires --require-scene-package\n";
        return 4;
    }
    if (options.require_transform_animation && (!packaged_scene.has_value() || !packaged_animation_clip.has_value())) {
        std::cerr << "--require-transform-animation requires --require-scene-package\n";
        return 4;
    }
    if (options.require_morph_package && (!packaged_scene.has_value() || !packaged_morph_payload.has_value() ||
                                          !packaged_morph_animation_clip.has_value())) {
        std::cerr << "--require-morph-package requires --require-scene-package\n";
        return 4;
    }
    if (options.require_quaternion_animation && packaged_quaternion_animation_tracks.empty()) {
        std::cerr << "--require-quaternion-animation requires --require-scene-package\n";
        return 4;
    }
    if (options.require_audio_gameplay_mixer && !packaged_audio_samples.has_value()) {
        std::cerr << "--require-audio-gameplay-mixer requires --require-scene-package with packaged gameplay audio\n";
        return 4;
    }
    if (options.require_audio_production && !packaged_audio_samples.has_value()) {
        std::cerr << "--require-audio-production requires --require-scene-package with packaged gameplay audio\n";
        return 4;
    }
    if (options.require_package_streaming_safe_point &&
        (!runtime_package.has_value() || options.required_scene_package_path.empty())) {
        std::cerr << "--require-package-streaming-safe-point requires --require-scene-package\n";
        return 4;
    }
    if (options.require_scene_collision_package && !runtime_package.has_value()) {
        std::cerr << "--require-scene-collision-package requires --require-scene-package\n";
        return 4;
    }
    if (options.require_compute_morph_skin && !activate_compute_morph_skinned_scene(packaged_scene)) {
        std::cerr
            << "--require-compute-morph-skin requires a packaged scene mesh node and skinned mesh package asset\n";
        return 4;
    }
    if (options.require_vulkan_renderer && options.require_compute_morph_async_telemetry) {
        std::cerr << "Vulkan compute morph package smoke does not support async telemetry requirements; use the D3D12 "
                     "lane for that package smoke\n";
        return 4;
    }
    mirakana::runtime::RuntimePackageStreamingExecutionResult package_streaming_result;
    if (options.require_package_streaming_safe_point) {
        package_streaming_result =
            execute_package_streaming_safe_point_smoke(*runtime_package, options.required_scene_package_path);
        if (!package_streaming_result.succeeded()) {
            print_package_streaming_diagnostics(package_streaming_result);
            return 4;
        }
    }
    const auto collision_package =
        evaluate_scene_collision_package(options, runtime_package ? &*runtime_package : nullptr);
    if (options.require_scene_collision_package && !collision_package.ready) {
        print_collision_package_diagnostics(collision_package);
        std::cerr << "required scene collision package was not ready\n";
        return 4;
    }
    UiAtlasMetadataRuntimeState ui_atlas_metadata;
    if (options.require_native_ui_textured_sprite_atlas) {
        ui_atlas_metadata = load_required_ui_atlas_metadata(runtime_package ? &*runtime_package : nullptr);
        if (ui_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            print_ui_atlas_metadata_diagnostics("sample_generated_desktop_runtime_3d_package", ui_atlas_metadata);
            return 4;
        }
    }
    UiGlyphAtlasMetadataRuntimeState ui_text_glyph_atlas_metadata;
    if (options.require_native_ui_text_glyph_atlas) {
        ui_text_glyph_atlas_metadata =
            load_required_ui_text_glyph_atlas_metadata(runtime_package ? &*runtime_package : nullptr);
        if (ui_text_glyph_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            print_ui_atlas_metadata_diagnostics("sample_generated_desktop_runtime_3d_package",
                                                ui_text_glyph_atlas_metadata);
            return 4;
        }
    }
    const bool requires_ui_atlas_metadata =
        options.require_native_ui_textured_sprite_atlas || options.require_native_ui_text_glyph_atlas;
    const bool requires_native_ui_texture_overlay = requires_ui_atlas_metadata;
    const auto ui_atlas_metadata_status = [&]() noexcept {
        if (!requires_ui_atlas_metadata) {
            return UiAtlasMetadataStatus::not_requested;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            ui_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            return ui_atlas_metadata.status;
        }
        if (options.require_native_ui_text_glyph_atlas &&
            ui_text_glyph_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            return ui_text_glyph_atlas_metadata.status;
        }
        return UiAtlasMetadataStatus::ready;
    }();
    const auto ui_atlas_metadata_pages =
        options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.pages : ui_text_glyph_atlas_metadata.pages;
    const auto ui_atlas_metadata_bindings =
        options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.bindings : std::size_t{0};
    const auto ui_atlas_metadata_glyphs =
        options.require_native_ui_text_glyph_atlas ? ui_text_glyph_atlas_metadata.glyphs : std::size_t{0};
    const auto native_ui_overlay_atlas_asset = [&]() noexcept {
        if (options.require_native_ui_textured_sprite_atlas) {
            return ui_atlas_metadata.atlas_page;
        }
        if (options.require_native_ui_text_glyph_atlas) {
            return ui_text_glyph_atlas_metadata.atlas_page;
        }
        return mirakana::AssetId{};
    }();

    auto d3d12_shader_bytecode = load_packaged_d3d12_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shader_bytecode.ready() && options.require_d3d12_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_3d_package shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shader_bytecode.status) << ": "
                  << d3d12_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_postprocess_bytecode.ready() && options.require_postprocess && !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_postprocess_bytecode.status) << ": "
                  << d3d12_postprocess_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_native_ui_overlay_bytecode = load_packaged_d3d12_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay &&
        !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_native_ui_overlay_bytecode.status) << ": "
                  << d3d12_native_ui_overlay_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_morph_shader_bytecode = load_packaged_d3d12_scene_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_morph_shader_bytecode.ready() && options.require_d3d12_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_3d_package morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_morph_shader_bytecode.status) << ": "
                  << d3d12_morph_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders || (options.require_compute_morph && !options.require_vulkan_renderer))) {
        std::cout << "sample_generated_desktop_runtime_3d_package compute_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_compute_morph_shader_bytecode.status)
                  << ": " << d3d12_compute_morph_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_tangent_frame_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_tangent_frame_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_tangent_frame_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders ||
         (options.require_compute_morph_normal_tangent && !options.require_vulkan_renderer))) {
        std::cout << "sample_generated_desktop_runtime_3d_package compute_morph_tangent_frame_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         d3d12_compute_morph_tangent_frame_shader_bytecode.status)
                  << ": " << d3d12_compute_morph_tangent_frame_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_compute_morph_skinned_shader_bytecode =
        load_packaged_d3d12_scene_compute_morph_skinned_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_compute_morph_skinned_shader_bytecode.ready() &&
        (options.require_d3d12_scene_shaders ||
         (options.require_compute_morph_skin && !options.require_vulkan_renderer))) {
        std::cout << "sample_generated_desktop_runtime_3d_package compute_morph_skinned_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         d3d12_compute_morph_skinned_shader_bytecode.status)
                  << ": " << d3d12_compute_morph_skinned_shader_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shadow_receiver_bytecode =
        load_packaged_d3d12_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shadow_receiver_bytecode.status) << ": "
                  << d3d12_shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shifted_shadow_receiver_bytecode =
        load_packaged_d3d12_shifted_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shifted_shadow_receiver_bytecode.ready() && options.require_shadow_morph_composition &&
        !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package shifted_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shifted_shadow_receiver_bytecode.status)
                  << ": " << d3d12_shifted_shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto d3d12_shadow_bytecode = load_packaged_d3d12_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!d3d12_shadow_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(d3d12_shadow_bytecode.status) << ": "
                  << d3d12_shadow_bytecode.diagnostic << '\n';
        return 4;
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready() && options.require_vulkan_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                  << vulkan_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready() && options.require_postprocess && options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                  << vulkan_postprocess_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_native_ui_overlay_bytecode =
        load_packaged_vulkan_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay &&
        options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_native_ui_overlay_bytecode.status)
                  << ": " << vulkan_native_ui_overlay_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_morph_shader_bytecode = load_packaged_vulkan_scene_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_morph_shader_bytecode.ready() && options.require_vulkan_scene_shaders) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_morph_shader_bytecode.status) << ": "
                  << vulkan_morph_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_shader_bytecode.ready() && options.require_vulkan_renderer &&
        ((options.require_compute_morph && !options.require_compute_morph_normal_tangent &&
          !options.require_compute_morph_skin) ||
         options.require_directional_shadow)) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_compute_morph_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_compute_morph_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_tangent_frame_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_tangent_frame_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_tangent_frame_shader_bytecode.ready() && options.require_vulkan_renderer &&
        options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_compute_morph_tangent_frame_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         vulkan_compute_morph_tangent_frame_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_tangent_frame_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_compute_morph_skinned_shader_bytecode =
        load_packaged_vulkan_scene_compute_morph_skinned_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_compute_morph_skinned_shader_bytecode.ready() && options.require_vulkan_renderer &&
        options.require_compute_morph_skin) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_compute_morph_skinned_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(
                         vulkan_compute_morph_skinned_shader_bytecode.status)
                  << ": " << vulkan_compute_morph_skinned_shader_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_receiver_bytecode =
        load_packaged_vulkan_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_receiver_bytecode.status) << ": "
                  << vulkan_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shifted_shadow_receiver_bytecode =
        load_packaged_vulkan_shifted_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shifted_shadow_receiver_bytecode.ready() && options.require_shadow_morph_composition &&
        options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_shifted_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shifted_shadow_receiver_bytecode.status)
                  << ": " << vulkan_shifted_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_bytecode = load_packaged_vulkan_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_bytecode.ready() && options.require_directional_shadow && options.require_vulkan_renderer) {
        std::cout << "sample_generated_desktop_runtime_3d_package vulkan_shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_bytecode.status) << ": "
                  << vulkan_shadow_bytecode.diagnostic << '\n';
        return 6;
    }

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    const auto& d3d12_scene_bytecode =
        options.require_directional_shadow ? d3d12_shadow_receiver_bytecode : d3d12_shader_bytecode;
    const bool require_graphics_morph_scene =
        options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph;
    const bool d3d12_morph_ready = !require_graphics_morph_scene || d3d12_morph_shader_bytecode.ready();
    const bool d3d12_shadow_ready = !options.require_directional_shadow || d3d12_shadow_bytecode.ready();
    const bool d3d12_native_ui_overlay_ready =
        !options.require_native_ui_overlay || d3d12_native_ui_overlay_bytecode.ready();
    if (d3d12_scene_bytecode.ready() && d3d12_morph_ready && d3d12_shadow_ready &&
        (!options.require_compute_morph || d3d12_compute_morph_shader_bytecode.ready()) &&
        (!options.require_compute_morph_normal_tangent || d3d12_compute_morph_tangent_frame_shader_bytecode.ready()) &&
        (!options.require_compute_morph_skin || d3d12_compute_morph_skinned_shader_bytecode.ready()) &&
        d3d12_native_ui_overlay_ready && d3d12_postprocess_bytecode.ready() && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::SdlDesktopPresentationD3d12SceneRendererDesc{
            .vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_scene_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_scene_bytecode.vertex_shader.bytecode.data(),
                                                              d3d12_scene_bytecode.vertex_shader.bytecode.size()},
                },
            .fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_scene_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_scene_bytecode.fragment_shader.bytecode.data(),
                                                              d3d12_scene_bytecode.fragment_shader.bytecode.size()},
                },
            .postprocess_vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_postprocess_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{d3d12_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                              d3d12_postprocess_bytecode.vertex_shader.bytecode.size()},
                },
            .postprocess_fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_postprocess_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{d3d12_postprocess_bytecode.fragment_shader.bytecode.data(),
                                                      d3d12_postprocess_bytecode.fragment_shader.bytecode.size()},
                },
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(d3d12_native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(d3d12_native_ui_overlay_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = options.require_postprocess_depth_input,
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset = native_ui_overlay_atlas_asset,
            .enable_native_ui_overlay_textures = requires_native_ui_texture_overlay,
        });
        if (options.require_compute_morph_skin) {
            d3d12_scene_renderer->skinned_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.data(),
                        d3d12_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.data(),
                        d3d12_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.size()},
            };
            d3d12_scene_renderer->skinned_vertex_buffers = runtime_compute_morph_skinned_scene_vertex_buffers();
            d3d12_scene_renderer->skinned_vertex_attributes = runtime_compute_morph_skinned_scene_vertex_attributes();
            d3d12_scene_renderer->compute_morph_skinned_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_skinned_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_compute_morph) {
            const auto& selected_compute_morph_shader_bytecode = options.require_compute_morph_normal_tangent
                                                                     ? d3d12_compute_morph_tangent_frame_shader_bytecode
                                                                     : d3d12_compute_morph_shader_bytecode;
            d3d12_scene_renderer->compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{selected_compute_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                  selected_compute_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        selected_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                        selected_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            d3d12_scene_renderer->enable_compute_morph_tangent_frame_output =
                options.require_compute_morph_normal_tangent;
            d3d12_scene_renderer->compute_morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_morph_package) {
            d3d12_scene_renderer->morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->morph_mesh_assets = {packaged_morph_mesh_asset_id()};
            d3d12_scene_renderer->morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        }
        if (options.require_directional_shadow) {
            if (require_graphics_morph_scene) {
                d3d12_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = d3d12_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{
                            d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),
                            d3d12_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},
                };
            }
            d3d12_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shadow_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.vertex_shader.bytecode.data(),
                                                          d3d12_shadow_bytecode.vertex_shader.bytecode.size()},
            };
            d3d12_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = d3d12_shadow_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{d3d12_shadow_bytecode.fragment_shader.bytecode.data(),
                                                          d3d12_shadow_bytecode.fragment_shader.bytecode.size()},
            };
        }
    }

    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
    const auto& vulkan_scene_bytecode =
        options.require_directional_shadow ? vulkan_shadow_receiver_bytecode : vulkan_shader_bytecode;
    const bool vulkan_morph_ready = !require_graphics_morph_scene || vulkan_morph_shader_bytecode.ready();
    const bool vulkan_shadow_ready = !options.require_directional_shadow || vulkan_shadow_bytecode.ready();
    const bool vulkan_native_ui_overlay_ready =
        !options.require_native_ui_overlay || vulkan_native_ui_overlay_bytecode.ready();
    if (vulkan_scene_bytecode.ready() && vulkan_morph_ready && vulkan_shadow_ready &&
        (!options.require_compute_morph || options.require_compute_morph_skin ||
         (options.require_compute_morph_normal_tangent ? vulkan_compute_morph_tangent_frame_shader_bytecode.ready()
                                                       : vulkan_compute_morph_shader_bytecode.ready())) &&
        (!options.require_compute_morph_skin || vulkan_compute_morph_skinned_shader_bytecode.ready()) &&
        (!options.require_directional_shadow || vulkan_compute_morph_shader_bytecode.ready()) &&
        vulkan_native_ui_overlay_ready && vulkan_postprocess_bytecode.ready() && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        vulkan_scene_renderer.emplace(mirakana::SdlDesktopPresentationVulkanSceneRendererDesc{
            .vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_scene_bytecode.vertex_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{vulkan_scene_bytecode.vertex_shader.bytecode.data(),
                                                              vulkan_scene_bytecode.vertex_shader.bytecode.size()},
                },
            .fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_scene_bytecode.fragment_shader.entry_point,
                    .bytecode = std::span<const std::uint8_t>{vulkan_scene_bytecode.fragment_shader.bytecode.data(),
                                                              vulkan_scene_bytecode.fragment_shader.bytecode.size()},
                },
            .postprocess_vertex_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_postprocess_bytecode.vertex_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{vulkan_postprocess_bytecode.vertex_shader.bytecode.data(),
                                                      vulkan_postprocess_bytecode.vertex_shader.bytecode.size()},
                },
            .postprocess_fragment_shader =
                mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_postprocess_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{vulkan_postprocess_bytecode.fragment_shader.bytecode.data(),
                                                      vulkan_postprocess_bytecode.fragment_shader.bytecode.size()},
                },
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = options.require_postprocess_depth_input,
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset = native_ui_overlay_atlas_asset,
            .enable_native_ui_overlay_textures = requires_native_ui_texture_overlay,
        });
        if (options.require_compute_morph_skin) {
            vulkan_scene_renderer->skinned_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.data(),
                        vulkan_compute_morph_skinned_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->compute_morph_skinned_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.data(),
                        vulkan_compute_morph_skinned_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->skinned_vertex_buffers = runtime_compute_morph_skinned_scene_vertex_buffers();
            vulkan_scene_renderer->skinned_vertex_attributes = runtime_compute_morph_skinned_scene_vertex_attributes();
            vulkan_scene_renderer->compute_morph_skinned_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_skinned_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_compute_morph) {
            const auto& selected_compute_morph_shader_bytecode =
                options.require_compute_morph_normal_tangent ? vulkan_compute_morph_tangent_frame_shader_bytecode
                                                             : vulkan_compute_morph_shader_bytecode;
            vulkan_scene_renderer->compute_morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{selected_compute_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                  selected_compute_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = selected_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{
                        selected_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                        selected_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->enable_compute_morph_tangent_frame_output =
                options.require_compute_morph_normal_tangent;
            vulkan_scene_renderer->compute_morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        } else if (options.require_morph_package) {
            vulkan_scene_renderer->morph_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_morph_shader_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_morph_shader_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_morph_shader_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->morph_mesh_assets = {packaged_morph_mesh_asset_id()};
            vulkan_scene_renderer->morph_mesh_bindings = {
                mirakana::SdlDesktopPresentationSceneMorphMeshBinding{.mesh = packaged_mesh_asset_id(),
                                                                      .morph_mesh = packaged_morph_mesh_asset_id()},
            };
        }
        if (options.require_directional_shadow) {
            if (require_graphics_morph_scene) {
                vulkan_scene_renderer->skinned_scene_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                    .entry_point = vulkan_shifted_shadow_receiver_bytecode.fragment_shader.entry_point,
                    .bytecode =
                        std::span<const std::uint8_t>{
                            vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.data(),
                            vulkan_shifted_shadow_receiver_bytecode.fragment_shader.bytecode.size()},
                };
            }
            vulkan_scene_renderer->compute_morph_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_compute_morph_shader_bytecode.fragment_shader.entry_point,
                .bytecode =
                    std::span<const std::uint8_t>{vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.data(),
                                                  vulkan_compute_morph_shader_bytecode.fragment_shader.bytecode.size()},
            };
            vulkan_scene_renderer->shadow_vertex_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shadow_bytecode.vertex_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.vertex_shader.bytecode.data(),
                                                          vulkan_shadow_bytecode.vertex_shader.bytecode.size()},
            };
            vulkan_scene_renderer->shadow_fragment_shader = mirakana::SdlDesktopPresentationShaderBytecode{
                .entry_point = vulkan_shadow_bytecode.fragment_shader.entry_point,
                .bytecode = std::span<const std::uint8_t>{vulkan_shadow_bytecode.fragment_shader.bytecode.data(),
                                                          vulkan_shadow_bytecode.fragment_shader.bytecode.size()},
            };
        }
    }

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "sample-generated-desktop-runtime-3d-package",
        .extent = mirakana::WindowExtent{.width = 960, .height = 540},
        .video_driver_hint = options.video_driver_hint,
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_scene_renderer.has_value()) {
        host_desc.d3d12_scene_renderer = &*d3d12_scene_renderer;
    }
    if (vulkan_scene_renderer.has_value()) {
        host_desc.vulkan_scene_renderer = &*vulkan_scene_renderer;
    }

    mirakana::SdlDesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::d3d12) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_scene_gpu_bindings_unavailable status="
                  << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
                  << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_postprocess_unavailable status="
                  << mirakana::sdl_desktop_presentation_postprocess_status_name(
                         host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        return 8;
    }
    if (options.require_postprocess_depth_input && !host.presentation_report().postprocess_depth_input_ready) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_postprocess_depth_input_unavailable\n";
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        return 8;
    }
    if (options.require_native_ui_overlay && !host.presentation_report().native_ui_overlay_ready) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_native_ui_overlay_unavailable status="
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(
                         host.presentation_report().native_ui_overlay_status)
                  << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
            std::cout << "sample_generated_desktop_runtime_3d_package native_ui_overlay_diagnostic="
                      << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 10;
    }
    if (requires_native_ui_texture_overlay) {
        const auto texture_overlay_report = host.presentation_report();
        if (texture_overlay_report.native_ui_texture_overlay_status !=
                mirakana::SdlDesktopPresentationNativeUiTextureOverlayStatus::ready ||
            !texture_overlay_report.native_ui_texture_overlay_atlas_ready) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_native_ui_texture_overlay_unavailable "
                         "status="
                      << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(
                             texture_overlay_report.native_ui_texture_overlay_status)
                      << " atlas_ready=" << (texture_overlay_report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
                      << '\n';
            print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
            for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
                std::cout << "sample_generated_desktop_runtime_3d_package native_ui_texture_overlay_diagnostic="
                          << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status)
                          << ": " << diagnostic.message << '\n';
            }
            return 11;
        }
    }
    if (options.require_directional_shadow && !host.presentation_report().directional_shadow_ready) {
        std::cout << "sample_generated_desktop_runtime_3d_package required_directional_shadow_unavailable status="
                  << mirakana::sdl_desktop_presentation_directional_shadow_status_name(
                         host.presentation_report().directional_shadow_status)
                  << '\n';
        print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
        for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
            std::cout << "sample_generated_desktop_runtime_3d_package directional_shadow_diagnostic="
                      << mirakana::sdl_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 9;
    }
    if (options.require_directional_shadow_filtering) {
        const auto report = host.presentation_report();
        if (report.directional_shadow_filter_mode !=
                mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
            report.directional_shadow_filter_tap_count != 9 || report.directional_shadow_filter_radius_texels != 1.0F) {
            std::cout << "sample_generated_desktop_runtime_3d_package "
                         "required_directional_shadow_filtering_unavailable mode="
                      << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(
                             report.directional_shadow_filter_mode)
                      << " taps=" << report.directional_shadow_filter_tap_count
                      << " radius_texels=" << report.directional_shadow_filter_radius_texels << '\n';
            print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
            return 9;
        }
    }

    const auto audio_production_probe = packaged_audio_samples.has_value()
                                            ? validate_audio_production_package_evidence(*packaged_audio_samples)
                                            : AudioProductionProbeResult{};

    GeneratedDesktopRuntime3DPackageGame game(
        host.input(), host.renderer(), options.throttle, std::move(packaged_scene), std::move(packaged_animation_clip),
        packaged_animation_bindings(), std::move(packaged_morph_payload), std::move(packaged_morph_animation_clip),
        std::move(packaged_quaternion_animation_tracks), std::move(packaged_audio_samples),
        options.require_native_ui_textured_sprite_atlas, std::move(ui_atlas_metadata.palette),
        options.require_native_ui_text_glyph_atlas, std::move(ui_text_glyph_atlas_metadata.glyph_atlas));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;
    const auto postprocess_policy = mirakana::evaluate_sdl_desktop_presentation_postprocess_policy(report);
    const auto renderer_quality =
        mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));
    const auto playable_3d =
        evaluate_playable_3d_slice(options, result, game, package_streaming_result,
                                   runtime_package ? runtime_package->records().size() : 0U, report, renderer_quality);
    const auto gameplay_systems_status = game.gameplay_systems_status(options.max_frames);
    const auto gameplay_systems_core_ready = game.gameplay_systems_passed(options.max_frames);
    const auto input_context_rebinding = validate_sample_input_context_rebinding();
    const auto gameplay_runtime_scheduler_probe = options.require_gameplay_systems
                                                      ? validate_gameplay_runtime_scheduler_package_evidence()
                                                      : GameplayRuntimeSchedulerProbeResult{};
    const auto world_entity_model_probe = options.require_gameplay_systems
                                              ? validate_world_entity_model_package_evidence()
                                              : WorldEntityModelProbeResult{};
    const auto addressable_content_probe =
        options.require_gameplay_systems && runtime_package.has_value()
            ? validate_addressable_content_streaming_package_evidence(*runtime_package)
            : AddressableContentStreamingProbeResult{};
    const auto rpg_systems_probe =
        options.require_gameplay_systems ? validate_rpg_systems_package_evidence("sample3d") : RpgSystemsProbeResult{};
    const auto sandbox_world_probe = options.require_gameplay_systems
                                         ? validate_sandbox_world_package_evidence("sample3d")
                                         : SandboxWorldProbeResult{};
    const auto simulation_management_probe = options.require_gameplay_systems
                                                 ? validate_simulation_management_package_evidence("sample3d")
                                                 : SimulationManagementProbeResult{};
    const auto network_replication_probe = options.require_gameplay_systems
                                               ? validate_network_replication_package_evidence("sample3d")
                                               : NetworkReplicationProbeResult{};
    const auto rendering_vfx_profiling_probe = options.require_rendering_vfx_profiling
                                                   ? validate_rendering_vfx_profiling_package_evidence()
                                                   : RenderingVfxProfilingProbeResult{};
    const auto renderer_quality_matrix_probe = options.require_renderer_quality_matrix
                                                   ? validate_renderer_quality_matrix_package_evidence()
                                                   : RendererQualityMatrixProbeResult{};
    const auto gameplay_systems_ready =
        gameplay_systems_core_ready &&
        (!options.require_gameplay_systems ||
         (game.gameplay_systems_scene_binding_ready() && input_context_rebinding.ready &&
          gameplay_runtime_scheduler_probe.ready && world_entity_model_probe.ready && addressable_content_probe.ready &&
          rpg_systems_probe.ready && sandbox_world_probe.ready && simulation_management_probe.ready &&
          network_replication_probe.reviewed));
    const auto gameplay_systems_diagnostics =
        game.gameplay_systems_diagnostics_count(options.max_frames) +
        ((options.require_gameplay_systems && !game.gameplay_systems_scene_binding_ready()) ? 1U : 0U) +
        ((options.require_gameplay_systems && !input_context_rebinding.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !gameplay_runtime_scheduler_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !world_entity_model_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !addressable_content_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !rpg_systems_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !sandbox_world_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !simulation_management_probe.ready) ? 1U : 0U) +
        ((options.require_gameplay_systems && !network_replication_probe.reviewed) ? 1U : 0U);
    const auto visible_3d = evaluate_visible_3d_production_proof(options, result, report, renderer_quality, playable_3d,
                                                                 gameplay_systems_ready);
    const auto entity_scale_culling_probe = options.require_entity_scale_culling
                                                ? validate_entity_scale_culling_package_evidence()
                                                : EntityScaleCullingProbeResult{};
    const auto package_upload_staging = options.require_package_upload_staging
                                            ? run_package_upload_staging_evidence(host.presentation())
                                            : mirakana::runtime_rhi::RuntimePackageUploadStagingEvidence{};

    const auto& audio_gameplay_mixer = game.gameplay_systems_audio_gameplay_mixer_probe();
    std::cout
        << "sample_generated_desktop_runtime_3d_package status=" << status_name(result.status)
        << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback=" << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " scene_gpu_status="
        << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " scene_gpu_mesh_bindings=" << scene_gpu_stats.mesh_bindings
        << " scene_gpu_skinned_mesh_bindings=" << scene_gpu_stats.skinned_mesh_bindings
        << " scene_gpu_material_bindings=" << scene_gpu_stats.material_bindings
        << " scene_gpu_mesh_uploads=" << scene_gpu_stats.mesh_uploads
        << " scene_gpu_skinned_mesh_uploads=" << scene_gpu_stats.skinned_mesh_uploads
        << " scene_gpu_texture_uploads=" << scene_gpu_stats.texture_uploads
        << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
        << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
        << " scene_gpu_morph_mesh_bindings=" << scene_gpu_stats.morph_mesh_bindings
        << " scene_gpu_morph_mesh_uploads=" << scene_gpu_stats.morph_mesh_uploads
        << " scene_gpu_morph_mesh_resolved=" << scene_gpu_stats.morph_mesh_bindings_resolved
        << " scene_gpu_uploaded_morph_bytes=" << scene_gpu_stats.uploaded_morph_bytes
        << " renderer_gpu_morph_draws=" << report.renderer_stats.gpu_morph_draws
        << " renderer_morph_descriptor_binds=" << report.renderer_stats.morph_descriptor_binds
        << " scene_gpu_compute_morph_mesh_bindings=" << scene_gpu_stats.compute_morph_mesh_bindings
        << " scene_gpu_compute_morph_dispatches=" << scene_gpu_stats.compute_morph_mesh_dispatches
        << " scene_gpu_compute_morph_queue_waits=" << scene_gpu_stats.compute_morph_queue_waits
        << " scene_gpu_compute_morph_async_compute_queue_submits="
        << scene_gpu_stats.compute_morph_async_compute_queue_submits
        << " scene_gpu_compute_morph_async_graphics_queue_waits="
        << scene_gpu_stats.compute_morph_async_graphics_queue_waits
        << " scene_gpu_compute_morph_async_graphics_queue_submits="
        << scene_gpu_stats.compute_morph_async_graphics_queue_submits
        << " scene_gpu_compute_morph_async_last_compute_fence="
        << scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value
        << " scene_gpu_compute_morph_async_last_graphics_wait_fence="
        << scene_gpu_stats.compute_morph_async_last_graphics_queue_wait_fence_value
        << " scene_gpu_compute_morph_async_last_graphics_submit_fence="
        << scene_gpu_stats.compute_morph_async_last_graphics_submitted_fence_value
        << " scene_gpu_compute_morph_mesh_resolved=" << scene_gpu_stats.compute_morph_mesh_bindings_resolved
        << " scene_gpu_compute_morph_draws=" << scene_gpu_stats.compute_morph_mesh_draws
        << " scene_gpu_compute_morph_tangent_frame_output="
        << (options.require_compute_morph_normal_tangent && !options.require_compute_morph_skin ? 1 : 0)
        << " scene_gpu_compute_morph_skinned_mesh_bindings=" << scene_gpu_stats.compute_morph_skinned_mesh_bindings
        << " scene_gpu_compute_morph_skinned_dispatches=" << scene_gpu_stats.compute_morph_skinned_mesh_dispatches
        << " scene_gpu_compute_morph_skinned_queue_waits=" << scene_gpu_stats.compute_morph_skinned_queue_waits
        << " scene_gpu_compute_morph_skinned_mesh_resolved="
        << scene_gpu_stats.compute_morph_skinned_mesh_bindings_resolved
        << " scene_gpu_compute_morph_skinned_draws=" << scene_gpu_stats.compute_morph_skinned_mesh_draws
        << " scene_gpu_compute_morph_output_position_bytes=" << scene_gpu_stats.compute_morph_output_position_bytes
        << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
        << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
        << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
        << " scene_gpu_uploaded_mesh_bytes=" << scene_gpu_stats.uploaded_mesh_bytes
        << " scene_gpu_uploaded_material_factor_bytes=" << scene_gpu_stats.uploaded_material_factor_bytes
        << " scene_gpu_mesh_resolved=" << scene_gpu_stats.mesh_bindings_resolved
        << " scene_gpu_skinned_mesh_resolved=" << scene_gpu_stats.skinned_mesh_bindings_resolved
        << " scene_gpu_material_resolved=" << scene_gpu_stats.material_bindings_resolved << " postprocess_status="
        << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
        << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
        << " postprocess_policy_status="
        << mirakana::sdl_desktop_presentation_postprocess_policy_status_name(postprocess_policy.status)
        << " postprocess_policy_ready=" << (postprocess_policy.ready ? 1 : 0)
        << " postprocess_policy_diagnostics=" << postprocess_policy.diagnostics_count
        << " postprocess_policy_effects=" << postprocess_policy.effect_count
        << " postprocess_policy_postprocess_passes=" << postprocess_policy.postprocess_pass_count
        << " postprocess_policy_framegraph_passes=" << postprocess_policy.framegraph_pass_count
        << " postprocess_policy_framegraph_barrier_step_budget=" << postprocess_policy.framegraph_barrier_step_budget
        << " postprocess_policy_scene_color_required=" << (postprocess_policy.scene_color_required ? 1 : 0)
        << " postprocess_policy_scene_depth_required=" << (postprocess_policy.scene_depth_required ? 1 : 0)
        << " postprocess_policy_color_grading_effect=" << (postprocess_policy.color_grading_effect ? 1 : 0)
        << " postprocess_policy_backend_shader_evidence_ready="
        << (postprocess_policy.backend_shader_evidence_ready ? 1 : 0) << " directional_shadow_status="
        << mirakana::sdl_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
        << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
        << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
        << " directional_shadow_filter_mode="
        << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(report.directional_shadow_filter_mode)
        << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
        << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
        << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
        << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
        << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
        << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
        << " ui_overlay_draws=" << report.native_ui_overlay_draws
        << " ui_atlas_metadata_requested=" << (requires_ui_atlas_metadata ? 1 : 0)
        << " ui_atlas_metadata_status=" << ui_atlas_metadata_status_name(ui_atlas_metadata_status)
        << " ui_atlas_metadata_pages=" << ui_atlas_metadata_pages
        << " ui_atlas_metadata_bindings=" << ui_atlas_metadata_bindings
        << " ui_atlas_metadata_glyphs=" << ui_atlas_metadata_glyphs
        << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
        << " ui_texture_overlay_status="
        << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(
               report.native_ui_texture_overlay_status)
        << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
        << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
        << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
        << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
        << " package_streaming_status=" << package_streaming_status_name(package_streaming_result.status)
        << " package_streaming_ready=" << (package_streaming_result.succeeded() ? 1 : 0)
        << " package_streaming_resident_bytes=" << package_streaming_result.estimated_resident_bytes
        << " package_streaming_resident_budget_bytes=" << package_streaming_result.resident_budget_bytes
        << " package_streaming_committed_records=" << package_streaming_result.replacement.committed_record_count
        << " package_streaming_stale_handles=" << package_streaming_result.replacement.stale_handle_count
        << " package_streaming_required_preload_assets=" << package_streaming_result.required_preload_asset_count
        << " package_streaming_resident_resource_kinds=" << package_streaming_result.resident_resource_kind_count
        << " package_streaming_resident_packages=" << package_streaming_result.resident_package_count
        << " package_streaming_diagnostics=" << package_streaming_result.diagnostics.size()
        << " package_upload_staging_status="
        << package_upload_staging_status_name(options.require_package_upload_staging, package_upload_staging)
        << " package_upload_staging_ready=" << (package_upload_staging.ready ? 1 : 0)
        << " package_upload_staging_diagnostics=" << package_upload_staging.diagnostics.size()
        << " package_upload_staging_package_transactions=" << package_upload_staging.package_transactions
        << " package_upload_staging_texture_uploads=" << package_upload_staging.texture_uploads
        << " package_upload_staging_mesh_uploads=" << package_upload_staging.mesh_uploads
        << " package_upload_staging_skinned_mesh_uploads=" << package_upload_staging.skinned_mesh_uploads
        << " package_upload_staging_morph_mesh_uploads=" << package_upload_staging.morph_mesh_uploads
        << " package_upload_staging_texture_bindings=" << package_upload_staging.texture_bindings
        << " package_upload_staging_mesh_bindings=" << package_upload_staging.mesh_bindings
        << " package_upload_staging_skinned_mesh_bindings=" << package_upload_staging.skinned_mesh_bindings
        << " package_upload_staging_morph_mesh_bindings=" << package_upload_staging.morph_mesh_bindings
        << " package_upload_staging_staging_pool_leases=" << package_upload_staging.staging_pool_leases
        << " package_upload_staging_ring_backed_uploads=" << package_upload_staging.ring_backed_uploads
        << " package_upload_staging_resource_updates_ready=" << (package_upload_staging.resource_updates_ready ? 1 : 0)
        << " package_upload_staging_resource_updates=" << package_upload_staging.resource_updates
        << " package_upload_staging_resource_update_submitted_fences="
        << package_upload_staging.resource_update_submitted_fences
        << " package_upload_staging_resource_update_graphics_ready_updates="
        << package_upload_staging.resource_update_graphics_ready_updates
        << " package_upload_staging_resource_update_graphics_queue_waits_recorded="
        << package_upload_staging.resource_update_graphics_queue_waits_recorded
        << " package_upload_staging_resource_update_same_queue_graphics_updates="
        << package_upload_staging.resource_update_same_queue_graphics_updates
        << " package_upload_staging_uploaded_bytes=" << package_upload_staging.uploaded_bytes
        << " package_upload_staging_submitted_fences=" << package_upload_staging.submitted_fences
        << " package_upload_staging_upload_queue_waits_recorded=" << package_upload_staging.upload_queue_waits_recorded
        << " package_upload_staging_copy_queue_submits=" << package_upload_staging.copy_queue_submits
        << " package_upload_staging_graphics_queue_submits=" << package_upload_staging.graphics_queue_submits
        << " package_upload_staging_queue_waits=" << package_upload_staging.queue_waits
        << " package_upload_staging_fence_waits=" << package_upload_staging.fence_waits
        << " package_upload_staging_graphics_waited_for_copy="
        << (package_upload_staging.graphics_waited_for_copy ? 1 : 0)
        << " collision_package_status=" << collision_package_status_name(collision_package.status)
        << " collision_package_ready=" << (collision_package.ready ? 1 : 0)
        << " collision_package_diagnostics=" << collision_package.diagnostics_count
        << " collision_package_bodies=" << collision_package.body_count
        << " collision_package_triggers=" << collision_package.trigger_count
        << " collision_package_contacts=" << collision_package.contact_count
        << " collision_package_trigger_overlaps=" << collision_package.trigger_overlap_count
        << " collision_package_world_ready=" << (collision_package.world_ready ? 1 : 0)
        << " collision_query_batch_ready=" << (collision_package.query_batch_ready ? 1 : 0)
        << " collision_query_batch_source_order_ready=" << (collision_package.query_batch_source_order_ready ? 1 : 0)
        << " collision_query_readiness_status="
        << physics_collision_query_readiness_status_name(collision_package.query_readiness_status)
        << " collision_query_readiness_diagnostic="
        << physics_collision_query_readiness_diagnostic_name(collision_package.query_readiness_diagnostic)
        << " collision_query_readiness_diagnostics=" << collision_package.query_readiness_diagnostics
        << " collision_query_batch_rows=" << collision_package.query_batch_rows
        << " collision_query_batch_hits=" << collision_package.query_batch_hits
        << " collision_query_batch_no_hits=" << collision_package.query_batch_no_hits
        << " collision_query_batch_invalid_requests=" << collision_package.query_batch_invalid_requests
        << " collision_query_batch_budget_rejections=" << collision_package.query_batch_budget_rejections
        << " framegraph_passes=" << report.framegraph_passes
        << " framegraph_passes_executed=" << report.renderer_stats.framegraph_passes_executed
        << " framegraph_render_passes_recorded=" << report.renderer_stats.framegraph_render_passes_recorded
        << " framegraph_barrier_steps_executed=" << report.renderer_stats.framegraph_barrier_steps_executed
        << " renderer_quality_status="
        << mirakana::sdl_desktop_presentation_quality_gate_status_name(renderer_quality.status)
        << " renderer_quality_ready=" << (renderer_quality.ready ? 1 : 0)
        << " renderer_quality_diagnostics=" << renderer_quality.diagnostics_count
        << " renderer_quality_expected_framegraph_passes=" << renderer_quality.expected_framegraph_passes
        << " renderer_quality_expected_framegraph_render_passes=" << renderer_quality.expected_framegraph_render_passes
        << " renderer_quality_expected_framegraph_barrier_steps=" << renderer_quality.expected_framegraph_barrier_steps
        << " renderer_quality_framegraph_passes_ok=" << (renderer_quality.framegraph_passes_current ? 1 : 0)
        << " renderer_quality_framegraph_render_passes_ok="
        << (renderer_quality.framegraph_render_passes_current ? 1 : 0)
        << " renderer_quality_framegraph_barrier_steps_ok="
        << (renderer_quality.framegraph_barrier_steps_current ? 1 : 0)
        << " renderer_quality_framegraph_execution_budget_ok="
        << (renderer_quality.framegraph_execution_budget_current ? 1 : 0)
        << " renderer_quality_scene_gpu_ready=" << (renderer_quality.scene_gpu_ready ? 1 : 0)
        << " renderer_quality_postprocess_ready=" << (renderer_quality.postprocess_ready ? 1 : 0)
        << " renderer_quality_postprocess_depth_input_ready="
        << (renderer_quality.postprocess_depth_input_ready ? 1 : 0)
        << " renderer_quality_directional_shadow_ready=" << (renderer_quality.directional_shadow_ready ? 1 : 0)
        << " renderer_quality_directional_shadow_filter_ready="
        << (renderer_quality.directional_shadow_filter_ready ? 1 : 0) << " renderer_quality_matrix_status="
        << renderer_quality_matrix_status_name(renderer_quality_matrix_probe.status)
        << " renderer_quality_matrix_reviewed=" << (renderer_quality_matrix_probe.reviewed ? 1 : 0)
        << " renderer_quality_matrix_ready=" << (renderer_quality_matrix_probe.ready ? 1 : 0)
        << " renderer_quality_matrix_rows=" << renderer_quality_matrix_probe.rows
        << " renderer_quality_matrix_ready_rows=" << renderer_quality_matrix_probe.ready_rows
        << " renderer_quality_matrix_host_gated_rows=" << renderer_quality_matrix_probe.host_gated_rows
        << " renderer_quality_matrix_dependency_gated_rows=" << renderer_quality_matrix_probe.dependency_gated_rows
        << " renderer_quality_matrix_unsupported_rows=" << renderer_quality_matrix_probe.unsupported_rows
        << " renderer_quality_matrix_host_validated_backends=" << renderer_quality_matrix_probe.host_validated_backends
        << " renderer_quality_matrix_replay_hash=" << renderer_quality_matrix_probe.replay_hash
        << " renderer_quality_matrix_d3d12_ready=" << (renderer_quality_matrix_probe.d3d12_ready ? 1 : 0)
        << " renderer_quality_matrix_vulkan_strict_ready="
        << (renderer_quality_matrix_probe.vulkan_strict_ready ? 1 : 0)
        << " renderer_quality_matrix_metal_ready=" << (renderer_quality_matrix_probe.metal_ready ? 1 : 0)
        << " renderer_quality_matrix_requires_metal_host_evidence="
        << (renderer_quality_matrix_probe.requires_metal_host_evidence ? 1 : 0)
        << " renderer_quality_matrix_metal_host_evidence="
        << (renderer_quality_matrix_probe.has_metal_host_evidence ? 1 : 0)
        << " renderer_quality_matrix_selected_package_evidence_ready="
        << (renderer_quality_matrix_probe.selected_package_evidence_ready ? 1 : 0)
        << " renderer_quality_matrix_general_renderer_quality_ready="
        << (renderer_quality_matrix_probe.general_renderer_quality_ready ? 1 : 0)
        << " renderer_quality_matrix_invoked_gpu_commands="
        << (renderer_quality_matrix_probe.invoked_gpu_commands ? 1 : 0)
        << " renderer_quality_matrix_invoked_native_capture="
        << (renderer_quality_matrix_probe.invoked_native_capture ? 1 : 0)
        << " renderer_quality_matrix_invoked_crash_upload="
        << (renderer_quality_matrix_probe.invoked_crash_upload ? 1 : 0)
        << " renderer_quality_matrix_diagnostics=" << renderer_quality_matrix_probe.diagnostics
        << " playable_3d_status=" << playable_3d_slice_status_name(playable_3d.status)
        << " playable_3d_ready=" << (playable_3d.ready ? 1 : 0)
        << " playable_3d_diagnostics=" << playable_3d.diagnostics_count
        << " playable_3d_expected_frames=" << playable_3d.expected_frames
        << " playable_3d_frames_ok=" << (playable_3d.frames_ok ? 1 : 0)
        << " playable_3d_game_frames_ok=" << (playable_3d.game_frames_ok ? 1 : 0)
        << " playable_3d_scene_mesh_plan_ready=" << (playable_3d.scene_mesh_plan_ready ? 1 : 0)
        << " playable_3d_camera_controller_ready=" << (playable_3d.camera_controller_ready ? 1 : 0)
        << " playable_3d_animation_ready=" << (playable_3d.animation_ready ? 1 : 0)
        << " playable_3d_morph_ready=" << (playable_3d.morph_ready ? 1 : 0)
        << " playable_3d_quaternion_ready=" << (playable_3d.quaternion_ready ? 1 : 0)
        << " playable_3d_package_streaming_ready=" << (playable_3d.package_streaming_ready ? 1 : 0)
        << " playable_3d_scene_gpu_ready=" << (playable_3d.scene_gpu_ready ? 1 : 0)
        << " playable_3d_postprocess_ready=" << (playable_3d.postprocess_ready ? 1 : 0)
        << " playable_3d_renderer_quality_ready=" << (playable_3d.renderer_quality_ready ? 1 : 0)
        << " playable_3d_compute_morph_selected=" << (playable_3d.compute_morph_selected ? 1 : 0)
        << " playable_3d_compute_morph_ready=" << (playable_3d.compute_morph_ready ? 1 : 0)
        << " playable_3d_compute_morph_normal_tangent_selected="
        << (playable_3d.compute_morph_normal_tangent_selected ? 1 : 0)
        << " playable_3d_compute_morph_normal_tangent_ready="
        << (playable_3d.compute_morph_normal_tangent_ready ? 1 : 0)
        << " playable_3d_compute_morph_skin_selected=" << (playable_3d.compute_morph_skin_selected ? 1 : 0)
        << " playable_3d_compute_morph_skin_ready=" << (playable_3d.compute_morph_skin_ready ? 1 : 0)
        << " playable_3d_compute_morph_async_telemetry_selected="
        << (playable_3d.compute_morph_async_telemetry_selected ? 1 : 0)
        << " playable_3d_compute_morph_async_telemetry_ready="
        << (playable_3d.compute_morph_async_telemetry_ready ? 1 : 0)
        << " visible_3d_status=" << visible_3d_production_proof_status_name(visible_3d.status)
        << " visible_3d_ready=" << (visible_3d.ready ? 1 : 0)
        << " visible_3d_diagnostics=" << visible_3d.diagnostics_count
        << " visible_3d_expected_frames=" << visible_3d.expected_frames
        << " visible_3d_presented_frames=" << visible_3d.presented_frames
        << " visible_3d_d3d12_selected=" << (visible_3d.d3d12_selected ? 1 : 0)
        << " visible_3d_vulkan_selected=" << (visible_3d.vulkan_selected ? 1 : 0)
        << " visible_3d_null_fallback_used=" << (visible_3d.null_fallback_used ? 1 : 0)
        << " visible_3d_scene_gpu_ready=" << (visible_3d.scene_gpu_ready ? 1 : 0)
        << " visible_3d_postprocess_ready=" << (visible_3d.postprocess_ready ? 1 : 0)
        << " visible_3d_renderer_quality_ready=" << (visible_3d.renderer_quality_ready ? 1 : 0)
        << " visible_3d_playable_ready=" << (visible_3d.playable_ready ? 1 : 0)
        << " visible_3d_ui_overlay_ready=" << (visible_3d.ui_overlay_ready ? 1 : 0)
        << " entity_scale_culling_status=" << entity_scale_culling_status_name(entity_scale_culling_probe.status)
        << " entity_scale_culling_ready=" << (entity_scale_culling_probe.ready ? 1 : 0)
        << " entity_scale_culling_rows=" << entity_scale_culling_probe.rows
        << " entity_scale_culling_visible_rows=" << entity_scale_culling_probe.visible_rows
        << " entity_scale_culling_culled_rows=" << entity_scale_culling_probe.culled_rows
        << " entity_scale_culling_lod_rows=" << entity_scale_culling_probe.lod_rows
        << " entity_scale_culling_priority_update_rows=" << entity_scale_culling_probe.priority_update_rows
        << " entity_scale_culling_normal_update_rows=" << entity_scale_culling_probe.normal_update_rows
        << " entity_scale_culling_background_update_rows=" << entity_scale_culling_probe.background_update_rows
        << " entity_scale_culling_projected_draw_cost=" << entity_scale_culling_probe.projected_draw_cost
        << " entity_scale_culling_projected_update_cost=" << entity_scale_culling_probe.projected_update_cost
        << " entity_scale_culling_budget_protected_rows=" << entity_scale_culling_probe.budget_protected_rows
        << " entity_scale_culling_diagnostics=" << entity_scale_culling_probe.diagnostics
        << " entity_scale_culling_budget_diagnostics=" << entity_scale_culling_probe.budget_diagnostics
        << " gameplay_systems_status=" << gameplay_systems_status_name(gameplay_systems_status)
        << " gameplay_systems_ready=" << (gameplay_systems_ready ? 1 : 0)
        << " gameplay_systems_diagnostics=" << gameplay_systems_diagnostics << " gameplay_runtime_scheduler_status="
        << gameplay_runtime_scheduler_status_name(gameplay_runtime_scheduler_probe.status)
        << " gameplay_runtime_scheduler_ready=" << (gameplay_runtime_scheduler_probe.ready ? 1 : 0)
        << " gameplay_runtime_scheduler_available_steps=" << gameplay_runtime_scheduler_probe.available_steps
        << " gameplay_runtime_scheduler_steps=" << gameplay_runtime_scheduler_probe.step_rows
        << " gameplay_runtime_scheduler_system_rows=" << gameplay_runtime_scheduler_probe.system_rows
        << " gameplay_runtime_scheduler_command_rows=" << gameplay_runtime_scheduler_probe.command_rows
        << " gameplay_runtime_scheduler_consumed_time_us=" << gameplay_runtime_scheduler_probe.consumed_time_us
        << " gameplay_runtime_scheduler_remaining_time_us=" << gameplay_runtime_scheduler_probe.remaining_time_us
        << " gameplay_runtime_scheduler_budget_limited=" << (gameplay_runtime_scheduler_probe.budget_limited ? 1 : 0)
        << " gameplay_runtime_scheduler_replay_hash=" << gameplay_runtime_scheduler_probe.replay_hash
        << " gameplay_runtime_scheduler_diagnostics=" << gameplay_runtime_scheduler_probe.diagnostics
        << " world_entity_model_status=" << world_entity_model_status_name(world_entity_model_probe.status)
        << " world_entity_model_ready=" << (world_entity_model_probe.ready ? 1 : 0)
        << " world_entity_model_entities=" << world_entity_model_probe.entity_rows
        << " world_entity_model_components=" << world_entity_model_probe.component_rows
        << " world_entity_model_region_ownership_rows=" << world_entity_model_probe.region_ownership_rows
        << " world_entity_model_lifecycle_rows=" << world_entity_model_probe.lifecycle_rows
        << " world_entity_model_persistence_rows=" << world_entity_model_probe.persistence_rows
        << " world_entity_model_streaming_region_rows=" << world_entity_model_probe.streaming_region_rows
        << " world_entity_model_spawn_rows=" << world_entity_model_probe.spawn_rows
        << " world_entity_model_move_rows=" << world_entity_model_probe.move_rows
        << " world_entity_model_despawn_rows=" << world_entity_model_probe.despawn_rows
        << " world_entity_model_duplicate_entity_diagnostics=" << world_entity_model_probe.duplicate_entity_diagnostics
        << " world_entity_model_bridge_rejection_status="
        << world_entity_model_status_name(world_entity_model_probe.bridge_rejection_status)
        << " world_entity_model_bridge_rejection_diagnostics=" << world_entity_model_probe.bridge_rejection_diagnostics
        << " world_entity_model_bridge_rejection_persistence_rows="
        << world_entity_model_probe.bridge_rejection_persistence_rows
        << " world_entity_model_bridge_rejection_streaming_region_rows="
        << world_entity_model_probe.bridge_rejection_streaming_region_rows
        << " world_entity_model_bridge_rejection_streaming_diagnostics_present="
        << world_entity_model_probe.bridge_rejection_streaming_diagnostics_present
        << " world_entity_model_bridge_rejection_fail_closed="
        << (world_entity_model_probe.bridge_rejection_fail_closed ? 1 : 0)
        << " world_entity_model_diagnostics=" << world_entity_model_probe.diagnostics
        << " addressable_content_status=" << addressable_content_status_name(addressable_content_probe.status)
        << " addressable_content_ready=" << (addressable_content_probe.ready ? 1 : 0)
        << " addressable_content_address_rows=" << addressable_content_probe.address_rows
        << " addressable_content_dependency_rows=" << addressable_content_probe.dependency_rows
        << " addressable_content_load_rows=" << addressable_content_probe.load_rows
        << " addressable_content_release_rows=" << addressable_content_probe.release_rows
        << " addressable_content_refcount_rows=" << addressable_content_probe.refcount_rows
        << " addressable_content_resident_bytes=" << addressable_content_probe.resident_bytes
        << " addressable_content_resident_budget_bytes=" << addressable_content_probe.resident_budget_bytes
        << " addressable_content_budget_rejection_status="
        << addressable_content_status_name(addressable_content_probe.budget_rejection_status)
        << " addressable_content_budget_rejection_diagnostics="
        << addressable_content_probe.budget_rejection_diagnostics
        << " addressable_content_package_io=" << (addressable_content_probe.package_io ? 1 : 0)
        << " addressable_content_async_execution=" << (addressable_content_probe.async_execution ? 1 : 0)
        << " addressable_content_committed=" << (addressable_content_probe.committed ? 1 : 0)
        << " addressable_content_diagnostics=" << addressable_content_probe.diagnostics
        << " rpg_systems_status=" << rpg_systems_status_name(rpg_systems_probe.status)
        << " rpg_systems_ready=" << (rpg_systems_probe.ready ? 1 : 0)
        << " rpg_systems_party_members=" << rpg_systems_probe.party_members
        << " rpg_systems_enemy_members=" << rpg_systems_probe.enemy_members
        << " rpg_systems_stat_rows=" << rpg_systems_probe.stat_rows
        << " rpg_systems_progression_rows=" << rpg_systems_probe.progression_rows
        << " rpg_systems_skill_rows=" << rpg_systems_probe.skill_rows
        << " rpg_systems_skill_blocked_rows=" << rpg_systems_probe.blocked_skill_rows
        << " rpg_systems_equipment_rows=" << rpg_systems_probe.equipment_rows
        << " rpg_systems_equipment_blocked_rows=" << rpg_systems_probe.blocked_equipment_rows
        << " rpg_systems_combat_turn_rows=" << rpg_systems_probe.combat_turn_rows
        << " rpg_systems_combat_rounds=" << rpg_systems_probe.combat_rounds
        << " rpg_systems_reward_rows=" << rpg_systems_probe.reward_rows
        << " rpg_systems_save_validation_rows=" << rpg_systems_probe.save_validation_rows
        << " rpg_systems_save_validation_repairable_rows=" << rpg_systems_probe.repairable_save_validation_rows
        << " rpg_systems_replay_hash=" << rpg_systems_probe.replay_hash
        << " rpg_systems_invoked_combat_execution=" << (rpg_systems_probe.invoked_combat_execution ? 1 : 0)
        << " rpg_systems_invoked_reward_application=" << (rpg_systems_probe.invoked_reward_application ? 1 : 0)
        << " rpg_systems_invoked_save_io=" << (rpg_systems_probe.invoked_save_io ? 1 : 0)
        << " rpg_systems_diagnostics=" << rpg_systems_probe.diagnostics
        << " sandbox_world_status=" << sandbox_world_status_name(sandbox_world_probe.status)
        << " sandbox_world_ready=" << (sandbox_world_probe.ready ? 1 : 0)
        << " sandbox_world_chunk_rows=" << sandbox_world_probe.chunk_rows
        << " sandbox_world_resident_chunk_rows=" << sandbox_world_probe.resident_chunk_rows
        << " sandbox_world_existing_cell_rows=" << sandbox_world_probe.existing_cell_rows
        << " sandbox_world_placement_intent_rows=" << sandbox_world_probe.placement_intent_rows
        << " sandbox_world_placement_accepted_rows=" << sandbox_world_probe.placement_accepted_rows
        << " sandbox_world_placement_rejected_rows=" << sandbox_world_probe.placement_rejected_rows
        << " sandbox_world_destruction_intent_rows=" << sandbox_world_probe.destruction_intent_rows
        << " sandbox_world_destruction_accepted_rows=" << sandbox_world_probe.destruction_accepted_rows
        << " sandbox_world_destruction_rejected_rows=" << sandbox_world_probe.destruction_rejected_rows
        << " sandbox_world_construction_cost_rows=" << sandbox_world_probe.construction_cost_rows
        << " sandbox_world_mutation_rows=" << sandbox_world_probe.mutation_rows
        << " sandbox_world_persistence_rows=" << sandbox_world_probe.persistence_rows
        << " sandbox_world_persistence_repairable_rows=" << sandbox_world_probe.persistence_repairable_rows
        << " sandbox_world_rejected_unsafe_mutation_rows=" << sandbox_world_probe.rejected_unsafe_mutation_rows
        << " sandbox_world_replay_hash=" << sandbox_world_probe.replay_hash
        << " sandbox_world_invoked_world_mutation=" << (sandbox_world_probe.invoked_world_mutation ? 1 : 0)
        << " sandbox_world_invoked_persistence_io=" << (sandbox_world_probe.invoked_persistence_io ? 1 : 0)
        << " sandbox_world_invoked_package_io=" << (sandbox_world_probe.invoked_package_io ? 1 : 0)
        << " sandbox_world_diagnostics=" << sandbox_world_probe.diagnostics
        << " simulation_management_status=" << simulation_management_status_name(simulation_management_probe.status)
        << " simulation_management_ready=" << (simulation_management_probe.ready ? 1 : 0)
        << " simulation_management_tick_count=" << simulation_management_probe.tick_count
        << " simulation_management_resource_balance_rows=" << simulation_management_probe.resource_balance_rows
        << " simulation_management_job_rows=" << simulation_management_probe.job_rows
        << " simulation_management_job_assignment_rows=" << simulation_management_probe.job_assignment_rows
        << " simulation_management_logistics_links=" << simulation_management_probe.logistics_links
        << " simulation_management_logistics_transfer_rows=" << simulation_management_probe.logistics_transfer_rows
        << " simulation_management_logistics_scheduled_transfer_rows="
        << simulation_management_probe.logistics_scheduled_transfer_rows
        << " simulation_management_economy_summary_rows=" << simulation_management_probe.economy_summary_rows
        << " simulation_management_population_need_rows=" << simulation_management_probe.population_need_rows
        << " simulation_management_need_deficit_rows=" << simulation_management_probe.need_deficit_rows
        << " simulation_management_schedule_rows=" << simulation_management_probe.schedule_rows
        << " simulation_management_save_review_rows=" << simulation_management_probe.save_review_rows
        << " simulation_management_save_review_repairable_rows="
        << simulation_management_probe.save_review_repairable_rows
        << " simulation_management_dashboard_rows=" << simulation_management_probe.dashboard_rows
        << " simulation_management_replay_hash=" << simulation_management_probe.replay_hash
        << " simulation_management_invoked_economy_execution="
        << (simulation_management_probe.invoked_economy_execution ? 1 : 0)
        << " simulation_management_invoked_save_io=" << (simulation_management_probe.invoked_save_io ? 1 : 0)
        << " simulation_management_invoked_runtime_ui=" << (simulation_management_probe.invoked_runtime_ui ? 1 : 0)
        << " simulation_management_invoked_package_io=" << (simulation_management_probe.invoked_package_io ? 1 : 0)
        << " simulation_management_diagnostics=" << simulation_management_probe.diagnostics
        << " network_replication_status=" << network_replication_status_name(network_replication_probe.status)
        << " network_replication_reviewed=" << (network_replication_probe.reviewed ? 1 : 0)
        << " network_replication_ready=" << (network_replication_probe.ready ? 1 : 0)
        << " network_replication_object_rows=" << network_replication_probe.object_rows
        << " network_replication_input_rows=" << network_replication_probe.input_rows
        << " network_replication_snapshot_rows=" << network_replication_probe.snapshot_rows
        << " network_replication_rollback_rows=" << network_replication_probe.rollback_rows
        << " network_replication_rejected_unsafe_rows=" << network_replication_probe.rejected_unsafe_rows
        << " network_replication_replay_hash=" << network_replication_probe.replay_hash
        << " network_replication_requires_transport_host_evidence="
        << (network_replication_probe.requires_transport_host_evidence ? 1 : 0)
        << " network_replication_transport_host_evidence="
        << (network_replication_probe.has_transport_host_evidence ? 1 : 0)
        << " network_replication_invoked_network_io=" << (network_replication_probe.invoked_network_io ? 1 : 0)
        << " network_replication_invoked_rollback_execution="
        << (network_replication_probe.invoked_rollback_execution ? 1 : 0)
        << " network_replication_invoked_world_mutation=" << (network_replication_probe.invoked_world_mutation ? 1 : 0)
        << " network_replication_diagnostics=" << network_replication_probe.diagnostics
        << " rendering_vfx_profiling_status="
        << rendering_vfx_profiling_status_name(rendering_vfx_profiling_probe.status)
        << " rendering_vfx_profiling_reviewed=" << (rendering_vfx_profiling_probe.reviewed ? 1 : 0)
        << " rendering_vfx_profiling_ready=" << (rendering_vfx_profiling_probe.ready ? 1 : 0)
        << " rendering_vfx_profiling_feature_rows=" << rendering_vfx_profiling_probe.feature_rows
        << " rendering_vfx_profiling_gpu_particle_budget_rows="
        << rendering_vfx_profiling_probe.gpu_particle_budget_rows
        << " rendering_vfx_profiling_postprocess_rows=" << rendering_vfx_profiling_probe.postprocess_rows
        << " rendering_vfx_profiling_backend_timing_rows=" << rendering_vfx_profiling_probe.backend_timing_rows
        << " rendering_vfx_profiling_backend_evidence_rows=" << rendering_vfx_profiling_probe.backend_evidence_rows
        << " rendering_vfx_profiling_backend_evidence_ready=" << rendering_vfx_profiling_probe.backend_evidence_ready
        << " rendering_vfx_profiling_backend_evidence_host_gated="
        << rendering_vfx_profiling_probe.backend_evidence_host_gated
        << " rendering_vfx_profiling_cpu_profile_rows=" << rendering_vfx_profiling_probe.cpu_profile_rows
        << " rendering_vfx_profiling_package_counter_rows=" << rendering_vfx_profiling_probe.package_counter_rows
        << " rendering_vfx_profiling_package_counter_ready=" << rendering_vfx_profiling_probe.package_counter_ready
        << " rendering_vfx_profiling_package_counter_host_gated="
        << rendering_vfx_profiling_probe.package_counter_host_gated
        << " rendering_vfx_profiling_crash_telemetry_handoff_rows="
        << rendering_vfx_profiling_probe.crash_telemetry_handoff_rows
        << " rendering_vfx_profiling_host_validated_backends=" << rendering_vfx_profiling_probe.host_validated_backends
        << " rendering_vfx_profiling_rejected_unsafe_rows=" << rendering_vfx_profiling_probe.rejected_unsafe_rows
        << " rendering_vfx_profiling_replay_hash=" << rendering_vfx_profiling_probe.replay_hash
        << " rendering_vfx_profiling_d3d12_host_evidence_ready="
        << (rendering_vfx_profiling_probe.d3d12_host_evidence_ready ? 1 : 0)
        << " rendering_vfx_profiling_vulkan_strict_host_evidence_ready="
        << (rendering_vfx_profiling_probe.vulkan_strict_host_evidence_ready ? 1 : 0)
        << " rendering_vfx_profiling_metal_host_evidence_ready="
        << (rendering_vfx_profiling_probe.metal_host_evidence_ready ? 1 : 0)
        << " rendering_vfx_profiling_requires_metal_host_evidence="
        << (rendering_vfx_profiling_probe.requires_metal_host_evidence ? 1 : 0)
        << " rendering_vfx_profiling_metal_host_evidence="
        << (rendering_vfx_profiling_probe.has_metal_host_evidence ? 1 : 0)
        << " rendering_vfx_profiling_invoked_gpu_commands="
        << (rendering_vfx_profiling_probe.invoked_gpu_commands ? 1 : 0)
        << " rendering_vfx_profiling_invoked_native_capture="
        << (rendering_vfx_profiling_probe.invoked_native_capture ? 1 : 0)
        << " rendering_vfx_profiling_invoked_crash_upload="
        << (rendering_vfx_profiling_probe.invoked_crash_upload ? 1 : 0)
        << " rendering_vfx_profiling_diagnostics=" << rendering_vfx_profiling_probe.diagnostics
        << " gameplay_systems_ticks=" << game.gameplay_systems_ticks()
        << " gameplay_systems_physics_ticks=" << game.gameplay_systems_physics_ticks()
        << " gameplay_systems_authored_collision_bodies=" << game.gameplay_systems_authored_collision_bodies()
        << " gameplay_systems_collision_package_ready=" << (collision_package.ready ? 1 : 0)
        << " gameplay_systems_controller_grounded=" << (game.gameplay_systems_controller_grounded() ? 1 : 0)
        << " gameplay_systems_navigation_path_points=" << game.gameplay_systems_navigation_path_points()
        << " gameplay_systems_navigation_reached=" << (game.gameplay_systems_navigation_reached_destination() ? 1 : 0)
        << " gameplay_systems_navigation_plan_status="
        << navigation_grid_agent_path_status_name(game.gameplay_systems_navigation_plan_status())
        << " gameplay_systems_navigation_plan_diagnostic="
        << navigation_grid_agent_path_diagnostic_name(game.gameplay_systems_navigation_plan_diagnostic())
        << " gameplay_systems_navigation_agent_status="
        << navigation_agent_status_name(game.gameplay_systems_navigation_agent_status())
        << " gameplay_systems_navigation_navmesh_status="
        << navigation_navmesh_path_status_name(game.gameplay_systems_navigation_navmesh_status())
        << " gameplay_systems_navigation_navmesh_diagnostic="
        << navigation_navmesh_path_diagnostic_name(game.gameplay_systems_navigation_navmesh_diagnostic())
        << " gameplay_systems_navigation_navmesh_polygons=" << game.gameplay_systems_navigation_navmesh_polygons()
        << " gameplay_systems_navigation_navmesh_dynamic_obstacles="
        << game.gameplay_systems_navigation_navmesh_dynamic_obstacles()
        << " gameplay_systems_navigation_navmesh_total_cost=" << game.gameplay_systems_navigation_navmesh_total_cost()
        << " gameplay_systems_navigation_navmesh_readiness_status="
        << navigation_navmesh_readiness_status_name(game.gameplay_systems_navigation_navmesh_readiness_status())
        << " gameplay_systems_navigation_navmesh_readiness_diagnostic="
        << navigation_navmesh_readiness_diagnostic_name(game.gameplay_systems_navigation_navmesh_readiness_diagnostic())
        << " gameplay_systems_navigation_navmesh_readiness_diagnostics="
        << game.gameplay_systems_navigation_navmesh_readiness_diagnostics()
        << " gameplay_systems_navigation_navmesh_scene_refs=" << game.gameplay_systems_navigation_navmesh_scene_refs()
        << " gameplay_systems_navigation_navmesh_points=" << game.gameplay_systems_navigation_navmesh_points()
        << " gameplay_systems_navigation_navmesh_visited_polygons="
        << game.gameplay_systems_navigation_navmesh_visited_polygons() << " gameplay_systems_navigation_crowd_status="
        << navigation_crowd_plan_status_name(game.gameplay_systems_navigation_crowd_status())
        << " gameplay_systems_navigation_crowd_diagnostic="
        << navigation_crowd_plan_diagnostic_name(game.gameplay_systems_navigation_crowd_diagnostic())
        << " gameplay_systems_navigation_crowd_rows=" << game.gameplay_systems_navigation_crowd_rows()
        << " gameplay_systems_navigation_crowd_source_order_ready="
        << (game.gameplay_systems_navigation_crowd_source_order_ready() ? 1 : 0)
        << " gameplay_systems_navigation_crowd_agents=" << game.gameplay_systems_navigation_crowd_agents()
        << " gameplay_systems_navigation_crowd_route_successes="
        << game.gameplay_systems_navigation_crowd_route_successes()
        << " gameplay_systems_navigation_crowd_avoidance_successes="
        << game.gameplay_systems_navigation_crowd_avoidance_successes()
        << " gameplay_systems_navigation_crowd_applied_neighbors="
        << game.gameplay_systems_navigation_crowd_applied_neighbors()
        << " gameplay_systems_navigation_crowd_dynamic_obstacles="
        << game.gameplay_systems_navigation_crowd_dynamic_obstacles()
        << " gameplay_systems_navigation_crowd_readiness_status="
        << navigation_crowd_readiness_status_name(game.gameplay_systems_navigation_crowd_readiness_status())
        << " gameplay_systems_navigation_crowd_readiness_diagnostic="
        << navigation_crowd_readiness_diagnostic_name(game.gameplay_systems_navigation_crowd_readiness_diagnostic())
        << " gameplay_systems_navigation_crowd_readiness_diagnostics="
        << game.gameplay_systems_navigation_crowd_readiness_diagnostics()
        << " gameplay_systems_navigation_crowd_readiness_rows="
        << game.gameplay_systems_navigation_crowd_readiness_rows()
        << " gameplay_systems_navigation_crowd_readiness_source_order_ready="
        << (game.gameplay_systems_navigation_crowd_readiness_source_order_ready() ? 1 : 0)
        << " gameplay_systems_navigation_crowd_readiness_applied_neighbors="
        << game.gameplay_systems_navigation_crowd_readiness_applied_neighbors()
        << " gameplay_systems_navigation_crowd_readiness_dynamic_obstacles="
        << game.gameplay_systems_navigation_crowd_readiness_dynamic_obstacles()
        << " gameplay_systems_local_avoidance_status="
        << navigation_local_avoidance_status_name(game.gameplay_systems_local_avoidance_status())
        << " gameplay_systems_local_avoidance_diagnostic="
        << navigation_local_avoidance_diagnostic_name(game.gameplay_systems_local_avoidance_diagnostic())
        << " gameplay_systems_local_avoidance_steps=" << game.gameplay_systems_local_avoidance_steps()
        << " gameplay_systems_local_avoidance_applied_neighbors="
        << game.gameplay_systems_local_avoidance_applied_neighbors() << " gameplay_systems_physics_policy_status="
        << physics_character_dynamic_policy_status_name(game.gameplay_systems_physics_policy_status())
        << " gameplay_systems_physics_policy_diagnostic="
        << physics_character_dynamic_policy_diagnostic_name(game.gameplay_systems_physics_policy_diagnostic())
        << " gameplay_systems_physics_policy_rows=" << game.gameplay_systems_physics_policy_rows()
        << " gameplay_systems_physics_policy_dynamic_pushes=" << game.gameplay_systems_physics_policy_dynamic_pushes()
        << " gameplay_systems_physics_policy_solid_contacts=" << game.gameplay_systems_physics_policy_solid_contacts()
        << " gameplay_systems_physics_policy_trigger_overlaps="
        << game.gameplay_systems_physics_policy_trigger_overlaps() << " gameplay_systems_advanced_controller_status="
        << physics_advanced_controller_status_name(game.gameplay_systems_advanced_controller_status())
        << " gameplay_systems_advanced_controller_diagnostic="
        << physics_advanced_controller_diagnostic_name(game.gameplay_systems_advanced_controller_diagnostic())
        << " gameplay_systems_advanced_controller_movement_rows="
        << game.gameplay_systems_advanced_controller_movement_rows()
        << " gameplay_systems_advanced_controller_platform_rows="
        << game.gameplay_systems_advanced_controller_platform_rows()
        << " gameplay_systems_advanced_controller_platform_applied="
        << game.gameplay_systems_advanced_controller_platform_applied()
        << " gameplay_systems_advanced_controller_constraint_rows="
        << game.gameplay_systems_advanced_controller_constraint_rows()
        << " gameplay_systems_advanced_controller_replay_changed="
        << (game.gameplay_systems_advanced_controller_replay_changed() ? 1 : 0)
        << " gameplay_systems_advanced_controller_replay_before_bodies="
        << game.gameplay_systems_advanced_controller_replay_before_bodies()
        << " gameplay_systems_advanced_controller_replay_after_bodies="
        << game.gameplay_systems_advanced_controller_replay_after_bodies()
        << " gameplay_systems_advanced_controller_final_x=" << game.gameplay_systems_advanced_controller_final_x()
        << " gameplay_systems_character_dynamics_status="
        << physics_character_dynamics_readiness_status_name(game.gameplay_systems_character_dynamics_status())
        << " gameplay_systems_character_dynamics_diagnostic="
        << physics_character_dynamics_readiness_diagnostic_name(game.gameplay_systems_character_dynamics_diagnostic())
        << " gameplay_systems_character_dynamics_movement_rows="
        << game.gameplay_systems_character_dynamics_movement_rows()
        << " gameplay_systems_character_dynamics_dynamic_pushes="
        << game.gameplay_systems_character_dynamics_dynamic_pushes()
        << " gameplay_systems_character_dynamics_step_ups=" << game.gameplay_systems_character_dynamics_step_ups()
        << " gameplay_systems_character_dynamics_walkable_slope_rows="
        << game.gameplay_systems_character_dynamics_walkable_slope_rows()
        << " gameplay_systems_character_dynamics_ground_probes="
        << game.gameplay_systems_character_dynamics_ground_probes()
        << " gameplay_systems_character_dynamics_moving_platform_rows="
        << game.gameplay_systems_character_dynamics_moving_platform_rows()
        << " gameplay_systems_character_dynamics_applied_platform_rows="
        << game.gameplay_systems_character_dynamics_applied_platform_rows()
        << " gameplay_systems_character_dynamics_constraint_rows="
        << game.gameplay_systems_character_dynamics_constraint_rows()
        << " gameplay_systems_character_dynamics_replay_changed="
        << (game.gameplay_systems_character_dynamics_replay_changed() ? 1 : 0)
        << " gameplay_systems_character_dynamics_diagnostics=" << game.gameplay_systems_character_dynamics_diagnostics()
        << " gameplay_systems_physics_constraints_status="
        << physics_constraint_status_name(game.gameplay_systems_physics_constraints_status())
        << " gameplay_systems_physics_constraints_diagnostic="
        << physics_constraint_diagnostic_name(game.gameplay_systems_physics_constraints_diagnostic())
        << " gameplay_systems_physics_constraints_rows=" << game.gameplay_systems_physics_constraints_rows()
        << " gameplay_systems_physics_constraints_fixed_rows=" << game.gameplay_systems_physics_constraints_fixed_rows()
        << " gameplay_systems_physics_constraints_linear_axis_rows="
        << game.gameplay_systems_physics_constraints_linear_axis_rows()
        << " gameplay_systems_physics_constraints_axis_limit_clamped="
        << game.gameplay_systems_physics_constraints_axis_limit_clamped()
        << " gameplay_systems_kinematic_motion_status="
        << physics_kinematic_motion_status_name(game.gameplay_systems_kinematic_motion_status())
        << " gameplay_systems_kinematic_motion_diagnostic="
        << physics_kinematic_motion_diagnostic_name(game.gameplay_systems_kinematic_motion_diagnostic())
        << " gameplay_systems_kinematic_motion_rows=" << game.gameplay_systems_kinematic_motion_rows()
        << " gameplay_systems_kinematic_motion_grounded=" << (game.gameplay_systems_kinematic_motion_grounded() ? 1 : 0)
        << " gameplay_systems_vehicle_status="
        << physics_simple_vehicle_status_name(game.gameplay_systems_vehicle_status())
        << " gameplay_systems_vehicle_diagnostic="
        << physics_simple_vehicle_diagnostic_name(game.gameplay_systems_vehicle_diagnostic())
        << " gameplay_systems_vehicle_wheel_rows=" << game.gameplay_systems_vehicle_wheel_rows()
        << " gameplay_systems_vehicle_grounded_wheels=" << game.gameplay_systems_vehicle_grounded_wheels()
        << " gameplay_systems_vehicle_wheel_probe_hits=" << game.gameplay_systems_vehicle_wheel_probe_hits()
        << " gameplay_systems_vehicle_final_x=" << game.gameplay_systems_vehicle_final_x()
        << " gameplay_systems_navigation_goal_x=" << game.gameplay_systems_navigation_goal_x()
        << " gameplay_systems_navigation_goal_y=" << game.gameplay_systems_navigation_goal_y()
        << " gameplay_systems_navigation_final_x=" << game.gameplay_systems_navigation_final_x()
        << " gameplay_systems_navigation_final_y=" << game.gameplay_systems_navigation_final_y()
        << " gameplay_systems_perception_status="
        << ai_perception_status_name(game.gameplay_systems_perception_status())
        << " gameplay_systems_perception_targets=" << game.gameplay_systems_perception_targets()
        << " gameplay_systems_perception_has_primary_target="
        << (game.gameplay_systems_perception_has_primary_target() ? 1 : 0)
        << " gameplay_systems_perception_primary_target_id=" << game.gameplay_systems_perception_primary_target_id()
        << " gameplay_systems_perception_primary_target_distance="
        << game.gameplay_systems_perception_primary_target_distance()
        << " gameplay_systems_perception_visible_count=" << game.gameplay_systems_perception_visible_count()
        << " gameplay_systems_perception_audible_count=" << game.gameplay_systems_perception_audible_count()
        << " gameplay_systems_blackboard_status="
        << ai_perception_blackboard_status_name(game.gameplay_systems_blackboard_status())
        << " gameplay_systems_blackboard_has_target=" << (game.gameplay_systems_blackboard_has_target() ? 1 : 0)
        << " gameplay_systems_blackboard_needs_move=" << (game.gameplay_systems_blackboard_needs_move() ? 1 : 0)
        << " gameplay_systems_blackboard_target_id=" << game.gameplay_systems_blackboard_target_id()
        << " gameplay_systems_blackboard_visible_count=" << game.gameplay_systems_blackboard_visible_count()
        << " gameplay_systems_behavior_status=" << behavior_tree_status_name(game.gameplay_systems_behavior_status())
        << " gameplay_systems_behavior_nodes=" << game.gameplay_systems_behavior_nodes()
        << " gameplay_systems_audio_voice_started=" << (game.gameplay_systems_audio_voice_started() ? 1 : 0)
        << " gameplay_systems_audio_status=" << audio_device_stream_status_name(game.gameplay_systems_audio_status())
        << " gameplay_systems_audio_diagnostic="
        << audio_device_stream_diagnostic_name(game.gameplay_systems_audio_diagnostic())
        << " gameplay_systems_audio_commands=" << game.gameplay_systems_audio_commands()
        << " gameplay_systems_audio_frames=" << game.gameplay_systems_audio_frames()
        << " gameplay_systems_audio_samples=" << game.gameplay_systems_audio_samples()
        << " gameplay_systems_audio_first_sample=" << game.gameplay_systems_audio_first_sample()
        << " gameplay_systems_audio_second_sample=" << game.gameplay_systems_audio_second_sample()
        << " gameplay_systems_audio_abs_sum=" << game.gameplay_systems_audio_abs_sum()
        << " audio_gameplay_mixer_ready=" << (audio_gameplay_mixer.ready ? 1 : 0)
        << " audio_gameplay_mixer_diagnostics=" << audio_gameplay_mixer.diagnostics
        << " audio_gameplay_mixer_buses=" << audio_gameplay_mixer.buses
        << " audio_gameplay_mixer_cues=" << audio_gameplay_mixer.cues
        << " audio_gameplay_mixer_triggers=" << audio_gameplay_mixer.triggers
        << " audio_gameplay_mixer_commands=" << audio_gameplay_mixer.commands
        << " audio_gameplay_mixer_paused_buses=" << audio_gameplay_mixer.paused_buses
        << " audio_gameplay_mixer_faded_buses=" << audio_gameplay_mixer.faded_buses
        << " audio_gameplay_mixer_looping_commands=" << audio_gameplay_mixer.looping_commands
        << " audio_gameplay_mixer_spatial_commands=" << audio_gameplay_mixer.spatial_commands
        << " audio_gameplay_mixer_render_commands=" << audio_gameplay_mixer.render_commands
        << " audio_gameplay_mixer_render_frames=" << audio_gameplay_mixer.render_frames
        << " audio_gameplay_mixer_render_samples=" << audio_gameplay_mixer.render_samples
        << " audio_gameplay_mixer_sample_abs_sum=" << audio_gameplay_mixer.sample_abs_sum
        << " audio_gameplay_mixer_payload_diagnostics=" << audio_gameplay_mixer.payload_diagnostics
        << " audio_production_status=" << audio_production_status_name(audio_production_probe.status)
        << " audio_production_reviewed=" << (audio_production_probe.reviewed ? 1 : 0)
        << " audio_production_ready=" << (audio_production_probe.production_audio_ready ? 1 : 0)
        << " audio_production_selected_package_ready="
        << (audio_production_probe.selected_package_evidence_ready ? 1 : 0)
        << " audio_production_package_evidence_ready=" << (audio_production_probe.package_evidence_ready ? 1 : 0)
        << " audio_production_decoded_source_rows=" << audio_production_probe.decoded_source_rows
        << " audio_production_streaming_chunk_rows=" << audio_production_probe.streaming_chunk_rows
        << " audio_production_format_conversion_policy_rows=" << audio_production_probe.format_conversion_policy_rows
        << " audio_production_bus_budget_rows=" << audio_production_probe.bus_budget_rows
        << " audio_production_voice_budget_rows=" << audio_production_probe.voice_budget_rows
        << " audio_production_dsp_graph_rows=" << audio_production_probe.dsp_graph_rows
        << " audio_production_listener_rows=" << audio_production_probe.listener_rows
        << " audio_production_spatial_source_rows=" << audio_production_probe.spatial_source_rows
        << " audio_production_hrtf_host_gate_rows=" << audio_production_probe.hrtf_host_gate_rows
        << " audio_production_device_lifecycle_rows=" << audio_production_probe.device_lifecycle_rows
        << " audio_production_device_host_evidence=" << (audio_production_probe.device_host_evidence_available ? 1 : 0)
        << " audio_production_hrtf_host_evidence=" << (audio_production_probe.hrtf_host_evidence_available ? 1 : 0)
        << " audio_production_unsupported_claim_rows=" << audio_production_probe.unsupported_claim_rows
        << " audio_production_invoked_codec_decode=" << (audio_production_probe.invoked_codec_decode ? 1 : 0)
        << " audio_production_invoked_background_streaming="
        << (audio_production_probe.invoked_background_streaming ? 1 : 0)
        << " audio_production_invoked_middleware=" << (audio_production_probe.invoked_middleware ? 1 : 0)
        << " audio_production_invoked_hrtf=" << (audio_production_probe.invoked_hrtf ? 1 : 0)
        << " audio_production_invoked_device_callback=" << (audio_production_probe.invoked_device_callback ? 1 : 0)
        << " audio_production_invoked_device_io=" << (audio_production_probe.invoked_device_io ? 1 : 0)
        << " audio_production_diagnostics=" << audio_production_probe.diagnostics
        << " audio_production_replay_hash=" << audio_production_probe.replay_hash
        << " gameplay_systems_interaction_ready=" << (game.gameplay_systems_interaction_ready() ? 1 : 0)
        << " gameplay_systems_interaction_diagnostics=" << game.gameplay_systems_interaction_diagnostics()
        << " gameplay_systems_interaction_rows=" << game.gameplay_systems_interaction_rows()
        << " gameplay_systems_interaction_feedback_rows=" << game.gameplay_systems_interaction_feedback_rows()
        << " gameplay_systems_interaction_final_session_state="
        << runtime_gameplay_session_state_name(game.gameplay_systems_interaction_final_session_state())
        << " gameplay_systems_scene_binding_ready=" << (game.gameplay_systems_scene_binding_ready() ? 1 : 0)
        << " gameplay_systems_scene_binding_source_rows=" << game.gameplay_systems_scene_binding_source_rows()
        << " gameplay_systems_scene_binding_rows=" << game.gameplay_systems_scene_binding_rows()
        << " gameplay_systems_scene_binding_systems=" << game.gameplay_systems_scene_binding_systems()
        << " gameplay_systems_scene_binding_component_rows=" << game.gameplay_systems_scene_binding_component_rows()
        << " gameplay_systems_scene_binding_diagnostics=" << game.gameplay_systems_scene_binding_diagnostics()
        << " gameplay_systems_scene_interaction_rows=" << game.gameplay_systems_scene_interaction_rows()
        << " gameplay_systems_scene_interaction_diagnostics=" << game.gameplay_systems_scene_interaction_diagnostics()
        << " gameplay_systems_scene_interaction_final_session_state="
        << runtime_scene_gameplay_session_state_name(game.gameplay_systems_scene_interaction_final_session_state())
        << " input_context_rebinding_ready=" << (input_context_rebinding.ready ? 1 : 0)
        << " input_context_rebinding_layers=" << input_context_rebinding.requested_layers
        << " input_context_rebinding_active_contexts=" << input_context_rebinding.active_contexts
        << " input_context_rebinding_capture_active=" << (input_context_rebinding.capture_context_active ? 1 : 0)
        << " input_context_rebinding_gameplay_consumed=" << (input_context_rebinding.gameplay_input_consumed ? 1 : 0)
        << " input_rebinding_profile_overlays_applied=" << input_context_rebinding.profile_overlays_applied
        << " input_rebinding_action_capture_status="
        << runtime_input_rebinding_capture_status_name(input_context_rebinding.action_capture_status)
        << " input_rebinding_axis_capture_status="
        << runtime_input_rebinding_capture_status_name(input_context_rebinding.axis_capture_status)
        << " input_rebinding_focus_consumed=" << (input_context_rebinding.focus_gameplay_consumed ? 1 : 0)
        << " input_rebinding_focus_retained=" << (input_context_rebinding.focus_retained ? 1 : 0)
        << " input_rebinding_presentation_rows=" << input_context_rebinding.presentation_rows
        << " input_rebinding_glyph_lookup_keys=" << input_context_rebinding.glyph_lookup_keys
        << " input_rebinding_diagnostics=" << input_context_rebinding.diagnostics << " runtime_profile_resume_status="
        << runtime_session_profile_resume_status_name(game.gameplay_systems_runtime_profile_resume_status())
        << " runtime_profile_resume_ready=" << (game.gameplay_systems_runtime_profile_resume_ready() ? 1 : 0)
        << " runtime_profile_resume_diagnostics=" << game.gameplay_systems_runtime_profile_resume_diagnostics()
        << " runtime_profile_resume_loaded_documents="
        << game.gameplay_systems_runtime_profile_resume_loaded_documents()
        << " runtime_profile_resume_defaulted_documents="
        << game.gameplay_systems_runtime_profile_resume_defaulted_documents()
        << " runtime_profile_resume_save_schema_version="
        << game.gameplay_systems_runtime_profile_resume_save_schema_version()
        << " runtime_profile_resume_settings_schema_version="
        << game.gameplay_systems_runtime_profile_resume_settings_schema_version()
        << " runtime_menu_hud_ready=" << (game.gameplay_systems_runtime_menu_hud_ready() ? 1 : 0)
        << " runtime_menu_hud_diagnostics=" << game.gameplay_systems_runtime_menu_hud_diagnostics()
        << " runtime_menu_hud_display_rows=" << game.gameplay_systems_runtime_menu_hud_display_rows()
        << " runtime_menu_hud_command_rows=" << game.gameplay_systems_runtime_menu_hud_command_rows()
        << " runtime_menu_hud_dialogue_rows=" << game.gameplay_systems_runtime_menu_hud_dialogue_rows()
        << " runtime_menu_hud_input_binding_prompt_rows="
        << game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows()
        << " gameplay_systems_animation_state=" << game.gameplay_systems_animation_state()
        << " gameplay_systems_final_actor_x=" << game.gameplay_systems_final_actor_x()
        << " hud_boxes=" << game.hud_boxes_submitted() << " hud_images=" << game.hud_images_submitted()
        << " hud_text_glyphs=" << game.hud_text_glyph_sprites_submitted()
        << " text_glyphs_available=" << game.hud_text_glyphs_available()
        << " text_glyphs_resolved=" << game.hud_text_glyphs_resolved()
        << " text_glyphs_missing=" << game.hud_text_glyphs_missing()
        << " text_glyph_sprites_submitted=" << game.hud_text_glyph_sprites_submitted()
        << " frames=" << result.frames_run << " game_frames=" << game.frames()
        << " scene_meshes=" << game.scene_meshes_submitted() << " scene_materials=" << game.scene_materials_resolved()
        << " scene_mesh_plan_meshes=" << game.scene_mesh_plan_meshes()
        << " scene_mesh_plan_draws=" << game.scene_mesh_plan_draws()
        << " scene_mesh_plan_unique_meshes=" << game.scene_mesh_plan_unique_meshes()
        << " scene_mesh_plan_unique_materials=" << game.scene_mesh_plan_unique_materials()
        << " scene_mesh_plan_diagnostics=" << game.scene_mesh_plan_diagnostics()
        << " camera_primary=" << (game.primary_camera_controller_passed(options.max_frames) ? 1 : 0)
        << " camera_controller_ticks=" << game.camera_controller_ticks() << " final_camera_x=" << game.final_camera_x()
        << " transform_animation=" << (game.transform_animation_passed(options.max_frames) ? 1 : 0)
        << " transform_animation_ticks=" << game.transform_animation_ticks()
        << " transform_animation_samples=" << game.transform_animation_samples()
        << " transform_animation_applied=" << game.transform_animation_applied()
        << " transform_animation_failures=" << game.transform_animation_failures()
        << " final_mesh_x=" << game.final_mesh_x()
        << " morph_package=" << (game.morph_package_passed(options.max_frames) ? 1 : 0)
        << " morph_package_ticks=" << game.morph_package_ticks()
        << " morph_package_samples=" << game.morph_package_samples()
        << " morph_package_weights=" << game.morph_package_weights()
        << " morph_package_vertices=" << game.morph_package_vertices()
        << " morph_package_failures=" << game.morph_package_failures()
        << " morph_first_position_x=" << game.morph_first_position_x()
        << " quaternion_animation=" << (game.quaternion_animation_passed(options.max_frames) ? 1 : 0)
        << " quaternion_animation_ticks=" << game.quaternion_animation_ticks()
        << " quaternion_animation_tracks=" << game.quaternion_animation_tracks_sampled()
        << " quaternion_animation_failures=" << game.quaternion_animation_failures()
        << " quaternion_animation_scene_applied=" << game.quaternion_animation_scene_applied()
        << " quaternion_animation_final_z=" << game.final_quaternion_z()
        << " quaternion_animation_final_w=" << game.final_quaternion_w()
        << " quaternion_animation_scene_rotation_z=" << game.final_quaternion_scene_rotation_z() << '\n';
    print_presentation_report("sample_generated_desktop_runtime_3d_package", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_generated_desktop_runtime_3d_package presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
        std::cout << "sample_generated_desktop_runtime_3d_package native_ui_overlay_diagnostic="
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
        std::cout << "sample_generated_desktop_runtime_3d_package native_ui_texture_overlay_diagnostic="
                  << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    print_package_streaming_diagnostics(package_streaming_result);
    for (const auto& diagnostic : package_upload_staging.diagnostics) {
        std::cout << "sample_generated_desktop_runtime_3d_package package_upload_staging_diagnostic=" << diagnostic.code
                  << ": " << diagnostic.message << '\n';
    }
    print_ui_atlas_metadata_diagnostics("sample_generated_desktop_runtime_3d_package", ui_atlas_metadata);
    print_ui_atlas_metadata_diagnostics("sample_generated_desktop_runtime_3d_package", ui_text_glyph_atlas_metadata);

    if (options.smoke) {
        if (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
            game.frames() != options.max_frames) {
            return 3;
        }
        if (!options.required_scene_package_path.empty() &&
            (game.scene_meshes_submitted() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_materials_resolved() != static_cast<std::size_t>(options.max_frames) ||
             game.scene_mesh_plan_meshes() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_draws() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_unique_meshes() != static_cast<std::uint64_t>(options.max_frames) ||
             game.scene_mesh_plan_unique_materials() != static_cast<std::uint64_t>(options.max_frames) ||
             !game.scene_mesh_plan_succeeded() || game.scene_mesh_plan_diagnostics() != 0)) {
            return 3;
        }
        if (options.require_primary_camera_controller && !game.primary_camera_controller_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_transform_animation && !game.transform_animation_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_morph_package && !game.morph_package_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_quaternion_animation && !game.quaternion_animation_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_package_streaming_safe_point &&
            (!package_streaming_result.succeeded() || package_streaming_result.estimated_resident_bytes == 0 ||
             package_streaming_result.replacement.committed_record_count == 0 ||
             package_streaming_result.replacement.committed_record_count != runtime_package->records().size() ||
             package_streaming_result.required_preload_asset_count != 1 ||
             package_streaming_result.resident_resource_kind_count != 11 ||
             package_streaming_result.resident_package_count != 1 || !package_streaming_result.diagnostics.empty())) {
            return 3;
        }
        if (options.require_package_upload_staging && !package_upload_staging.ready) {
            return 3;
        }
        if (options.require_renderer_quality_gates && !renderer_quality.ready) {
            return 3;
        }
        if (options.require_playable_3d_slice && !playable_3d.ready) {
            return 3;
        }
        if ((options.require_visible_3d_production_proof || options.require_vulkan_visible_3d_production_proof) &&
            !visible_3d.ready) {
            return 3;
        }
        if (options.require_entity_scale_culling && !entity_scale_culling_probe.ready) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_entity_scale_culling_unavailable"
                      << " entity_scale_culling_status="
                      << entity_scale_culling_status_name(entity_scale_culling_probe.status)
                      << " entity_scale_culling_rows=" << entity_scale_culling_probe.rows
                      << " entity_scale_culling_visible_rows=" << entity_scale_culling_probe.visible_rows
                      << " entity_scale_culling_culled_rows=" << entity_scale_culling_probe.culled_rows
                      << " entity_scale_culling_lod_rows=" << entity_scale_culling_probe.lod_rows
                      << " entity_scale_culling_projected_draw_cost=" << entity_scale_culling_probe.projected_draw_cost
                      << " entity_scale_culling_projected_update_cost="
                      << entity_scale_culling_probe.projected_update_cost
                      << " entity_scale_culling_budget_diagnostics=" << entity_scale_culling_probe.budget_diagnostics
                      << '\n';
            return 15;
        }
        if (options.require_runtime_profile_resume &&
            (!game.gameplay_systems_runtime_profile_resume_ready() ||
             game.gameplay_systems_runtime_profile_resume_loaded_documents() != 3U ||
             game.gameplay_systems_runtime_profile_resume_defaulted_documents() != 0U ||
             game.gameplay_systems_runtime_profile_resume_save_schema_version() != 3U ||
             game.gameplay_systems_runtime_profile_resume_settings_schema_version() != 2U)) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_runtime_profile_resume_unavailable"
                      << " runtime_profile_resume_status="
                      << runtime_session_profile_resume_status_name(
                             game.gameplay_systems_runtime_profile_resume_status())
                      << " runtime_profile_resume_diagnostics="
                      << game.gameplay_systems_runtime_profile_resume_diagnostics()
                      << " runtime_profile_resume_loaded_documents="
                      << game.gameplay_systems_runtime_profile_resume_loaded_documents()
                      << " runtime_profile_resume_defaulted_documents="
                      << game.gameplay_systems_runtime_profile_resume_defaulted_documents()
                      << " runtime_profile_resume_save_schema_version="
                      << game.gameplay_systems_runtime_profile_resume_save_schema_version()
                      << " runtime_profile_resume_settings_schema_version="
                      << game.gameplay_systems_runtime_profile_resume_settings_schema_version() << '\n';
            return 16;
        }
        if (options.require_runtime_menu_hud &&
            (!game.gameplay_systems_runtime_menu_hud_ready() ||
             game.gameplay_systems_runtime_menu_hud_diagnostics() != 0U ||
             game.gameplay_systems_runtime_menu_hud_display_rows() != 6U ||
             game.gameplay_systems_runtime_menu_hud_command_rows() != 2U ||
             game.gameplay_systems_runtime_menu_hud_dialogue_rows() != 1U ||
             game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() != 1U)) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_runtime_menu_hud_unavailable"
                      << " runtime_menu_hud_ready=" << (game.gameplay_systems_runtime_menu_hud_ready() ? 1 : 0)
                      << " runtime_menu_hud_diagnostics=" << game.gameplay_systems_runtime_menu_hud_diagnostics()
                      << " runtime_menu_hud_display_rows=" << game.gameplay_systems_runtime_menu_hud_display_rows()
                      << " runtime_menu_hud_command_rows=" << game.gameplay_systems_runtime_menu_hud_command_rows()
                      << " runtime_menu_hud_dialogue_rows=" << game.gameplay_systems_runtime_menu_hud_dialogue_rows()
                      << " runtime_menu_hud_input_binding_prompt_rows="
                      << game.gameplay_systems_runtime_menu_hud_input_binding_prompt_rows() << '\n';
            return 17;
        }
        if (options.require_audio_gameplay_mixer && !audio_gameplay_mixer.ready) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_audio_gameplay_mixer_unavailable"
                      << " audio_gameplay_mixer_ready=" << (audio_gameplay_mixer.ready ? 1 : 0)
                      << " audio_gameplay_mixer_diagnostics=" << audio_gameplay_mixer.diagnostics
                      << " audio_gameplay_mixer_buses=" << audio_gameplay_mixer.buses
                      << " audio_gameplay_mixer_cues=" << audio_gameplay_mixer.cues
                      << " audio_gameplay_mixer_triggers=" << audio_gameplay_mixer.triggers
                      << " audio_gameplay_mixer_commands=" << audio_gameplay_mixer.commands
                      << " audio_gameplay_mixer_paused_buses=" << audio_gameplay_mixer.paused_buses
                      << " audio_gameplay_mixer_faded_buses=" << audio_gameplay_mixer.faded_buses
                      << " audio_gameplay_mixer_looping_commands=" << audio_gameplay_mixer.looping_commands
                      << " audio_gameplay_mixer_spatial_commands=" << audio_gameplay_mixer.spatial_commands
                      << " audio_gameplay_mixer_render_commands=" << audio_gameplay_mixer.render_commands
                      << " audio_gameplay_mixer_render_frames=" << audio_gameplay_mixer.render_frames
                      << " audio_gameplay_mixer_render_samples=" << audio_gameplay_mixer.render_samples
                      << " audio_gameplay_mixer_sample_abs_sum=" << audio_gameplay_mixer.sample_abs_sum
                      << " audio_gameplay_mixer_payload_diagnostics=" << audio_gameplay_mixer.payload_diagnostics
                      << '\n';
            return 18;
        }
        if (options.require_audio_production && !audio_production_probe.package_evidence_ready) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_audio_production_unavailable"
                      << " audio_production_status=" << audio_production_status_name(audio_production_probe.status)
                      << " audio_production_reviewed=" << (audio_production_probe.reviewed ? 1 : 0)
                      << " audio_production_ready=" << (audio_production_probe.production_audio_ready ? 1 : 0)
                      << " audio_production_selected_package_ready="
                      << (audio_production_probe.selected_package_evidence_ready ? 1 : 0)
                      << " audio_production_package_evidence_ready="
                      << (audio_production_probe.package_evidence_ready ? 1 : 0)
                      << " audio_production_decoded_source_rows=" << audio_production_probe.decoded_source_rows
                      << " audio_production_streaming_chunk_rows=" << audio_production_probe.streaming_chunk_rows
                      << " audio_production_format_conversion_policy_rows="
                      << audio_production_probe.format_conversion_policy_rows
                      << " audio_production_bus_budget_rows=" << audio_production_probe.bus_budget_rows
                      << " audio_production_voice_budget_rows=" << audio_production_probe.voice_budget_rows
                      << " audio_production_dsp_graph_rows=" << audio_production_probe.dsp_graph_rows
                      << " audio_production_listener_rows=" << audio_production_probe.listener_rows
                      << " audio_production_spatial_source_rows=" << audio_production_probe.spatial_source_rows
                      << " audio_production_hrtf_host_gate_rows=" << audio_production_probe.hrtf_host_gate_rows
                      << " audio_production_device_lifecycle_rows=" << audio_production_probe.device_lifecycle_rows
                      << " audio_production_device_host_evidence="
                      << (audio_production_probe.device_host_evidence_available ? 1 : 0)
                      << " audio_production_hrtf_host_evidence="
                      << (audio_production_probe.hrtf_host_evidence_available ? 1 : 0)
                      << " audio_production_unsupported_claim_rows=" << audio_production_probe.unsupported_claim_rows
                      << " audio_production_invoked_codec_decode="
                      << (audio_production_probe.invoked_codec_decode ? 1 : 0)
                      << " audio_production_invoked_background_streaming="
                      << (audio_production_probe.invoked_background_streaming ? 1 : 0)
                      << " audio_production_invoked_middleware=" << (audio_production_probe.invoked_middleware ? 1 : 0)
                      << " audio_production_invoked_hrtf=" << (audio_production_probe.invoked_hrtf ? 1 : 0)
                      << " audio_production_invoked_device_callback="
                      << (audio_production_probe.invoked_device_callback ? 1 : 0)
                      << " audio_production_invoked_device_io=" << (audio_production_probe.invoked_device_io ? 1 : 0)
                      << " audio_production_diagnostics=" << audio_production_probe.diagnostics
                      << " audio_production_replay_hash=" << audio_production_probe.replay_hash << '\n';
            return 22;
        }
        if (options.require_gameplay_systems && !gameplay_systems_ready) {
            std::cout
                << "sample_generated_desktop_runtime_3d_package required_gameplay_systems_unavailable"
                << " gameplay_systems_status=" << gameplay_systems_status_name(gameplay_systems_status)
                << " gameplay_systems_ready=" << (gameplay_systems_ready ? 1 : 0)
                << " gameplay_systems_diagnostics=" << gameplay_systems_diagnostics
                << " gameplay_runtime_scheduler_status="
                << gameplay_runtime_scheduler_status_name(gameplay_runtime_scheduler_probe.status)
                << " gameplay_runtime_scheduler_ready=" << (gameplay_runtime_scheduler_probe.ready ? 1 : 0)
                << " gameplay_runtime_scheduler_steps=" << gameplay_runtime_scheduler_probe.step_rows
                << " gameplay_runtime_scheduler_system_rows=" << gameplay_runtime_scheduler_probe.system_rows
                << " gameplay_runtime_scheduler_command_rows=" << gameplay_runtime_scheduler_probe.command_rows
                << " gameplay_runtime_scheduler_budget_limited="
                << (gameplay_runtime_scheduler_probe.budget_limited ? 1 : 0)
                << " gameplay_runtime_scheduler_diagnostics=" << gameplay_runtime_scheduler_probe.diagnostics
                << " world_entity_model_status=" << world_entity_model_status_name(world_entity_model_probe.status)
                << " world_entity_model_ready=" << (world_entity_model_probe.ready ? 1 : 0)
                << " world_entity_model_entities=" << world_entity_model_probe.entity_rows
                << " world_entity_model_components=" << world_entity_model_probe.component_rows
                << " world_entity_model_region_ownership_rows=" << world_entity_model_probe.region_ownership_rows
                << " world_entity_model_lifecycle_rows=" << world_entity_model_probe.lifecycle_rows
                << " world_entity_model_persistence_rows=" << world_entity_model_probe.persistence_rows
                << " world_entity_model_streaming_region_rows=" << world_entity_model_probe.streaming_region_rows
                << " world_entity_model_duplicate_entity_diagnostics="
                << world_entity_model_probe.duplicate_entity_diagnostics
                << " world_entity_model_bridge_rejection_status="
                << world_entity_model_status_name(world_entity_model_probe.bridge_rejection_status)
                << " world_entity_model_bridge_rejection_diagnostics="
                << world_entity_model_probe.bridge_rejection_diagnostics
                << " world_entity_model_bridge_rejection_persistence_rows="
                << world_entity_model_probe.bridge_rejection_persistence_rows
                << " world_entity_model_bridge_rejection_streaming_region_rows="
                << world_entity_model_probe.bridge_rejection_streaming_region_rows
                << " world_entity_model_bridge_rejection_streaming_diagnostics_present="
                << world_entity_model_probe.bridge_rejection_streaming_diagnostics_present
                << " world_entity_model_bridge_rejection_fail_closed="
                << (world_entity_model_probe.bridge_rejection_fail_closed ? 1 : 0)
                << " world_entity_model_diagnostics=" << world_entity_model_probe.diagnostics
                << " addressable_content_status=" << addressable_content_status_name(addressable_content_probe.status)
                << " addressable_content_ready=" << (addressable_content_probe.ready ? 1 : 0)
                << " addressable_content_address_rows=" << addressable_content_probe.address_rows
                << " addressable_content_dependency_rows=" << addressable_content_probe.dependency_rows
                << " addressable_content_load_rows=" << addressable_content_probe.load_rows
                << " addressable_content_release_rows=" << addressable_content_probe.release_rows
                << " addressable_content_refcount_rows=" << addressable_content_probe.refcount_rows
                << " addressable_content_resident_bytes=" << addressable_content_probe.resident_bytes
                << " addressable_content_budget_rejection_status="
                << addressable_content_status_name(addressable_content_probe.budget_rejection_status)
                << " addressable_content_budget_rejection_diagnostics="
                << addressable_content_probe.budget_rejection_diagnostics
                << " addressable_content_package_io=" << (addressable_content_probe.package_io ? 1 : 0)
                << " addressable_content_async_execution=" << (addressable_content_probe.async_execution ? 1 : 0)
                << " addressable_content_committed=" << (addressable_content_probe.committed ? 1 : 0)
                << " addressable_content_diagnostics=" << addressable_content_probe.diagnostics
                << " rpg_systems_status=" << rpg_systems_status_name(rpg_systems_probe.status)
                << " rpg_systems_ready=" << (rpg_systems_probe.ready ? 1 : 0)
                << " rpg_systems_diagnostics=" << rpg_systems_probe.diagnostics
                << " rpg_systems_replay_hash=" << rpg_systems_probe.replay_hash
                << " sandbox_world_status=" << sandbox_world_status_name(sandbox_world_probe.status)
                << " sandbox_world_ready=" << (sandbox_world_probe.ready ? 1 : 0)
                << " sandbox_world_diagnostics=" << sandbox_world_probe.diagnostics
                << " sandbox_world_replay_hash=" << sandbox_world_probe.replay_hash << " simulation_management_status="
                << simulation_management_status_name(simulation_management_probe.status)
                << " simulation_management_ready=" << (simulation_management_probe.ready ? 1 : 0)
                << " simulation_management_diagnostics=" << simulation_management_probe.diagnostics
                << " simulation_management_replay_hash=" << simulation_management_probe.replay_hash
                << " network_replication_status=" << network_replication_status_name(network_replication_probe.status)
                << " network_replication_reviewed=" << (network_replication_probe.reviewed ? 1 : 0)
                << " network_replication_ready=" << (network_replication_probe.ready ? 1 : 0)
                << " network_replication_object_rows=" << network_replication_probe.object_rows
                << " network_replication_input_rows=" << network_replication_probe.input_rows
                << " network_replication_snapshot_rows=" << network_replication_probe.snapshot_rows
                << " network_replication_rollback_rows=" << network_replication_probe.rollback_rows
                << " network_replication_rejected_unsafe_rows=" << network_replication_probe.rejected_unsafe_rows
                << " network_replication_transport_host_evidence="
                << (network_replication_probe.has_transport_host_evidence ? 1 : 0)
                << " network_replication_diagnostics=" << network_replication_probe.diagnostics
                << " network_replication_replay_hash=" << network_replication_probe.replay_hash << '\n';
            return 3;
        }
        if (options.require_renderer_quality_matrix &&
            !(renderer_quality_matrix_probe.reviewed || renderer_quality_matrix_probe.ready)) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_renderer_quality_matrix_unavailable"
                      << " renderer_quality_matrix_status="
                      << renderer_quality_matrix_status_name(renderer_quality_matrix_probe.status)
                      << " renderer_quality_matrix_reviewed=" << (renderer_quality_matrix_probe.reviewed ? 1 : 0)
                      << " renderer_quality_matrix_ready=" << (renderer_quality_matrix_probe.ready ? 1 : 0)
                      << " renderer_quality_matrix_rows=" << renderer_quality_matrix_probe.rows
                      << " renderer_quality_matrix_ready_rows=" << renderer_quality_matrix_probe.ready_rows
                      << " renderer_quality_matrix_host_gated_rows=" << renderer_quality_matrix_probe.host_gated_rows
                      << " renderer_quality_matrix_dependency_gated_rows="
                      << renderer_quality_matrix_probe.dependency_gated_rows
                      << " renderer_quality_matrix_unsupported_rows=" << renderer_quality_matrix_probe.unsupported_rows
                      << " renderer_quality_matrix_host_validated_backends="
                      << renderer_quality_matrix_probe.host_validated_backends
                      << " renderer_quality_matrix_d3d12_ready=" << (renderer_quality_matrix_probe.d3d12_ready ? 1 : 0)
                      << " renderer_quality_matrix_vulkan_strict_ready="
                      << (renderer_quality_matrix_probe.vulkan_strict_ready ? 1 : 0)
                      << " renderer_quality_matrix_metal_ready=" << (renderer_quality_matrix_probe.metal_ready ? 1 : 0)
                      << " renderer_quality_matrix_requires_metal_host_evidence="
                      << (renderer_quality_matrix_probe.requires_metal_host_evidence ? 1 : 0)
                      << " renderer_quality_matrix_metal_host_evidence="
                      << (renderer_quality_matrix_probe.has_metal_host_evidence ? 1 : 0)
                      << " renderer_quality_matrix_selected_package_evidence_ready="
                      << (renderer_quality_matrix_probe.selected_package_evidence_ready ? 1 : 0)
                      << " renderer_quality_matrix_general_renderer_quality_ready="
                      << (renderer_quality_matrix_probe.general_renderer_quality_ready ? 1 : 0)
                      << " renderer_quality_matrix_diagnostics=" << renderer_quality_matrix_probe.diagnostics
                      << " renderer_quality_matrix_replay_hash=" << renderer_quality_matrix_probe.replay_hash << '\n';
            return 20;
        }
        if (options.require_rendering_vfx_profiling &&
            !(rendering_vfx_profiling_probe.reviewed || rendering_vfx_profiling_probe.ready)) {
            std::cout << "sample_generated_desktop_runtime_3d_package required_rendering_vfx_profiling_unavailable"
                      << " rendering_vfx_profiling_status="
                      << rendering_vfx_profiling_status_name(rendering_vfx_profiling_probe.status)
                      << " rendering_vfx_profiling_reviewed=" << (rendering_vfx_profiling_probe.reviewed ? 1 : 0)
                      << " rendering_vfx_profiling_ready=" << (rendering_vfx_profiling_probe.ready ? 1 : 0)
                      << " rendering_vfx_profiling_feature_rows=" << rendering_vfx_profiling_probe.feature_rows
                      << " rendering_vfx_profiling_gpu_particle_budget_rows="
                      << rendering_vfx_profiling_probe.gpu_particle_budget_rows
                      << " rendering_vfx_profiling_postprocess_rows=" << rendering_vfx_profiling_probe.postprocess_rows
                      << " rendering_vfx_profiling_backend_timing_rows="
                      << rendering_vfx_profiling_probe.backend_timing_rows
                      << " rendering_vfx_profiling_backend_evidence_rows="
                      << rendering_vfx_profiling_probe.backend_evidence_rows
                      << " rendering_vfx_profiling_backend_evidence_ready="
                      << rendering_vfx_profiling_probe.backend_evidence_ready
                      << " rendering_vfx_profiling_backend_evidence_host_gated="
                      << rendering_vfx_profiling_probe.backend_evidence_host_gated
                      << " rendering_vfx_profiling_cpu_profile_rows=" << rendering_vfx_profiling_probe.cpu_profile_rows
                      << " rendering_vfx_profiling_package_counter_rows="
                      << rendering_vfx_profiling_probe.package_counter_rows
                      << " rendering_vfx_profiling_package_counter_ready="
                      << rendering_vfx_profiling_probe.package_counter_ready
                      << " rendering_vfx_profiling_package_counter_host_gated="
                      << rendering_vfx_profiling_probe.package_counter_host_gated
                      << " rendering_vfx_profiling_crash_telemetry_handoff_rows="
                      << rendering_vfx_profiling_probe.crash_telemetry_handoff_rows
                      << " rendering_vfx_profiling_host_validated_backends="
                      << rendering_vfx_profiling_probe.host_validated_backends
                      << " rendering_vfx_profiling_d3d12_host_evidence_ready="
                      << (rendering_vfx_profiling_probe.d3d12_host_evidence_ready ? 1 : 0)
                      << " rendering_vfx_profiling_vulkan_strict_host_evidence_ready="
                      << (rendering_vfx_profiling_probe.vulkan_strict_host_evidence_ready ? 1 : 0)
                      << " rendering_vfx_profiling_metal_host_evidence_ready="
                      << (rendering_vfx_profiling_probe.metal_host_evidence_ready ? 1 : 0)
                      << " rendering_vfx_profiling_requires_metal_host_evidence="
                      << (rendering_vfx_profiling_probe.requires_metal_host_evidence ? 1 : 0)
                      << " rendering_vfx_profiling_metal_host_evidence="
                      << (rendering_vfx_profiling_probe.has_metal_host_evidence ? 1 : 0)
                      << " rendering_vfx_profiling_diagnostics=" << rendering_vfx_profiling_probe.diagnostics
                      << " rendering_vfx_profiling_replay_hash=" << rendering_vfx_profiling_probe.replay_hash << '\n';
            return 19;
        }
        if (options.require_scene_collision_package && !collision_package.ready) {
            return 3;
        }
        if (options.require_morph_package && options.require_scene_gpu_bindings && !options.require_compute_morph &&
            (scene_gpu_stats.morph_mesh_bindings < 1 || scene_gpu_stats.morph_mesh_uploads < 1 ||
             scene_gpu_stats.morph_mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.uploaded_morph_bytes == 0 ||
             report.renderer_stats.gpu_morph_draws != static_cast<std::uint64_t>(options.max_frames) ||
             report.renderer_stats.morph_descriptor_binds != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_compute_morph && !options.require_compute_morph_skin &&
            options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_mesh_bindings < 1 || scene_gpu_stats.compute_morph_mesh_dispatches < 1 ||
             scene_gpu_stats.compute_morph_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_mesh_draws != static_cast<std::size_t>(options.max_frames) ||
             report.renderer_stats.gpu_morph_draws != 0 || report.renderer_stats.morph_descriptor_binds != 0)) {
            return 3;
        }
        if (options.require_compute_morph_skin && options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_skinned_mesh_bindings < 1 ||
             scene_gpu_stats.compute_morph_skinned_mesh_dispatches < 1 ||
             scene_gpu_stats.compute_morph_skinned_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_skinned_mesh_bindings_resolved !=
                 static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_skinned_mesh_draws != static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.compute_morph_output_position_bytes == 0 ||
             report.renderer_stats.gpu_skinning_draws != static_cast<std::uint64_t>(options.max_frames) ||
             report.renderer_stats.skinned_palette_descriptor_binds !=
                 static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_compute_morph_async_telemetry && options.require_scene_gpu_bindings &&
            (scene_gpu_stats.compute_morph_async_compute_queue_submits < 1 ||
             scene_gpu_stats.compute_morph_async_graphics_queue_waits < 1 ||
             scene_gpu_stats.compute_morph_async_graphics_queue_submits < 1 ||
             scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value == 0 ||
             scene_gpu_stats.compute_morph_async_last_graphics_queue_wait_fence_value !=
                 scene_gpu_stats.compute_morph_async_last_compute_submitted_fence_value ||
             scene_gpu_stats.compute_morph_async_last_graphics_submitted_fence_value == 0)) {
            return 3;
        }
        if (options.require_scene_gpu_bindings &&
            ((scene_gpu_stats.mesh_bindings == 0 && scene_gpu_stats.skinned_mesh_bindings == 0) ||
             scene_gpu_stats.material_bindings == 0 ||
             (scene_gpu_stats.mesh_bindings_resolved + scene_gpu_stats.skinned_mesh_bindings_resolved) !=
                 static_cast<std::size_t>(options.max_frames) ||
             scene_gpu_stats.material_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
        const auto expected_texture_overlay_sprites =
            static_cast<std::uint64_t>(options.max_frames) *
            static_cast<std::uint64_t>((options.require_native_ui_textured_sprite_atlas ? 1U : 0U) +
                                       (options.require_native_ui_text_glyph_atlas ? 1U : 0U));
        const auto expected_ui_overlay_sprites =
            static_cast<std::uint64_t>(options.max_frames) + expected_texture_overlay_sprites;
        if (options.require_native_ui_overlay &&
            (!game.hud_passed(options.max_frames) ||
             report.native_ui_overlay_status != mirakana::SdlDesktopPresentationNativeUiOverlayStatus::ready ||
             !report.native_ui_overlay_requested || !report.native_ui_overlay_ready ||
             report.native_ui_overlay_sprites_submitted != expected_ui_overlay_sprites ||
             report.native_ui_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            (game.hud_images_submitted() != options.max_frames ||
             ui_atlas_metadata.status != UiAtlasMetadataStatus::ready || ui_atlas_metadata.pages != 1 ||
             ui_atlas_metadata.bindings != 1)) {
            return 3;
        }
        if (options.require_native_ui_text_glyph_atlas &&
            (game.hud_text_glyph_sprites_submitted() != options.max_frames ||
             game.hud_text_glyphs_available() != options.max_frames ||
             game.hud_text_glyphs_resolved() != options.max_frames || game.hud_text_glyphs_missing() != 0 ||
             ui_text_glyph_atlas_metadata.status != UiAtlasMetadataStatus::ready ||
             ui_text_glyph_atlas_metadata.pages != 1 || ui_text_glyph_atlas_metadata.glyphs != 1)) {
            return 3;
        }
        if (requires_native_ui_texture_overlay &&
            (expected_texture_overlay_sprites == 0 ||
             report.native_ui_texture_overlay_status !=
                 mirakana::SdlDesktopPresentationNativeUiTextureOverlayStatus::ready ||
             !report.native_ui_texture_overlay_requested || !report.native_ui_texture_overlay_atlas_ready ||
             report.native_ui_texture_overlay_sprites_submitted != expected_texture_overlay_sprites ||
             report.native_ui_texture_overlay_texture_binds != expected_texture_overlay_sprites ||
             report.native_ui_texture_overlay_draws != expected_texture_overlay_sprites)) {
            return 3;
        }
        const std::uint32_t expected_framegraph_passes = options.require_directional_shadow ? 3U : 2U;
        const auto expected_framegraph_render_pass_count =
            static_cast<std::uint64_t>(options.max_frames) * expected_framegraph_passes;
        const auto expected_framegraph_barrier_step_count = expected_framegraph_barrier_steps(
            options.require_directional_shadow, options.require_postprocess_depth_input, options.max_frames);
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             !postprocess_policy.ready || postprocess_policy.diagnostics_count != 0 ||
             postprocess_policy.effect_count != 1 || postprocess_policy.postprocess_pass_count != 1 ||
             postprocess_policy.framegraph_pass_count != 2 || postprocess_policy.framegraph_barrier_step_budget != 2 ||
             !postprocess_policy.scene_color_required || !postprocess_policy.color_grading_effect ||
             !postprocess_policy.backend_shader_evidence_ready ||
             (options.require_postprocess_depth_input && !postprocess_policy.scene_depth_required) ||
             report.framegraph_passes != expected_framegraph_passes ||
             report.renderer_stats.framegraph_passes_executed !=
                 static_cast<std::uint64_t>(options.max_frames) * expected_framegraph_passes ||
             report.renderer_stats.framegraph_render_passes_recorded != expected_framegraph_render_pass_count ||
             report.renderer_stats.framegraph_barrier_steps_executed != expected_framegraph_barrier_step_count ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
        if (options.require_directional_shadow &&
            (report.directional_shadow_status != mirakana::SdlDesktopPresentationDirectionalShadowStatus::ready ||
             !report.directional_shadow_requested || !report.directional_shadow_ready)) {
            return 3;
        }
        if (options.require_directional_shadow_filtering &&
            (report.directional_shadow_filter_mode !=
                 mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
             report.directional_shadow_filter_tap_count != 9 ||
             report.directional_shadow_filter_radius_texels != 1.0F)) {
            return 3;
        }
    }
    return 0;
}
