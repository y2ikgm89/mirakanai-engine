// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/core/diagnostics.hpp"
#include "mirakana/core/job_execution.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/platform/win32/win32_cpu_sets.hpp"
#include "mirakana/renderer/frame_graph_rhi.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_game_host.hpp"
#include "mirakana/runtime_host/win32/win32_desktop_presentation.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <mutex>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <utility>
#include <vector>

namespace {

struct DesktopRuntimeGameOptions {
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
    bool require_d3d12_shadow_cascade_policy{false};
    bool require_vulkan_shadow_cascade_policy{false};
    bool require_lighting_shadow_policy{false};
    bool require_scene_scale_policy{false};
    bool require_d3d12_instanced_draw_evidence{false};
    bool require_vulkan_instanced_draw_evidence{false};
    bool require_d3d12_postprocess_evidence{false};
    bool require_vulkan_postprocess_evidence{false};
    bool require_gpu_memory_policy{false};
    bool require_memory_diagnostics{false};
    bool require_d3d12_gpu_memory_evidence{false};
    bool require_vulkan_gpu_memory_evidence{false};
    bool require_debug_profiling_policy{false};
    bool require_d3d12_debug_profiling_evidence{false};
    bool require_vulkan_debug_profiling_evidence{false};
    bool require_job_scheduling_evidence{false};
    bool require_job_execution_foundation{false};
    bool require_job_execution_topology_policy{false};
    bool require_job_execution_work_stealing{false};
    bool require_job_execution_placement_policy{false};
    bool require_windows_cpu_set_worker_placement{false};
    bool require_renderer_quality_gates{false};
    bool require_framegraph_multiqueue_evidence{false};
    bool require_vulkan_framegraph_multiqueue_evidence{false};
    bool require_native_ui_overlay{false};
    bool require_native_ui_textured_sprite_atlas{false};
    bool require_gpu_skinning{false};
    bool require_d3d12_gpu_skinning_evidence{false};
    bool require_vulkan_gpu_skinning_evidence{false};
    bool require_quaternion_animation{false};
    std::uint32_t max_frames{0};
    std::string required_config_path;
    std::string required_scene_package_path;
};

constexpr std::string_view kExpectedConfigFormat{"format=GameEngine.SampleDesktopRuntimeGame.Config.v1"};
constexpr std::string_view kRuntimeSceneVertexShaderPath{"shaders/sample_desktop_runtime_game_scene.vs.dxil"};
constexpr std::string_view kRuntimeSceneSkinnedVertexShaderPath{
    "shaders/sample_desktop_runtime_game_scene_skinned.vs.dxil"};
constexpr std::string_view kRuntimeSceneFragmentShaderPath{"shaders/sample_desktop_runtime_game_scene.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver.ps.dxil"};
constexpr std::string_view kRuntimeShadowReceiverSkinnedFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.dxil"};
constexpr std::string_view kRuntimeSceneVulkanVertexShaderPath{"shaders/sample_desktop_runtime_game_scene.vs.spv"};
constexpr std::string_view kRuntimeSceneSkinnedVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_scene_skinned.vs.spv"};
constexpr std::string_view kRuntimeSceneVulkanFragmentShaderPath{"shaders/sample_desktop_runtime_game_scene.ps.spv"};
constexpr std::string_view kRuntimeSceneVulkanComputeMappingShaderPath{
    "shaders/sample_desktop_runtime_game_scene_mapping.cs.spv"};
constexpr std::string_view kRuntimeShadowReceiverVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver.ps.spv"};
constexpr std::string_view kRuntimeShadowReceiverSkinnedVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_shadow_receiver_skinned.ps.spv"};
constexpr std::string_view kRuntimePostprocessVertexShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.vs.dxil"};
constexpr std::string_view kRuntimePostprocessFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.ps.dxil"};
constexpr std::string_view kRuntimePostprocessVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.vs.spv"};
constexpr std::string_view kRuntimePostprocessVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_postprocess.ps.spv"};
constexpr std::string_view kRuntimeShadowVertexShaderPath{"shaders/sample_desktop_runtime_game_shadow.vs.dxil"};
constexpr std::string_view kRuntimeShadowFragmentShaderPath{"shaders/sample_desktop_runtime_game_shadow.ps.dxil"};
constexpr std::string_view kRuntimeShadowVulkanVertexShaderPath{"shaders/sample_desktop_runtime_game_shadow.vs.spv"};
constexpr std::string_view kRuntimeShadowVulkanFragmentShaderPath{"shaders/sample_desktop_runtime_game_shadow.ps.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVertexShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.vs.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.ps.dxil"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanVertexShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.vs.spv"};
constexpr std::string_view kRuntimeNativeUiOverlayVulkanFragmentShaderPath{
    "shaders/sample_desktop_runtime_game_ui_overlay.ps.spv"};
constexpr std::string_view kHudAtlasProofResourceId{"hud.texture_atlas_proof"};
constexpr std::string_view kHudAtlasProofAssetUri{"runtime/assets/desktop_runtime/base_color.texture.geasset"};

[[nodiscard]] mirakana::AssetId asset_id_from_game_asset_key(std::string_view key) {
    return mirakana::asset_id_from_key_v2(mirakana::AssetKeyV2{.value = std::string{key}});
}

[[nodiscard]] mirakana::AssetId packaged_scene_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/scene");
}

[[nodiscard]] mirakana::AssetId packaged_ui_atlas_metadata_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/ui-atlas-metadata");
}

[[nodiscard]] mirakana::AssetId packaged_quaternion_animation_asset_id() {
    return asset_id_from_game_asset_key("sample/desktop-runtime/animations/packaged-pose");
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

constexpr mirakana::SceneNodeId kPackagedMeshNode{1};
constexpr mirakana::SceneNodeId kPrimaryCameraNode{3};

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

void print_ui_atlas_metadata_diagnostics(std::string_view prefix, const UiAtlasMetadataRuntimeState& state) {
    for (const auto& diagnostic : state.diagnostics) {
        std::cout << prefix << " ui_atlas_metadata_diagnostic=" << ui_atlas_metadata_status_name(state.status) << ": "
                  << diagnostic << '\n';
    }
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_scene_vertex_buffers() {
    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeMeshPayload{.has_normals = true, .has_uvs = true, .has_tangent_frame = true});
    return layout.vertex_buffers;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_scene_vertex_attributes() {
    const auto layout = mirakana::runtime_rhi::make_runtime_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeMeshPayload{.has_normals = true, .has_uvs = true, .has_tangent_frame = true});
    return layout.vertex_attributes;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexBufferLayoutDesc> runtime_skinned_scene_vertex_buffers() {
    const auto layout = mirakana::runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeSkinnedMeshPayload{});
    return layout.vertex_buffers;
}

[[nodiscard]] std::vector<mirakana::rhi::VertexAttributeDesc> runtime_skinned_scene_vertex_attributes() {
    const auto layout = mirakana::runtime_rhi::make_runtime_skinned_mesh_vertex_layout_desc(
        mirakana::runtime::RuntimeSkinnedMeshPayload{});
    return layout.vertex_attributes;
}

class SampleDesktopRuntimeGame final : public mirakana::GameApp {
  public:
    SampleDesktopRuntimeGame(mirakana::VirtualInput& input, mirakana::IRenderer& renderer, bool throttle,
                             std::optional<mirakana::RuntimeSceneRenderInstance> scene, bool scene_gpu_mode,
                             std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks,
                             bool lighting_shadow_policy_mode, bool textured_ui_atlas_mode,
                             mirakana::UiRendererImagePalette image_palette, std::uint32_t scene_mesh_instance_count,
                             mirakana::Win32DesktopPresentation* presentation = nullptr)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)),
          image_palette_(std::move(image_palette)), scene_gpu_mode_(scene_gpu_mode),
          lighting_shadow_policy_mode_(lighting_shadow_policy_mode), textured_ui_atlas_mode_(textured_ui_atlas_mode),
          scene_mesh_instance_count_(scene_mesh_instance_count), presentation_(presentation) {}

    void on_start(mirakana::EngineContext&) override {
        input_.press(mirakana::Key::right);
        ui_ok_ = build_hud();
        theme_.add(mirakana::UiThemeColor{.token = "hud.panel",
                                          .color = mirakana::Color{.r = 0.06F, .g = 0.08F, .b = 0.09F, .a = 1.0F}});
        renderer_.set_clear_color(mirakana::Color{.r = 0.02F, .g = 0.03F, .b = 0.035F, .a = 1.0F});
    }

    bool on_update(mirakana::EngineContext&, double) override {
        renderer_.begin_frame();

        const auto axis =
            input_.digital_axis(mirakana::Key::left, mirakana::Key::right, mirakana::Key::down, mirakana::Key::up);
        if (!scene_gpu_mode_) {
            transform_.position = transform_.position + axis;
            renderer_.draw_sprite(mirakana::SpriteCommand{
                .transform = transform_, .color = mirakana::Color{.r = 0.8F, .g = 0.35F, .b = 0.15F, .a = 1.0F}});
        }
        if (scene_.has_value()) {
            std::optional<mirakana::SceneRenderPacket> rebuilt_packet;
            const auto* render_packet = &scene_->render_packet;
            if (scene_->scene.has_value()) {
                if (auto* camera = scene_->scene->find_node(kPrimaryCameraNode);
                    camera != nullptr && camera->components.camera.has_value() && camera->components.camera->primary) {
                    camera->transform.position.x += axis.x;
                    final_camera_x_ = camera->transform.position.x;
                    ++camera_controller_ticks_;
                }
                if (!quaternion_animation_tracks_.empty()) {
                    const auto apply_result = mirakana::sample_and_apply_runtime_scene_render_animation_pose_3d(
                        *scene_, packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                    quaternion_animation_tracks_sampled_ += apply_result.sampled_track_count;
                    if (apply_result.succeeded) {
                        const auto pose = mirakana::sample_animation_local_pose_3d(
                            packaged_quaternion_animation_skeleton(), quaternion_animation_tracks_, 1.0F);
                        const auto* animated = scene_->scene->find_node(kPackagedMeshNode);
                        quaternion_animation_scene_applied_ +=
                            static_cast<std::uint32_t>(apply_result.applied_sample_count);
                        if (pose.joints.size() == 1U && animated != nullptr) {
                            ++quaternion_animation_ticks_;
                            quaternion_animation_seen_ = true;
                            final_quaternion_z_ = pose.joints[0].rotation.z;
                            final_quaternion_w_ = pose.joints[0].rotation.w;
                            final_quaternion_scene_rotation_z_ = animated->transform.rotation_radians.z;
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
            if (lighting_shadow_policy_mode_) {
                const auto lighting_policy = mirakana::plan_scene_lighting_shadow_policy(
                    *render_packet, mirakana::SceneLightingShadowPolicyDesc{
                                        .max_light_count = 8,
                                        .max_shadowed_light_count = 1,
                                        .shadow_map =
                                            mirakana::SceneShadowMapDesc{
                                                .extent = mirakana::rhi::Extent2D{.width = 1024, .height = 1024},
                                                .depth_format = mirakana::rhi::Format::depth24_stencil8,
                                                .directional_cascade_count = 1,
                                            },
                                    });
                lighting_shadow_policy_ok_ = lighting_shadow_policy_ok_ && lighting_policy.succeeded();
                lighting_shadow_policy_light_count_ = lighting_policy.light_count;
                lighting_shadow_policy_directional_light_count_ = lighting_policy.directional_light_count;
                lighting_shadow_policy_point_light_count_ = lighting_policy.point_light_count;
                lighting_shadow_policy_spot_light_count_ = lighting_policy.spot_light_count;
                lighting_shadow_policy_shadowed_light_count_ = lighting_policy.shadowed_light_count;
                lighting_shadow_policy_directional_cascade_count_ = lighting_policy.directional_cascade_count;
                lighting_shadow_policy_shadow_atlas_width_ = lighting_policy.shadow_atlas_extent.width;
                lighting_shadow_policy_shadow_atlas_height_ = lighting_policy.shadow_atlas_extent.height;
                lighting_shadow_policy_light_rows_ = lighting_policy.light_rows.size();
                lighting_shadow_policy_diagnostics_ += lighting_policy.diagnostics.size();
                if (lighting_policy.succeeded() && lighting_policy.light_count > 0 &&
                    lighting_policy.shadowed_light_count > 0 &&
                    lighting_policy.light_rows.size() == static_cast<std::size_t>(lighting_policy.light_count) &&
                    lighting_policy.directional_cascade_count > 0 && lighting_policy.shadow_atlas_extent.width > 0 &&
                    lighting_policy.shadow_atlas_extent.height > 0) {
                    ++lighting_shadow_policy_ready_frames_;
                }
            }
            mirakana::ScenePbrGpuSubmitContext pbr_gpu{};
            const mirakana::ScenePbrGpuSubmitContext* pbr_ptr = nullptr;
            if (scene_gpu_mode_ && presentation_ != nullptr) {
                auto* device = presentation_->scene_pbr_frame_rhi_device();
                const auto scene_ubo = presentation_->scene_pbr_frame_uniform_buffer();
                if (device != nullptr && scene_ubo.value != 0) {
                    const float aspect = renderer_.backbuffer_extent().height > 0
                                             ? static_cast<float>(renderer_.backbuffer_extent().width) /
                                                   static_cast<float>(renderer_.backbuffer_extent().height)
                                             : (16.0F / 9.0F);
                    mirakana::SceneCameraMatrices camera{};
                    mirakana::Vec3 cam_pos{.x = 0.0F, .y = 0.0F, .z = 5.0F};
                    if (const auto* primary = render_packet->primary_camera(); primary != nullptr) {
                        camera = mirakana::make_scene_camera_matrices(*primary, aspect);
                        cam_pos = mirakana::Vec3{.x = primary->world_from_node.at(0, 3),
                                                 .y = primary->world_from_node.at(1, 3),
                                                 .z = primary->world_from_node.at(2, 3)};
                    }
                    pbr_gpu.device = device;
                    pbr_gpu.scene_frame_uniform = scene_ubo;
                    pbr_gpu.camera = camera;
                    pbr_gpu.camera_world_position = cam_pos;
                    pbr_gpu.viewport_aspect = aspect;
                    pbr_ptr = &pbr_gpu;
                }
            }
            const auto scene_submit = mirakana::submit_scene_render_packet(
                renderer_, *render_packet,
                mirakana::SceneRenderSubmitDesc{
                    .fallback_mesh_color = mirakana::Color{.r = 0.8F, .g = 0.35F, .b = 0.15F, .a = 1.0F},
                    .material_palette = &scene_->material_palette,
                    .pbr_gpu = pbr_ptr,
                    .mesh_instance_count = scene_mesh_instance_count_,
                });
            scene_meshes_submitted_ += scene_submit.meshes_submitted;
            scene_materials_resolved_ += scene_submit.material_colors_resolved;
            primary_camera_seen_ = primary_camera_seen_ || scene_submit.has_primary_camera;
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

        renderer_.end_frame();
        ++frames_;
        input_.begin_frame();

        if (throttle_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
        return !input_.key_down(mirakana::Key::escape);
    }

    [[nodiscard]] bool hud_passed(std::uint32_t expected_frames) const noexcept {
        const bool image_rows_ok = !textured_ui_atlas_mode_ || hud_images_submitted_ == expected_frames;
        return ui_ok_ && ui_text_updates_ok_ && hud_boxes_submitted_ == expected_frames && image_rows_ok;
    }

    [[nodiscard]] bool packaged_scene_passed(std::uint32_t expected_frames) const noexcept {
        return primary_camera_seen_ && camera_controller_ticks_ == expected_frames &&
               final_camera_x_ == static_cast<float>(expected_frames) &&
               scene_meshes_submitted_ == static_cast<std::size_t>(expected_frames) &&
               scene_materials_resolved_ == static_cast<std::size_t>(expected_frames) && scene_mesh_plan_ok_ &&
               scene_mesh_plan_meshes_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_draws_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_unique_meshes_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_unique_materials_ == static_cast<std::uint64_t>(expected_frames) &&
               scene_mesh_plan_diagnostics_ == 0;
    }

    [[nodiscard]] bool quaternion_animation_passed(std::uint32_t expected_frames) const noexcept {
        return quaternion_animation_seen_ && quaternion_animation_ticks_ == expected_frames &&
               quaternion_animation_tracks_sampled_ ==
                   static_cast<std::uint64_t>(expected_frames) * quaternion_animation_tracks_.size() &&
               quaternion_animation_scene_applied_ == expected_frames && quaternion_animation_failures_ == 0 &&
               std::abs(final_quaternion_z_ - 1.0F) < 0.0001F && std::abs(final_quaternion_w_) < 0.0001F &&
               std::abs(final_quaternion_scene_rotation_z_ - 3.14159265F) < 0.0001F;
    }

    [[nodiscard]] bool lighting_shadow_policy_passed(std::uint32_t expected_frames) const noexcept {
        return lighting_shadow_policy_ok_ && lighting_shadow_policy_ready_frames_ == expected_frames &&
               lighting_shadow_policy_diagnostics_ == 0 && lighting_shadow_policy_light_count_ > 0 &&
               lighting_shadow_policy_shadowed_light_count_ > 0 &&
               lighting_shadow_policy_light_rows_ == static_cast<std::size_t>(lighting_shadow_policy_light_count_) &&
               lighting_shadow_policy_directional_cascade_count_ > 0 &&
               lighting_shadow_policy_shadow_atlas_width_ > 0 && lighting_shadow_policy_shadow_atlas_height_ > 0;
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

    [[nodiscard]] bool primary_camera_seen() const noexcept {
        return primary_camera_seen_;
    }

    [[nodiscard]] std::uint32_t camera_controller_ticks() const noexcept {
        return camera_controller_ticks_;
    }

    [[nodiscard]] float final_camera_x() const noexcept {
        return final_camera_x_;
    }

    [[nodiscard]] std::size_t hud_boxes_submitted() const noexcept {
        return hud_boxes_submitted_;
    }

    [[nodiscard]] std::size_t hud_images_submitted() const noexcept {
        return hud_images_submitted_;
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

    [[nodiscard]] std::uint32_t lighting_shadow_policy_light_count() const noexcept {
        return lighting_shadow_policy_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_directional_light_count() const noexcept {
        return lighting_shadow_policy_directional_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_point_light_count() const noexcept {
        return lighting_shadow_policy_point_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_spot_light_count() const noexcept {
        return lighting_shadow_policy_spot_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadowed_light_count() const noexcept {
        return lighting_shadow_policy_shadowed_light_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_directional_cascade_count() const noexcept {
        return lighting_shadow_policy_directional_cascade_count_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadow_atlas_width() const noexcept {
        return lighting_shadow_policy_shadow_atlas_width_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_shadow_atlas_height() const noexcept {
        return lighting_shadow_policy_shadow_atlas_height_;
    }

    [[nodiscard]] std::size_t lighting_shadow_policy_light_rows() const noexcept {
        return lighting_shadow_policy_light_rows_;
    }

    [[nodiscard]] std::size_t lighting_shadow_policy_diagnostics() const noexcept {
        return lighting_shadow_policy_diagnostics_;
    }

    [[nodiscard]] std::uint32_t lighting_shadow_policy_ready_frames() const noexcept {
        return lighting_shadow_policy_ready_frames_;
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
        status.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 144.0F, .height = 24.0F};
        status.text = mirakana::ui::TextContent{
            .label = "3D Meshes 0", .localization_key = "hud.status", .font_family = "engine-default"};
        status.style.background_token = "hud.panel";
        status.accessibility_label = "3D diagnostics";
        if (!hud_.try_add_element(status)) {
            return false;
        }
        if (!textured_ui_atlas_mode_) {
            return true;
        }

        mirakana::ui::ElementDesc atlas_image;
        atlas_image.id = mirakana::ui::ElementId{"hud.texture_atlas_proof"};
        atlas_image.parent = root.id;
        atlas_image.role = mirakana::ui::SemanticRole::image;
        atlas_image.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 32.0F, .height = 32.0F};
        atlas_image.image.resource_id = std::string{kHudAtlasProofResourceId};
        atlas_image.image.asset_uri = std::string{kHudAtlasProofAssetUri};
        atlas_image.accessibility_label = "Texture atlas proof";
        return hud_.try_add_element(atlas_image);
    }

    void update_hud_text() {
        const auto text = std::string{"3D Meshes "} + std::to_string(scene_meshes_submitted_);
        ui_text_updates_ok_ =
            ui_text_updates_ok_ &&
            hud_.set_text(mirakana::ui::ElementId{"hud.status"},
                          mirakana::ui::TextContent{
                              .label = text, .localization_key = "hud.status", .font_family = "engine-default"});
    }

    mirakana::VirtualInput& input_;
    mirakana::IRenderer& renderer_;
    mirakana::Transform2D transform_;
    mirakana::ui::UiDocument hud_;
    mirakana::UiRendererTheme theme_;
    mirakana::UiRendererImagePalette image_palette_;
    bool throttle_{true};
    std::optional<mirakana::RuntimeSceneRenderInstance> scene_;
    std::vector<mirakana::AnimationJointTrack3dDesc> quaternion_animation_tracks_;
    bool scene_gpu_mode_{false};
    bool lighting_shadow_policy_mode_{false};
    bool textured_ui_atlas_mode_{false};
    std::uint32_t scene_mesh_instance_count_{1};
    mirakana::Win32DesktopPresentation* presentation_{nullptr};
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
    std::uint32_t camera_controller_ticks_{0};
    std::uint32_t quaternion_animation_ticks_{0};
    std::uint64_t quaternion_animation_tracks_sampled_{0};
    std::uint32_t quaternion_animation_failures_{0};
    std::uint32_t quaternion_animation_scene_applied_{0};
    std::uint32_t lighting_shadow_policy_ready_frames_{0};
    std::uint32_t lighting_shadow_policy_light_count_{0};
    std::uint32_t lighting_shadow_policy_directional_light_count_{0};
    std::uint32_t lighting_shadow_policy_point_light_count_{0};
    std::uint32_t lighting_shadow_policy_spot_light_count_{0};
    std::uint32_t lighting_shadow_policy_shadowed_light_count_{0};
    std::uint32_t lighting_shadow_policy_directional_cascade_count_{0};
    std::uint32_t lighting_shadow_policy_shadow_atlas_width_{0};
    std::uint32_t lighting_shadow_policy_shadow_atlas_height_{0};
    std::size_t lighting_shadow_policy_light_rows_{0};
    std::size_t lighting_shadow_policy_diagnostics_{0};
    float final_camera_x_{0.0F};
    float final_quaternion_z_{0.0F};
    float final_quaternion_w_{1.0F};
    float final_quaternion_scene_rotation_z_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool primary_camera_seen_{false};
    bool quaternion_animation_seen_{false};
    bool scene_mesh_plan_ok_{true};
    bool lighting_shadow_policy_ok_{true};
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
    std::cout << "sample_desktop_runtime_game [--smoke] [--max-frames N] "
                 "[--require-config PATH] [--require-scene-package PATH] [--require-d3d12-scene-shaders] "
                 "[--require-vulkan-scene-shaders] [--require-d3d12-renderer] [--require-vulkan-renderer] "
                 "[--require-scene-gpu-bindings] [--require-postprocess] [--require-postprocess-depth-input] "
                 "[--require-directional-shadow] [--require-directional-shadow-filtering] "
                 "[--require-d3d12-shadow-cascade-policy] [--require-vulkan-shadow-cascade-policy] "
                 "[--require-lighting-shadow-policy] "
                 "[--require-scene-scale-policy] [--require-d3d12-instanced-draw-evidence] "
                 "[--require-vulkan-instanced-draw-evidence] "
                 "[--require-d3d12-postprocess-evidence] "
                 "[--require-vulkan-postprocess-evidence] "
                 "[--require-gpu-memory-policy] [--require-memory-diagnostics] [--require-d3d12-gpu-memory-evidence] "
                 "[--require-vulkan-gpu-memory-evidence] "
                 "[--require-debug-profiling-policy] [--require-d3d12-debug-profiling-evidence] "
                 "[--require-vulkan-debug-profiling-evidence] "
                 "[--require-job-scheduling-evidence] [--require-job-execution-foundation] "
                 "[--require-job-execution-topology-policy] [--require-job-execution-work-stealing] "
                 "[--require-job-execution-placement-policy] [--require-windows-cpu-set-worker-placement] "
                 "[--require-renderer-quality-gates] "
                 "[--require-framegraph-multiqueue-evidence] "
                 "[--require-native-ui-overlay] "
                 "[--require-native-ui-textured-sprite-atlas] "
                 "[--require-gpu-skinning] [--require-d3d12-gpu-skinning-evidence] "
                 "[--require-vulkan-gpu-skinning-evidence] "
                 "[--require-quaternion-animation]\n";
}

[[nodiscard]] bool parse_args(int argc, char** argv, DesktopRuntimeGameOptions& options) {
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
        if (arg == "--require-d3d12-shadow-cascade-policy") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_d3d12_shadow_cascade_policy = true;
            continue;
        }
        if (arg == "--require-vulkan-shadow-cascade-policy") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_vulkan_shadow_cascade_policy = true;
            continue;
        }
        if (arg == "--require-lighting-shadow-policy") {
            options.require_lighting_shadow_policy = true;
            continue;
        }
        if (arg == "--require-scene-scale-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            continue;
        }
        if (arg == "--require-d3d12-instanced-draw-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            options.require_d3d12_instanced_draw_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-instanced-draw-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_scene_scale_policy = true;
            options.require_vulkan_instanced_draw_evidence = true;
            continue;
        }
        if (arg == "--require-d3d12-postprocess-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_d3d12_postprocess_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-postprocess-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_vulkan_postprocess_evidence = true;
            continue;
        }
        if (arg == "--require-gpu-memory-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            continue;
        }
        if (arg == "--require-memory-diagnostics") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_memory_diagnostics = true;
            continue;
        }
        if (arg == "--require-d3d12-gpu-memory-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_d3d12_gpu_memory_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-gpu-memory-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_memory_policy = true;
            options.require_vulkan_gpu_memory_evidence = true;
            continue;
        }
        if (arg == "--require-debug-profiling-policy") {
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            continue;
        }
        if (arg == "--require-d3d12-debug-profiling-evidence") {
            options.require_d3d12_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            options.require_d3d12_debug_profiling_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-debug-profiling-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_debug_profiling_policy = true;
            options.require_vulkan_debug_profiling_evidence = true;
            continue;
        }
        if (arg == "--require-job-scheduling-evidence") {
            options.require_job_scheduling_evidence = true;
            continue;
        }
        if (arg == "--require-job-execution-foundation") {
            options.require_job_execution_foundation = true;
            continue;
        }
        if (arg == "--require-job-execution-topology-policy") {
            options.require_job_execution_topology_policy = true;
            continue;
        }
        if (arg == "--require-job-execution-work-stealing") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            continue;
        }
        if (arg == "--require-job-execution-placement-policy") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            options.require_job_execution_placement_policy = true;
            continue;
        }
        if (arg == "--require-windows-cpu-set-worker-placement") {
            options.require_job_execution_foundation = true;
            options.require_job_execution_topology_policy = true;
            options.require_job_execution_work_stealing = true;
            options.require_job_execution_placement_policy = true;
            options.require_windows_cpu_set_worker_placement = true;
            continue;
        }
        if (arg == "--require-renderer-quality-gates") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_renderer_quality_gates = true;
            continue;
        }
        if (arg == "--require-framegraph-multiqueue-evidence") {
            options.require_framegraph_multiqueue_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-framegraph-multiqueue-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_vulkan_framegraph_multiqueue_evidence = true;
            continue;
        }
        if (arg == "--require-native-ui-overlay") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_native_ui_overlay = true;
            continue;
        }
        if (arg == "--require-native-ui-textured-sprite-atlas") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_native_ui_overlay = true;
            options.require_native_ui_textured_sprite_atlas = true;
            continue;
        }
        if (arg == "--require-gpu-skinning") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            continue;
        }
        if (arg == "--require-d3d12-gpu-skinning-evidence") {
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            options.require_d3d12_gpu_skinning_evidence = true;
            continue;
        }
        if (arg == "--require-vulkan-gpu-skinning-evidence") {
            options.require_vulkan_renderer = true;
            options.require_scene_gpu_bindings = true;
            options.require_gpu_skinning = true;
            options.require_vulkan_gpu_skinning_evidence = true;
            continue;
        }
        if (arg == "--require-quaternion-animation") {
            options.require_quaternion_animation = true;
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

    if (options.smoke) {
        if (options.max_frames == 0) {
            options.max_frames = 2;
        }
        options.throttle = false;
    }
    return true;
}

[[nodiscard]] mirakana::Win32DesktopPresentationSceneScalePolicyDesc
make_scene_scale_policy_desc(const DesktopRuntimeGameOptions& options,
                             bool backend_instancing_evidence_ready) noexcept {
    mirakana::Win32DesktopPresentationSceneScalePolicyDesc desc;
    if (options.require_scene_scale_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_instancing_evidence =
            options.require_d3d12_instanced_draw_evidence || options.require_vulkan_instanced_draw_evidence;
        desc.backend_instancing_evidence_ready = backend_instancing_evidence_ready;
    }
    return desc;
}

[[nodiscard]] std::uint64_t expected_d3d12_instanced_draw_instances(const DesktopRuntimeGameOptions& options) noexcept {
    if (!options.require_d3d12_instanced_draw_evidence) {
        return 0;
    }
    return static_cast<std::uint64_t>(options.max_frames) * 3U;
}

[[nodiscard]] std::uint64_t
expected_vulkan_instanced_draw_instances(const DesktopRuntimeGameOptions& options) noexcept {
    if (!options.require_vulkan_instanced_draw_evidence) {
        return 0;
    }
    return static_cast<std::uint64_t>(options.max_frames) * 3U;
}

[[nodiscard]] std::uint32_t scene_mesh_instance_count(const DesktopRuntimeGameOptions& options) noexcept {
    return (options.require_d3d12_instanced_draw_evidence || options.require_vulkan_instanced_draw_evidence) ? 3U : 1U;
}

[[nodiscard]] bool
directional_shadow_cascade_policy_matches(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    const bool atlas_matches =
        report.directional_shadow_cascade_tile_width > 0 &&
        report.directional_shadow_atlas_width ==
            report.directional_shadow_cascade_count * report.directional_shadow_cascade_tile_width &&
        report.directional_shadow_atlas_height == report.directional_shadow_cascade_tile_width;
    return report.directional_shadow_cascade_count == 4 && atlas_matches &&
           report.directional_shadow_light_space_cascades == report.directional_shadow_cascade_count &&
           report.directional_shadow_cascade_splits == report.directional_shadow_cascade_count + 1U;
}

[[nodiscard]] bool d3d12_shadow_cascade_policy_ready(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 &&
           directional_shadow_cascade_policy_matches(report);
}

[[nodiscard]] bool vulkan_shadow_cascade_policy_ready(const mirakana::Win32DesktopPresentationReport& report) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan &&
           directional_shadow_cascade_policy_matches(report);
}

[[nodiscard]] bool
d3d12_framegraph_multiqueue_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                           const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 && evidence.ready;
}

[[nodiscard]] bool
vulkan_framegraph_multiqueue_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                            const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan && evidence.ready;
}

[[nodiscard]] bool gpu_skinning_evidence_matches(const mirakana::Win32DesktopPresentationReport& report,
                                                 std::uint32_t max_frames) noexcept {
    const auto expected_frames = static_cast<std::uint64_t>(max_frames);
    return report.renderer_stats.gpu_skinning_draws == expected_frames &&
           report.renderer_stats.skinned_palette_descriptor_binds == expected_frames &&
           report.scene_gpu_stats.skinned_mesh_uploads >= 1 && report.scene_gpu_stats.skinned_mesh_bindings >= 1 &&
           report.scene_gpu_stats.skinned_mesh_bindings_resolved == static_cast<std::size_t>(max_frames);
}

[[nodiscard]] bool d3d12_gpu_skinning_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                                     std::uint32_t max_frames) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12 &&
           gpu_skinning_evidence_matches(report, max_frames);
}

[[nodiscard]] bool vulkan_gpu_skinning_evidence_ready(const mirakana::Win32DesktopPresentationReport& report,
                                                      std::uint32_t max_frames) noexcept {
    return report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan &&
           gpu_skinning_evidence_matches(report, max_frames);
}

[[nodiscard]] mirakana::Win32DesktopPresentationGpuMemoryPolicyDesc
make_gpu_memory_policy_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationGpuMemoryPolicyDesc desc;
    if (options.require_gpu_memory_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_memory_evidence =
            options.require_d3d12_gpu_memory_evidence || options.require_vulkan_gpu_memory_evidence;
        desc.require_os_video_memory_budget = options.require_d3d12_gpu_memory_evidence;
        desc.require_declared_budget_evidence = true;
        desc.require_residency_pressure_evidence = true;
        desc.require_package_counter_evidence = true;
        desc.declared_local_budget_bytes = 64ULL * 1024ULL * 1024ULL;
        desc.residency_pressure_event_count = 1;
        desc.package_counter_evidence_ready = true;
    }
    return desc;
}

[[nodiscard]] mirakana::Win32DesktopPresentationDebugProfilingPolicyDesc
make_debug_profiling_policy_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationDebugProfilingPolicyDesc desc;
    if (options.require_debug_profiling_policy) {
        desc.require_scene_gpu_bindings = true;
        desc.expected_frames = options.max_frames;
        desc.require_backend_profiling_evidence =
            options.require_d3d12_debug_profiling_evidence || options.require_vulkan_debug_profiling_evidence;
        desc.require_cpu_profile_zone_evidence = true;
        desc.require_trace_capture_handoff_evidence = true;
        desc.require_package_counter_evidence = true;
        desc.cpu_profile_zone_count = 2;
        desc.trace_capture_handoff_row_count = 1;
        desc.package_counter_evidence_ready = true;
    }
    return desc;
}

[[nodiscard]] std::uint64_t positive_count_for_bytes(std::uint64_t bytes, std::uint64_t count) noexcept {
    return count > 0U ? count : static_cast<std::uint64_t>(bytes > 0U ? 1U : 0U);
}

[[nodiscard]] std::uint64_t
selected_gpu_memory_budget(const mirakana::Win32DesktopPresentationGpuMemoryPolicyReport& gpu_memory_policy) noexcept {
    if (gpu_memory_policy.os_local_budget_bytes > 0U) {
        return gpu_memory_policy.os_local_budget_bytes;
    }
    return 64ULL * 1024ULL * 1024ULL;
}

[[nodiscard]] mirakana::MemoryDiagnosticsSummary
summarize_package_memory_diagnostics(const std::optional<mirakana::runtime::RuntimeAssetPackage>& runtime_package,
                                     const mirakana::Win32DesktopPresentationGpuMemoryPolicyReport& gpu_memory_policy) {
    std::vector<mirakana::MemoryCounterRow> rows;

    if (runtime_package.has_value()) {
        const auto package_bytes = mirakana::runtime::estimate_runtime_asset_package_resident_bytes(*runtime_package);
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::package_resident_cpu,
            .name = "package.runtime_scene",
            .bytes = package_bytes,
            .allocation_count = static_cast<std::uint64_t>(runtime_package->records().size()),
            .high_water_bytes = package_bytes,
            .budget_bytes = 0U,
        });
    }

    const auto resident_gpu_bytes = gpu_memory_policy.committed_byte_estimate > 0U
                                        ? gpu_memory_policy.committed_byte_estimate
                                        : gpu_memory_policy.total_counted_bytes;
    if (resident_gpu_bytes > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::resident_gpu,
            .name = "rhi.committed_resources",
            .bytes = resident_gpu_bytes,
            .allocation_count = positive_count_for_bytes(resident_gpu_bytes, gpu_memory_policy.request_count),
            .high_water_bytes = resident_gpu_bytes,
            .budget_bytes = selected_gpu_memory_budget(gpu_memory_policy),
        });
    }

    if (gpu_memory_policy.upload_bytes_written > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::upload_staging,
            .name = "rhi.upload_staging",
            .bytes = gpu_memory_policy.upload_bytes_written,
            .allocation_count = positive_count_for_bytes(gpu_memory_policy.upload_bytes_written,
                                                         gpu_memory_policy.upload_pressure_request_count),
            .high_water_bytes = gpu_memory_policy.upload_bytes_written,
            .budget_bytes = 0U,
        });
    }

    const auto transient_allocations = gpu_memory_policy.transient_placed_allocations > 0U
                                           ? gpu_memory_policy.transient_placed_allocations
                                           : gpu_memory_policy.transient_heap_allocations;
    if (transient_allocations > 0U || gpu_memory_policy.transient_placed_resources_alive > 0U) {
        rows.push_back(mirakana::MemoryCounterRow{
            .lifetime_class = mirakana::MemoryLifetimeClass::transient_gpu,
            .name = "rhi.transient_textures",
            .bytes = 0U,
            .allocation_count = transient_allocations,
            .high_water_bytes = 0U,
            .budget_bytes = 0U,
        });
    }

    return mirakana::summarize_memory_diagnostics(
        rows, mirakana::MemoryDiagnosticsOptions{.budget_pressure_warning_ratio = 0.95});
}

[[nodiscard]] const mirakana::MemoryClassDiagnosticsSummary*
find_memory_class_summary(const mirakana::MemoryDiagnosticsSummary& summary,
                          mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.lifetime_class == lifetime_class) {
            return &class_summary;
        }
    }
    return nullptr;
}

[[nodiscard]] std::uint64_t memory_class_bytes(const mirakana::MemoryDiagnosticsSummary& summary,
                                               mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->bytes;
}

[[nodiscard]] std::uint64_t memory_class_allocations(const mirakana::MemoryDiagnosticsSummary& summary,
                                                     mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->allocation_count;
}

[[nodiscard]] std::uint64_t memory_class_budget(const mirakana::MemoryDiagnosticsSummary& summary,
                                                mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? 0U : class_summary->budget_bytes;
}

[[nodiscard]] std::string_view memory_class_pressure(const mirakana::MemoryDiagnosticsSummary& summary,
                                                     mirakana::MemoryLifetimeClass lifetime_class) noexcept {
    const auto* class_summary = find_memory_class_summary(summary, lifetime_class);
    return class_summary == nullptr ? "missing" : mirakana::memory_budget_pressure_label(class_summary->pressure);
}

[[nodiscard]] bool memory_diagnostic_code_present(const mirakana::MemoryDiagnosticsSummary& summary,
                                                  mirakana::MemoryDiagnosticsCode code) noexcept {
    for (const auto diagnostic_code : summary.diagnostic_codes) {
        if (diagnostic_code == code) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] std::uint64_t
memory_diagnostics_budgeted_class_count(const mirakana::MemoryDiagnosticsSummary& summary) noexcept {
    std::uint64_t count = 0U;
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.budget_bytes > 0U) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::uint64_t memory_diagnostics_pressure_class_count(const mirakana::MemoryDiagnosticsSummary& summary,
                                                                    mirakana::MemoryBudgetPressure pressure) noexcept {
    std::uint64_t count = 0U;
    for (const auto& class_summary : summary.class_summaries) {
        if (class_summary.pressure == pressure) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::size_t memory_diagnostics_transient_gpu_aliasing_barriers(
    bool requested, const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return requested ? evidence.aliasing_barriers_recorded : 0U;
}

[[nodiscard]] bool memory_diagnostics_transient_gpu_framegraph_aliasing_ready(
    bool requested, const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    return requested && evidence.ready && evidence.aliasing_barriers_recorded > 0U;
}

[[nodiscard]] mirakana::Win32DesktopPresentationQualityGateDesc
make_renderer_quality_gate_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::Win32DesktopPresentationQualityGateDesc desc;
    if (options.require_renderer_quality_gates) {
        desc.require_scene_gpu_bindings = true;
        desc.require_postprocess = true;
        desc.require_postprocess_depth_input = true;
        desc.require_directional_shadow = true;
        desc.require_directional_shadow_filtering = true;
        desc.expected_frames = options.max_frames;
    }
    return desc;
}

[[nodiscard]] std::string_view lighting_shadow_policy_status_name(bool requested, bool ready) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return ready ? "ready" : "blocked";
}

[[nodiscard]] std::string_view
frame_graph_multi_queue_status_name(bool requested,
                                    const mirakana::FrameGraphRhiMultiQueuePackageEvidence& evidence) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return evidence.ready ? "ready" : "blocked";
}

[[nodiscard]] mirakana::JobSchedulingExecutionEvidence build_package_job_scheduling_evidence(bool requested) {
    if (!requested) {
        return {};
    }

    const std::array<mirakana::JobWorkerTopologyRow, 1> topologies{
        mirakana::JobWorkerTopologyRow{.name = "desktop_runtime.package_pool",
                                       .logical_processor_count = 8,
                                       .worker_count = 2,
                                       .queue_count = 2,
                                       .processor_group_count = 1,
                                       .numa_node_count = 1,
                                       .processor_groups_accounted_for = true,
                                       .numa_topology_known = true},
    };
    const std::array<mirakana::JobSchedulingWorkItemRow, 3> work_items{
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.load_scene",
                                           .worker_id = 0,
                                           .batch_size = 32,
                                           .scratch_bytes = 1024,
                                           .worker_local_output_count = 1,
                                           .merge_order = 0},
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.resolve_gameplay",
                                           .worker_id = 1,
                                           .dependency_job_ids = {"package.load_scene"},
                                           .batch_size = 16,
                                           .scratch_bytes = 512,
                                           .worker_local_output_count = 1,
                                           .merge_order = 1},
        mirakana::JobSchedulingWorkItemRow{.job_id = "package.submit_render_intent",
                                           .worker_id = 0,
                                           .dependency_job_ids = {"package.resolve_gameplay"},
                                           .batch_size = 8,
                                           .scratch_bytes = 256,
                                           .worker_local_output_count = 1,
                                           .merge_order = 2},
    };
    return mirakana::build_job_scheduling_execution_evidence(
        topologies, work_items,
        mirakana::JobSchedulingExecutionOptions{.queue_capacity_per_worker = 4,
                                                .minimum_batch_size = 4,
                                                .maximum_batch_size = 64,
                                                .scratch_budget_bytes_per_worker = 4096,
                                                .frame_index = 0});
}

[[nodiscard]] bool job_scheduling_evidence_ready(const mirakana::JobSchedulingExecutionEvidence& evidence) noexcept {
    return evidence.scheduling_summary.status == mirakana::JobSchedulingDiagnosticsStatus::ready &&
           evidence.scratch_summary.status == mirakana::MemoryDiagnosticsStatus::ready &&
           evidence.queue_rows.size() == 2U && evidence.worker_scratch_rows.size() == 2U &&
           evidence.execution_order.size() == 3U && evidence.scheduling_summary.total_submitted_jobs == 3U &&
           evidence.scheduling_summary.total_completed_jobs == 3U &&
           evidence.scheduling_summary.total_deterministic_merge_count == 3U &&
           evidence.scheduling_summary.total_nondeterministic_merge_count == 0U &&
           evidence.scheduling_summary.total_scratch_misuse_count == 0U;
}

[[nodiscard]] std::string_view
job_scheduling_evidence_status_name(bool requested, const mirakana::JobSchedulingExecutionEvidence& evidence) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return job_scheduling_evidence_ready(evidence) ? "ready" : "blocked";
}

[[nodiscard]] mirakana::JobExecutionTopologyPolicyDesc make_package_job_execution_topology_policy_desc() {
    return mirakana::JobExecutionTopologyPolicyDesc{.name = "desktop_runtime.package_execution_pool",
                                                    .observed_logical_processor_count = 8,
                                                    .fallback_logical_processor_count = 1,
                                                    .worker_count_limit = 2,
                                                    .reserved_logical_processor_count = 1,
                                                    .queue_capacity_per_worker = 4,
                                                    .scratch_budget_bytes_per_worker = 4096,
                                                    .frame_index = 0,
                                                    .processor_group_count = 1,
                                                    .numa_node_count = 1,
                                                    .processor_groups_accounted_for = true,
                                                    .numa_topology_known = true};
}

[[nodiscard]] mirakana::JobExecutionTopologyPolicy
build_package_job_execution_topology_policy_evidence(bool requested) {
    if (!requested) {
        return {};
    }
    return mirakana::select_job_execution_topology_policy(make_package_job_execution_topology_policy_desc());
}

[[nodiscard]] bool job_execution_topology_policy_ready(const mirakana::JobExecutionTopologyPolicy& policy) noexcept {
    return policy.status == mirakana::JobExecutionTopologyPolicyStatus::ready &&
           policy.observed_logical_processor_count == 8U && policy.effective_logical_processor_count == 8U &&
           policy.selected_worker_count == 2U && policy.worker_count_limit == 2U &&
           policy.reserved_logical_processor_count == 1U && policy.worker_count_limited_by_cap &&
           !policy.hardware_concurrency_fallback_used && !policy.requested_worker_count_used &&
           !policy.worker_count_clamped_to_logical_processors && !policy.processor_group_policy_applied &&
           !policy.numa_policy_applied && !policy.affinity_policy_applied && !policy.simd_dispatch_applied &&
           !policy.gpu_async_overlap_applied && policy.topology_row.processor_group_count == 1U &&
           policy.topology_row.numa_node_count == 1U && policy.topology_row.processor_groups_accounted_for &&
           policy.topology_row.numa_topology_known && policy.diagnostics.empty();
}

[[nodiscard]] std::string_view
job_execution_topology_policy_status_name(bool requested, const mirakana::JobExecutionTopologyPolicy& policy) noexcept {
    if (!requested) {
        return "not_requested";
    }
    return job_execution_topology_policy_ready(policy) ? "ready" : "blocked";
}

struct JobExecutionFoundationEvidence {
    bool requested{false};
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] JobExecutionFoundationEvidence build_package_job_execution_foundation_evidence(bool requested) {
    JobExecutionFoundationEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    std::atomic_uint64_t task_side_effects{0};
    auto policy_desc = make_package_job_execution_topology_policy_desc();
    policy_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(policy_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }
    auto pool = mirakana::JobExecutionPool(topology_policy.pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    const auto body = [&task_side_effects](mirakana::JobExecutionContext& context) {
        if (context.stop_token.stop_requested()) {
            throw std::runtime_error("job execution foundation task received an unexpected stop request");
        }
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("job execution foundation worker scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x2A};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("job execution foundation worker scratch release failed");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{.evidence =
                                           mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_load_scene",
                                                                              .worker_id = 0,
                                                                              .batch_size = 32,
                                                                              .scratch_bytes = 32,
                                                                              .worker_local_output_count = 1,
                                                                              .merge_order = 0},
                                       .body = body},
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_resolve_gameplay",
                                                           .worker_id = 1,
                                                           .dependency_job_ids = {"package.execute_load_scene"},
                                                           .batch_size = 16,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 1},
            .body = body},
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.execute_submit_render_intent",
                                                           .worker_id = 0,
                                                           .dependency_job_ids = {"package.execute_resolve_gameplay"},
                                                           .batch_size = 8,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 2},
            .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
job_execution_foundation_diagnostic_count(const JobExecutionFoundationEvidence& evidence) noexcept {
    return evidence.run_result.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool job_execution_foundation_ready(const JobExecutionFoundationEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           scheduling.total_submitted_jobs == 3U && run.tasks_executed == 3U && run.tasks_failed == 0U &&
           evidence.task_side_effects == 3U && run.scheduling_evidence.execution_order.size() == 3U &&
           run.scheduling_evidence.queue_rows.size() == 2U && scheduling.total_deterministic_merge_count == 3U &&
           scheduling.total_nondeterministic_merge_count == 0U && scheduling.total_blocked_dependency_count == 0U &&
           scheduling.total_dependency_cycle_count == 0U && scheduling.total_queue_overflow_count == 0U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U && scratch.total_bytes > 0U &&
           scratch.high_water_bytes > 0U && job_execution_foundation_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
job_execution_foundation_status_name(const JobExecutionFoundationEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_foundation_ready(evidence) ? "ready" : "blocked";
}

struct JobExecutionWorkStealingEvidence {
    bool requested{false};
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] JobExecutionWorkStealingEvidence build_package_job_execution_work_stealing_evidence(bool requested) {
    JobExecutionWorkStealingEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    auto policy_desc = make_package_job_execution_topology_policy_desc();
    policy_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(policy_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }
    auto pool = mirakana::JobExecutionPool(topology_policy.pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    std::mutex observed_mutex;
    std::condition_variable observed_cv;
    std::uint32_t started_task_count{0};
    std::atomic_uint64_t task_side_effects{0};
    const auto body = [&](mirakana::JobExecutionContext& context) {
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("job execution work stealing worker scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x42};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("job execution work stealing worker scratch release failed");
        }

        bool both_tasks_started = false;
        {
            std::unique_lock lock(observed_mutex);
            ++started_task_count;
            observed_cv.notify_all();
            both_tasks_started = observed_cv.wait_for(lock, std::chrono::seconds{2},
                                                      [&started_task_count] { return started_task_count >= 2U; });
        }
        if (!both_tasks_started) {
            throw std::runtime_error("job execution work stealing task did not run concurrently");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.steal_prepare",
                                                                                      .worker_id = 0,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 0},
                                       .body = body},
        mirakana::JobExecutionTaskDesc{.evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.steal_execute",
                                                                                      .worker_id = 0,
                                                                                      .batch_size = 32,
                                                                                      .scratch_bytes = 32,
                                                                                      .worker_local_output_count = 1,
                                                                                      .merge_order = 1},
                                       .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
job_execution_work_stealing_diagnostic_count(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    return evidence.run_result.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool job_execution_work_stealing_ready(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           scheduling.total_submitted_jobs == 2U && run.tasks_executed == 2U && run.tasks_failed == 0U &&
           evidence.task_side_effects == 2U && run.scheduling_evidence.execution_order.size() == 2U &&
           run.scheduling_evidence.queue_rows.size() == 2U && run.work_stealing_applied &&
           run.steal_attempt_count >= run.steal_success_count && run.steal_success_count > 0U &&
           scheduling.total_steal_attempt_count >= scheduling.total_steal_success_count &&
           scheduling.total_steal_success_count > 0U && scheduling.total_deterministic_merge_count == 2U &&
           scheduling.total_nondeterministic_merge_count == 0U && scheduling.total_blocked_dependency_count == 0U &&
           scheduling.total_dependency_cycle_count == 0U && scheduling.total_queue_overflow_count == 0U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U && scratch.total_bytes > 0U &&
           scratch.high_water_bytes > 0U && job_execution_work_stealing_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
job_execution_work_stealing_status_name(const JobExecutionWorkStealingEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_work_stealing_ready(evidence) ? "ready" : "blocked";
}

struct JobExecutionPlacementPolicyEvidence {
    bool requested{false};
    mirakana::JobExecutionPlacementPolicy ready_policy;
    mirakana::JobExecutionPlacementPolicy host_evidence_policy;
};

[[nodiscard]] JobExecutionPlacementPolicyEvidence
build_package_job_execution_placement_policy_evidence(bool requested) {
    JobExecutionPlacementPolicyEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    auto topology_desc = make_package_job_execution_topology_policy_desc();
    topology_desc.enable_work_stealing = true;
    const auto topology_policy = mirakana::select_job_execution_topology_policy(topology_desc);
    if (!topology_policy.ready()) {
        return evidence;
    }

    evidence.ready_policy = mirakana::select_job_execution_placement_policy(mirakana::JobExecutionPlacementPolicyDesc{
        .name = "desktop_runtime.package_execution_placement",
        .topology_policy = topology_policy,
        .requested_mode = mirakana::JobExecutionPlacementPolicyMode::os_default,
    });
    evidence.host_evidence_policy =
        mirakana::select_job_execution_placement_policy(mirakana::JobExecutionPlacementPolicyDesc{
            .name = "desktop_runtime.package_execution_placement.numa_probe",
            .topology_policy = topology_policy,
            .requested_mode = mirakana::JobExecutionPlacementPolicyMode::prefer_local_numa,
        });
    return evidence;
}

[[nodiscard]] bool job_execution_placement_policy_ready(const JobExecutionPlacementPolicyEvidence& evidence) noexcept {
    const auto& policy = evidence.ready_policy;
    const auto& host_evidence_policy = evidence.host_evidence_policy;
    return evidence.requested && policy.status == mirakana::JobExecutionPlacementPolicyStatus::ready &&
           policy.requested_mode == mirakana::JobExecutionPlacementPolicyMode::os_default &&
           policy.selected_mode == mirakana::JobExecutionPlacementPolicyMode::os_default &&
           policy.inherited_worker_count == 2U && policy.pool_desc.work_stealing_enabled &&
           policy.numa_node_count == 1U && policy.performance_core_count == 0U && policy.efficiency_core_count == 0U &&
           !policy.smt_sibling_topology_known && !policy.affinity_policy_applied && !policy.numa_policy_applied &&
           !policy.simd_dispatch_applied && !policy.gpu_async_overlap_applied && policy.diagnostics.empty() &&
           host_evidence_policy.status == mirakana::JobExecutionPlacementPolicyStatus::host_evidence_required &&
           host_evidence_policy.diagnostic_codes.size() == 1U &&
           std::ranges::find(host_evidence_policy.diagnostic_codes,
                             mirakana::JobExecutionPlacementPolicyDiagnosticCode::missing_numa_evidence) !=
               host_evidence_policy.diagnostic_codes.end();
}

[[nodiscard]] std::string_view
job_execution_placement_policy_status_name(const JobExecutionPlacementPolicyEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return job_execution_placement_policy_ready(evidence) ? "ready" : "blocked";
}

struct WindowsCpuSetWorkerPlacementEvidence {
    bool requested{false};
    mirakana::win32::Win32CpuSetWorkerPlacementPlan placement_plan;
    mirakana::JobExecutionPoolStatus pool_status{mirakana::JobExecutionPoolStatus::invalid_configuration};
    mirakana::JobExecutionRunResult run_result;
    std::uint64_t task_side_effects{0};
};

[[nodiscard]] WindowsCpuSetWorkerPlacementEvidence
build_package_windows_cpu_set_worker_placement_evidence(bool requested) {
    WindowsCpuSetWorkerPlacementEvidence evidence;
    evidence.requested = requested;
    if (!requested) {
        return evidence;
    }

    const auto topology_policy =
        mirakana::select_job_execution_topology_policy(make_package_job_execution_topology_policy_desc());
    if (!topology_policy.ready()) {
        return evidence;
    }

    auto cpu_sets = mirakana::win32::query_win32_cpu_sets();
    evidence.placement_plan =
        mirakana::win32::select_win32_cpu_set_worker_placement(mirakana::win32::Win32CpuSetWorkerPlacementDesc{
            .cpu_sets = std::move(cpu_sets),
            .worker_count = topology_policy.selected_worker_count,
            .mode = mirakana::JobExecutionPlacementPolicyMode::prefer_performance_cores,
        });
    if (!evidence.placement_plan.ready()) {
        return evidence;
    }

    auto pool_desc = topology_policy.pool_desc;
    pool_desc.placement_requested_mode = mirakana::JobExecutionPlacementPolicyMode::prefer_performance_cores;
    pool_desc.placement_selected_mode = mirakana::JobExecutionPlacementPolicyMode::prefer_performance_cores;
    pool_desc.worker_placement_callback =
        mirakana::win32::make_win32_cpu_set_worker_placement_callback(evidence.placement_plan);

    auto pool = mirakana::JobExecutionPool(pool_desc);
    evidence.pool_status = pool.status();
    if (evidence.pool_status != mirakana::JobExecutionPoolStatus::ready) {
        return evidence;
    }

    std::atomic_uint64_t task_side_effects{0};
    const auto body = [&task_side_effects](mirakana::JobExecutionContext& context) {
        if (context.stop_token.stop_requested()) {
            throw std::runtime_error("windows CPU set worker placement task received an unexpected stop request");
        }
        const auto lease = context.scratch.acquire(32, 16, context.worker_id);
        if (!lease.valid()) {
            throw std::runtime_error("windows CPU set worker placement scratch lease failed");
        }
        lease.bytes.front() = std::byte{0x7B};
        const auto release_status = context.scratch.release(lease, context.worker_id);
        if (release_status != mirakana::ScratchLeaseStatus::released) {
            throw std::runtime_error("windows CPU set worker placement scratch release failed");
        }
        task_side_effects.fetch_add(1, std::memory_order_relaxed);
    };

    auto batch = mirakana::JobExecutionBatchDesc{};
    batch.tasks = {
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.windows_cpu_set_prepare",
                                                           .worker_id = 0,
                                                           .batch_size = 32,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 0},
            .body = body},
        mirakana::JobExecutionTaskDesc{
            .evidence = mirakana::JobSchedulingWorkItemRow{.job_id = "package.windows_cpu_set_execute",
                                                           .worker_id = 1,
                                                           .batch_size = 32,
                                                           .scratch_bytes = 32,
                                                           .worker_local_output_count = 1,
                                                           .merge_order = 1},
            .body = body},
    };
    batch.options.minimum_batch_size = 4;
    batch.options.maximum_batch_size = 64;

    evidence.run_result = pool.execute(batch);
    evidence.task_side_effects = task_side_effects.load(std::memory_order_relaxed);
    return evidence;
}

[[nodiscard]] std::size_t
windows_cpu_set_worker_placement_diagnostic_count(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    return evidence.placement_plan.diagnostics.size() + evidence.run_result.diagnostics.size() +
           evidence.run_result.worker_placement_diagnostic_count +
           evidence.run_result.scheduling_evidence.scheduling_summary.diagnostics.size() +
           evidence.run_result.scheduling_evidence.scratch_summary.diagnostics.size();
}

[[nodiscard]] bool
windows_cpu_set_worker_placement_ready(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    const auto& run = evidence.run_result;
    const auto& scheduling = run.scheduling_evidence.scheduling_summary;
    const auto& scratch = run.scheduling_evidence.scratch_summary;
    return evidence.requested && evidence.placement_plan.ready() &&
           evidence.placement_plan.selected_cpu_set_count > 0U && evidence.placement_plan.worker_rows.size() == 2U &&
           evidence.pool_status == mirakana::JobExecutionPoolStatus::ready &&
           run.status == mirakana::JobExecutionRunStatus::ready && run.worker_threads_started == 2U &&
           run.worker_placement_attempt_count == 2U && run.worker_placement_applied_count == 2U &&
           run.worker_placement_diagnostic_count == 0U && run.worker_placement_selected_cpu_set_count == 2U &&
           scheduling.worker_topology_row_count == 1U && scheduling.total_submitted_jobs == 2U &&
           run.tasks_executed == 2U && run.tasks_failed == 0U && evidence.task_side_effects == 2U &&
           run.scheduling_evidence.execution_order.size() == 2U && run.scheduling_evidence.queue_rows.size() == 2U &&
           scratch.status == mirakana::MemoryDiagnosticsStatus::ready &&
           run.scheduling_evidence.worker_scratch_rows.size() == 2U &&
           windows_cpu_set_worker_placement_diagnostic_count(evidence) == 0U;
}

[[nodiscard]] std::string_view
windows_cpu_set_worker_placement_status_name(const WindowsCpuSetWorkerPlacementEvidence& evidence) noexcept {
    if (!evidence.requested) {
        return "not_requested";
    }
    return windows_cpu_set_worker_placement_ready(evidence) ? "ready" : "blocked";
}

[[nodiscard]] mirakana::FrameGraphRhiMultiQueuePackageEvidence
run_frame_graph_multi_queue_package_evidence(mirakana::Win32DesktopPresentation& presentation) {
    auto* device = presentation.scene_pbr_frame_rhi_device();
    if (device != nullptr) {
        return mirakana::execute_frame_graph_rhi_multi_queue_package_evidence(*device);
    }

    mirakana::FrameGraphRhiMultiQueuePackageEvidence evidence;
    evidence.diagnostics.push_back(mirakana::FrameGraphDiagnostic{
        .code = mirakana::FrameGraphDiagnosticCode::invalid_pass,
        .pass = "framegraph_multiqueue",
        .resource = "package.alias.early",
        .message = "Frame Graph multi-queue package evidence requires a native RHI device",
    });
    return evidence;
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
        auto decoded_tracks = make_quaternion_animation_tracks(quaternion_payload.payload);
        if (!mirakana::is_valid_animation_joint_tracks_3d(packaged_quaternion_animation_skeleton(), decoded_tracks)) {
            std::cerr << "runtime scene package quaternion animation clip does not match its smoke skeleton: "
                      << package_path << '\n';
            return false;
        }

        quaternion_animation_tracks = std::move(decoded_tracks);
        runtime_package = std::move(package_result.package);
        scene = std::move(instance);
    } catch (const std::exception& exception) {
        std::cerr << "failed to read required scene package '" << package_path << "': " << exception.what() << '\n';
        return false;
    }

    return true;
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
load_packaged_vulkan_scene_shaders(const char* executable_path) {
    mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
    return mirakana::load_desktop_shader_bytecode_pair(mirakana::DesktopShaderBytecodeLoadDesc{
        .filesystem = &filesystem,
        .vertex_path = std::string{kRuntimeSceneVulkanVertexShaderPath},
        .fragment_path = std::string{kRuntimeSceneVulkanFragmentShaderPath},
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

[[nodiscard]] mirakana::Win32DesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::Win32DesktopPresentationShaderBytecode{
        .entry_point = bytecode.entry_point,
        .bytecode = std::span<const std::uint8_t>{bytecode.bytecode.data(), bytecode.bytecode.size()},
    };
}

[[nodiscard]] mirakana::DesktopShaderBytecodeBlob load_packaged_single_shader_blob(const char* executable_path,
                                                                                   std::string_view relative_path,
                                                                                   std::string_view entry_point) {
    mirakana::DesktopShaderBytecodeBlob blob;
    blob.entry_point = std::string{entry_point};
    try {
        mirakana::RootedFileSystem filesystem(executable_directory(executable_path));
        const std::string path{relative_path};
        if (!filesystem.exists(path)) {
            return blob;
        }
        const auto text = filesystem.read_text(path);
        blob.bytecode.assign(text.begin(), text.end());
    } catch (const std::exception&) {
        blob.bytecode.clear();
    }
    return blob;
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

void print_presentation_report(std::string_view prefix, const mirakana::Win32DesktopGameHost& host) {
    const auto report = host.presentation_report();
    const auto postprocess_policy = mirakana::evaluate_win32_desktop_presentation_postprocess_policy(report);
    std::cout << prefix << " presentation_report=requested="
              << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
              << " selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
              << " fallback=" << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
              << " used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
              << " diagnostics=" << report.diagnostics_count << " backend_reports=" << report.backend_reports_count
              << " scene_gpu_status="
              << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
              << " scene_gpu_mesh_bindings=" << report.scene_gpu_stats.mesh_bindings
              << " scene_gpu_material_bindings=" << report.scene_gpu_stats.material_bindings
              << " scene_gpu_mesh_uploads=" << report.scene_gpu_stats.mesh_uploads
              << " scene_gpu_texture_uploads=" << report.scene_gpu_stats.texture_uploads
              << " scene_gpu_material_uploads=" << report.scene_gpu_stats.material_uploads
              << " scene_gpu_material_pipeline_layouts=" << report.scene_gpu_stats.material_pipeline_layouts
              << " scene_gpu_uploaded_texture_bytes=" << report.scene_gpu_stats.uploaded_texture_bytes
              << " scene_gpu_uploaded_mesh_bytes=" << report.scene_gpu_stats.uploaded_mesh_bytes
              << " scene_gpu_uploaded_material_factor_bytes=" << report.scene_gpu_stats.uploaded_material_factor_bytes
              << " scene_gpu_morph_mesh_bindings=" << report.scene_gpu_stats.morph_mesh_bindings
              << " scene_gpu_morph_mesh_uploads=" << report.scene_gpu_stats.morph_mesh_uploads
              << " scene_gpu_uploaded_morph_bytes=" << report.scene_gpu_stats.uploaded_morph_bytes
              << " scene_gpu_mesh_resolved=" << report.scene_gpu_stats.mesh_bindings_resolved
              << " scene_gpu_skinned_mesh_bindings=" << report.scene_gpu_stats.skinned_mesh_bindings
              << " scene_gpu_skinned_mesh_uploads=" << report.scene_gpu_stats.skinned_mesh_uploads
              << " scene_gpu_skinned_mesh_resolved=" << report.scene_gpu_stats.skinned_mesh_bindings_resolved
              << " scene_gpu_material_resolved=" << report.scene_gpu_stats.material_bindings_resolved
              << " postprocess_status="
              << mirakana::win32_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
              << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
              << " postprocess_policy_status="
              << mirakana::win32_desktop_presentation_postprocess_policy_status_name(postprocess_policy.status)
              << " postprocess_policy_ready=" << (postprocess_policy.ready ? 1 : 0)
              << " postprocess_policy_diagnostics=" << postprocess_policy.diagnostics_count
              << " postprocess_policy_effects=" << postprocess_policy.effect_count
              << " postprocess_policy_postprocess_passes=" << postprocess_policy.postprocess_pass_count
              << " postprocess_policy_framegraph_passes=" << postprocess_policy.framegraph_pass_count
              << " postprocess_policy_framegraph_barrier_step_budget="
              << postprocess_policy.framegraph_barrier_step_budget
              << " postprocess_policy_scene_color_required=" << (postprocess_policy.scene_color_required ? 1 : 0)
              << " postprocess_policy_scene_depth_required=" << (postprocess_policy.scene_depth_required ? 1 : 0)
              << " postprocess_policy_color_grading_effect=" << (postprocess_policy.color_grading_effect ? 1 : 0)
              << " postprocess_policy_backend_shader_evidence_ready="
              << (postprocess_policy.backend_shader_evidence_ready ? 1 : 0) << " directional_shadow_status="
              << mirakana::win32_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
              << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
              << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
              << " directional_shadow_filter_mode="
              << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
                     report.directional_shadow_filter_mode)
              << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
              << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
              << " directional_shadow_cascade_count=" << report.directional_shadow_cascade_count
              << " directional_shadow_cascade_tile_width=" << report.directional_shadow_cascade_tile_width
              << " directional_shadow_atlas_width=" << report.directional_shadow_atlas_width
              << " directional_shadow_atlas_height=" << report.directional_shadow_atlas_height
              << " directional_shadow_light_space_cascades=" << report.directional_shadow_light_space_cascades
              << " directional_shadow_cascade_splits=" << report.directional_shadow_cascade_splits
              << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
              << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
              << " ui_overlay_draws=" << report.native_ui_overlay_draws
              << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
              << " ui_texture_overlay_status="
              << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                     report.native_ui_texture_overlay_status)
              << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
              << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
              << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
              << " framegraph_passes=" << report.framegraph_passes
              << " framegraph_passes_executed=" << report.renderer_stats.framegraph_passes_executed
              << " framegraph_render_passes_recorded=" << report.renderer_stats.framegraph_render_passes_recorded
              << " framegraph_barrier_steps_executed=" << report.renderer_stats.framegraph_barrier_steps_executed
              << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
              << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
              << " rhi_instanced_draw_calls=" << report.rhi_instanced_draw_calls
              << " rhi_instanced_indexed_draw_calls=" << report.rhi_instanced_indexed_draw_calls
              << " rhi_instanced_instances_submitted=" << report.rhi_instanced_instances_submitted
              << " renderer_frames_finished=" << report.renderer_stats.frames_finished << '\n';
    for (const auto& backend_report : host.presentation_backend_reports()) {
        std::cout << prefix << " presentation_backend_report="
                  << mirakana::win32_desktop_presentation_backend_name(backend_report.backend) << ":"
                  << mirakana::win32_desktop_presentation_backend_report_status_name(backend_report.status) << ":"
                  << mirakana::win32_desktop_presentation_fallback_reason_name(backend_report.fallback_reason) << ": "
                  << backend_report.diagnostic << '\n';
    }
}

} // namespace

int main(int argc, char** argv) {
    DesktopRuntimeGameOptions options;
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
    std::vector<mirakana::AnimationJointTrack3dDesc> packaged_quaternion_animation_tracks;
    if (!load_required_scene_package(argc > 0 ? argv[0] : nullptr, options.required_scene_package_path, runtime_package,
                                     packaged_scene, packaged_quaternion_animation_tracks)) {
        return 4;
    }
    if (options.require_scene_gpu_bindings && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-scene-gpu-bindings requires --require-scene-package\n";
        return 4;
    }
    if (options.require_gpu_skinning && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-gpu-skinning requires --require-scene-package\n";
        return 4;
    }
    if (options.require_lighting_shadow_policy && (!runtime_package.has_value() || !packaged_scene.has_value())) {
        std::cerr << "--require-lighting-shadow-policy requires --require-scene-package\n";
        return 4;
    }
    if (options.require_quaternion_animation && packaged_quaternion_animation_tracks.empty()) {
        std::cerr << "--require-quaternion-animation requires --require-scene-package\n";
        return 4;
    }
    UiAtlasMetadataRuntimeState ui_atlas_metadata;
    if (options.require_native_ui_textured_sprite_atlas) {
        ui_atlas_metadata = load_required_ui_atlas_metadata(runtime_package.has_value() ? &*runtime_package : nullptr);
        if (ui_atlas_metadata.status != UiAtlasMetadataStatus::ready) {
            std::cout << "sample_desktop_runtime_game required_ui_atlas_metadata_unavailable status="
                      << ui_atlas_metadata_status_name(ui_atlas_metadata.status) << " pages=" << ui_atlas_metadata.pages
                      << " bindings=" << ui_atlas_metadata.bindings << '\n';
            print_ui_atlas_metadata_diagnostics("sample_desktop_runtime_game", ui_atlas_metadata);
            return 4;
        }
    }

    auto shader_bytecode = load_packaged_d3d12_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shader_bytecode.ready()) {
        std::cout << "sample_desktop_runtime_game shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shader_bytecode.status) << ": "
                  << shader_bytecode.diagnostic << '\n';
        if (options.require_d3d12_scene_shaders) {
            return 4;
        }
    }
    auto postprocess_bytecode = load_packaged_d3d12_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!postprocess_bytecode.ready()) {
        std::cout << "sample_desktop_runtime_game postprocess_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(postprocess_bytecode.status) << ": "
                  << postprocess_bytecode.diagnostic << '\n';
        if (options.require_postprocess && !options.require_vulkan_renderer) {
            return 4;
        }
    }
    auto shadow_receiver_bytecode = load_packaged_d3d12_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shadow_receiver_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shadow_receiver_bytecode.status) << ": "
                  << shadow_receiver_bytecode.diagnostic << '\n';
        return 4;
    }
    auto shadow_bytecode = load_packaged_d3d12_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!shadow_bytecode.ready() && options.require_directional_shadow && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(shadow_bytecode.status) << ": "
                  << shadow_bytecode.diagnostic << '\n';
        return 4;
    }
    auto native_ui_overlay_bytecode = load_packaged_d3d12_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay && !options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(native_ui_overlay_bytecode.status) << ": "
                  << native_ui_overlay_bytecode.diagnostic << '\n';
        return 4;
    }

    const auto d3d12_skinned_vertex_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneSkinnedVertexShaderPath, "vs_skinned");
    const auto vulkan_skinned_vertex_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneSkinnedVulkanVertexShaderPath, "vs_skinned");
    if (options.require_gpu_skinning) {
        if (!options.require_vulkan_renderer && d3d12_skinned_vertex_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_gpu_skinning_missing_skinned_d3d12_vs\n";
            return 4;
        }
        if (options.require_vulkan_renderer && vulkan_skinned_vertex_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_gpu_skinning_missing_skinned_vulkan_vs\n";
            return 6;
        }
    }

    const auto d3d12_skinned_shadow_receiver_fragment_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeShadowReceiverSkinnedFragmentShaderPath, "ps_shadow_receiver");
    const auto vulkan_skinned_shadow_receiver_fragment_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeShadowReceiverSkinnedVulkanFragmentShaderPath, "ps_shadow_receiver");
    if (options.require_directional_shadow && options.require_gpu_skinning) {
        if (!options.require_vulkan_renderer && d3d12_skinned_shadow_receiver_fragment_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_skinned_shadow_receiver_missing_d3d12_ps\n";
            return 4;
        }
        if (options.require_vulkan_renderer && vulkan_skinned_shadow_receiver_fragment_blob.bytecode.empty()) {
            std::cout << "sample_desktop_runtime_game required_skinned_shadow_receiver_missing_vulkan_ps\n";
            return 6;
        }
    }

    auto vulkan_shader_bytecode = load_packaged_vulkan_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shader_bytecode.ready()) {
        if (options.require_vulkan_scene_shaders) {
            std::cout << "sample_desktop_runtime_game vulkan_shader_diagnostic="
                      << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shader_bytecode.status) << ": "
                      << vulkan_shader_bytecode.diagnostic << '\n';
            return 6;
        }
    }
    auto vulkan_postprocess_bytecode = load_packaged_vulkan_postprocess_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_postprocess_bytecode.ready()) {
        if (options.require_postprocess && options.require_vulkan_renderer) {
            std::cout << "sample_desktop_runtime_game vulkan_postprocess_shader_diagnostic="
                      << mirakana::desktop_shader_bytecode_load_status_name(vulkan_postprocess_bytecode.status) << ": "
                      << vulkan_postprocess_bytecode.diagnostic << '\n';
            return 6;
        }
    }
    auto vulkan_shadow_receiver_bytecode =
        load_packaged_vulkan_shadow_receiver_scene_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_receiver_bytecode.ready() && options.require_directional_shadow &&
        options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_shadow_receiver_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_receiver_bytecode.status) << ": "
                  << vulkan_shadow_receiver_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_shadow_bytecode = load_packaged_vulkan_shadow_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_shadow_bytecode.ready() && options.require_directional_shadow && options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_shadow_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_shadow_bytecode.status) << ": "
                  << vulkan_shadow_bytecode.diagnostic << '\n';
        return 6;
    }
    auto vulkan_native_ui_overlay_bytecode =
        load_packaged_vulkan_native_ui_overlay_shaders(argc > 0 ? argv[0] : nullptr);
    if (!vulkan_native_ui_overlay_bytecode.ready() && options.require_native_ui_overlay &&
        options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_native_ui_overlay_shader_diagnostic="
                  << mirakana::desktop_shader_bytecode_load_status_name(vulkan_native_ui_overlay_bytecode.status)
                  << ": " << vulkan_native_ui_overlay_bytecode.diagnostic << '\n';
        return 6;
    }
    const auto vulkan_compute_mapping_blob = load_packaged_single_shader_blob(
        argc > 0 ? argv[0] : nullptr, kRuntimeSceneVulkanComputeMappingShaderPath, "cs_vulkan_mapping_proof");
    if (vulkan_compute_mapping_blob.bytecode.empty() && options.require_vulkan_renderer) {
        std::cout << "sample_desktop_runtime_game vulkan_compute_mapping_shader_diagnostic=missing: "
                  << kRuntimeSceneVulkanComputeMappingShaderPath << '\n';
        return 6;
    }

    std::optional<mirakana::Win32DesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    const auto& d3d12_scene_bytecode = options.require_directional_shadow ? shadow_receiver_bytecode : shader_bytecode;
    const bool d3d12_shadow_ready = !options.require_directional_shadow || shadow_bytecode.ready();
    const bool d3d12_native_ui_overlay_ready = !options.require_native_ui_overlay || native_ui_overlay_bytecode.ready();
    if (d3d12_scene_bytecode.ready() && postprocess_bytecode.ready() && d3d12_shadow_ready &&
        d3d12_native_ui_overlay_ready && runtime_package.has_value() && packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::Win32DesktopPresentationD3d12SceneRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(d3d12_scene_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(d3d12_scene_bytecode.fragment_shader),
            .skinned_vertex_shader = to_presentation_shader_bytecode(d3d12_skinned_vertex_blob),
            .skinned_scene_fragment_shader =
                to_presentation_shader_bytecode(d3d12_skinned_shadow_receiver_fragment_blob),
            .postprocess_vertex_shader = to_presentation_shader_bytecode(postprocess_bytecode.vertex_shader),
            .postprocess_fragment_shader = to_presentation_shader_bytecode(postprocess_bytecode.fragment_shader),
            .shadow_vertex_shader = to_presentation_shader_bytecode(shadow_bytecode.vertex_shader),
            .shadow_fragment_shader = to_presentation_shader_bytecode(shadow_bytecode.fragment_shader),
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(native_ui_overlay_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .skinned_vertex_buffers = runtime_skinned_scene_vertex_buffers(),
            .skinned_vertex_attributes = runtime_skinned_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = true,
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset =
                options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.atlas_page : mirakana::AssetId{},
            .enable_native_ui_overlay_textures = options.require_native_ui_textured_sprite_atlas,
        });
    }

    std::optional<mirakana::Win32DesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
    const auto& vulkan_scene_bytecode =
        options.require_directional_shadow ? vulkan_shadow_receiver_bytecode : vulkan_shader_bytecode;
    const bool vulkan_shadow_ready = !options.require_directional_shadow || vulkan_shadow_bytecode.ready();
    const bool vulkan_native_ui_overlay_ready =
        !options.require_native_ui_overlay || vulkan_native_ui_overlay_bytecode.ready();
    const bool vulkan_compute_mapping_ready = !vulkan_compute_mapping_blob.bytecode.empty();
    if (vulkan_scene_bytecode.ready() && vulkan_postprocess_bytecode.ready() && vulkan_shadow_ready &&
        vulkan_native_ui_overlay_ready && vulkan_compute_mapping_ready && runtime_package.has_value() &&
        packaged_scene.has_value()) {
        vulkan_scene_renderer.emplace(mirakana::Win32DesktopPresentationVulkanSceneRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.fragment_shader),
            .skinned_vertex_shader = to_presentation_shader_bytecode(vulkan_skinned_vertex_blob),
            .compute_morph_shader = to_presentation_shader_bytecode(vulkan_compute_mapping_blob),
            .skinned_scene_fragment_shader =
                to_presentation_shader_bytecode(vulkan_skinned_shadow_receiver_fragment_blob),
            .postprocess_vertex_shader = to_presentation_shader_bytecode(vulkan_postprocess_bytecode.vertex_shader),
            .postprocess_fragment_shader = to_presentation_shader_bytecode(vulkan_postprocess_bytecode.fragment_shader),
            .shadow_vertex_shader = to_presentation_shader_bytecode(vulkan_shadow_bytecode.vertex_shader),
            .shadow_fragment_shader = to_presentation_shader_bytecode(vulkan_shadow_bytecode.fragment_shader),
            .native_ui_overlay_vertex_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.vertex_shader),
            .native_ui_overlay_fragment_shader =
                to_presentation_shader_bytecode(vulkan_native_ui_overlay_bytecode.fragment_shader),
            .package = &*runtime_package,
            .packet = &packaged_scene->render_packet,
            .vertex_buffers = runtime_scene_vertex_buffers(),
            .vertex_attributes = runtime_scene_vertex_attributes(),
            .skinned_vertex_buffers = runtime_skinned_scene_vertex_buffers(),
            .skinned_vertex_attributes = runtime_skinned_scene_vertex_attributes(),
            .enable_postprocess = true,
            .enable_postprocess_depth_input = true,
            .enable_directional_shadow_smoke = options.require_directional_shadow,
            .enable_native_ui_overlay = options.require_native_ui_overlay,
            .native_ui_overlay_atlas_asset =
                options.require_native_ui_textured_sprite_atlas ? ui_atlas_metadata.atlas_page : mirakana::AssetId{},
            .enable_native_ui_overlay_textures = options.require_native_ui_textured_sprite_atlas,
        });
    }

    mirakana::Win32DesktopGameHostDesc host_desc{
        .title = "Sample Desktop Runtime Game",
        .extent = mirakana::WindowExtent{.width = 800, .height = 450},
        .prefer_vulkan = options.require_vulkan_renderer,
    };
    if (d3d12_scene_renderer.has_value()) {
        host_desc.d3d12_scene_renderer = &*d3d12_scene_renderer;
    }
    if (vulkan_scene_renderer.has_value()) {
        host_desc.vulkan_scene_renderer = &*vulkan_scene_renderer;
    }

    mirakana::Win32DesktopGameHost host(host_desc);
    if (options.require_d3d12_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::d3d12) {
        std::cout << "sample_desktop_runtime_game required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::Win32DesktopPresentationBackend::vulkan) {
        std::cout << "sample_desktop_runtime_game required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout << "sample_desktop_runtime_game required_scene_gpu_bindings_unavailable status="
                  << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::Win32DesktopPresentationPostprocessStatus::ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_unavailable status="
                  << mirakana::win32_desktop_presentation_postprocess_status_name(
                         host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_postprocess_depth_input && !host.presentation_report().postprocess_depth_input_ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_depth_input_unavailable ready=0\n";
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_directional_shadow && !host.presentation_report().directional_shadow_ready) {
        std::cout << "sample_desktop_runtime_game required_directional_shadow_unavailable status="
                  << mirakana::win32_desktop_presentation_directional_shadow_status_name(
                         host.presentation_report().directional_shadow_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
            std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
                      << mirakana::win32_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 9;
    }
    if (options.require_directional_shadow_filtering) {
        const auto report = host.presentation_report();
        if (report.directional_shadow_filter_mode !=
                mirakana::Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
            report.directional_shadow_filter_tap_count != 9 || report.directional_shadow_filter_radius_texels != 1.0F) {
            std::cout << "sample_desktop_runtime_game required_directional_shadow_filtering_unavailable mode="
                      << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
                             report.directional_shadow_filter_mode)
                      << " taps=" << report.directional_shadow_filter_tap_count
                      << " radius_texels=" << report.directional_shadow_filter_radius_texels << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_d3d12_shadow_cascade_policy) {
        const auto report = host.presentation_report();
        if (!directional_shadow_cascade_policy_matches(report)) {
            std::cout << "sample_desktop_runtime_game required_d3d12_shadow_cascade_policy_unavailable"
                      << " cascades=" << report.directional_shadow_cascade_count
                      << " tile_width=" << report.directional_shadow_cascade_tile_width
                      << " atlas_width=" << report.directional_shadow_atlas_width
                      << " atlas_height=" << report.directional_shadow_atlas_height
                      << " light_space_cascades=" << report.directional_shadow_light_space_cascades
                      << " cascade_splits=" << report.directional_shadow_cascade_splits << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_vulkan_shadow_cascade_policy) {
        const auto report = host.presentation_report();
        if (!vulkan_shadow_cascade_policy_ready(report)) {
            std::cout << "sample_desktop_runtime_game required_vulkan_shadow_cascade_policy_unavailable"
                      << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
                      << " cascades=" << report.directional_shadow_cascade_count
                      << " tile_width=" << report.directional_shadow_cascade_tile_width
                      << " atlas_width=" << report.directional_shadow_atlas_width
                      << " atlas_height=" << report.directional_shadow_atlas_height
                      << " light_space_cascades=" << report.directional_shadow_light_space_cascades
                      << " cascade_splits=" << report.directional_shadow_cascade_splits << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_native_ui_overlay && !host.presentation_report().native_ui_overlay_ready) {
        std::cout << "sample_desktop_runtime_game required_native_ui_overlay_unavailable status="
                  << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(
                         host.presentation_report().native_ui_overlay_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
            std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                      << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 10;
    }
    if (options.require_native_ui_textured_sprite_atlas) {
        const auto report = host.presentation_report();
        if (report.native_ui_texture_overlay_status !=
                mirakana::Win32DesktopPresentationNativeUiTextureOverlayStatus::ready ||
            !report.native_ui_texture_overlay_atlas_ready) {
            std::cout << "sample_desktop_runtime_game required_native_ui_textured_sprite_atlas_unavailable status="
                      << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                             report.native_ui_texture_overlay_status)
                      << " atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0) << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
                std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                          << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
                                 diagnostic.status)
                          << ": " << diagnostic.message << '\n';
            }
            return 11;
        }
    }

    SampleDesktopRuntimeGame game(host.input(), host.renderer(), options.throttle, std::move(packaged_scene),
                                  host.scene_gpu_bindings_ready(), std::move(packaged_quaternion_animation_tracks),
                                  options.require_lighting_shadow_policy,
                                  options.require_native_ui_textured_sprite_atlas, std::move(ui_atlas_metadata.palette),
                                  scene_mesh_instance_count(options), &host.presentation());
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;
    const auto postprocess_policy = mirakana::evaluate_win32_desktop_presentation_postprocess_policy(report);
    const auto d3d12_postprocess_execution = mirakana::evaluate_win32_desktop_presentation_d3d12_postprocess_execution(
        report, static_cast<std::uint64_t>(options.max_frames), options.require_d3d12_postprocess_evidence);
    const auto vulkan_postprocess_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_postprocess_execution(
            report, static_cast<std::uint64_t>(options.max_frames), options.require_vulkan_postprocess_evidence);
    const auto d3d12_instanced_draw_execution =
        mirakana::evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution(
            report, expected_d3d12_instanced_draw_instances(options));
    const auto vulkan_instanced_draw_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution(
            report, expected_vulkan_instanced_draw_instances(options));
    const auto scene_scale_policy = mirakana::evaluate_win32_desktop_presentation_scene_scale_policy(
        report, make_scene_scale_policy_desc(options, d3d12_instanced_draw_execution.ready ||
                                                          vulkan_instanced_draw_execution.ready));
    const auto d3d12_gpu_memory_execution = mirakana::evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution(
        report, options.require_d3d12_gpu_memory_evidence);
    const auto vulkan_gpu_memory_execution = mirakana::evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution(
        report, options.require_vulkan_gpu_memory_evidence);
    const auto gpu_memory_policy =
        mirakana::evaluate_win32_desktop_presentation_gpu_memory_policy(report, make_gpu_memory_policy_desc(options));
    const auto memory_diagnostics = summarize_package_memory_diagnostics(runtime_package, gpu_memory_policy);
    const auto d3d12_debug_profiling_execution =
        mirakana::evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution(
            report, options.require_d3d12_debug_profiling_evidence);
    const auto vulkan_debug_profiling_execution =
        mirakana::evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution(
            report, options.require_vulkan_debug_profiling_evidence);
    const auto debug_profiling_policy = mirakana::evaluate_win32_desktop_presentation_debug_profiling_policy(
        report, make_debug_profiling_policy_desc(options));
    const auto job_scheduling_evidence = build_package_job_scheduling_evidence(options.require_job_scheduling_evidence);
    const auto job_execution_topology_policy =
        build_package_job_execution_topology_policy_evidence(options.require_job_execution_topology_policy);
    const auto job_execution_foundation =
        build_package_job_execution_foundation_evidence(options.require_job_execution_foundation);
    const auto job_execution_work_stealing =
        build_package_job_execution_work_stealing_evidence(options.require_job_execution_work_stealing);
    const auto job_execution_placement_policy =
        build_package_job_execution_placement_policy_evidence(options.require_job_execution_placement_policy);
    const auto windows_cpu_set_worker_placement =
        build_package_windows_cpu_set_worker_placement_evidence(options.require_windows_cpu_set_worker_placement);
    const auto renderer_quality =
        mirakana::evaluate_win32_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));
    const bool framegraph_multiqueue_requested =
        options.require_framegraph_multiqueue_evidence || options.require_vulkan_framegraph_multiqueue_evidence;
    const auto framegraph_multiqueue = framegraph_multiqueue_requested
                                           ? run_frame_graph_multi_queue_package_evidence(host.presentation())
                                           : mirakana::FrameGraphRhiMultiQueuePackageEvidence{};
    const bool lighting_shadow_policy_ready = game.lighting_shadow_policy_passed(options.max_frames);

    std::cout
        << "sample_desktop_runtime_game status=" << status_name(result.status)
        << " renderer=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_requested=" << mirakana::win32_desktop_presentation_backend_name(report.requested_backend)
        << " presentation_selected=" << mirakana::win32_desktop_presentation_backend_name(report.selected_backend)
        << " presentation_fallback="
        << mirakana::win32_desktop_presentation_fallback_reason_name(report.fallback_reason)
        << " presentation_used_null_fallback=" << (report.used_null_fallback ? 1 : 0)
        << " presentation_backend_reports=" << report.backend_reports_count
        << " presentation_diagnostics=" << report.diagnostics_count << " frames=" << result.frames_run
        << " game_frames=" << game.frames() << " scene_meshes=" << game.scene_meshes_submitted()
        << " scene_materials=" << game.scene_materials_resolved()
        << " scene_mesh_plan_meshes=" << game.scene_mesh_plan_meshes()
        << " scene_mesh_plan_draws=" << game.scene_mesh_plan_draws()
        << " scene_mesh_plan_unique_meshes=" << game.scene_mesh_plan_unique_meshes()
        << " scene_mesh_plan_unique_materials=" << game.scene_mesh_plan_unique_materials()
        << " scene_mesh_plan_diagnostics=" << game.scene_mesh_plan_diagnostics()
        << " camera_primary=" << (game.primary_camera_seen() ? 1 : 0)
        << " camera_controller_ticks=" << game.camera_controller_ticks() << " camera_final_x=" << game.final_camera_x()
        << " quaternion_animation=" << (game.quaternion_animation_passed(options.max_frames) ? 1 : 0)
        << " quaternion_animation_ticks=" << game.quaternion_animation_ticks()
        << " quaternion_animation_tracks=" << game.quaternion_animation_tracks_sampled()
        << " quaternion_animation_failures=" << game.quaternion_animation_failures()
        << " quaternion_animation_scene_applied=" << game.quaternion_animation_scene_applied()
        << " quaternion_animation_final_z=" << game.final_quaternion_z()
        << " quaternion_animation_final_w=" << game.final_quaternion_w()
        << " quaternion_animation_scene_rotation_z=" << game.final_quaternion_scene_rotation_z()
        << " hud_boxes=" << game.hud_boxes_submitted() << " hud_images=" << game.hud_images_submitted()
        << " scene_gpu_status="
        << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
        << " scene_gpu_mesh_bindings=" << scene_gpu_stats.mesh_bindings
        << " scene_gpu_material_bindings=" << scene_gpu_stats.material_bindings
        << " scene_gpu_mesh_uploads=" << scene_gpu_stats.mesh_uploads
        << " scene_gpu_texture_uploads=" << scene_gpu_stats.texture_uploads
        << " scene_gpu_material_uploads=" << scene_gpu_stats.material_uploads
        << " scene_gpu_material_pipeline_layouts=" << scene_gpu_stats.material_pipeline_layouts
        << " scene_gpu_uploaded_texture_bytes=" << scene_gpu_stats.uploaded_texture_bytes
        << " scene_gpu_uploaded_mesh_bytes=" << scene_gpu_stats.uploaded_mesh_bytes
        << " scene_gpu_uploaded_material_factor_bytes=" << scene_gpu_stats.uploaded_material_factor_bytes
        << " scene_gpu_morph_mesh_bindings=" << scene_gpu_stats.morph_mesh_bindings
        << " scene_gpu_morph_mesh_uploads=" << scene_gpu_stats.morph_mesh_uploads
        << " scene_gpu_uploaded_morph_bytes=" << scene_gpu_stats.uploaded_morph_bytes
        << " scene_gpu_mesh_resolved=" << scene_gpu_stats.mesh_bindings_resolved
        << " scene_gpu_skinned_mesh_bindings=" << scene_gpu_stats.skinned_mesh_bindings
        << " scene_gpu_skinned_mesh_uploads=" << scene_gpu_stats.skinned_mesh_uploads
        << " scene_gpu_skinned_mesh_resolved=" << scene_gpu_stats.skinned_mesh_bindings_resolved
        << " scene_gpu_material_resolved=" << scene_gpu_stats.material_bindings_resolved << " postprocess_status="
        << mirakana::win32_desktop_presentation_postprocess_status_name(report.postprocess_status)
        << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
        << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
        << " postprocess_policy_status="
        << mirakana::win32_desktop_presentation_postprocess_policy_status_name(postprocess_policy.status)
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
        << (postprocess_policy.backend_shader_evidence_ready ? 1 : 0) << " postprocess_d3d12_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_postprocess_execution_status_name(
               d3d12_postprocess_execution.status)
        << " postprocess_d3d12_execution_ready=" << (d3d12_postprocess_execution.ready ? 1 : 0)
        << " postprocess_d3d12_execution_selected=" << (d3d12_postprocess_execution.d3d12_backend_selected ? 1 : 0)
        << " postprocess_d3d12_execution_shader_evidence_ready="
        << (d3d12_postprocess_execution.backend_shader_evidence_ready ? 1 : 0)
        << " postprocess_d3d12_execution_expected_passes=" << d3d12_postprocess_execution.expected_postprocess_passes
        << " postprocess_d3d12_execution_passes=" << d3d12_postprocess_execution.postprocess_passes_executed
        << " postprocess_d3d12_execution_passes_ok=" << (d3d12_postprocess_execution.postprocess_passes_current ? 1 : 0)
        << " vulkan_postprocess_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_postprocess_execution_status_name(
               vulkan_postprocess_execution.status)
        << " vulkan_postprocess_execution_ready=" << (vulkan_postprocess_execution.ready ? 1 : 0)
        << " vulkan_postprocess_execution_selected=" << (vulkan_postprocess_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_postprocess_execution_shader_evidence_ready="
        << (vulkan_postprocess_execution.backend_shader_evidence_ready ? 1 : 0)
        << " vulkan_postprocess_execution_expected_passes=" << vulkan_postprocess_execution.expected_postprocess_passes
        << " vulkan_postprocess_execution_passes=" << vulkan_postprocess_execution.postprocess_passes_executed
        << " vulkan_postprocess_execution_passes_ok="
        << (vulkan_postprocess_execution.postprocess_passes_current ? 1 : 0)
        << " d3d12_instanced_draw_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_instanced_draw_execution_status_name(
               d3d12_instanced_draw_execution.status)
        << " d3d12_instanced_draw_execution_ready=" << (d3d12_instanced_draw_execution.ready ? 1 : 0)
        << " d3d12_instanced_draw_execution_selected="
        << (d3d12_instanced_draw_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_instanced_draw_execution_expected_instances="
        << d3d12_instanced_draw_execution.expected_instances_submitted
        << " d3d12_instanced_draw_calls=" << d3d12_instanced_draw_execution.instanced_draw_calls
        << " d3d12_instanced_indexed_draw_calls=" << d3d12_instanced_draw_execution.instanced_indexed_draw_calls
        << " d3d12_instanced_instances_submitted=" << d3d12_instanced_draw_execution.instanced_instances_submitted
        << " d3d12_instanced_draws_ok=" << (d3d12_instanced_draw_execution.instanced_draws_current ? 1 : 0)
        << " d3d12_instanced_instances_ok=" << (d3d12_instanced_draw_execution.instanced_instances_current ? 1 : 0)
        << " vulkan_instanced_draw_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_instanced_draw_execution_status_name(
               vulkan_instanced_draw_execution.status)
        << " vulkan_instanced_draw_execution_ready=" << (vulkan_instanced_draw_execution.ready ? 1 : 0)
        << " vulkan_instanced_draw_execution_selected="
        << (vulkan_instanced_draw_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_instanced_draw_execution_expected_instances="
        << vulkan_instanced_draw_execution.expected_instances_submitted
        << " vulkan_instanced_draw_calls=" << vulkan_instanced_draw_execution.instanced_draw_calls
        << " vulkan_instanced_indexed_draw_calls=" << vulkan_instanced_draw_execution.instanced_indexed_draw_calls
        << " vulkan_instanced_instances_submitted=" << vulkan_instanced_draw_execution.instanced_instances_submitted
        << " vulkan_instanced_draws_ok=" << (vulkan_instanced_draw_execution.instanced_draws_current ? 1 : 0)
        << " vulkan_instanced_instances_ok=" << (vulkan_instanced_draw_execution.instanced_instances_current ? 1 : 0)
        << " rhi_instanced_draw_calls=" << report.rhi_instanced_draw_calls
        << " rhi_instanced_indexed_draw_calls=" << report.rhi_instanced_indexed_draw_calls
        << " rhi_instanced_instances_submitted=" << report.rhi_instanced_instances_submitted
        << " directional_shadow_status="
        << mirakana::win32_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
        << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
        << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
        << " directional_shadow_filter_mode="
        << mirakana::win32_desktop_presentation_directional_shadow_filter_mode_name(
               report.directional_shadow_filter_mode)
        << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
        << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
        << " directional_shadow_cascade_count=" << report.directional_shadow_cascade_count
        << " directional_shadow_cascade_tile_width=" << report.directional_shadow_cascade_tile_width
        << " directional_shadow_atlas_width=" << report.directional_shadow_atlas_width
        << " directional_shadow_atlas_height=" << report.directional_shadow_atlas_height
        << " directional_shadow_light_space_cascades=" << report.directional_shadow_light_space_cascades
        << " directional_shadow_cascade_splits=" << report.directional_shadow_cascade_splits
        << " d3d12_shadow_cascade_policy_ready="
        << (options.require_d3d12_shadow_cascade_policy && d3d12_shadow_cascade_policy_ready(report) ? 1 : 0)
        << " d3d12_shadow_cascade_policy_selected="
        << (options.require_d3d12_shadow_cascade_policy &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_shadow_cascade_policy_ready="
        << (options.require_vulkan_shadow_cascade_policy && vulkan_shadow_cascade_policy_ready(report) ? 1 : 0)
        << " vulkan_shadow_cascade_policy_selected="
        << (options.require_vulkan_shadow_cascade_policy &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0)
        << " lighting_shadow_policy_status="
        << lighting_shadow_policy_status_name(options.require_lighting_shadow_policy, lighting_shadow_policy_ready)
        << " lighting_shadow_policy_ready=" << (lighting_shadow_policy_ready ? 1 : 0)
        << " lighting_shadow_policy_diagnostics=" << game.lighting_shadow_policy_diagnostics()
        << " lighting_shadow_policy_lights=" << game.lighting_shadow_policy_light_count()
        << " lighting_shadow_policy_directional_lights=" << game.lighting_shadow_policy_directional_light_count()
        << " lighting_shadow_policy_point_lights=" << game.lighting_shadow_policy_point_light_count()
        << " lighting_shadow_policy_spot_lights=" << game.lighting_shadow_policy_spot_light_count()
        << " lighting_shadow_policy_shadowed_lights=" << game.lighting_shadow_policy_shadowed_light_count()
        << " lighting_shadow_policy_directional_cascades=" << game.lighting_shadow_policy_directional_cascade_count()
        << " lighting_shadow_policy_shadow_atlas_width=" << game.lighting_shadow_policy_shadow_atlas_width()
        << " lighting_shadow_policy_shadow_atlas_height=" << game.lighting_shadow_policy_shadow_atlas_height()
        << " lighting_shadow_policy_light_rows=" << game.lighting_shadow_policy_light_rows()
        << " lighting_shadow_policy_ready_frames=" << game.lighting_shadow_policy_ready_frames()
        << " scene_scale_policy_status="
        << mirakana::win32_desktop_presentation_scene_scale_policy_status_name(scene_scale_policy.status)
        << " scene_scale_policy_ready=" << (scene_scale_policy.ready ? 1 : 0)
        << " scene_scale_policy_diagnostics=" << scene_scale_policy.diagnostics_count
        << " scene_scale_policy_scene_resources_ready=" << (scene_scale_policy.scene_resources_ready ? 1 : 0)
        << " scene_scale_policy_expected_frames=" << scene_scale_policy.expected_frames
        << " scene_scale_policy_frames_finished=" << scene_scale_policy.frames_finished
        << " scene_scale_policy_frames_current=" << (scene_scale_policy.frames_current ? 1 : 0)
        << " scene_scale_policy_draw_groups=" << scene_scale_policy.draw_group_count
        << " scene_scale_policy_static_mesh_groups=" << scene_scale_policy.static_mesh_draw_groups
        << " scene_scale_policy_skinned_mesh_groups=" << scene_scale_policy.skinned_mesh_draw_groups
        << " scene_scale_policy_morph_mesh_groups=" << scene_scale_policy.morph_mesh_draw_groups
        << " scene_scale_policy_sprite_groups=" << scene_scale_policy.sprite_draw_groups
        << " scene_scale_policy_requested_instances=" << scene_scale_policy.requested_instance_count
        << " scene_scale_policy_visible_instances=" << scene_scale_policy.visible_instance_count
        << " scene_scale_policy_culled_instances=" << scene_scale_policy.culled_instance_count
        << " scene_scale_policy_draw_calls=" << scene_scale_policy.draw_call_count
        << " scene_scale_policy_instanced_draw_calls=" << scene_scale_policy.instanced_draw_call_count
        << " scene_scale_policy_instanced_visible_instances=" << scene_scale_policy.instanced_visible_instance_count
        << " scene_scale_policy_lod_groups=" << scene_scale_policy.lod_group_count
        << " scene_scale_policy_cpu_culling_groups=" << scene_scale_policy.cpu_culling_group_count
        << " scene_scale_policy_backend_instancing_evidence_required="
        << (scene_scale_policy.backend_instancing_evidence_required ? 1 : 0)
        << " scene_scale_policy_backend_instancing_evidence_ready="
        << (scene_scale_policy.backend_instancing_evidence_ready ? 1 : 0)
        << " scene_scale_policy_performance_measurement_required="
        << (scene_scale_policy.performance_measurement_required ? 1 : 0)
        << " scene_scale_policy_performance_measurement_ready="
        << (scene_scale_policy.performance_measurement_ready ? 1 : 0) << " gpu_memory_policy_status="
        << mirakana::win32_desktop_presentation_gpu_memory_policy_status_name(gpu_memory_policy.status)
        << " gpu_memory_policy_ready=" << (gpu_memory_policy.ready ? 1 : 0)
        << " gpu_memory_policy_diagnostics=" << gpu_memory_policy.diagnostics_count
        << " gpu_memory_policy_scene_resources_ready=" << (gpu_memory_policy.scene_resources_ready ? 1 : 0)
        << " gpu_memory_policy_expected_frames=" << gpu_memory_policy.expected_frames
        << " gpu_memory_policy_frames_finished=" << gpu_memory_policy.frames_finished
        << " gpu_memory_policy_frames_current=" << (gpu_memory_policy.frames_current ? 1 : 0)
        << " gpu_memory_policy_requests=" << gpu_memory_policy.request_count
        << " gpu_memory_policy_requested_bytes=" << gpu_memory_policy.total_requested_bytes
        << " gpu_memory_policy_counted_bytes=" << gpu_memory_policy.total_counted_bytes
        << " gpu_memory_policy_os_local_budget_bytes=" << gpu_memory_policy.os_local_budget_bytes
        << " gpu_memory_policy_os_local_usage_bytes=" << gpu_memory_policy.os_local_usage_bytes
        << " gpu_memory_policy_committed_byte_estimate=" << gpu_memory_policy.committed_byte_estimate
        << " gpu_memory_policy_transient_heap_allocations=" << gpu_memory_policy.transient_heap_allocations
        << " gpu_memory_policy_transient_placed_allocations=" << gpu_memory_policy.transient_placed_allocations
        << " gpu_memory_policy_transient_placed_resources_alive=" << gpu_memory_policy.transient_placed_resources_alive
        << " gpu_memory_policy_upload_bytes_written=" << gpu_memory_policy.upload_bytes_written
        << " gpu_memory_policy_transient_heap_requests=" << gpu_memory_policy.transient_heap_request_count
        << " gpu_memory_policy_upload_pressure_requests=" << gpu_memory_policy.upload_pressure_request_count
        << " gpu_memory_policy_declared_budget_requests=" << gpu_memory_policy.declared_budget_request_count
        << " gpu_memory_policy_residency_pressure_requests=" << gpu_memory_policy.residency_pressure_request_count
        << " gpu_memory_policy_package_counter_requests=" << gpu_memory_policy.package_counter_request_count
        << " gpu_memory_policy_memory_budget_evidence_ready="
        << (gpu_memory_policy.memory_budget_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_residency_pressure_evidence_ready="
        << (gpu_memory_policy.residency_pressure_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_package_counter_evidence_ready="
        << (gpu_memory_policy.package_counter_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_residency_pressure_events=" << gpu_memory_policy.residency_pressure_event_count
        << " gpu_memory_policy_backend_memory_evidence_required="
        << (gpu_memory_policy.backend_memory_evidence_required ? 1 : 0)
        << " gpu_memory_policy_backend_memory_evidence_ready="
        << (gpu_memory_policy.backend_memory_evidence_ready ? 1 : 0)
        << " gpu_memory_policy_os_video_memory_budget_required="
        << (gpu_memory_policy.os_video_memory_budget_required ? 1 : 0)
        << " gpu_memory_policy_os_video_memory_budget_available="
        << (gpu_memory_policy.os_video_memory_budget_available ? 1 : 0)
        << " memory_diagnostics_status=" << mirakana::memory_diagnostics_status_label(memory_diagnostics.status)
        << " memory_diagnostics_ready="
        << (memory_diagnostics.status == mirakana::MemoryDiagnosticsStatus::ready ? 1 : 0)
        << " memory_diagnostics_rows=" << memory_diagnostics.row_count
        << " memory_diagnostics_classes=" << memory_diagnostics.class_summaries.size()
        << " memory_diagnostics_total_bytes=" << memory_diagnostics.total_bytes
        << " memory_diagnostics_high_water_bytes=" << memory_diagnostics.high_water_bytes
        << " memory_diagnostics_total_allocation_count=" << memory_diagnostics.total_allocation_count
        << " memory_diagnostics_diagnostics=" << memory_diagnostics.diagnostics.size()
        << " memory_diagnostics_budgeted_classes=" << memory_diagnostics_budgeted_class_count(memory_diagnostics)
        << " memory_diagnostics_budget_pressure_classes="
        << memory_diagnostics_pressure_class_count(memory_diagnostics, mirakana::MemoryBudgetPressure::warning)
        << " memory_diagnostics_budget_exceeded_classes="
        << memory_diagnostics_pressure_class_count(memory_diagnostics, mirakana::MemoryBudgetPressure::exceeded)
        << " memory_diagnostics_invalid_counter="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::invalid_counter) ? 1
                                                                                                                 : 0)
        << " memory_diagnostics_stale_generation="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::stale_generation) ? 1
                                                                                                                  : 0)
        << " memory_diagnostics_use_after_safe_point="
        << (memory_diagnostic_code_present(memory_diagnostics, mirakana::MemoryDiagnosticsCode::use_after_safe_point)
                ? 1
                : 0)
        << " memory_diagnostics_package_resident_cpu_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::package_resident_cpu)
        << " memory_diagnostics_package_resident_cpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::package_resident_cpu)
        << " memory_diagnostics_resident_gpu_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_budget_bytes="
        << memory_class_budget(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_resident_gpu_pressure="
        << memory_class_pressure(memory_diagnostics, mirakana::MemoryLifetimeClass::resident_gpu)
        << " memory_diagnostics_upload_staging_bytes="
        << memory_class_bytes(memory_diagnostics, mirakana::MemoryLifetimeClass::upload_staging)
        << " memory_diagnostics_upload_staging_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::upload_staging)
        << " memory_diagnostics_transient_gpu_allocations="
        << memory_class_allocations(memory_diagnostics, mirakana::MemoryLifetimeClass::transient_gpu)
        << " memory_diagnostics_transient_gpu_aliasing_barriers="
        << memory_diagnostics_transient_gpu_aliasing_barriers(framegraph_multiqueue_requested, framegraph_multiqueue)
        << " memory_diagnostics_transient_gpu_framegraph_aliasing_ready="
        << (memory_diagnostics_transient_gpu_framegraph_aliasing_ready(framegraph_multiqueue_requested,
                                                                       framegraph_multiqueue)
                ? 1
                : 0)
        << " d3d12_gpu_memory_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_gpu_memory_execution_status_name(
               d3d12_gpu_memory_execution.status)
        << " d3d12_gpu_memory_execution_ready=" << (d3d12_gpu_memory_execution.ready ? 1 : 0)
        << " d3d12_gpu_memory_execution_selected=" << (d3d12_gpu_memory_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_gpu_memory_execution_os_video_memory_budget_available="
        << (d3d12_gpu_memory_execution.os_video_memory_budget_available ? 1 : 0)
        << " d3d12_gpu_memory_execution_committed_byte_estimate_available="
        << (d3d12_gpu_memory_execution.committed_byte_estimate_available ? 1 : 0)
        << " d3d12_gpu_memory_execution_local_video_memory_budget_bytes="
        << d3d12_gpu_memory_execution.local_video_memory_budget_bytes
        << " d3d12_gpu_memory_execution_local_video_memory_usage_bytes="
        << d3d12_gpu_memory_execution.local_video_memory_usage_bytes
        << " d3d12_gpu_memory_execution_committed_resources_byte_estimate="
        << d3d12_gpu_memory_execution.committed_resources_byte_estimate
        << " d3d12_gpu_memory_execution_transient_heap_allocations="
        << d3d12_gpu_memory_execution.transient_heap_allocations
        << " d3d12_gpu_memory_execution_transient_placed_allocations="
        << d3d12_gpu_memory_execution.transient_placed_allocations
        << " d3d12_gpu_memory_execution_transient_placed_resources_alive="
        << d3d12_gpu_memory_execution.transient_placed_resources_alive
        << " d3d12_gpu_memory_execution_upload_bytes_written=" << d3d12_gpu_memory_execution.upload_bytes_written
        << " d3d12_gpu_memory_execution_budget_ok=" << (d3d12_gpu_memory_execution.memory_budget_current ? 1 : 0)
        << " d3d12_gpu_memory_execution_transient_heap_ok="
        << (d3d12_gpu_memory_execution.transient_heap_current ? 1 : 0) << " vulkan_gpu_memory_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_gpu_memory_execution_status_name(
               vulkan_gpu_memory_execution.status)
        << " vulkan_gpu_memory_execution_ready=" << (vulkan_gpu_memory_execution.ready ? 1 : 0)
        << " vulkan_gpu_memory_execution_selected=" << (vulkan_gpu_memory_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_gpu_memory_execution_committed_byte_estimate_available="
        << (vulkan_gpu_memory_execution.committed_byte_estimate_available ? 1 : 0)
        << " vulkan_gpu_memory_execution_committed_resources_byte_estimate="
        << vulkan_gpu_memory_execution.committed_resources_byte_estimate
        << " vulkan_gpu_memory_execution_transient_heap_allocations="
        << vulkan_gpu_memory_execution.transient_heap_allocations
        << " vulkan_gpu_memory_execution_transient_placed_allocations="
        << vulkan_gpu_memory_execution.transient_placed_allocations
        << " vulkan_gpu_memory_execution_transient_placed_resources_alive="
        << vulkan_gpu_memory_execution.transient_placed_resources_alive
        << " vulkan_gpu_memory_execution_upload_bytes_written=" << vulkan_gpu_memory_execution.upload_bytes_written
        << " vulkan_gpu_memory_execution_framegraph_barrier_steps_executed="
        << vulkan_gpu_memory_execution.framegraph_barrier_steps_executed
        << " vulkan_gpu_memory_execution_budget_ok=" << (vulkan_gpu_memory_execution.memory_budget_current ? 1 : 0)
        << " vulkan_gpu_memory_execution_transient_heap_ok="
        << (vulkan_gpu_memory_execution.transient_heap_current ? 1 : 0) << " debug_profiling_policy_status="
        << mirakana::win32_desktop_presentation_debug_profiling_policy_status_name(debug_profiling_policy.status)
        << " debug_profiling_policy_ready=" << (debug_profiling_policy.ready ? 1 : 0)
        << " debug_profiling_policy_diagnostics=" << debug_profiling_policy.diagnostics_count
        << " debug_profiling_policy_scene_resources_ready=" << (debug_profiling_policy.scene_resources_ready ? 1 : 0)
        << " debug_profiling_policy_expected_frames=" << debug_profiling_policy.expected_frames
        << " debug_profiling_policy_frames_finished=" << debug_profiling_policy.frames_finished
        << " debug_profiling_policy_frames_current=" << (debug_profiling_policy.frames_current ? 1 : 0)
        << " debug_profiling_policy_requests=" << debug_profiling_policy.request_count
        << " debug_profiling_policy_gpu_timestamp_ticks_per_second="
        << debug_profiling_policy.gpu_timestamp_ticks_per_second
        << " debug_profiling_policy_gpu_debug_scopes_begun=" << debug_profiling_policy.gpu_debug_scopes_begun
        << " debug_profiling_policy_gpu_debug_scopes_ended=" << debug_profiling_policy.gpu_debug_scopes_ended
        << " debug_profiling_policy_gpu_debug_markers_inserted=" << debug_profiling_policy.gpu_debug_markers_inserted
        << " debug_profiling_policy_framegraph_barrier_steps_executed="
        << debug_profiling_policy.framegraph_barrier_steps_executed
        << " debug_profiling_policy_framegraph_render_passes_recorded="
        << debug_profiling_policy.framegraph_render_passes_recorded
        << " debug_profiling_policy_gpu_timestamp_requests=" << debug_profiling_policy.gpu_timestamp_request_count
        << " debug_profiling_policy_gpu_debug_marker_requests=" << debug_profiling_policy.gpu_debug_marker_request_count
        << " debug_profiling_policy_capture_handoff_requests=" << debug_profiling_policy.capture_handoff_request_count
        << " debug_profiling_policy_cpu_profile_zones=" << debug_profiling_policy.cpu_profile_zone_count
        << " debug_profiling_policy_trace_capture_handoff_rows="
        << debug_profiling_policy.trace_capture_handoff_row_count
        << " debug_profiling_policy_cpu_profile_zone_requests=" << debug_profiling_policy.cpu_profile_zone_request_count
        << " debug_profiling_policy_trace_capture_handoff_requests="
        << debug_profiling_policy.trace_capture_handoff_request_count
        << " debug_profiling_policy_package_counter_requests=" << debug_profiling_policy.package_counter_request_count
        << " debug_profiling_policy_cpu_profile_zone_evidence_ready="
        << (debug_profiling_policy.cpu_profile_zone_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_trace_capture_handoff_evidence_ready="
        << (debug_profiling_policy.trace_capture_handoff_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_package_counter_evidence_ready="
        << (debug_profiling_policy.package_counter_evidence_ready ? 1 : 0)
        << " debug_profiling_policy_backend_profiling_evidence_required="
        << (debug_profiling_policy.backend_profiling_evidence_required ? 1 : 0)
        << " debug_profiling_policy_backend_profiling_evidence_ready="
        << (debug_profiling_policy.backend_profiling_evidence_ready ? 1 : 0)
        << " d3d12_debug_profiling_execution_status="
        << mirakana::win32_desktop_presentation_d3d12_debug_profiling_execution_status_name(
               d3d12_debug_profiling_execution.status)
        << " d3d12_debug_profiling_execution_ready=" << (d3d12_debug_profiling_execution.ready ? 1 : 0)
        << " d3d12_debug_profiling_execution_selected="
        << (d3d12_debug_profiling_execution.d3d12_backend_selected ? 1 : 0)
        << " d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second="
        << d3d12_debug_profiling_execution.gpu_timestamp_ticks_per_second
        << " d3d12_debug_profiling_execution_gpu_debug_scopes_begun="
        << d3d12_debug_profiling_execution.gpu_debug_scopes_begun
        << " d3d12_debug_profiling_execution_gpu_debug_scopes_ended="
        << d3d12_debug_profiling_execution.gpu_debug_scopes_ended
        << " d3d12_debug_profiling_execution_gpu_debug_markers_inserted="
        << d3d12_debug_profiling_execution.gpu_debug_markers_inserted
        << " d3d12_debug_profiling_execution_framegraph_barrier_steps_executed="
        << d3d12_debug_profiling_execution.framegraph_barrier_steps_executed
        << " d3d12_debug_profiling_execution_framegraph_render_passes_recorded="
        << d3d12_debug_profiling_execution.framegraph_render_passes_recorded
        << " d3d12_debug_profiling_execution_gpu_timestamps_ok="
        << (d3d12_debug_profiling_execution.gpu_timestamps_current ? 1 : 0)
        << " d3d12_debug_profiling_execution_gpu_debug_markers_ok="
        << (d3d12_debug_profiling_execution.gpu_debug_markers_current ? 1 : 0)
        << " d3d12_debug_profiling_execution_frame_diagnostics_ok="
        << (d3d12_debug_profiling_execution.frame_diagnostics_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_status="
        << mirakana::win32_desktop_presentation_vulkan_debug_profiling_execution_status_name(
               vulkan_debug_profiling_execution.status)
        << " vulkan_debug_profiling_execution_ready=" << (vulkan_debug_profiling_execution.ready ? 1 : 0)
        << " vulkan_debug_profiling_execution_selected="
        << (vulkan_debug_profiling_execution.vulkan_backend_selected ? 1 : 0)
        << " vulkan_debug_profiling_execution_gpu_timestamp_ticks_per_second="
        << vulkan_debug_profiling_execution.gpu_timestamp_ticks_per_second
        << " vulkan_debug_profiling_execution_gpu_debug_scopes_begun="
        << vulkan_debug_profiling_execution.gpu_debug_scopes_begun
        << " vulkan_debug_profiling_execution_gpu_debug_scopes_ended="
        << vulkan_debug_profiling_execution.gpu_debug_scopes_ended
        << " vulkan_debug_profiling_execution_gpu_debug_markers_inserted="
        << vulkan_debug_profiling_execution.gpu_debug_markers_inserted
        << " vulkan_debug_profiling_execution_framegraph_barrier_steps_executed="
        << vulkan_debug_profiling_execution.framegraph_barrier_steps_executed
        << " vulkan_debug_profiling_execution_framegraph_render_passes_recorded="
        << vulkan_debug_profiling_execution.framegraph_render_passes_recorded
        << " vulkan_debug_profiling_execution_gpu_timestamps_ok="
        << (vulkan_debug_profiling_execution.gpu_timestamps_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_gpu_debug_markers_ok="
        << (vulkan_debug_profiling_execution.gpu_debug_markers_current ? 1 : 0)
        << " vulkan_debug_profiling_execution_frame_diagnostics_ok="
        << (vulkan_debug_profiling_execution.frame_diagnostics_current ? 1 : 0) << " job_scheduling_evidence_status="
        << job_scheduling_evidence_status_name(options.require_job_scheduling_evidence, job_scheduling_evidence)
        << " job_scheduling_evidence_ready=" << (job_scheduling_evidence_ready(job_scheduling_evidence) ? 1 : 0)
        << " job_scheduling_evidence_diagnostics=" << job_scheduling_evidence.scheduling_summary.diagnostics.size()
        << " job_scheduling_evidence_scratch_diagnostics=" << job_scheduling_evidence.scratch_summary.diagnostics.size()
        << " job_scheduling_evidence_topology_rows="
        << job_scheduling_evidence.scheduling_summary.worker_topology_row_count
        << " job_scheduling_evidence_worker_count=" << job_scheduling_evidence.scheduling_summary.worker_count
        << " job_scheduling_evidence_queue_count=" << job_scheduling_evidence.scheduling_summary.queue_count
        << " job_scheduling_evidence_queue_rows=" << job_scheduling_evidence.queue_rows.size()
        << " job_scheduling_evidence_submitted_jobs=" << job_scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_scheduling_evidence_completed_jobs=" << job_scheduling_evidence.scheduling_summary.total_completed_jobs
        << " job_scheduling_evidence_execution_rows=" << job_scheduling_evidence.execution_order.size()
        << " job_scheduling_evidence_deterministic_merges="
        << job_scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_scheduling_evidence_nondeterministic_merges="
        << job_scheduling_evidence.scheduling_summary.total_nondeterministic_merge_count
        << " job_scheduling_evidence_blocked_dependencies="
        << job_scheduling_evidence.scheduling_summary.total_blocked_dependency_count
        << " job_scheduling_evidence_dependency_cycles="
        << job_scheduling_evidence.scheduling_summary.total_dependency_cycle_count
        << " job_scheduling_evidence_queue_overflows="
        << job_scheduling_evidence.scheduling_summary.total_queue_overflow_count
        << " job_scheduling_evidence_scratch_misuse="
        << job_scheduling_evidence.scheduling_summary.total_scratch_misuse_count
        << " job_scheduling_evidence_scratch_rows=" << job_scheduling_evidence.worker_scratch_rows.size()
        << " job_scheduling_evidence_scratch_bytes=" << job_scheduling_evidence.scratch_summary.total_bytes
        << " job_scheduling_evidence_scratch_high_water_bytes="
        << job_scheduling_evidence.scratch_summary.high_water_bytes
        << " job_scheduling_evidence_native_threads_started=0"
        << " job_scheduling_evidence_thread_pool_started=0"
        << " job_scheduling_evidence_affinity_policy_applied=0"
        << " job_scheduling_evidence_numa_policy_applied=0"
        << " job_scheduling_evidence_simd_dispatch_applied=0"
        << " job_scheduling_evidence_gpu_async_overlap_applied=0"
        << " job_execution_topology_policy_status="
        << job_execution_topology_policy_status_name(options.require_job_execution_topology_policy,
                                                     job_execution_topology_policy)
        << " job_execution_topology_policy_ready="
        << (job_execution_topology_policy_ready(job_execution_topology_policy) ? 1 : 0)
        << " job_execution_topology_policy_diagnostics=" << job_execution_topology_policy.diagnostics.size()
        << " job_execution_topology_policy_observed_logical_processors="
        << job_execution_topology_policy.observed_logical_processor_count
        << " job_execution_topology_policy_effective_logical_processors="
        << job_execution_topology_policy.effective_logical_processor_count
        << " job_execution_topology_policy_selected_worker_count="
        << job_execution_topology_policy.selected_worker_count
        << " job_execution_topology_policy_worker_count_limit=" << job_execution_topology_policy.worker_count_limit
        << " job_execution_topology_policy_reserved_logical_processors="
        << job_execution_topology_policy.reserved_logical_processor_count
        << " job_execution_topology_policy_fallback_used="
        << (job_execution_topology_policy.hardware_concurrency_fallback_used ? 1 : 0)
        << " job_execution_topology_policy_worker_count_limited_by_cap="
        << (job_execution_topology_policy.worker_count_limited_by_cap ? 1 : 0)
        << " job_execution_topology_policy_requested_worker_count_used="
        << (job_execution_topology_policy.requested_worker_count_used ? 1 : 0)
        << " job_execution_topology_policy_clamped_to_logical_processors="
        << (job_execution_topology_policy.worker_count_clamped_to_logical_processors ? 1 : 0)
        << " job_execution_topology_policy_processor_group_count="
        << job_execution_topology_policy.topology_row.processor_group_count
        << " job_execution_topology_policy_numa_node_count="
        << job_execution_topology_policy.topology_row.numa_node_count
        << " job_execution_topology_policy_processor_group_policy_applied=0"
        << " job_execution_topology_policy_numa_policy_applied=0"
        << " job_execution_topology_policy_affinity_policy_applied=0"
        << " job_execution_topology_policy_simd_dispatch_applied=0"
        << " job_execution_topology_policy_gpu_async_overlap_applied=0"
        << " job_execution_topology_policy_cuda_path_used=0"
        << " job_execution_topology_policy_hip_path_used=0"
        << " job_execution_topology_policy_sycl_path_used=0"
        << " job_execution_foundation_status=" << job_execution_foundation_status_name(job_execution_foundation)
        << " job_execution_foundation_ready=" << (job_execution_foundation_ready(job_execution_foundation) ? 1 : 0)
        << " job_execution_foundation_pool_status="
        << mirakana::job_execution_pool_status_label(job_execution_foundation.pool_status)
        << " job_execution_foundation_run_status="
        << mirakana::job_execution_run_status_label(job_execution_foundation.run_result.status)
        << " job_execution_foundation_diagnostics="
        << job_execution_foundation_diagnostic_count(job_execution_foundation)
        << " job_execution_foundation_worker_threads_started="
        << job_execution_foundation.run_result.worker_threads_started << " job_execution_foundation_tasks_submitted="
        << job_execution_foundation.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_execution_foundation_tasks_executed=" << job_execution_foundation.run_result.tasks_executed
        << " job_execution_foundation_tasks_failed=" << job_execution_foundation.run_result.tasks_failed
        << " job_execution_foundation_task_side_effects=" << job_execution_foundation.task_side_effects
        << " job_execution_foundation_execution_rows="
        << job_execution_foundation.run_result.scheduling_evidence.execution_order.size()
        << " job_execution_foundation_queue_rows="
        << job_execution_foundation.run_result.scheduling_evidence.queue_rows.size()
        << " job_execution_foundation_deterministic_merges="
        << job_execution_foundation.run_result.scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_execution_foundation_scratch_rows="
        << job_execution_foundation.run_result.scheduling_evidence.worker_scratch_rows.size()
        << " job_execution_foundation_scratch_bytes="
        << job_execution_foundation.run_result.scheduling_evidence.scratch_summary.total_bytes
        << " job_execution_foundation_scratch_high_water_bytes="
        << job_execution_foundation.run_result.scheduling_evidence.scratch_summary.high_water_bytes
        << " job_execution_foundation_work_stealing_applied=0"
        << " job_execution_foundation_affinity_policy_applied=0"
        << " job_execution_foundation_numa_policy_applied=0"
        << " job_execution_foundation_simd_dispatch_applied=0"
        << " job_execution_foundation_gpu_async_overlap_applied=0"
        << " job_execution_foundation_cuda_path_used=0"
        << " job_execution_foundation_hip_path_used=0"
        << " job_execution_foundation_sycl_path_used=0"
        << " job_execution_work_stealing_status="
        << job_execution_work_stealing_status_name(job_execution_work_stealing) << " job_execution_work_stealing_ready="
        << (job_execution_work_stealing_ready(job_execution_work_stealing) ? 1 : 0)
        << " job_execution_work_stealing_pool_status="
        << mirakana::job_execution_pool_status_label(job_execution_work_stealing.pool_status)
        << " job_execution_work_stealing_run_status="
        << mirakana::job_execution_run_status_label(job_execution_work_stealing.run_result.status)
        << " job_execution_work_stealing_diagnostics="
        << job_execution_work_stealing_diagnostic_count(job_execution_work_stealing)
        << " job_execution_work_stealing_worker_threads_started="
        << job_execution_work_stealing.run_result.worker_threads_started
        << " job_execution_work_stealing_tasks_submitted="
        << job_execution_work_stealing.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " job_execution_work_stealing_tasks_executed=" << job_execution_work_stealing.run_result.tasks_executed
        << " job_execution_work_stealing_tasks_failed=" << job_execution_work_stealing.run_result.tasks_failed
        << " job_execution_work_stealing_task_side_effects=" << job_execution_work_stealing.task_side_effects
        << " job_execution_work_stealing_execution_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.execution_order.size()
        << " job_execution_work_stealing_queue_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.queue_rows.size()
        << " job_execution_work_stealing_deterministic_merges="
        << job_execution_work_stealing.run_result.scheduling_evidence.scheduling_summary.total_deterministic_merge_count
        << " job_execution_work_stealing_steal_attempts=" << job_execution_work_stealing.run_result.steal_attempt_count
        << " job_execution_work_stealing_steal_successes=" << job_execution_work_stealing.run_result.steal_success_count
        << " job_execution_work_stealing_worker_waits=" << job_execution_work_stealing.run_result.worker_wait_count
        << " job_execution_work_stealing_scratch_rows="
        << job_execution_work_stealing.run_result.scheduling_evidence.worker_scratch_rows.size()
        << " job_execution_work_stealing_scratch_bytes="
        << job_execution_work_stealing.run_result.scheduling_evidence.scratch_summary.total_bytes
        << " job_execution_work_stealing_scratch_high_water_bytes="
        << job_execution_work_stealing.run_result.scheduling_evidence.scratch_summary.high_water_bytes
        << " job_execution_work_stealing_applied="
        << (job_execution_work_stealing.run_result.work_stealing_applied ? 1 : 0)
        << " job_execution_work_stealing_affinity_policy_applied=0"
        << " job_execution_work_stealing_numa_policy_applied=0"
        << " job_execution_work_stealing_simd_dispatch_applied=0"
        << " job_execution_work_stealing_gpu_async_overlap_applied=0"
        << " job_execution_work_stealing_cuda_path_used=0"
        << " job_execution_work_stealing_hip_path_used=0"
        << " job_execution_work_stealing_sycl_path_used=0"
        << " job_execution_placement_policy_status="
        << job_execution_placement_policy_status_name(job_execution_placement_policy)
        << " job_execution_placement_policy_ready="
        << (job_execution_placement_policy_ready(job_execution_placement_policy) ? 1 : 0)
        << " job_execution_placement_policy_requested_mode="
        << mirakana::job_execution_placement_policy_mode_label(
               job_execution_placement_policy.ready_policy.requested_mode)
        << " job_execution_placement_policy_selected_mode="
        << mirakana::job_execution_placement_policy_mode_label(
               job_execution_placement_policy.ready_policy.selected_mode)
        << " job_execution_placement_policy_diagnostics="
        << job_execution_placement_policy.ready_policy.diagnostics.size()
        << " job_execution_placement_policy_host_evidence_required_diagnostics="
        << job_execution_placement_policy.host_evidence_policy.diagnostic_codes.size()
        << " job_execution_placement_policy_inherited_worker_count="
        << job_execution_placement_policy.ready_policy.inherited_worker_count
        << " job_execution_placement_policy_inherited_work_stealing_enabled="
        << (job_execution_placement_policy.ready_policy.pool_desc.work_stealing_enabled ? 1 : 0)
        << " job_execution_placement_policy_numa_node_count="
        << job_execution_placement_policy.ready_policy.numa_node_count
        << " job_execution_placement_policy_performance_core_count="
        << job_execution_placement_policy.ready_policy.performance_core_count
        << " job_execution_placement_policy_efficiency_core_count="
        << job_execution_placement_policy.ready_policy.efficiency_core_count
        << " job_execution_placement_policy_smt_sibling_topology_known="
        << (job_execution_placement_policy.ready_policy.smt_sibling_topology_known ? 1 : 0)
        << " job_execution_placement_policy_affinity_policy_applied="
        << (job_execution_placement_policy.ready_policy.affinity_policy_applied ? 1 : 0)
        << " job_execution_placement_policy_numa_policy_applied="
        << (job_execution_placement_policy.ready_policy.numa_policy_applied ? 1 : 0)
        << " job_execution_placement_policy_simd_dispatch_applied="
        << (job_execution_placement_policy.ready_policy.simd_dispatch_applied ? 1 : 0)
        << " job_execution_placement_policy_gpu_async_overlap_applied="
        << (job_execution_placement_policy.ready_policy.gpu_async_overlap_applied ? 1 : 0)
        << " job_execution_placement_policy_cuda_path_used=0"
        << " job_execution_placement_policy_hip_path_used=0"
        << " job_execution_placement_policy_sycl_path_used=0"
        << " windows_cpu_set_worker_placement_status="
        << windows_cpu_set_worker_placement_status_name(windows_cpu_set_worker_placement)
        << " windows_cpu_set_worker_placement_ready="
        << (windows_cpu_set_worker_placement_ready(windows_cpu_set_worker_placement) ? 1 : 0)
        << " windows_cpu_set_worker_placement_pool_status="
        << mirakana::job_execution_pool_status_label(windows_cpu_set_worker_placement.pool_status)
        << " windows_cpu_set_worker_placement_run_status="
        << mirakana::job_execution_run_status_label(windows_cpu_set_worker_placement.run_result.status)
        << " windows_cpu_set_worker_placement_diagnostics="
        << windows_cpu_set_worker_placement_diagnostic_count(windows_cpu_set_worker_placement)
        << " windows_cpu_set_worker_placement_topology_rows="
        << windows_cpu_set_worker_placement.run_result.scheduling_evidence.scheduling_summary.worker_topology_row_count
        << " windows_cpu_set_worker_placement_selected_cpu_sets="
        << windows_cpu_set_worker_placement.placement_plan.selected_cpu_set_count
        << " windows_cpu_set_worker_placement_worker_rows="
        << windows_cpu_set_worker_placement.placement_plan.worker_rows.size()
        << " windows_cpu_set_worker_placement_worker_threads_started="
        << windows_cpu_set_worker_placement.run_result.worker_threads_started
        << " windows_cpu_set_worker_placement_attempts="
        << windows_cpu_set_worker_placement.run_result.worker_placement_attempt_count
        << " windows_cpu_set_worker_placement_applied="
        << windows_cpu_set_worker_placement.run_result.worker_placement_applied_count
        << " windows_cpu_set_worker_placement_selected_cpu_set_applications="
        << windows_cpu_set_worker_placement.run_result.worker_placement_selected_cpu_set_count
        << " windows_cpu_set_worker_placement_tasks_submitted="
        << windows_cpu_set_worker_placement.run_result.scheduling_evidence.scheduling_summary.total_submitted_jobs
        << " windows_cpu_set_worker_placement_tasks_executed="
        << windows_cpu_set_worker_placement.run_result.tasks_executed
        << " windows_cpu_set_worker_placement_tasks_failed=" << windows_cpu_set_worker_placement.run_result.tasks_failed
        << " windows_cpu_set_worker_placement_task_side_effects=" << windows_cpu_set_worker_placement.task_side_effects
        << " windows_cpu_set_worker_placement_native_thread_handles_exposed=0"
        << " windows_cpu_set_worker_placement_linux_affinity_applied=0"
        << " windows_cpu_set_worker_placement_numa_allocation_applied=0"
        << " windows_cpu_set_worker_placement_hybrid_smt_policy_applied=0"
        << " windows_cpu_set_worker_placement_simd_dispatch_applied=0"
        << " windows_cpu_set_worker_placement_gpu_async_overlap_applied=0"
        << " windows_cpu_set_worker_placement_cuda_path_used=0"
        << " windows_cpu_set_worker_placement_hip_path_used=0"
        << " windows_cpu_set_worker_placement_sycl_path_used=0"
        << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
        << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
        << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
        << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
        << " ui_overlay_draws=" << report.native_ui_overlay_draws
        << " ui_atlas_metadata_requested=" << (options.require_native_ui_textured_sprite_atlas ? 1 : 0)
        << " ui_atlas_metadata_status=" << ui_atlas_metadata_status_name(ui_atlas_metadata.status)
        << " ui_atlas_metadata_pages=" << ui_atlas_metadata.pages
        << " ui_atlas_metadata_bindings=" << ui_atlas_metadata.bindings
        << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
        << " ui_texture_overlay_status="
        << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(
               report.native_ui_texture_overlay_status)
        << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
        << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
        << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
        << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
        << " framegraph_passes=" << report.framegraph_passes
        << " framegraph_passes_executed=" << report.renderer_stats.framegraph_passes_executed
        << " framegraph_render_passes_recorded=" << report.renderer_stats.framegraph_render_passes_recorded
        << " framegraph_barrier_steps_executed=" << report.renderer_stats.framegraph_barrier_steps_executed
        << " framegraph_multiqueue_status="
        << frame_graph_multi_queue_status_name(framegraph_multiqueue_requested, framegraph_multiqueue)
        << " d3d12_framegraph_multiqueue_evidence_ready="
        << (options.require_framegraph_multiqueue_evidence &&
                    d3d12_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)
                ? 1
                : 0)
        << " d3d12_framegraph_multiqueue_evidence_selected="
        << (options.require_framegraph_multiqueue_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_framegraph_multiqueue_evidence_ready="
        << (options.require_vulkan_framegraph_multiqueue_evidence &&
                    vulkan_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)
                ? 1
                : 0)
        << " vulkan_framegraph_multiqueue_evidence_selected="
        << (options.require_vulkan_framegraph_multiqueue_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0)
        << " framegraph_multiqueue_ready=" << (framegraph_multiqueue.ready ? 1 : 0)
        << " framegraph_multiqueue_diagnostics=" << framegraph_multiqueue.diagnostics.size()
        << " framegraph_multiqueue_command_lists_submitted=" << framegraph_multiqueue.command_lists_submitted
        << " framegraph_multiqueue_queue_waits_recorded=" << framegraph_multiqueue.queue_waits_recorded
        << " framegraph_multiqueue_barriers_recorded=" << framegraph_multiqueue.barriers_recorded
        << " framegraph_multiqueue_aliasing_barriers_recorded=" << framegraph_multiqueue.aliasing_barriers_recorded
        << " framegraph_multiqueue_pass_callbacks_invoked=" << framegraph_multiqueue.pass_callbacks_invoked
        << " framegraph_multiqueue_submitted_pass_fences=" << framegraph_multiqueue.submitted_pass_fences
        << " framegraph_multiqueue_copy_queue_submits=" << framegraph_multiqueue.copy_queue_submits
        << " framegraph_multiqueue_graphics_queue_submits=" << framegraph_multiqueue.graphics_queue_submits
        << " framegraph_multiqueue_queue_waits=" << framegraph_multiqueue.queue_waits
        << " framegraph_multiqueue_graphics_waited_for_copy="
        << (framegraph_multiqueue.graphics_waited_for_copy ? 1 : 0) << " renderer_quality_status="
        << mirakana::win32_desktop_presentation_quality_gate_status_name(renderer_quality.status)
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
        << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
        << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
        << " d3d12_gpu_skinning_evidence_ready="
        << (options.require_d3d12_gpu_skinning_evidence && d3d12_gpu_skinning_evidence_ready(report, options.max_frames)
                ? 1
                : 0)
        << " d3d12_gpu_skinning_evidence_selected="
        << (options.require_d3d12_gpu_skinning_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::d3d12
                ? 1
                : 0)
        << " vulkan_gpu_skinning_evidence_ready="
        << (options.require_vulkan_gpu_skinning_evidence &&
                    vulkan_gpu_skinning_evidence_ready(report, options.max_frames)
                ? 1
                : 0)
        << " vulkan_gpu_skinning_evidence_selected="
        << (options.require_vulkan_gpu_skinning_evidence &&
                    report.selected_backend == mirakana::Win32DesktopPresentationBackend::vulkan
                ? 1
                : 0)
        << '\n';
    print_presentation_report("sample_desktop_runtime_game", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                  << mirakana::win32_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
        std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                  << mirakana::win32_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.postprocess_diagnostics()) {
        std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                  << mirakana::win32_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
        std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
                  << mirakana::win32_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                  << mirakana::win32_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                  << mirakana::win32_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status)
                  << ": " << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : framegraph_multiqueue.diagnostics) {
        std::cout << "sample_desktop_runtime_game framegraph_multiqueue_diagnostic=" << diagnostic.message << '\n';
    }

    if (options.smoke) {
        if (result.status != mirakana::DesktopRunStatus::completed || result.frames_run != options.max_frames ||
            game.frames() != options.max_frames) {
            return 3;
        }
        if (!game.hud_passed(options.max_frames)) {
            return 3;
        }
        if (!options.required_scene_package_path.empty() && !game.packaged_scene_passed(options.max_frames)) {
            return 3;
        }
        if (options.require_quaternion_animation && !game.quaternion_animation_passed(options.max_frames)) {
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
        if (options.require_gpu_skinning && !gpu_skinning_evidence_matches(report, options.max_frames)) {
            return 3;
        }
        if (options.require_d3d12_gpu_skinning_evidence &&
            !d3d12_gpu_skinning_evidence_ready(report, options.max_frames)) {
            return 3;
        }
        if (options.require_vulkan_gpu_skinning_evidence &&
            !vulkan_gpu_skinning_evidence_ready(report, options.max_frames)) {
            return 3;
        }
        const std::uint32_t expected_framegraph_passes = options.require_directional_shadow ? 3U : 2U;
        const auto expected_frames = static_cast<std::uint64_t>(options.max_frames);
        const auto expected_framegraph_render_passes = expected_frames * expected_framegraph_passes;
        const std::uint64_t expected_framegraph_barrier_steps =
            options.require_directional_shadow        ? (expected_frames == 0 ? 0 : 9 + ((expected_frames - 1) * 6))
            : options.require_postprocess_depth_input ? (expected_frames == 0 ? 0 : 1 + (expected_frames * 4))
                                                      : expected_frames * 2;
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::Win32DesktopPresentationPostprocessStatus::ready ||
             !postprocess_policy.ready || postprocess_policy.diagnostics_count != 0 ||
             postprocess_policy.effect_count != 1 || postprocess_policy.postprocess_pass_count != 1 ||
             postprocess_policy.framegraph_pass_count != 2 || postprocess_policy.framegraph_barrier_step_budget != 2 ||
             !postprocess_policy.scene_color_required || !postprocess_policy.color_grading_effect ||
             !postprocess_policy.backend_shader_evidence_ready ||
             (options.require_postprocess_depth_input && !postprocess_policy.scene_depth_required) ||
             report.framegraph_passes != expected_framegraph_passes ||
             report.renderer_stats.framegraph_passes_executed !=
                 static_cast<std::uint64_t>(options.max_frames) * expected_framegraph_passes ||
             report.renderer_stats.framegraph_render_passes_recorded != expected_framegraph_render_passes ||
             report.renderer_stats.framegraph_barrier_steps_executed != expected_framegraph_barrier_steps ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
        if (options.require_directional_shadow &&
            (report.directional_shadow_status != mirakana::Win32DesktopPresentationDirectionalShadowStatus::ready ||
             !report.directional_shadow_ready)) {
            return 3;
        }
        if (options.require_directional_shadow_filtering &&
            (report.directional_shadow_filter_mode !=
                 mirakana::Win32DesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
             report.directional_shadow_filter_tap_count != 9 ||
             report.directional_shadow_filter_radius_texels != 1.0F)) {
            return 3;
        }
        if (options.require_d3d12_shadow_cascade_policy && !d3d12_shadow_cascade_policy_ready(report)) {
            return 3;
        }
        if (options.require_vulkan_shadow_cascade_policy && !vulkan_shadow_cascade_policy_ready(report)) {
            return 3;
        }
        if (options.require_lighting_shadow_policy && !lighting_shadow_policy_ready) {
            return 3;
        }
        if (options.require_scene_scale_policy && !scene_scale_policy.ready) {
            return 3;
        }
        if (options.require_gpu_memory_policy && !gpu_memory_policy.ready) {
            return 3;
        }
        if (options.require_memory_diagnostics &&
            memory_diagnostics.status != mirakana::MemoryDiagnosticsStatus::ready) {
            return 3;
        }
        if (options.require_d3d12_gpu_memory_evidence && !d3d12_gpu_memory_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_gpu_memory_evidence && !vulkan_gpu_memory_execution.ready) {
            return 3;
        }
        if (options.require_debug_profiling_policy && !debug_profiling_policy.ready) {
            return 3;
        }
        if (options.require_d3d12_debug_profiling_evidence && !d3d12_debug_profiling_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_debug_profiling_evidence && !vulkan_debug_profiling_execution.ready) {
            return 3;
        }
        if (options.require_job_scheduling_evidence && !job_scheduling_evidence_ready(job_scheduling_evidence)) {
            return 3;
        }
        if (options.require_job_execution_topology_policy &&
            !job_execution_topology_policy_ready(job_execution_topology_policy)) {
            return 3;
        }
        if (options.require_job_execution_foundation && !job_execution_foundation_ready(job_execution_foundation)) {
            return 3;
        }
        if (options.require_job_execution_work_stealing &&
            !job_execution_work_stealing_ready(job_execution_work_stealing)) {
            return 3;
        }
        if (options.require_job_execution_placement_policy &&
            !job_execution_placement_policy_ready(job_execution_placement_policy)) {
            return 3;
        }
        if (options.require_windows_cpu_set_worker_placement &&
            !windows_cpu_set_worker_placement_ready(windows_cpu_set_worker_placement)) {
            return 3;
        }
        if (options.require_d3d12_instanced_draw_evidence && !d3d12_instanced_draw_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_instanced_draw_evidence && !vulkan_instanced_draw_execution.ready) {
            return 3;
        }
        if (options.require_d3d12_postprocess_evidence && !d3d12_postprocess_execution.ready) {
            return 3;
        }
        if (options.require_vulkan_postprocess_evidence && !vulkan_postprocess_execution.ready) {
            return 3;
        }
        if (options.require_renderer_quality_gates && !renderer_quality.ready) {
            return 3;
        }
        if (options.require_framegraph_multiqueue_evidence &&
            !d3d12_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)) {
            return 3;
        }
        if (options.require_vulkan_framegraph_multiqueue_evidence &&
            !vulkan_framegraph_multiqueue_evidence_ready(report, framegraph_multiqueue)) {
            return 3;
        }
        const auto expected_ui_overlay_sprites =
            static_cast<std::uint64_t>(options.max_frames) *
            static_cast<std::uint64_t>(options.require_native_ui_textured_sprite_atlas ? 2U : 1U);
        if (options.require_native_ui_overlay &&
            (report.native_ui_overlay_status != mirakana::Win32DesktopPresentationNativeUiOverlayStatus::ready ||
             !report.native_ui_overlay_ready ||
             report.native_ui_overlay_sprites_submitted != expected_ui_overlay_sprites ||
             report.native_ui_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            (ui_atlas_metadata.status != UiAtlasMetadataStatus::ready || ui_atlas_metadata.pages == 0 ||
             ui_atlas_metadata.bindings == 0)) {
            return 3;
        }
        if (options.require_native_ui_textured_sprite_atlas &&
            (report.native_ui_texture_overlay_status !=
                 mirakana::Win32DesktopPresentationNativeUiTextureOverlayStatus::ready ||
             !report.native_ui_texture_overlay_atlas_ready ||
             report.native_ui_texture_overlay_sprites_submitted != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_texture_binds != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
    }
    return 0;
}
