// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/shadow_map.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace mirakana {
namespace {

constexpr auto shadow_depth_resource = "shadow.depth";
constexpr auto shadow_depth_pass = "shadow.directional.depth";
constexpr auto shadow_receiver_pass = "shadow.receiver.resolve";

[[nodiscard]] bool is_valid_shadow_extent(rhi::Extent2D extent) noexcept {
    return extent.width != 0 && extent.height != 0;
}

[[nodiscard]] bool is_supported_shadow_depth_format(rhi::Format format) noexcept {
    return format == rhi::Format::depth24_stencil8;
}

void append_diagnostic(ShadowMapPlan& plan, ShadowMapDiagnosticCode code, std::string message) {
    plan.diagnostics.push_back(ShadowMapDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

void append_diagnostic(ShadowReceiverPlan& plan, ShadowReceiverDiagnosticCode code, std::string message) {
    plan.diagnostics.push_back(ShadowReceiverDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

void append_diagnostic(DirectionalShadowLightSpacePlan& plan, DirectionalShadowLightSpaceDiagnosticCode code,
                       std::string message) {
    plan.diagnostics.push_back(DirectionalShadowLightSpaceDiagnostic{
        .code = code,
        .message = std::move(message),
    });
}

[[nodiscard]] FrameGraphV1Desc make_shadow_frame_graph_v1_desc() {
    FrameGraphV1Desc frame_graph;
    frame_graph.resources.push_back(FrameGraphResourceV1Desc{
        .name = std::string(shadow_depth_resource),
        .lifetime = FrameGraphResourceLifetime::transient,
    });
    frame_graph.passes.push_back(FrameGraphPassV1Desc{
        .name = std::string(shadow_depth_pass),
        .reads = {},
        .writes = {FrameGraphResourceAccess{.resource = std::string(shadow_depth_resource),
                                            .access = FrameGraphAccess::depth_attachment_write}},
    });
    frame_graph.passes.push_back(FrameGraphPassV1Desc{
        .name = std::string(shadow_receiver_pass),
        .reads = {FrameGraphResourceAccess{.resource = std::string(shadow_depth_resource),
                                           .access = FrameGraphAccess::shader_read}},
        .writes = {},
    });
    return frame_graph;
}

[[nodiscard]] bool is_unit_interval(float value) noexcept {
    return std::isfinite(value) && value >= 0.0F && value <= 1.0F;
}

[[nodiscard]] bool is_finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] Vec3 normalize_vec3(Vec3 value) noexcept {
    const auto value_length = length(value);
    if (value_length <= 0.000001F || !std::isfinite(value_length)) {
        return Vec3{};
    }
    return value * (1.0F / value_length);
}

[[nodiscard]] bool is_supported_filter_mode(ShadowReceiverFilterMode mode) noexcept {
    return mode == ShadowReceiverFilterMode::none || mode == ShadowReceiverFilterMode::fixed_pcf_3x3;
}

[[nodiscard]] std::uint32_t shadow_receiver_filter_tap_count(ShadowReceiverFilterMode mode) noexcept {
    switch (mode) {
    case ShadowReceiverFilterMode::none:
        return 1;
    case ShadowReceiverFilterMode::fixed_pcf_3x3:
        return 9;
    }
    return 0;
}

[[nodiscard]] bool is_valid_shadow_depth_texture(const rhi::TextureDesc& desc) noexcept {
    return desc.extent.width != 0 && desc.extent.height != 0 && desc.extent.depth == 1 &&
           desc.format == rhi::Format::depth24_stencil8 &&
           desc.usage == (rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource);
}

[[nodiscard]] bool is_valid_shadow_map_plan_for_receiver(const ShadowMapPlan& plan) noexcept {
    if (!plan.succeeded() || plan.caster_count == 0 || plan.receiver_count == 0 ||
        !is_valid_shadow_depth_texture(plan.depth_texture) || !plan.frame_graph_plan.succeeded() ||
        plan.frame_graph_plan.pass_count != 2 || plan.frame_graph_plan.ordered_passes.size() != 2 ||
        plan.frame_graph_plan.ordered_passes[0] != shadow_depth_pass ||
        plan.frame_graph_plan.ordered_passes[1] != shadow_receiver_pass) {
        return false;
    }
    if (plan.directional_cascade_count == 0) {
        return false;
    }
    const std::uint64_t expected_w = static_cast<std::uint64_t>(plan.cascade_tile_extent.width) *
                                     static_cast<std::uint64_t>(plan.directional_cascade_count);
    return expected_w == static_cast<std::uint64_t>(plan.depth_texture.extent.width) &&
           plan.cascade_tile_extent.height == plan.depth_texture.extent.height;
}

struct DirectionalLightBasis {
    Vec3 direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 right{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    Vec3 up{.x = 0.0F, .y = 0.0F, .z = 1.0F};
};

[[nodiscard]] DirectionalLightBasis make_directional_light_basis(Vec3 light_direction) noexcept {
    DirectionalLightBasis basis;
    basis.direction = normalize_vec3(light_direction);

    const auto world_y = Vec3{.x = 0.0F, .y = 1.0F, .z = 0.0F};
    const auto world_x = Vec3{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    const auto reference = std::abs(dot(basis.direction, world_y)) > 0.95F ? world_x : world_y;
    basis.right = normalize_vec3(cross(basis.direction, reference));
    basis.up = normalize_vec3(cross(basis.right, basis.direction));
    return basis;
}

[[nodiscard]] float snap_to_texel(float value, float texel_world_size) noexcept {
    if (texel_world_size <= 0.0F) {
        return value;
    }
    return std::round(value / texel_world_size) * texel_world_size;
}

[[nodiscard]] Vec3 snap_focus_center(Vec3 center, const DirectionalLightBasis& basis, float texel_world_size,
                                     ShadowLightSpaceTexelSnap snap) noexcept {
    if (snap != ShadowLightSpaceTexelSnap::enabled) {
        return center;
    }

    const auto light_x = dot(basis.right, center);
    const auto light_y = dot(basis.up, center);
    const auto snapped_x = snap_to_texel(light_x, texel_world_size);
    const auto snapped_y = snap_to_texel(light_y, texel_world_size);
    return center + (basis.right * (snapped_x - light_x)) + (basis.up * (snapped_y - light_y));
}

[[nodiscard]] Mat4 make_light_view_from_world(Vec3 right, Vec3 up, Vec3 direction, Vec3 center) noexcept {
    auto view = Mat4::identity();
    view.at(0, 0) = right.x;
    view.at(0, 1) = right.y;
    view.at(0, 2) = right.z;
    view.at(0, 3) = -dot(right, center);
    view.at(1, 0) = up.x;
    view.at(1, 1) = up.y;
    view.at(1, 2) = up.z;
    view.at(1, 3) = -dot(up, center);
    view.at(2, 0) = direction.x;
    view.at(2, 1) = direction.y;
    view.at(2, 2) = direction.z;
    view.at(2, 3) = -dot(direction, center);
    return view;
}

[[nodiscard]] Mat4 make_shadow_orthographic_clip_from_view(float width, float height, float near_z,
                                                           float far_z) noexcept {
    auto clip = Mat4::identity();
    clip.at(0, 0) = 2.0F / width;
    clip.at(1, 1) = 2.0F / height;
    const auto depth = near_z - far_z;
    clip.at(2, 2) = 1.0F / depth;
    clip.at(2, 3) = near_z / depth;
    return clip;
}

[[nodiscard]] Vec3 matrix_column_xyz(const Mat4& m, std::size_t column) noexcept {
    return Vec3{.x = m.at(0, column), .y = m.at(1, column), .z = m.at(2, column)};
}

[[nodiscard]] Vec3 normalized_matrix_column(const Mat4& m, std::size_t column) noexcept {
    return normalize_vec3(matrix_column_xyz(m, column));
}

/// Appends the eight world-space corners of the camera frustum slice between view-space distances \p d_near and
/// \p d_far (positive distances along the camera forward axis from the eye).
void append_frustum_slice_corners_world(const DirectionalShadowCascadeCameraInput& cam, float d_near, float d_far,
                                        std::vector<Vec3>& corners) {
    const auto eye = Vec3{
        .x = cam.world_from_camera.at(0, 3), .y = cam.world_from_camera.at(1, 3), .z = cam.world_from_camera.at(2, 3)};
    const auto right = normalized_matrix_column(cam.world_from_camera, 0);
    const auto up = normalized_matrix_column(cam.world_from_camera, 1);
    const auto forward = normalized_matrix_column(cam.world_from_camera, 2);

    float half_w_near = 0.0F;
    float half_h_near = 0.0F;
    float half_w_far = 0.0F;
    float half_h_far = 0.0F;

    if (cam.perspective) {
        const auto tan_half = std::tan(cam.vertical_fov_radians * 0.5F);
        if (!std::isfinite(tan_half) || tan_half <= 0.0F || !std::isfinite(cam.viewport_aspect) ||
            cam.viewport_aspect <= 0.0F) {
            return;
        }
        half_h_near = d_near * tan_half;
        half_w_near = half_h_near * cam.viewport_aspect;
        half_h_far = d_far * tan_half;
        half_w_far = half_h_far * cam.viewport_aspect;
    } else {
        if (!std::isfinite(cam.orthographic_height) || cam.orthographic_height <= 0.0F ||
            !std::isfinite(cam.viewport_aspect) || cam.viewport_aspect <= 0.0F) {
            return;
        }
        half_h_near = cam.orthographic_height * 0.5F;
        half_w_near = half_h_near * cam.viewport_aspect;
        half_h_far = half_h_near;
        half_w_far = half_w_near;
    }

    constexpr std::array<float, 2> signs{-1.0F, 1.0F};
    for (const auto sx : signs) {
        for (const auto sy : signs) {
            const auto offset_near = right * (sx * half_w_near) + up * (sy * half_h_near);
            const auto offset_far = right * (sx * half_w_far) + up * (sy * half_h_far);
            corners.push_back(eye + forward * d_near + offset_near);
            corners.push_back(eye + forward * d_far + offset_far);
        }
    }
}

} // namespace

ShadowMapPlan build_shadow_map_plan(const ShadowMapDesc& desc) {
    ShadowMapPlan plan;
    plan.light = desc.light;
    plan.selected_light_index = desc.light.source_index;
    plan.caster_count = desc.caster_count;
    plan.receiver_count = desc.receiver_count;

    if (desc.light.type == ShadowMapLightType::unknown) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::missing_light, "shadow map plan requires a light");
    } else if (desc.light.type != ShadowMapLightType::directional) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::unsupported_light_type,
                          "directional shadow map planning supports directional lights only");
    }

    if (desc.light.type != ShadowMapLightType::unknown && !desc.light.casts_shadows) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::light_does_not_cast_shadows,
                          "selected light does not cast shadows");
    }

    if (!is_valid_shadow_extent(desc.extent)) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::invalid_extent, "shadow map extent must be non-zero");
    }

    if (!is_supported_shadow_depth_format(desc.depth_format)) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::unsupported_depth_format,
                          "directional shadow map planning requires depth24_stencil8");
    }

    if (desc.caster_count == 0) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::missing_casters,
                          "shadow map plan requires at least one caster");
    }

    if (desc.receiver_count == 0) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::missing_receivers,
                          "shadow map plan requires at least one receiver");
    }

    if (desc.directional_cascade_count < 1 || desc.directional_cascade_count > 8) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::invalid_directional_cascade_count,
                          "directional shadow cascade count must be between 1 and 8");
    }

    if (!plan.diagnostics.empty()) {
        return plan;
    }

    const std::uint64_t atlas_width =
        static_cast<std::uint64_t>(desc.extent.width) * static_cast<std::uint64_t>(desc.directional_cascade_count);
    if (atlas_width > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max())) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::shadow_atlas_extent_overflow,
                          "shadow depth atlas width overflow");
        return plan;
    }

    plan.directional_cascade_count = desc.directional_cascade_count;
    plan.cascade_tile_extent = desc.extent;
    plan.depth_texture = rhi::TextureDesc{
        .extent =
            rhi::Extent3D{.width = static_cast<std::uint32_t>(atlas_width), .height = desc.extent.height, .depth = 1},
        .format = desc.depth_format,
        .usage = rhi::TextureUsage::depth_stencil | rhi::TextureUsage::shader_resource,
    };
    plan.frame_graph_plan = compile_frame_graph_v1(make_shadow_frame_graph_v1_desc());
    if (!plan.frame_graph_plan.succeeded()) {
        append_diagnostic(plan, ShadowMapDiagnosticCode::frame_graph_failed,
                          "shadow map frame graph declaration failed validation");
    } else {
        plan.frame_graph_execution = schedule_frame_graph_v1_execution(plan.frame_graph_plan);
    }
    return plan;
}

bool has_shadow_map_diagnostic(const ShadowMapPlan& plan, ShadowMapDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const ShadowMapDiagnostic& diagnostic) { return diagnostic.code == code; });
}

ShadowReceiverPlan build_shadow_receiver_plan(const ShadowReceiverDesc& desc) {
    ShadowReceiverPlan plan;
    plan.depth_bias = desc.depth_bias;
    plan.lit_intensity = desc.lit_intensity;
    plan.shadow_intensity = desc.shadow_intensity;
    plan.filter_mode = desc.filter_mode;
    plan.filter_radius_texels = desc.filter_radius_texels;

    if (desc.shadow_map == nullptr || !is_valid_shadow_map_plan_for_receiver(*desc.shadow_map)) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_shadow_map_plan,
                          "shadow receiver plan requires a successful shadow map plan");
    }

    if (!std::isfinite(desc.depth_bias) || desc.depth_bias < 0.0F || desc.depth_bias > 1.0F) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_depth_bias,
                          "shadow receiver depth bias must be finite and in [0, 1]");
    }

    if (!is_unit_interval(desc.lit_intensity)) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_lit_intensity,
                          "shadow receiver lit intensity must be finite and in [0, 1]");
    }

    if (!is_unit_interval(desc.shadow_intensity)) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_shadow_intensity,
                          "shadow receiver shadow intensity must be finite and in [0, 1]");
    }

    if (is_unit_interval(desc.lit_intensity) && is_unit_interval(desc.shadow_intensity) &&
        desc.shadow_intensity > desc.lit_intensity) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::shadow_intensity_exceeds_lit_intensity,
                          "shadow receiver shadow intensity must not exceed lit intensity");
    }

    if (!is_supported_filter_mode(desc.filter_mode)) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_filter_mode,
                          "shadow receiver filter mode is unsupported");
    }

    if (!std::isfinite(desc.filter_radius_texels) || desc.filter_radius_texels < 0.0F ||
        desc.filter_radius_texels > 8.0F) {
        append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_filter_radius_texels,
                          "shadow receiver filter radius must be finite and in [0, 8] texels");
    }

    if (desc.shadow_map != nullptr && is_valid_shadow_map_plan_for_receiver(*desc.shadow_map)) {
        if (desc.light_space == nullptr || !desc.light_space->succeeded()) {
            append_diagnostic(plan, ShadowReceiverDiagnosticCode::invalid_light_space_plan,
                              "shadow receiver plan requires a successful directional shadow light-space plan");
        } else {
            const auto cascade_count = desc.shadow_map->directional_cascade_count;
            if (desc.light_space->clip_from_world_cascades.size() != static_cast<std::size_t>(cascade_count)) {
                append_diagnostic(plan, ShadowReceiverDiagnosticCode::light_space_cascade_mismatch,
                                  "shadow receiver light-space clip matrix count must match shadow map cascade count");
            } else if (cascade_count > 1 && desc.light_space->cascade_split_distances.size() !=
                                                static_cast<std::size_t>(cascade_count) + 1U) {
                append_diagnostic(plan, ShadowReceiverDiagnosticCode::light_space_cascade_mismatch,
                                  "shadow receiver light-space cascade split count must be cascade_count + 1");
            }
        }
    }

    if (!plan.diagnostics.empty()) {
        return plan;
    }

    plan.depth_texture = desc.shadow_map->depth_texture;
    plan.receiver_count = desc.shadow_map->receiver_count;
    plan.directional_cascade_count = desc.shadow_map->directional_cascade_count;
    plan.cascade_tile_extent = desc.shadow_map->cascade_tile_extent;
    plan.clip_from_world_cascades = desc.light_space->clip_from_world_cascades;
    plan.cascade_split_distances = desc.light_space->cascade_split_distances;
    plan.filter_tap_count = shadow_receiver_filter_tap_count(desc.filter_mode);
    plan.sampler = rhi::SamplerDesc{
        .min_filter = rhi::SamplerFilter::nearest,
        .mag_filter = rhi::SamplerFilter::nearest,
        .address_u = rhi::SamplerAddressMode::clamp_to_edge,
        .address_v = rhi::SamplerAddressMode::clamp_to_edge,
        .address_w = rhi::SamplerAddressMode::clamp_to_edge,
    };
    plan.descriptor_set_layout = rhi::DescriptorSetLayoutDesc{{
        rhi::DescriptorBindingDesc{
            .binding = shadow_receiver_depth_texture_binding(),
            .type = rhi::DescriptorType::sampled_texture,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
        rhi::DescriptorBindingDesc{
            .binding = shadow_receiver_sampler_binding(),
            .type = rhi::DescriptorType::sampler,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
        rhi::DescriptorBindingDesc{
            .binding = shadow_receiver_constants_binding(),
            .type = rhi::DescriptorType::uniform_buffer,
            .count = 1,
            .stages = rhi::ShaderStageVisibility::fragment,
        },
    }};
    return plan;
}

void pack_shadow_receiver_constants(const std::span<std::uint8_t> dst,
                                    const DirectionalShadowLightSpacePlan& light_space,
                                    const std::uint32_t cascade_count, const Mat4& camera_view_from_world) {
    if (dst.size() < shadow_receiver_constants_byte_size()) {
        return;
    }
    std::memset(dst.data(), 0, dst.size());
    std::memcpy(dst.data(), &cascade_count, sizeof(cascade_count));

    constexpr std::size_t k_splits_offset = 16;
    constexpr std::size_t k_matrix_offset = 64;
    constexpr std::size_t k_camera_view_offset = 576;
    const auto split_count = std::min<std::size_t>(light_space.cascade_split_distances.size(), 12);
    if (split_count != 0) {
        std::memcpy(&dst[k_splits_offset], light_space.cascade_split_distances.data(), split_count * sizeof(float));
    }
    for (std::uint32_t i = 0; i < cascade_count && i < 8U; ++i) {
        if (i >= light_space.clip_from_world_cascades.size()) {
            break;
        }
        const auto& m = light_space.clip_from_world_cascades[i];
        std::memcpy(&dst[k_matrix_offset + (static_cast<std::size_t>(i) * 64U)], m.values().data(), 64U);
    }
    std::memcpy(&dst[k_camera_view_offset], camera_view_from_world.values().data(), 64U);
}

bool has_shadow_receiver_diagnostic(const ShadowReceiverPlan& plan, ShadowReceiverDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics,
                               [code](const ShadowReceiverDiagnostic& diagnostic) { return diagnostic.code == code; });
}

DirectionalShadowLightSpacePlan build_directional_shadow_light_space_plan(const DirectionalShadowLightSpaceDesc& desc) {
    DirectionalShadowLightSpacePlan plan;
    plan.focus_center = desc.focus_center;
    plan.snapped_focus_center = desc.focus_center;
    plan.focus_radius = desc.focus_radius;
    plan.depth_radius = desc.depth_radius;
    plan.texel_snap = desc.texel_snap;

    if (desc.shadow_map == nullptr || !is_valid_shadow_map_plan_for_receiver(*desc.shadow_map)) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_shadow_map_plan,
                          "directional shadow light-space policy requires a successful shadow map plan");
    }

    const auto direction_length = length(desc.light_direction);
    if (!is_finite_vec3(desc.light_direction) || !std::isfinite(direction_length) || direction_length <= 0.000001F) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_light_direction,
                          "directional shadow light-space policy requires a finite non-zero light direction");
    }

    if (!is_finite_vec3(desc.focus_center)) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_center,
                          "directional shadow light-space policy requires a finite focus center");
    }

    if (!std::isfinite(desc.focus_radius) || desc.focus_radius <= 0.0F) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_focus_radius,
                          "directional shadow light-space policy requires a positive finite focus radius");
    }

    if (!std::isfinite(desc.depth_radius) || desc.depth_radius <= 0.0F) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_depth_radius,
                          "directional shadow light-space policy requires a positive finite depth radius");
    }

    if (!plan.diagnostics.empty()) {
        return plan;
    }

    const auto& shadow_map = *desc.shadow_map;
    const auto cascade_count = shadow_map.directional_cascade_count;
    const auto tile_width = shadow_map.cascade_tile_extent.width;
    if (tile_width == 0) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::invalid_shadow_map_plan,
                          "directional shadow light-space policy requires a non-zero cascade tile width");
        return plan;
    }

    const auto basis = make_directional_light_basis(desc.light_direction);
    plan.light_direction = basis.direction;
    plan.right = basis.right;
    plan.up = basis.up;

    if (cascade_count == 1) {
        plan.ortho_width = desc.focus_radius * 2.0F;
        plan.ortho_height = desc.focus_radius * 2.0F;
        plan.near_z = -desc.depth_radius;
        plan.far_z = desc.depth_radius;
        plan.texel_world_size = plan.ortho_width / static_cast<float>(tile_width);
        plan.snapped_focus_center = snap_focus_center(desc.focus_center, basis, plan.texel_world_size, desc.texel_snap);
        plan.view_from_world =
            make_light_view_from_world(plan.right, plan.up, plan.light_direction, plan.snapped_focus_center);
        const auto clip_from_view =
            make_shadow_orthographic_clip_from_view(plan.ortho_width, plan.ortho_height, plan.near_z, plan.far_z);
        plan.clip_from_view_cascades.push_back(clip_from_view);
        plan.clip_from_world_cascades.push_back(clip_from_view * plan.view_from_world);
        return plan;
    }

    if (desc.cascade_camera == nullptr || !desc.cascade_camera->valid) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::missing_cascade_camera,
                          "multi-cascade directional shadow light-space policy requires a valid cascade camera input");
        return plan;
    }

    const auto& cam = *desc.cascade_camera;
    if (!std::isfinite(cam.near_plane) || cam.near_plane <= 0.0F || !std::isfinite(cam.far_plane) ||
        !(cam.far_plane > cam.near_plane) || !std::isfinite(cam.viewport_aspect) || cam.viewport_aspect <= 0.0F) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                          "cascade camera near/far planes and viewport aspect must be finite and valid");
        return plan;
    }

    if (cam.perspective) {
        if (!std::isfinite(cam.vertical_fov_radians) || cam.vertical_fov_radians <= 0.0F ||
            cam.vertical_fov_radians >= 3.13149285F) {
            append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                              "cascade camera vertical field of view must be finite and in (0, pi)");
            return plan;
        }
    } else {
        if (!std::isfinite(cam.orthographic_height) || cam.orthographic_height <= 0.0F) {
            append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                              "cascade camera orthographic height must be finite and positive");
            return plan;
        }
    }

    float lambda = desc.cascade_split_lambda;
    if (!std::isfinite(lambda)) {
        lambda = 0.75F;
    }
    std::vector<float> splits;
    try {
        splits = compute_practical_shadow_cascade_distances(cascade_count, cam.near_plane, cam.far_plane, lambda);
    } catch (const std::invalid_argument&) {
        append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                          "cascade split distances could not be computed from the cascade camera parameters");
        return plan;
    }

    const auto texel_guess = (desc.focus_radius * 2.0F) / static_cast<float>(tile_width);
    plan.snapped_focus_center = snap_focus_center(desc.focus_center, basis, texel_guess, desc.texel_snap);
    plan.view_from_world =
        make_light_view_from_world(plan.right, plan.up, plan.light_direction, plan.snapped_focus_center);
    plan.cascade_split_distances = std::move(splits);

    for (std::uint32_t cascade = 0; cascade < cascade_count; ++cascade) {
        const auto d0 = plan.cascade_split_distances[static_cast<std::size_t>(cascade)];
        const auto d1 = plan.cascade_split_distances[static_cast<std::size_t>(cascade) + 1U];
        std::vector<Vec3> corners;
        corners.reserve(8);
        append_frustum_slice_corners_world(cam, d0, d1, corners);
        if (corners.size() < 8) {
            append_diagnostic(plan, DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                              "cascade frustum slice could not be constructed from the cascade camera");
            plan.clip_from_view_cascades.clear();
            plan.clip_from_world_cascades.clear();
            plan.cascade_split_distances.clear();
            return plan;
        }

        auto min_x = std::numeric_limits<float>::infinity();
        auto max_x = -std::numeric_limits<float>::infinity();
        auto min_y = std::numeric_limits<float>::infinity();
        auto max_y = -std::numeric_limits<float>::infinity();
        auto min_z = std::numeric_limits<float>::infinity();
        auto max_z = -std::numeric_limits<float>::infinity();
        for (const auto& world_point : corners) {
            const auto lv = transform_point(plan.view_from_world, world_point);
            if (!std::isfinite(lv.x) || !std::isfinite(lv.y) || !std::isfinite(lv.z)) {
                append_diagnostic(plan,
                                  DirectionalShadowLightSpaceDiagnosticCode::unsupported_cascade_camera_projection,
                                  "cascade frustum slice produced non-finite coordinates in light space");
                plan.clip_from_view_cascades.clear();
                plan.clip_from_world_cascades.clear();
                plan.cascade_split_distances.clear();
                return plan;
            }
            min_x = std::min(min_x, lv.x);
            max_x = std::max(max_x, lv.x);
            min_y = std::min(min_y, lv.y);
            max_y = std::max(max_y, lv.y);
            min_z = std::min(min_z, lv.z);
            max_z = std::max(max_z, lv.z);
        }

        constexpr auto k_xy_margin_ratio = 0.05F;
        constexpr auto k_z_margin_ratio = 0.05F;
        const auto xy_span = std::max(max_x - min_x, max_y - min_y);
        const auto xy_margin = k_xy_margin_ratio * std::max(xy_span, 0.0001F);
        min_x -= xy_margin;
        max_x += xy_margin;
        min_y -= xy_margin;
        max_y += xy_margin;

        const auto ortho_w = std::max(max_x - min_x, 0.0001F);
        const auto ortho_h = std::max(max_y - min_y, 0.0001F);
        const auto z_span = std::max(max_z - min_z, 0.0001F);
        const auto z_margin = k_z_margin_ratio * z_span;
        const auto near_z = min_z - z_margin;
        const auto far_z = max_z + z_margin;

        const auto clip_from_view = make_shadow_orthographic_clip_from_view(ortho_w, ortho_h, near_z, far_z);
        plan.clip_from_view_cascades.push_back(clip_from_view);
        plan.clip_from_world_cascades.push_back(clip_from_view * plan.view_from_world);

        if (cascade == 0) {
            plan.ortho_width = ortho_w;
            plan.ortho_height = ortho_h;
            plan.near_z = near_z;
            plan.far_z = far_z;
            plan.texel_world_size = ortho_w / static_cast<float>(tile_width);
        }
    }

    return plan;
}

bool has_directional_shadow_light_space_diagnostic(const DirectionalShadowLightSpacePlan& plan,
                                                   DirectionalShadowLightSpaceDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const DirectionalShadowLightSpaceDiagnostic& diagnostic) {
        return diagnostic.code == code;
    });
}

std::vector<float> compute_practical_shadow_cascade_distances(const std::uint32_t cascade_count,
                                                              const float camera_near, const float camera_far,
                                                              const float lambda) {
    if (cascade_count == 0U) {
        throw std::invalid_argument("shadow cascade count must be at least 1");
    }
    if (!std::isfinite(camera_near) || camera_near <= 0.0F) {
        throw std::invalid_argument("shadow cascade camera_near must be finite and positive");
    }
    if (!std::isfinite(camera_far) || !(camera_far > camera_near)) {
        throw std::invalid_argument("shadow cascade camera_far must be finite and greater than camera_near");
    }
    if (!std::isfinite(lambda) || lambda < 0.0F || lambda > 1.0F) {
        throw std::invalid_argument("shadow cascade lambda must be finite and in [0, 1]");
    }

    std::vector<float> distances;
    distances.reserve(static_cast<std::size_t>(cascade_count) + 1U);
    const auto inv_cascades = 1.0F / static_cast<float>(cascade_count);
    for (std::uint32_t i = 0; i <= cascade_count; ++i) {
        const auto ratio = static_cast<float>(i) * inv_cascades;
        const auto log_split = camera_near * std::pow(camera_far / camera_near, ratio);
        const auto uniform_split = camera_near + ((camera_far - camera_near) * ratio);
        distances.push_back((lambda * log_split) + ((1.0F - lambda) * uniform_split));
    }
    return distances;
}

} // namespace mirakana
