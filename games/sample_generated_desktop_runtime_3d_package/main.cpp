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
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/entity_scale_culling.hpp"
#include "mirakana/runtime/package_streaming.hpp"
#include "mirakana/runtime/physics_collision_runtime.hpp"
#include "mirakana/runtime/resource_runtime.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_rhi/package_streaming_frame_graph.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
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
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.GeneratedDesktopRuntime3DPackage.Config.v1"};
constexpr std::string_view kRuntimeSceneVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneMorphVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_tangent_frame.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphTangentFrameShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_tangent_frame.cs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_skinned.vs.dxil"};
constexpr std::string_view kRuntimeSceneComputeMorphSkinnedShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_skinned.cs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow_receiver.ps.dxil"};
constexpr std::string_view kRuntimeShiftedShadowReceiverFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow_receiver_shifted.ps.dxil"};
constexpr std::string_view kRuntimeShadowVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow.vs.dxil"};
constexpr std::string_view kRuntimeShadowFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanMorphVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_tangent_frame.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphTangentFrameShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_tangent_frame.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_skinned.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMorphSkinnedShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene_compute_morph_skinned.cs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_scene.ps.spv"};
constexpr std::string_view kRuntimeShadowReceiverVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow_receiver.ps.spv"};
constexpr std::string_view kRuntimeShiftedShadowReceiverVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow_receiver_shifted.ps.spv"};
constexpr std::string_view kRuntimeShadowVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow.vs.spv"};
constexpr std::string_view kRuntimeShadowVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_shadow.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_postprocess.ps.dxil"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_postprocess.ps.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_ui_overlay.vs.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_ui_overlay.ps.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanVertexShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_ui_overlay.vs.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanFragmentShaderPath{
    "shaders/sample_generated_desktop_runtime_3d_package_ui_overlay.ps.spv"};
constexpr std::uint32_t kRuntimeSceneTangentSpaceStrideBytes{48};
constexpr std::uint64_t kPackageStreamingResidentBudgetBytes{67108864};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/scenes/packaged-3d-scene");
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
    GeneratedGameplaySystemsProbe()
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
          }) {}

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
               navigation_crowd_result_.status == mirakana::NavigationCrowdPlanStatus::success &&
               navigation_crowd_result_.diagnostic == mirakana::NavigationCrowdPlanDiagnostic::none &&
               navigation_crowd_result_.rows.size() == 2U && navigation_crowd_source_order_ready() &&
               navigation_crowd_result_.planned_agent_count == 2U &&
               navigation_crowd_result_.route_success_count == 2U &&
               navigation_crowd_result_.avoidance_success_count == 2U &&
               navigation_crowd_result_.applied_neighbor_count == 2U &&
               navigation_crowd_result_.dynamic_obstacle_count == 2U &&
               local_avoidance_status_ == mirakana::NavigationLocalAvoidanceStatus::success &&
               local_avoidance_diagnostic_ == mirakana::NavigationLocalAvoidanceDiagnostic::none &&
               local_avoidance_steps_ == expected_ticks && local_avoidance_applied_neighbor_count_ == expected_ticks &&
               physics_policy_result_.status == mirakana::PhysicsCharacterDynamicPolicy3DStatus::constrained &&
               physics_policy_result_.diagnostic == mirakana::PhysicsCharacterDynamicPolicy3DDiagnostic::none &&
               physics_policy_result_.rows.size() == 3U && physics_policy_dynamic_push_count_ == 1U &&
               physics_policy_solid_contact_count_ == 1U && physics_policy_trigger_overlap_count_ == 1U &&
               advanced_controller_result_.status == mirakana::PhysicsAdvancedController3DStatus::moved &&
               advanced_controller_result_.diagnostic == mirakana::PhysicsAdvancedController3DDiagnostic::none &&
               advanced_controller_result_.movement.rows.size() == 3U &&
               advanced_controller_result_.moving_platform_rows.size() == 1U &&
               advanced_controller_platform_applied_count_ == 1U &&
               advanced_controller_result_.constraints.status == mirakana::PhysicsJoint3DStatus::solved &&
               advanced_controller_result_.constraints.rows.size() == 1U &&
               advanced_controller_result_.replay_before.body_count == 5U &&
               advanced_controller_result_.replay_after.body_count == 5U &&
               advanced_controller_result_.replay_after.value != advanced_controller_result_.replay_before.value &&
               gameplay_systems_near(advanced_controller_result_.position.x, 3.25F) &&
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
               gameplay_systems_near(audio_stream_sample_abs_sum_, 0.7F) &&
               final_animation_sample_.to_state == "walk" && !final_animation_sample_.blending &&
               final_actor_position_.x > 0.0F;
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
            navigation_crowd_result_.status == mirakana::NavigationCrowdPlanStatus::success,
            navigation_crowd_result_.diagnostic == mirakana::NavigationCrowdPlanDiagnostic::none,
            navigation_crowd_result_.rows.size() == 2U,
            navigation_crowd_source_order_ready(),
            navigation_crowd_result_.planned_agent_count == 2U,
            navigation_crowd_result_.route_success_count == 2U,
            navigation_crowd_result_.avoidance_success_count == 2U,
            navigation_crowd_result_.applied_neighbor_count == 2U,
            navigation_crowd_result_.dynamic_obstacle_count == 2U,
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
            advanced_controller_result_.movement.rows.size() == 3U,
            advanced_controller_result_.moving_platform_rows.size() == 1U,
            advanced_controller_platform_applied_count_ == 1U,
            advanced_controller_result_.constraints.status == mirakana::PhysicsJoint3DStatus::solved,
            advanced_controller_result_.constraints.rows.size() == 1U,
            advanced_controller_result_.replay_before.body_count == 5U,
            advanced_controller_result_.replay_after.body_count == 5U,
            advanced_controller_result_.replay_after.value != advanced_controller_result_.replay_before.value,
            gameplay_systems_near(advanced_controller_result_.position.x, 3.25F),
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

    [[nodiscard]] std::string_view animation_state() const noexcept {
        return final_animation_sample_.to_state;
    }

    [[nodiscard]] float final_actor_x() const noexcept {
        return final_actor_position_.x;
    }

  private:
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
        request.movement.collision_mask = platform_layer | trigger_layer | dynamic_layer;
        request.movement.include_triggers = true;
        request.movement.skin_width = 0.02F;
        request.movement.ground_probe_distance = 0.2F;
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
        const auto clip =
            asset_id_from_game_asset_key("sample-generated-desktop-runtime-3d-package/audio/gameplay-systems");
        mixer.add_clip(mirakana::AudioClipDesc{.clip = clip,
                                               .sample_rate = 48000,
                                               .channel_count = 1,
                                               .frame_count = 4,
                                               .sample_format = mirakana::AudioSampleFormat::float32,
                                               .streaming = false,
                                               .buffered_frame_count = 4});
        const auto voice =
            mixer.play(mirakana::AudioVoiceDesc{.clip = clip, .bus = "master", .gain = 1.0F, .looping = false});
        audio_voice_started_ = voice != mirakana::null_audio_voice;

        const std::vector<mirakana::AudioClipSampleData> samples{
            mirakana::AudioClipSampleData{
                .clip = clip,
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
                .frame_count = 4,
                .interleaved_float_samples = {0.1F, 0.2F, 0.3F, 0.4F},
            },
        };
        const auto output = mirakana::render_audio_device_stream_interleaved_float(
            mixer,
            mirakana::AudioDeviceStreamRequest{
                .format = mirakana::AudioDeviceFormat{.sample_rate = 48000,
                                                      .channel_count = 1,
                                                      .sample_format = mirakana::AudioSampleFormat::float32},
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
    mirakana::PhysicsAuthoredCollisionScene3DBuildResult authored_collision_;
    mirakana::PhysicsCharacterController3DResult controller_result_;
    mirakana::PhysicsCharacterDynamicPolicy3DResult physics_policy_result_;
    mirakana::PhysicsAdvancedController3DResult advanced_controller_result_;
    mirakana::PhysicsConstraintSolve3DResult physics_constraints_result_;
    mirakana::PhysicsKinematicMotion3DResult kinematic_motion_result_;
    mirakana::PhysicsSimpleVehicle3DResult simple_vehicle_result_;
    mirakana::NavigationNavmeshPathResult navigation_navmesh_result_;
    mirakana::NavigationCrowdPlanResult navigation_crowd_result_;
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
                                         bool textured_ui_atlas_mode, mirakana::UiRendererImagePalette image_palette,
                                         bool text_glyph_ui_atlas_mode,
                                         mirakana::UiRendererGlyphAtlasPalette glyph_atlas)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          animation_clip_(std::move(animation_clip)), animation_bindings_(std::move(animation_bindings)),
          morph_payload_(std::move(morph_payload)), morph_animation_clip_(std::move(morph_animation_clip)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)),
          image_palette_(std::move(image_palette)), glyph_atlas_(std::move(glyph_atlas)),
          textured_ui_atlas_mode_(textured_ui_atlas_mode), text_glyph_ui_atlas_mode_(text_glyph_ui_atlas_mode) {}

    void on_start(mirakana::EngineContext&) override {
        renderer_.set_clear_color(mirakana::Color{.r = 0.025F, .g = 0.035F, .b = 0.045F, .a = 1.0F});
        input_.press(mirakana::Key::right);
        gameplay_systems_.start();
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
                 "[--require-renderer-quality-gates] "
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
                 "[--require-entity-scale-culling]\n";
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
    std::size_t query_batch_rows{0};
    std::size_t query_batch_hits{0};
    std::size_t query_batch_no_hits{0};
    std::size_t query_batch_invalid_requests{0};
    std::size_t query_batch_budget_rejections{0};
};

template <typename Row> [[nodiscard]] bool collision_query_source_order_ready(const std::vector<Row>& rows) noexcept {
    for (std::size_t index = 0; index < rows.size(); ++index) {
        if (rows[index].source_index != index) {
            return false;
        }
    }
    return true;
}

template <typename Row>
void accumulate_collision_query_rows(const std::vector<Row>& rows, CollisionPackageReport& report) noexcept {
    report.query_batch_rows += rows.size();
    for (const auto& row : rows) {
        switch (row.status) {
        case mirakana::PhysicsCollisionQueryRowStatus::hit:
            ++report.query_batch_hits;
            break;
        case mirakana::PhysicsCollisionQueryRowStatus::no_hit:
            ++report.query_batch_no_hits;
            break;
        case mirakana::PhysicsCollisionQueryRowStatus::invalid_request:
            ++report.query_batch_invalid_requests;
            break;
        }
    }
}

[[nodiscard]] bool
package_streaming_smoke_ready(const mirakana::runtime::RuntimePackageStreamingExecutionResult& package_streaming_result,
                              std::size_t expected_records) noexcept {
    return package_streaming_result.succeeded() && package_streaming_result.estimated_resident_bytes > 0 &&
           package_streaming_result.replacement.committed_record_count > 0 &&
           package_streaming_result.replacement.committed_record_count == expected_records &&
           package_streaming_result.required_preload_asset_count == 1 &&
           package_streaming_result.resident_resource_kind_count == 10 &&
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
    accumulate_collision_query_rows(raycast_batch.rows, report);
    accumulate_collision_query_rows(sweep_batch.rows, report);
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

    report.query_batch_source_order_ready =
        collision_query_source_order_ready(raycast_batch.rows) && collision_query_source_order_ready(sweep_batch.rows);
    report.query_batch_ready = raycast_batch.status == mirakana::PhysicsCollisionQueryBatchStatus::completed &&
                               sweep_batch.status == mirakana::PhysicsCollisionQueryBatchStatus::completed &&
                               raycast_batch.diagnostic == mirakana::PhysicsCollisionQueryBatchDiagnostic::none &&
                               sweep_batch.diagnostic == mirakana::PhysicsCollisionQueryBatchDiagnostic::none &&
                               report.query_batch_source_order_ready && report.query_batch_rows == 6U &&
                               report.query_batch_hits == 2U && report.query_batch_no_hits == 2U &&
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
                            std::vector<mirakana::AnimationJointTrack3dDesc>& quaternion_animation_tracks) {
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
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene, packaged_animation_clip, packaged_morph_payload,
                                     packaged_morph_animation_clip, packaged_quaternion_animation_tracks)) {
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

    GeneratedDesktopRuntime3DPackageGame game(
        host.input(), host.renderer(), options.throttle, std::move(packaged_scene), std::move(packaged_animation_clip),
        packaged_animation_bindings(), std::move(packaged_morph_payload), std::move(packaged_morph_animation_clip),
        std::move(packaged_quaternion_animation_tracks), options.require_native_ui_textured_sprite_atlas,
        std::move(ui_atlas_metadata.palette), options.require_native_ui_text_glyph_atlas,
        std::move(ui_text_glyph_atlas_metadata.glyph_atlas));
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;
    const auto renderer_quality =
        mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));
    const auto playable_3d =
        evaluate_playable_3d_slice(options, result, game, package_streaming_result,
                                   runtime_package ? runtime_package->records().size() : 0U, report, renderer_quality);
    const auto gameplay_systems_status = game.gameplay_systems_status(options.max_frames);
    const auto gameplay_systems_ready = game.gameplay_systems_passed(options.max_frames);
    const auto gameplay_systems_diagnostics = game.gameplay_systems_diagnostics_count(options.max_frames);
    const auto visible_3d = evaluate_visible_3d_production_proof(options, result, report, renderer_quality, playable_3d,
                                                                 gameplay_systems_ready);
    const auto entity_scale_culling_probe = options.require_entity_scale_culling
                                                ? validate_entity_scale_culling_package_evidence()
                                                : EntityScaleCullingProbeResult{};
    const auto package_upload_staging = options.require_package_upload_staging
                                            ? run_package_upload_staging_evidence(host.presentation())
                                            : mirakana::runtime_rhi::RuntimePackageUploadStagingEvidence{};

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
        << " directional_shadow_status="
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
        << (renderer_quality.directional_shadow_filter_ready ? 1 : 0)
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
        << " gameplay_systems_diagnostics=" << gameplay_systems_diagnostics
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
        << " gameplay_systems_navigation_crowd_status="
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
        << game.gameplay_systems_navigation_crowd_dynamic_obstacles() << " gameplay_systems_local_avoidance_status="
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
             package_streaming_result.resident_resource_kind_count != 10 ||
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
        if (options.require_gameplay_systems && !gameplay_systems_ready) {
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
