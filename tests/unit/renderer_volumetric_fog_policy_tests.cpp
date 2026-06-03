// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/renderer/volumetric_fog_policy.hpp"

#include <array>
#include <cstdint>
#include <limits>

namespace {

[[nodiscard]] mirakana::VolumetricFogLocalVolumeDesc make_valid_local_volume(std::uint32_t source_index) {
    return mirakana::VolumetricFogLocalVolumeDesc{
        .shape = mirakana::VolumetricFogLocalVolumeShape::box,
        .center_m = mirakana::Vec3{.x = 16.0F, .y = 4.0F, .z = -8.0F},
        .extent_m = mirakana::Vec3{.x = 12.0F, .y = 3.0F, .z = 20.0F},
        .density_delta = 0.12F,
        .albedo = mirakana::Vec3{.x = 0.72F, .y = 0.76F, .z = 0.80F},
        .subtracts_global_density = false,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::VolumetricFogPolicyDesc
make_valid_volumetric_fog_desc(std::span<const mirakana::VolumetricFogLocalVolumeDesc> local_volumes) {
    return mirakana::VolumetricFogPolicyDesc{
        .quality_tier = mirakana::VolumetricFogQualityTier::high,
        .froxel_grid =
            mirakana::VolumetricFogFroxelGridDesc{
                .width = 320,
                .height = 180,
                .depth_slices = 64,
            },
        .range_m = 160.0F,
        .density = 0.035F,
        .albedo = mirakana::Vec3{.x = 0.86F, .y = 0.90F, .z = 0.95F},
        .anisotropy = 0.35F,
        .temporal =
            mirakana::VolumetricFogTemporalDesc{
                .enabled = true,
                .history_weight = 0.85F,
            },
        .local_volumes = local_volumes,
        .raymarch_step_budget = 48,
        .scene_depth_available = true,
        .shader_contract_evidence_ready = true,
    };
}

} // namespace

MK_TEST("renderer volumetric fog policy plans froxel quality temporal and local volume rows") {
    const std::array local_volumes = {
        make_valid_local_volume(11),
        mirakana::VolumetricFogLocalVolumeDesc{
            .shape = mirakana::VolumetricFogLocalVolumeShape::ellipsoid,
            .center_m = mirakana::Vec3{.x = -2.0F, .y = 1.0F, .z = 3.0F},
            .extent_m = mirakana::Vec3{.x = 4.0F, .y = 2.0F, .z = 4.0F},
            .density_delta = -0.025F,
            .albedo = mirakana::Vec3{.x = 0.45F, .y = 0.50F, .z = 0.56F},
            .subtracts_global_density = true,
            .source_index = 12,
        },
    };

    const auto plan = mirakana::plan_volumetric_fog_policy(make_valid_volumetric_fog_desc(local_volumes));

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::VolumetricFogPolicyStatus::planned);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(plan.requires_scene_depth);
    MK_REQUIRE(plan.scene_depth_available);
    MK_REQUIRE(plan.requires_shader_contract_evidence);
    MK_REQUIRE(plan.shader_contract_evidence_ready);
    MK_REQUIRE(!plan.execution_evidence_ready);
    MK_REQUIRE(!plan.allocates_froxel_volume);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);

    MK_REQUIRE(plan.froxel_grid_rows.size() == 1);
    MK_REQUIRE(plan.froxel_grid_rows[0].width == 320);
    MK_REQUIRE(plan.froxel_grid_rows[0].height == 180);
    MK_REQUIRE(plan.froxel_grid_rows[0].depth_slices == 64);
    MK_REQUIRE(plan.froxel_grid_rows[0].voxel_count == 320U * 180U * 64U);
    MK_REQUIRE(plan.froxel_grid_rows[0].range_m == 160.0F);

    MK_REQUIRE(plan.quality_rows.size() == 1);
    MK_REQUIRE(plan.quality_rows[0].tier == mirakana::VolumetricFogQualityTier::high);
    MK_REQUIRE(plan.quality_rows[0].raymarch_step_budget == 48);
    MK_REQUIRE(plan.quality_rows[0].temporal_reprojection_enabled);
    MK_REQUIRE(plan.quality_rows[0].history_weight == 0.85F);
    MK_REQUIRE(!plan.quality_rows[0].ready);

    MK_REQUIRE(plan.depth_input_rows.size() == 1);
    MK_REQUIRE(plan.depth_input_rows[0].required);
    MK_REQUIRE(plan.depth_input_rows[0].available);

    MK_REQUIRE(plan.local_volume_rows.size() == 2);
    MK_REQUIRE(plan.local_volume_rows[0].shape == mirakana::VolumetricFogLocalVolumeShape::box);
    MK_REQUIRE(plan.local_volume_rows[0].source_index == 11);
    MK_REQUIRE(!plan.local_volume_rows[0].subtracts_global_density);
    MK_REQUIRE(plan.local_volume_rows[1].shape == mirakana::VolumetricFogLocalVolumeShape::ellipsoid);
    MK_REQUIRE(plan.local_volume_rows[1].source_index == 12);
    MK_REQUIRE(plan.local_volume_rows[1].subtracts_global_density);
}

MK_TEST("renderer volumetric fog policy requires execution evidence before ready promotion") {
    const std::array local_volumes = {
        make_valid_local_volume(1),
    };
    auto desc = make_valid_volumetric_fog_desc(local_volumes);
    desc.request_ready_promotion = true;

    const auto blocked_plan = mirakana::plan_volumetric_fog_policy(desc);

    MK_REQUIRE(!blocked_plan.succeeded());
    MK_REQUIRE(blocked_plan.status == mirakana::VolumetricFogPolicyStatus::blocked);
    MK_REQUIRE(!blocked_plan.ready());
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        blocked_plan, mirakana::VolumetricFogDiagnosticCode::missing_execution_evidence));

    desc.execution_evidence_ready = true;
    const auto ready_plan = mirakana::plan_volumetric_fog_policy(desc);

    MK_REQUIRE(ready_plan.succeeded());
    MK_REQUIRE(ready_plan.status == mirakana::VolumetricFogPolicyStatus::ready);
    MK_REQUIRE(ready_plan.ready());
    MK_REQUIRE(ready_plan.execution_evidence_ready);
    MK_REQUIRE(ready_plan.quality_rows[0].ready);
}

MK_TEST("renderer volumetric fog policy fails closed for invalid rows and unsafe claims") {
    const std::array local_volumes = {
        mirakana::VolumetricFogLocalVolumeDesc{
            .shape = mirakana::VolumetricFogLocalVolumeShape::unknown,
            .center_m = mirakana::Vec3{.x = 0.0F, .y = 0.0F, .z = 0.0F},
            .extent_m = mirakana::Vec3{.x = 0.0F, .y = 2.0F, .z = 2.0F},
            .density_delta = std::numeric_limits<float>::quiet_NaN(),
            .albedo = mirakana::Vec3{.x = 1.0F, .y = -0.1F, .z = 1.0F},
            .subtracts_global_density = false,
            .source_index = 99,
        },
    };
    auto desc = make_valid_volumetric_fog_desc(local_volumes);
    desc.quality_tier = mirakana::VolumetricFogQualityTier::unknown;
    desc.froxel_grid.width = 0;
    desc.froxel_grid.depth_slices = 0;
    desc.range_m = 0.0F;
    desc.density = -1.0F;
    desc.albedo.x = -0.1F;
    desc.albedo.y = 1.1F;
    desc.anisotropy = 1.5F;
    desc.temporal.enabled = false;
    desc.temporal.history_weight = 1.2F;
    desc.raymarch_step_budget = 0;
    desc.scene_depth_available = false;
    desc.shader_contract_evidence_ready = false;
    desc.request_froxel_allocation = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;

    const auto plan = mirakana::plan_volumetric_fog_policy(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::VolumetricFogPolicyStatus::blocked);
    MK_REQUIRE(!plan.ready());
    MK_REQUIRE(!plan.allocates_froxel_volume);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(
        mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::unsupported_quality_tier));
    MK_REQUIRE(
        mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_froxel_grid));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_range));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_density));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_albedo));
    MK_REQUIRE(
        mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_anisotropy));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::invalid_temporal_history_weight));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::invalid_raymarch_step_budget));
    MK_REQUIRE(
        mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::invalid_local_volume));
    MK_REQUIRE(
        mirakana::has_volumetric_fog_diagnostic(plan, mirakana::VolumetricFogDiagnosticCode::missing_scene_depth));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::missing_shader_contract_evidence));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::unsupported_froxel_allocation));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_volumetric_fog_diagnostic(
        plan, mirakana::VolumetricFogDiagnosticCode::unsupported_native_handle_claim));
}

int main() {
    return mirakana::test::run_all();
}
