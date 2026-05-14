// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/core/application.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/platform/input.hpp"
#include "mirakana/renderer/renderer.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_game_host.hpp"
#include "mirakana/runtime_host/sdl3/sdl_desktop_presentation.hpp"
#include "mirakana/runtime_host/shader_bytecode.hpp"
#include "mirakana/runtime_rhi/runtime_upload.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"
#include "mirakana/ui/ui.hpp"
#include "mirakana/ui_renderer/ui_renderer.hpp"

#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <iostream>
#include <optional>
#include <span>
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
    bool require_renderer_quality_gates{false};
    bool require_native_ui_overlay{false};
    bool require_native_ui_textured_sprite_atlas{false};
    bool require_gpu_skinning{false};
    bool require_quaternion_animation{false};
    std::uint32_t max_frames{0};
    std::string video_driver_hint;
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
                             bool textured_ui_atlas_mode, mirakana::UiRendererImagePalette image_palette,
                             mirakana::SdlDesktopPresentation* presentation = nullptr)
        : input_(input), renderer_(renderer), throttle_(throttle), scene_(std::move(scene)),
          quaternion_animation_tracks_(std::move(quaternion_animation_tracks)),
          image_palette_(std::move(image_palette)), scene_gpu_mode_(scene_gpu_mode),
          textured_ui_atlas_mode_(textured_ui_atlas_mode), presentation_(presentation) {}

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
    bool textured_ui_atlas_mode_{false};
    mirakana::SdlDesktopPresentation* presentation_{nullptr};
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
    float final_camera_x_{0.0F};
    float final_quaternion_z_{0.0F};
    float final_quaternion_w_{1.0F};
    float final_quaternion_scene_rotation_z_{0.0F};
    bool ui_ok_{false};
    bool ui_text_updates_ok_{true};
    bool primary_camera_seen_{false};
    bool quaternion_animation_seen_{false};
    bool scene_mesh_plan_ok_{true};
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
    std::cout << "sample_desktop_runtime_game [--smoke] [--max-frames N] [--video-driver NAME] "
                 "[--require-config PATH] [--require-scene-package PATH] [--require-d3d12-scene-shaders] "
                 "[--require-vulkan-scene-shaders] [--require-d3d12-renderer] [--require-vulkan-renderer] "
                 "[--require-scene-gpu-bindings] [--require-postprocess] [--require-postprocess-depth-input] "
                 "[--require-directional-shadow] [--require-directional-shadow-filtering] "
                 "[--require-renderer-quality-gates] [--require-native-ui-overlay] "
                 "[--require-native-ui-textured-sprite-atlas] "
                 "[--require-gpu-skinning] [--require-quaternion-animation]\n";
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
        if (arg == "--require-renderer-quality-gates") {
            options.require_scene_gpu_bindings = true;
            options.require_postprocess = true;
            options.require_postprocess_depth_input = true;
            options.require_directional_shadow = true;
            options.require_directional_shadow_filtering = true;
            options.require_renderer_quality_gates = true;
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
make_renderer_quality_gate_desc(const DesktopRuntimeGameOptions& options) noexcept {
    mirakana::SdlDesktopPresentationQualityGateDesc desc;
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

[[nodiscard]] mirakana::SdlDesktopPresentationShaderBytecode
to_presentation_shader_bytecode(const mirakana::DesktopShaderBytecodeBlob& bytecode) noexcept {
    return mirakana::SdlDesktopPresentationShaderBytecode{
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
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
              << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
              << " directional_shadow_status="
              << mirakana::sdl_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
              << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
              << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
              << " directional_shadow_filter_mode="
              << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(
                     report.directional_shadow_filter_mode)
              << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
              << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
              << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
              << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
              << " ui_overlay_draws=" << report.native_ui_overlay_draws
              << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
              << " ui_texture_overlay_status="
              << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(
                     report.native_ui_texture_overlay_status)
              << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
              << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
              << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
              << " framegraph_passes=" << report.framegraph_passes
              << " renderer_gpu_skinning_draws=" << report.renderer_stats.gpu_skinning_draws
              << " renderer_skinned_palette_descriptor_binds=" << report.renderer_stats.skinned_palette_descriptor_binds
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

    std::optional<mirakana::SdlDesktopPresentationD3d12SceneRendererDesc> d3d12_scene_renderer;
    const auto& d3d12_scene_bytecode = options.require_directional_shadow ? shadow_receiver_bytecode : shader_bytecode;
    const bool d3d12_shadow_ready = !options.require_directional_shadow || shadow_bytecode.ready();
    const bool d3d12_native_ui_overlay_ready = !options.require_native_ui_overlay || native_ui_overlay_bytecode.ready();
    if (d3d12_scene_bytecode.ready() && postprocess_bytecode.ready() && d3d12_shadow_ready &&
        d3d12_native_ui_overlay_ready && runtime_package.has_value() && packaged_scene.has_value()) {
        d3d12_scene_renderer.emplace(mirakana::SdlDesktopPresentationD3d12SceneRendererDesc{
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

    std::optional<mirakana::SdlDesktopPresentationVulkanSceneRendererDesc> vulkan_scene_renderer;
    const auto& vulkan_scene_bytecode =
        options.require_directional_shadow ? vulkan_shadow_receiver_bytecode : vulkan_shader_bytecode;
    const bool vulkan_shadow_ready = !options.require_directional_shadow || vulkan_shadow_bytecode.ready();
    const bool vulkan_native_ui_overlay_ready =
        !options.require_native_ui_overlay || vulkan_native_ui_overlay_bytecode.ready();
    if (vulkan_scene_bytecode.ready() && vulkan_postprocess_bytecode.ready() && vulkan_shadow_ready &&
        vulkan_native_ui_overlay_ready && runtime_package.has_value() && packaged_scene.has_value()) {
        vulkan_scene_renderer.emplace(mirakana::SdlDesktopPresentationVulkanSceneRendererDesc{
            .vertex_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.vertex_shader),
            .fragment_shader = to_presentation_shader_bytecode(vulkan_scene_bytecode.fragment_shader),
            .skinned_vertex_shader = to_presentation_shader_bytecode(vulkan_skinned_vertex_blob),
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

    mirakana::SdlDesktopGameHostDesc host_desc{
        .title = "Sample Desktop Runtime Game",
        .extent = mirakana::WindowExtent{.width = 800, .height = 450},
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
        std::cout << "sample_desktop_runtime_game required_d3d12_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_vulkan_renderer &&
        host.presentation_backend() != mirakana::SdlDesktopPresentationBackend::vulkan) {
        std::cout << "sample_desktop_runtime_game required_vulkan_renderer_unavailable renderer="
                  << host.presentation_backend_name() << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 7;
    }
    if (options.require_scene_gpu_bindings && !host.scene_gpu_bindings_ready()) {
        std::cout << "sample_desktop_runtime_game required_scene_gpu_bindings_unavailable status="
                  << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(host.scene_gpu_binding_status())
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
            std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                      << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 5;
    }
    if (options.require_postprocess &&
        host.presentation_report().postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_unavailable status="
                  << mirakana::sdl_desktop_presentation_postprocess_status_name(
                         host.presentation_report().postprocess_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::sdl_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_postprocess_depth_input && !host.presentation_report().postprocess_depth_input_ready) {
        std::cout << "sample_desktop_runtime_game required_postprocess_depth_input_unavailable ready=0\n";
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.postprocess_diagnostics()) {
            std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                      << mirakana::sdl_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 8;
    }
    if (options.require_directional_shadow && !host.presentation_report().directional_shadow_ready) {
        std::cout << "sample_desktop_runtime_game required_directional_shadow_unavailable status="
                  << mirakana::sdl_desktop_presentation_directional_shadow_status_name(
                         host.presentation_report().directional_shadow_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.presentation_diagnostics()) {
            std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                      << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                      << diagnostic.message << '\n';
        }
        for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
            std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
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
            std::cout << "sample_desktop_runtime_game required_directional_shadow_filtering_unavailable mode="
                      << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(
                             report.directional_shadow_filter_mode)
                      << " taps=" << report.directional_shadow_filter_tap_count
                      << " radius_texels=" << report.directional_shadow_filter_radius_texels << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            return 9;
        }
    }
    if (options.require_native_ui_overlay && !host.presentation_report().native_ui_overlay_ready) {
        std::cout << "sample_desktop_runtime_game required_native_ui_overlay_unavailable status="
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(
                         host.presentation_report().native_ui_overlay_status)
                  << '\n';
        print_presentation_report("sample_desktop_runtime_game", host);
        for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
            std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                      << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                      << diagnostic.message << '\n';
        }
        return 10;
    }
    if (options.require_native_ui_textured_sprite_atlas) {
        const auto report = host.presentation_report();
        if (report.native_ui_texture_overlay_status !=
                mirakana::SdlDesktopPresentationNativeUiTextureOverlayStatus::ready ||
            !report.native_ui_texture_overlay_atlas_ready) {
            std::cout << "sample_desktop_runtime_game required_native_ui_textured_sprite_atlas_unavailable status="
                      << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(
                             report.native_ui_texture_overlay_status)
                      << " atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0) << '\n';
            print_presentation_report("sample_desktop_runtime_game", host);
            for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
                std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                          << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status)
                          << ": " << diagnostic.message << '\n';
            }
            return 11;
        }
    }

    SampleDesktopRuntimeGame game(host.input(), host.renderer(), options.throttle, std::move(packaged_scene),
                                  host.scene_gpu_bindings_ready(), std::move(packaged_quaternion_animation_tracks),
                                  options.require_native_ui_textured_sprite_atlas, std::move(ui_atlas_metadata.palette),
                                  &host.presentation());
    const auto result = host.run(game, mirakana::DesktopRunConfig{.max_frames = options.max_frames});
    const auto report = host.presentation_report();
    const auto scene_gpu_stats = report.scene_gpu_stats;
    const auto renderer_quality =
        mirakana::evaluate_sdl_desktop_presentation_quality_gate(report, make_renderer_quality_gate_desc(options));

    std::cout << "sample_desktop_runtime_game status=" << status_name(result.status)
              << " renderer=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_requested=" << mirakana::sdl_desktop_presentation_backend_name(report.requested_backend)
              << " presentation_selected=" << mirakana::sdl_desktop_presentation_backend_name(report.selected_backend)
              << " presentation_fallback="
              << mirakana::sdl_desktop_presentation_fallback_reason_name(report.fallback_reason)
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
              << " camera_controller_ticks=" << game.camera_controller_ticks()
              << " camera_final_x=" << game.final_camera_x()
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
              << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(report.scene_gpu_status)
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
              << mirakana::sdl_desktop_presentation_postprocess_status_name(report.postprocess_status)
              << " postprocess_depth_input_requested=" << (report.postprocess_depth_input_requested ? 1 : 0)
              << " postprocess_depth_input_ready=" << (report.postprocess_depth_input_ready ? 1 : 0)
              << " directional_shadow_status="
              << mirakana::sdl_desktop_presentation_directional_shadow_status_name(report.directional_shadow_status)
              << " directional_shadow_requested=" << (report.directional_shadow_requested ? 1 : 0)
              << " directional_shadow_ready=" << (report.directional_shadow_ready ? 1 : 0)
              << " directional_shadow_filter_mode="
              << mirakana::sdl_desktop_presentation_directional_shadow_filter_mode_name(
                     report.directional_shadow_filter_mode)
              << " directional_shadow_filter_taps=" << report.directional_shadow_filter_tap_count
              << " directional_shadow_filter_radius_texels=" << report.directional_shadow_filter_radius_texels
              << " ui_overlay_requested=" << (report.native_ui_overlay_requested ? 1 : 0) << " ui_overlay_status="
              << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(report.native_ui_overlay_status)
              << " ui_overlay_ready=" << (report.native_ui_overlay_ready ? 1 : 0)
              << " ui_overlay_sprites_submitted=" << report.native_ui_overlay_sprites_submitted
              << " ui_overlay_draws=" << report.native_ui_overlay_draws
              << " ui_atlas_metadata_requested=" << (options.require_native_ui_textured_sprite_atlas ? 1 : 0)
              << " ui_atlas_metadata_status=" << ui_atlas_metadata_status_name(ui_atlas_metadata.status)
              << " ui_atlas_metadata_pages=" << ui_atlas_metadata.pages
              << " ui_atlas_metadata_bindings=" << ui_atlas_metadata.bindings
              << " ui_texture_overlay_requested=" << (report.native_ui_texture_overlay_requested ? 1 : 0)
              << " ui_texture_overlay_status="
              << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(
                     report.native_ui_texture_overlay_status)
              << " ui_texture_overlay_atlas_ready=" << (report.native_ui_texture_overlay_atlas_ready ? 1 : 0)
              << " ui_texture_overlay_sprites_submitted=" << report.native_ui_texture_overlay_sprites_submitted
              << " ui_texture_overlay_texture_binds=" << report.native_ui_texture_overlay_texture_binds
              << " ui_texture_overlay_draws=" << report.native_ui_texture_overlay_draws
              << " framegraph_passes=" << report.framegraph_passes << " renderer_quality_status="
              << mirakana::sdl_desktop_presentation_quality_gate_status_name(renderer_quality.status)
              << " renderer_quality_ready=" << (renderer_quality.ready ? 1 : 0)
              << " renderer_quality_diagnostics=" << renderer_quality.diagnostics_count
              << " renderer_quality_expected_framegraph_passes=" << renderer_quality.expected_framegraph_passes
              << " renderer_quality_framegraph_passes_ok=" << (renderer_quality.framegraph_passes_current ? 1 : 0)
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
              << '\n';
    print_presentation_report("sample_desktop_runtime_game", host);
    for (const auto& diagnostic : host.presentation_diagnostics()) {
        std::cout << "sample_desktop_runtime_game presentation_diagnostic="
                  << mirakana::sdl_desktop_presentation_fallback_reason_name(diagnostic.reason) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.scene_gpu_binding_diagnostics()) {
        std::cout << "sample_desktop_runtime_game scene_gpu_diagnostic="
                  << mirakana::sdl_desktop_presentation_scene_gpu_binding_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.postprocess_diagnostics()) {
        std::cout << "sample_desktop_runtime_game postprocess_diagnostic="
                  << mirakana::sdl_desktop_presentation_postprocess_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.directional_shadow_diagnostics()) {
        std::cout << "sample_desktop_runtime_game directional_shadow_diagnostic="
                  << mirakana::sdl_desktop_presentation_directional_shadow_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_overlay_diagnostic="
                  << mirakana::sdl_desktop_presentation_native_ui_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
    }
    for (const auto& diagnostic : host.native_ui_texture_overlay_diagnostics()) {
        std::cout << "sample_desktop_runtime_game native_ui_texture_overlay_diagnostic="
                  << mirakana::sdl_desktop_presentation_native_ui_texture_overlay_status_name(diagnostic.status) << ": "
                  << diagnostic.message << '\n';
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
        if (options.require_gpu_skinning &&
            (report.renderer_stats.gpu_skinning_draws != static_cast<std::uint64_t>(options.max_frames) ||
             report.renderer_stats.skinned_palette_descriptor_binds != static_cast<std::uint64_t>(options.max_frames) ||
             scene_gpu_stats.skinned_mesh_uploads < 1 || scene_gpu_stats.skinned_mesh_bindings < 1 ||
             scene_gpu_stats.skinned_mesh_bindings_resolved != static_cast<std::size_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess &&
            (report.postprocess_status != mirakana::SdlDesktopPresentationPostprocessStatus::ready ||
             report.framegraph_passes != (options.require_directional_shadow ? 3U : 2U) ||
             report.renderer_stats.framegraph_passes_executed !=
                 static_cast<std::uint64_t>(options.max_frames) *
                     static_cast<std::uint64_t>(options.require_directional_shadow ? 3U : 2U) ||
             report.renderer_stats.postprocess_passes_executed != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
        if (options.require_postprocess_depth_input && !report.postprocess_depth_input_ready) {
            return 3;
        }
        if (options.require_directional_shadow &&
            (report.directional_shadow_status != mirakana::SdlDesktopPresentationDirectionalShadowStatus::ready ||
             !report.directional_shadow_ready)) {
            return 3;
        }
        if (options.require_directional_shadow_filtering &&
            (report.directional_shadow_filter_mode !=
                 mirakana::SdlDesktopPresentationDirectionalShadowFilterMode::fixed_pcf_3x3 ||
             report.directional_shadow_filter_tap_count != 9 ||
             report.directional_shadow_filter_radius_texels != 1.0F)) {
            return 3;
        }
        if (options.require_renderer_quality_gates && !renderer_quality.ready) {
            return 3;
        }
        const auto expected_ui_overlay_sprites =
            static_cast<std::uint64_t>(options.max_frames) *
            static_cast<std::uint64_t>(options.require_native_ui_textured_sprite_atlas ? 2U : 1U);
        if (options.require_native_ui_overlay &&
            (report.native_ui_overlay_status != mirakana::SdlDesktopPresentationNativeUiOverlayStatus::ready ||
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
                 mirakana::SdlDesktopPresentationNativeUiTextureOverlayStatus::ready ||
             !report.native_ui_texture_overlay_atlas_ready ||
             report.native_ui_texture_overlay_sprites_submitted != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_texture_binds != static_cast<std::uint64_t>(options.max_frames) ||
             report.native_ui_texture_overlay_draws != static_cast<std::uint64_t>(options.max_frames))) {
            return 3;
        }
    }
    return 0;
}
