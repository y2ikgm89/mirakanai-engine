// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class VolumetricFogQualityTier : std::uint8_t {
    unknown = 0,
    low,
    medium,
    high,
    ultra,
    custom,
};

enum class VolumetricFogLocalVolumeShape : std::uint8_t {
    unknown = 0,
    box,
    sphere,
    ellipsoid,
    cylinder,
    cone,
    world,
};

enum class VolumetricFogPolicyStatus : std::uint8_t {
    blocked = 0,
    planned,
    ready,
};

enum class VolumetricFogDiagnosticCode : std::uint8_t {
    none = 0,
    unsupported_quality_tier,
    invalid_froxel_grid,
    invalid_range,
    invalid_density,
    invalid_albedo,
    invalid_anisotropy,
    invalid_temporal_history_weight,
    invalid_local_volume,
    invalid_raymarch_step_budget,
    missing_scene_depth,
    missing_shader_contract_evidence,
    missing_execution_evidence,
    missing_package_evidence,
    unsupported_froxel_allocation,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
};

[[nodiscard]] constexpr std::uint32_t volumetric_fog_scene_depth_texture_binding() noexcept {
    return 2;
}

[[nodiscard]] constexpr std::uint32_t volumetric_fog_scene_depth_sampler_binding() noexcept {
    return 3;
}

[[nodiscard]] constexpr std::uint32_t volumetric_fog_constants_binding() noexcept {
    return 5;
}

[[nodiscard]] constexpr std::uint32_t volumetric_fog_froxel_output_buffer_binding() noexcept {
    return 13;
}

[[nodiscard]] constexpr std::size_t volumetric_fog_constants_byte_size() noexcept {
    return 256;
}

struct VolumetricFogFroxelGridDesc {
    std::uint32_t width{160};
    std::uint32_t height{90};
    std::uint32_t depth_slices{64};
};

struct VolumetricFogTemporalDesc {
    bool enabled{true};
    float history_weight{0.9F};
};

struct VolumetricFogLocalVolumeDesc {
    VolumetricFogLocalVolumeShape shape{VolumetricFogLocalVolumeShape::box};
    Vec3 center_m{};
    Vec3 extent_m{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float density_delta{0.0F};
    Vec3 albedo{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    bool subtracts_global_density{false};
    std::uint32_t source_index{0};
};

struct VolumetricFogPolicyDesc {
    VolumetricFogQualityTier quality_tier{VolumetricFogQualityTier::medium};
    VolumetricFogFroxelGridDesc froxel_grid;
    float range_m{100.0F};
    float density{0.05F};
    Vec3 albedo{.x = 1.0F, .y = 1.0F, .z = 1.0F};
    float anisotropy{0.0F};
    VolumetricFogTemporalDesc temporal;
    std::span<const VolumetricFogLocalVolumeDesc> local_volumes;
    std::uint32_t raymarch_step_budget{64};
    bool scene_depth_available{false};
    bool shader_contract_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool package_evidence_ready{false};
    bool request_ready_promotion{false};
    bool request_froxel_allocation{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
};

struct VolumetricFogFroxelGridRow {
    std::uint32_t width{0};
    std::uint32_t height{0};
    std::uint32_t depth_slices{0};
    std::uint64_t voxel_count{0};
    float range_m{0.0F};
};

struct VolumetricFogQualityRow {
    VolumetricFogQualityTier tier{VolumetricFogQualityTier::unknown};
    std::uint32_t raymarch_step_budget{0};
    bool temporal_reprojection_enabled{false};
    float history_weight{0.0F};
    bool ready{false};
};

struct VolumetricFogDepthInputRow {
    std::uint32_t texture_binding_slot{0};
    std::uint32_t sampler_binding_slot{0};
    bool required{true};
    bool available{false};
};

struct VolumetricFogLocalVolumeRow {
    VolumetricFogLocalVolumeShape shape{VolumetricFogLocalVolumeShape::unknown};
    Vec3 center_m{};
    Vec3 extent_m{};
    float density_delta{0.0F};
    Vec3 albedo{};
    bool subtracts_global_density{false};
    std::uint32_t source_index{0};
};

struct VolumetricFogDiagnostic {
    VolumetricFogDiagnosticCode code{VolumetricFogDiagnosticCode::none};
    std::string field;
    std::uint32_t source_index{0};
    std::string message;
};

struct VolumetricFogPolicyPlan {
    VolumetricFogPolicyStatus status{VolumetricFogPolicyStatus::blocked};
    bool requires_scene_depth{true};
    bool scene_depth_available{false};
    bool requires_shader_contract_evidence{true};
    bool shader_contract_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool package_evidence_ready{false};
    bool allocates_froxel_volume{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<VolumetricFogFroxelGridRow> froxel_grid_rows;
    std::vector<VolumetricFogQualityRow> quality_rows;
    std::vector<VolumetricFogDepthInputRow> depth_input_rows;
    std::vector<VolumetricFogLocalVolumeRow> local_volume_rows;
    std::vector<VolumetricFogDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] VolumetricFogPolicyPlan plan_volumetric_fog_policy(const VolumetricFogPolicyDesc& desc);

void pack_volumetric_fog_constants(std::span<std::uint8_t> destination, const VolumetricFogPolicyDesc& desc);

[[nodiscard]] bool has_volumetric_fog_diagnostic(const VolumetricFogPolicyPlan& plan,
                                                 VolumetricFogDiagnosticCode code) noexcept;

} // namespace mirakana
