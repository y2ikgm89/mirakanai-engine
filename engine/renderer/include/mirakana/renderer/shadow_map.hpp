// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/mat4.hpp"
#include "mirakana/renderer/frame_graph.hpp"
#include "mirakana/rhi/rhi.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class ShadowMapLightType { unknown = 0, directional, point, spot };

enum class ShadowMapDiagnosticCode {
    none = 0,
    missing_light,
    unsupported_light_type,
    light_does_not_cast_shadows,
    invalid_extent,
    invalid_directional_cascade_count,
    shadow_atlas_extent_overflow,
    unsupported_depth_format,
    missing_casters,
    missing_receivers,
    frame_graph_failed,
};

enum class ShadowReceiverDiagnosticCode {
    none = 0,
    invalid_shadow_map_plan,
    invalid_light_space_plan,
    light_space_cascade_mismatch,
    invalid_depth_bias,
    invalid_lit_intensity,
    invalid_shadow_intensity,
    shadow_intensity_exceeds_lit_intensity,
    invalid_filter_mode,
    invalid_filter_radius_texels,
};

enum class ShadowReceiverFilterMode { none = 0, fixed_pcf_3x3 };

enum class DirectionalShadowLightSpaceDiagnosticCode {
    none = 0,
    invalid_shadow_map_plan,
    invalid_light_direction,
    invalid_focus_center,
    invalid_focus_radius,
    invalid_depth_radius,
    missing_cascade_camera,
    unsupported_cascade_camera_projection,
};

enum class ShadowLightSpaceTexelSnap { disabled = 0, enabled };

[[nodiscard]] constexpr std::uint32_t invalid_shadow_map_light_index() noexcept {
    return static_cast<std::uint32_t>(-1);
}

[[nodiscard]] constexpr std::uint32_t shadow_receiver_depth_texture_binding() noexcept {
    return 0;
}

[[nodiscard]] constexpr std::uint32_t shadow_receiver_sampler_binding() noexcept {
    return 1;
}

/// Uniform buffer binding for cascaded directional shadow receiver constants (D3D `register(b2)` / Vulkan binding 2).
[[nodiscard]] constexpr std::uint32_t shadow_receiver_constants_binding() noexcept {
    return 2;
}

/// D3D12/Vulkan constant-buffer size (256-byte multiple). Layout matches `pack_shadow_receiver_constants`.
[[nodiscard]] constexpr std::size_t shadow_receiver_constants_byte_size() noexcept {
    return 768;
}

struct ShadowMapLightDesc {
    ShadowMapLightType type{ShadowMapLightType::unknown};
    bool casts_shadows{false};
    std::uint32_t source_index{invalid_shadow_map_light_index()};
};

struct ShadowMapDesc {
    ShadowMapLightDesc light;
    /// Per-cascade shadow map resolution (atlas tile width/height). Atlas width is `extent.width *
    /// directional_cascade_count`.
    rhi::Extent2D extent;
    rhi::Format depth_format{rhi::Format::depth24_stencil8};
    std::uint32_t caster_count{0};
    std::uint32_t receiver_count{0};
    /// Number of directional shadow cascades (horizontal atlas). Must be >= 1 and <= 8.
    std::uint32_t directional_cascade_count{1};
};

struct ShadowMapDiagnostic {
    ShadowMapDiagnosticCode code{ShadowMapDiagnosticCode::none};
    std::string message;
};

struct ShadowMapPlan {
    ShadowMapLightDesc light;
    std::uint32_t selected_light_index{invalid_shadow_map_light_index()};
    std::uint32_t caster_count{0};
    std::uint32_t receiver_count{0};
    /// Matches `ShadowMapDesc::directional_cascade_count` when the plan succeeds.
    std::uint32_t directional_cascade_count{0};
    /// Tile resolution (one cascade) copied from the successful `ShadowMapDesc::extent`.
    rhi::Extent2D cascade_tile_extent{};
    rhi::TextureDesc depth_texture;
    /// Compiled Frame Graph v1 plan for the directional shadow-depth to receiver resolve subgraph.
    FrameGraphV1BuildResult frame_graph_plan;
    /// Deterministic barrier + pass_invoke schedule for `frame_graph_plan` when compilation succeeded.
    std::vector<FrameGraphExecutionStep> frame_graph_execution;
    std::vector<ShadowMapDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct DirectionalShadowLightSpacePlan;

struct ShadowReceiverDesc {
    const ShadowMapPlan* shadow_map{nullptr};
    /// Required whenever `shadow_map` is non-null (successful receiver plans copy GPU constants from this plan).
    const DirectionalShadowLightSpacePlan* light_space{nullptr};
    float depth_bias{0.005F};
    float lit_intensity{1.0F};
    float shadow_intensity{0.35F};
    ShadowReceiverFilterMode filter_mode{ShadowReceiverFilterMode::fixed_pcf_3x3};
    float filter_radius_texels{1.0F};
};

struct ShadowReceiverDiagnostic {
    ShadowReceiverDiagnosticCode code{ShadowReceiverDiagnosticCode::none};
    std::string message;
};

struct ShadowReceiverPlan {
    rhi::TextureDesc depth_texture;
    rhi::DescriptorSetLayoutDesc descriptor_set_layout;
    rhi::SamplerDesc sampler;
    std::uint32_t receiver_count{0};
    std::uint32_t directional_cascade_count{0};
    rhi::Extent2D cascade_tile_extent{};
    /// Row-major `clip_from_world` per cascade (same order as
    /// `DirectionalShadowLightSpacePlan::clip_from_world_cascades`).
    std::vector<Mat4> clip_from_world_cascades;
    /// View-space positive split distances (`cascade_count + 1` entries when cascades > 1); empty for single cascade.
    std::vector<float> cascade_split_distances;
    float depth_bias{0.005F};
    float lit_intensity{1.0F};
    float shadow_intensity{0.35F};
    ShadowReceiverFilterMode filter_mode{ShadowReceiverFilterMode::fixed_pcf_3x3};
    float filter_radius_texels{1.0F};
    std::uint32_t filter_tap_count{0};
    std::vector<ShadowReceiverDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

/// Camera frustum input for cascaded directional shadow matrix fitting (world-space corners along view rays).
///
/// When `valid` is true, `world_from_camera` is the scene camera node transform (local-to-world). Columns 0-2 are
/// orthonormal camera basis vectors; column 3 is the eye position. Forward is the camera +Z axis (third column).
struct DirectionalShadowCascadeCameraInput {
    Mat4 world_from_camera{Mat4::identity()};
    bool perspective{true};
    float vertical_fov_radians{1.04719758F};
    float orthographic_height{10.0F};
    float near_plane{0.1F};
    float far_plane{1000.0F};
    float viewport_aspect{16.0F / 9.0F};
    bool valid{false};
};

struct DirectionalShadowLightSpaceDesc {
    const ShadowMapPlan* shadow_map{nullptr};
    Vec3 light_direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 focus_center;
    float focus_radius{0.0F};
    float depth_radius{0.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
    /// Required when `shadow_map->directional_cascade_count > 1` (unless diagnostics are acceptable).
    const DirectionalShadowCascadeCameraInput* cascade_camera{nullptr};
    /// Passed to `compute_practical_shadow_cascade_distances` for multi-cascade splits (ignored for single cascade).
    float cascade_split_lambda{0.75F};
};

struct DirectionalShadowLightSpaceDiagnostic {
    DirectionalShadowLightSpaceDiagnosticCode code{DirectionalShadowLightSpaceDiagnosticCode::none};
    std::string message;
};

struct DirectionalShadowLightSpacePlan {
    Vec3 light_direction{.x = 0.0F, .y = -1.0F, .z = 0.0F};
    Vec3 right{.x = 1.0F, .y = 0.0F, .z = 0.0F};
    Vec3 up{.x = 0.0F, .y = 0.0F, .z = 1.0F};
    Vec3 focus_center;
    Vec3 snapped_focus_center;
    float focus_radius{0.0F};
    float depth_radius{0.0F};
    /// Orthographic half-extents for cascade 0 (xy plane in light view); informational for callers.
    float ortho_width{0.0F};
    float ortho_height{0.0F};
    float near_z{0.0F};
    float far_z{0.0F};
    float texel_world_size{0.0F};
    ShadowLightSpaceTexelSnap texel_snap{ShadowLightSpaceTexelSnap::enabled};
    Mat4 view_from_world;
    /// One entry per directional cascade; `clip_from_world_cascades[i] == clip_from_view_cascades[i] *
    /// view_from_world`.
    std::vector<Mat4> clip_from_view_cascades;
    std::vector<Mat4> clip_from_world_cascades;
    /// Camera-space positive distances along the view axis; empty for single-cascade; length `cascade_count + 1` when
    /// multi-cascade splits were computed from `DirectionalShadowCascadeCameraInput`.
    std::vector<float> cascade_split_distances;
    std::vector<DirectionalShadowLightSpaceDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] ShadowMapPlan build_shadow_map_plan(const ShadowMapDesc& desc);
[[nodiscard]] bool has_shadow_map_diagnostic(const ShadowMapPlan& plan, ShadowMapDiagnosticCode code) noexcept;
[[nodiscard]] ShadowReceiverPlan build_shadow_receiver_plan(const ShadowReceiverDesc& desc);
[[nodiscard]] bool has_shadow_receiver_diagnostic(const ShadowReceiverPlan& plan,
                                                  ShadowReceiverDiagnosticCode code) noexcept;

/// Packs `DirectionalShadowLightSpacePlan` data into the constant buffer layout consumed by directional shadow receiver
/// shaders (`shadow_receiver_constants_byte_size()` bytes, 256-byte padded for D3D12 cbuffer rules).
///
/// Layout (offsets): `uint32_t cascade_count` @0; padding to 16; 12 floats split distances @16 (unused when
/// `cascade_count <= 1`); 8 row-major `Mat4` `clip_from_world` @64; row-major `camera_view_from_world` @576 (view-space
/// depth for cascade selection uses row 2 dotted with world xyz + row 2 w); zero padding to 768.
void pack_shadow_receiver_constants(std::span<std::uint8_t> dst, const DirectionalShadowLightSpacePlan& light_space,
                                    std::uint32_t cascade_count, const Mat4& camera_view_from_world);
[[nodiscard]] DirectionalShadowLightSpacePlan
build_directional_shadow_light_space_plan(const DirectionalShadowLightSpaceDesc& desc);
[[nodiscard]] bool
has_directional_shadow_light_space_diagnostic(const DirectionalShadowLightSpacePlan& plan,
                                              DirectionalShadowLightSpaceDiagnosticCode code) noexcept;

/// Practical split scheme for cascaded directional shadow distances in camera view space (positive depth).
///
/// Blends logarithmic and uniform partitions of \p [camera_near, camera_far] (Zhang et al., GPU Gems 3 Ch.10 style).
/// \p cascade_count is the number of cascades (must be >= 1). Returns exactly `cascade_count + 1` values with
/// strictly increasing depths; index `0` equals \p camera_near and index `cascade_count` equals \p camera_far.
/// \p lambda in `[0, 1]` weights the logarithmic term (`1` = fully logarithmic, `0` = fully uniform).
///
/// \throws std::invalid_argument when inputs are non-finite, \p camera_near is not positive, \p camera_far is not
/// greater than \p camera_near, \p cascade_count is zero, or \p lambda is outside `[0, 1]`.
[[nodiscard]] std::vector<float> compute_practical_shadow_cascade_distances(std::uint32_t cascade_count,
                                                                            float camera_near, float camera_far,
                                                                            float lambda = 0.75F);

} // namespace mirakana
