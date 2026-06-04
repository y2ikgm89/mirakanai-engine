// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/volumetric_fog_policy.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

constexpr std::uint32_t max_froxel_dimension = 4096U;
constexpr std::uint32_t max_froxel_depth_slices = 512U;
constexpr float max_range_m = 100000.0F;
constexpr float max_density = 10.0F;
constexpr float min_anisotropy = -0.99F;
constexpr float max_anisotropy = 0.99F;
constexpr float max_albedo_component = 1.0F;
constexpr float max_local_density_delta = 10.0F;
constexpr std::uint32_t max_raymarch_step_budget = 1024U;

void add_diagnostic(VolumetricFogPolicyPlan& plan, VolumetricFogDiagnosticCode code, std::string field,
                    std::uint32_t source_index, std::string message) {
    plan.diagnostics.push_back(VolumetricFogDiagnostic{
        .code = code,
        .field = std::move(field),
        .source_index = source_index,
        .message = std::move(message),
    });
}

[[nodiscard]] bool finite_in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

[[nodiscard]] bool finite_vec3(Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool valid_albedo(Vec3 value) noexcept {
    return finite_in_range(value.x, 0.0F, max_albedo_component) &&
           finite_in_range(value.y, 0.0F, max_albedo_component) && finite_in_range(value.z, 0.0F, max_albedo_component);
}

[[nodiscard]] bool valid_extent(Vec3 value) noexcept {
    return finite_in_range(value.x, 0.0001F, max_range_m) && finite_in_range(value.y, 0.0001F, max_range_m) &&
           finite_in_range(value.z, 0.0001F, max_range_m);
}

[[nodiscard]] bool valid_quality_tier(VolumetricFogQualityTier tier) noexcept {
    switch (tier) {
    case VolumetricFogQualityTier::low:
    case VolumetricFogQualityTier::medium:
    case VolumetricFogQualityTier::high:
    case VolumetricFogQualityTier::ultra:
    case VolumetricFogQualityTier::custom:
        return true;
    case VolumetricFogQualityTier::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_local_volume_shape(VolumetricFogLocalVolumeShape shape) noexcept {
    switch (shape) {
    case VolumetricFogLocalVolumeShape::box:
    case VolumetricFogLocalVolumeShape::sphere:
    case VolumetricFogLocalVolumeShape::ellipsoid:
    case VolumetricFogLocalVolumeShape::cylinder:
    case VolumetricFogLocalVolumeShape::cone:
    case VolumetricFogLocalVolumeShape::world:
        return true;
    case VolumetricFogLocalVolumeShape::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_froxel_grid(VolumetricFogFroxelGridDesc grid) noexcept {
    return grid.width > 0U && grid.width <= max_froxel_dimension && grid.height > 0U &&
           grid.height <= max_froxel_dimension && grid.depth_slices > 0U &&
           grid.depth_slices <= max_froxel_depth_slices;
}

void write_u32(std::span<std::uint8_t> dst, std::size_t offset, std::uint32_t value) {
    std::memcpy(dst.data() + offset, &value, sizeof(std::uint32_t));
}

void write_f32(std::span<std::uint8_t> dst, std::size_t offset, float value) {
    std::memcpy(dst.data() + offset, &value, sizeof(float));
}

[[nodiscard]] bool valid_local_volume(const VolumetricFogLocalVolumeDesc& volume) noexcept {
    return valid_local_volume_shape(volume.shape) && finite_vec3(volume.center_m) && valid_extent(volume.extent_m) &&
           finite_in_range(volume.density_delta, -max_local_density_delta, max_local_density_delta) &&
           valid_albedo(volume.albedo);
}

void append_froxel_grid_row(VolumetricFogPolicyPlan& plan, const VolumetricFogPolicyDesc& desc) {
    plan.froxel_grid_rows.push_back(VolumetricFogFroxelGridRow{
        .width = desc.froxel_grid.width,
        .height = desc.froxel_grid.height,
        .depth_slices = desc.froxel_grid.depth_slices,
        .voxel_count = static_cast<std::uint64_t>(desc.froxel_grid.width) * desc.froxel_grid.height *
                       desc.froxel_grid.depth_slices,
        .range_m = desc.range_m,
    });
}

void append_quality_row(VolumetricFogPolicyPlan& plan, const VolumetricFogPolicyDesc& desc) {
    plan.quality_rows.push_back(VolumetricFogQualityRow{
        .tier = desc.quality_tier,
        .raymarch_step_budget = desc.raymarch_step_budget,
        .temporal_reprojection_enabled = desc.temporal.enabled,
        .history_weight = desc.temporal.history_weight,
        .ready = plan.ready(),
    });
}

void append_depth_input_row(VolumetricFogPolicyPlan& plan) {
    plan.depth_input_rows.push_back(VolumetricFogDepthInputRow{
        .texture_binding_slot = volumetric_fog_scene_depth_texture_binding(),
        .sampler_binding_slot = volumetric_fog_scene_depth_sampler_binding(),
        .required = plan.requires_scene_depth,
        .available = plan.scene_depth_available,
    });
}

void append_local_volume_rows(VolumetricFogPolicyPlan& plan,
                              std::span<const VolumetricFogLocalVolumeDesc> local_volumes) {
    for (const auto& volume : local_volumes) {
        plan.local_volume_rows.push_back(VolumetricFogLocalVolumeRow{
            .shape = volume.shape,
            .center_m = volume.center_m,
            .extent_m = volume.extent_m,
            .density_delta = volume.density_delta,
            .albedo = volume.albedo,
            .subtracts_global_density = volume.subtracts_global_density,
            .source_index = volume.source_index,
        });
    }
}

} // namespace

bool VolumetricFogPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool VolumetricFogPolicyPlan::ready() const noexcept {
    return status == VolumetricFogPolicyStatus::ready;
}

void pack_volumetric_fog_constants(std::span<std::uint8_t> destination, const VolumetricFogPolicyDesc& desc) {
    if (destination.size() < volumetric_fog_constants_byte_size()) {
        throw std::invalid_argument("volumetric fog constants destination is too small");
    }

    std::ranges::fill(destination, std::uint8_t{0});
    write_u32(destination, 0U, desc.froxel_grid.width);
    write_u32(destination, 4U, desc.froxel_grid.height);
    write_u32(destination, 8U, desc.froxel_grid.depth_slices);
    write_f32(destination, 12U, desc.range_m);
    write_f32(destination, 16U, desc.density);
    write_f32(destination, 20U, desc.anisotropy);
    write_f32(destination, 24U, desc.temporal.history_weight);
    write_f32(destination, 28U, desc.albedo.x);
    write_f32(destination, 32U, desc.albedo.y);
    write_f32(destination, 36U, desc.albedo.z);
}

VolumetricFogPolicyPlan plan_volumetric_fog_policy(const VolumetricFogPolicyDesc& desc) {
    VolumetricFogPolicyPlan plan{
        .status = VolumetricFogPolicyStatus::planned,
        .scene_depth_available = desc.scene_depth_available,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
    };

    if (!valid_quality_tier(desc.quality_tier)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::unsupported_quality_tier, "quality_tier", 0,
                       "volumetric fog requires a supported quality tier");
    }
    if (!valid_froxel_grid(desc.froxel_grid)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_froxel_grid, "froxel_grid", 0,
                       "volumetric fog froxel grid dimensions and depth slices must be bounded positive values");
    }
    if (!finite_in_range(desc.range_m, 0.0001F, max_range_m)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_range, "range_m", 0,
                       "volumetric fog range must be finite and positive");
    }
    if (!finite_in_range(desc.density, 0.0F, max_density)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_density, "density", 0,
                       "volumetric fog density must be finite and non-negative");
    }
    if (!valid_albedo(desc.albedo)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_albedo, "albedo", 0,
                       "volumetric fog albedo must be finite non-negative linear color");
    }
    if (!finite_in_range(desc.anisotropy, min_anisotropy, max_anisotropy)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_anisotropy, "anisotropy", 0,
                       "volumetric fog anisotropy must be finite and bounded");
    }
    if (!finite_in_range(desc.temporal.history_weight, 0.0F, 1.0F)) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_temporal_history_weight, "temporal.history_weight", 0,
                       "volumetric fog temporal history weight must be finite and in [0, 1]");
    }
    if (desc.raymarch_step_budget == 0U || desc.raymarch_step_budget > max_raymarch_step_budget) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_raymarch_step_budget, "raymarch_step_budget", 0,
                       "volumetric fog raymarch step budget must be a non-zero bounded count");
    }
    if (!desc.scene_depth_available) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::missing_scene_depth, "scene_depth_available", 0,
                       "volumetric fog requires scene depth input evidence");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready", 0,
                       "volumetric fog requires validated shader contract evidence");
    }
    if (desc.request_ready_promotion && !desc.execution_evidence_ready) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::missing_execution_evidence, "execution_evidence_ready", 0,
                       "volumetric fog ready promotion requires D3D12 readback or package execution evidence");
    }
    if (desc.request_froxel_allocation) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::unsupported_froxel_allocation, "request_froxel_allocation", 0,
                       "volumetric fog policy planning must not allocate froxel textures in this slice");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::unsupported_backend_execution, "request_backend_execution", 0,
                       "volumetric fog policy planning must not invoke renderer or RHI backends in this slice");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, VolumetricFogDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access", 0,
                       "volumetric fog policy planning must not expose native renderer or RHI handles");
    }

    for (const auto& volume : desc.local_volumes) {
        if (!valid_local_volume(volume)) {
            add_diagnostic(plan, VolumetricFogDiagnosticCode::invalid_local_volume, "local_volumes",
                           volume.source_index,
                           "volumetric fog local volume rows require supported shape, finite center, positive extent, "
                           "bounded density delta, and valid albedo");
        }
    }

    if (!plan.succeeded()) {
        plan.status = VolumetricFogPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready) {
        plan.status = VolumetricFogPolicyStatus::ready;
    }

    if (valid_froxel_grid(desc.froxel_grid) && std::isfinite(desc.range_m)) {
        append_froxel_grid_row(plan, desc);
    }
    append_quality_row(plan, desc);
    append_depth_input_row(plan);
    append_local_volume_rows(plan, desc.local_volumes);

    return plan;
}

bool has_volumetric_fog_diagnostic(const VolumetricFogPolicyPlan& plan, VolumetricFogDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
