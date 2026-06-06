// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/cloud_layer_policy.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(CloudLayerPolicyPlan& plan, CloudLayerDiagnosticCode code, std::string field, std::string message) {
    plan.diagnostics.push_back(CloudLayerDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool valid_quality_tier(CloudLayerQualityTier tier) noexcept {
    switch (tier) {
    case CloudLayerQualityTier::low:
    case CloudLayerQualityTier::balanced:
    case CloudLayerQualityTier::high:
    case CloudLayerQualityTier::custom:
        return true;
    case CloudLayerQualityTier::unknown:
        return false;
    }
    return false;
}

void write_f32(std::span<std::uint8_t> destination, std::size_t offset, float value) {
    std::memcpy(destination.data() + offset, &value, sizeof(float));
}

void append_rows(CloudLayerPolicyPlan& plan, const CloudLayerPolicyDesc& desc) {
    plan.texture_rows.push_back(CloudLayerTextureRow{
        .cloud_map_asset_ref = desc.layer.cloud_map_asset_ref,
        .flow_map_asset_ref = desc.layer.flow_map_asset_ref,
        .cloud_map_binding_slot = cloud_layer_cloud_map_binding(),
        .flow_map_binding_slot = cloud_layer_flow_map_binding(),
        .sampler_binding_slot = cloud_layer_sampler_binding(),
        .package_evidence_ready = desc.package_evidence_ready,
        .uploads_texture = plan.uploads_textures,
    });
    plan.visual_rows.push_back(CloudLayerVisualRow{
        .coverage = desc.layer.coverage,
        .opacity = desc.layer.opacity,
        .altitude_m = desc.layer.altitude_m,
        .wind_velocity_mps = desc.layer.wind_velocity_mps,
        .sky_tint_response = desc.layer.sky_tint_response,
        .time_of_day_response = desc.layer.time_of_day_response,
    });
    plan.ibl_rows.push_back(CloudLayerIblRow{
        .mode = desc.layer.ibl_contribution_mode,
        .contribution = desc.layer.ibl_contribution,
    });
    plan.shader_contract_rows.push_back(CloudLayerShaderContractRow{
        .constants_binding_slot = cloud_layer_constants_binding(),
        .uses_latlong_projection = true,
        .uses_flow_map = true,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
    });
    plan.quality_rows.push_back(CloudLayerQualityRow{
        .tier = desc.quality_tier,
        .default_lower_tier_cloud_path = true,
        .ready = plan.ready(),
    });
}

} // namespace

bool CloudLayerPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool CloudLayerPolicyPlan::ready() const noexcept {
    return status == CloudLayerPolicyStatus::ready;
}

void pack_cloud_layer_constants(std::span<std::uint8_t> destination, const CloudLayerPolicyDesc& desc) {
    if (destination.size() < cloud_layer_constants_byte_size()) {
        throw std::invalid_argument("cloud layer constants destination is too small");
    }

    std::ranges::fill(destination, std::uint8_t{0});
    write_f32(destination, 0U, desc.layer.coverage);
    write_f32(destination, 4U, desc.layer.opacity);
    write_f32(destination, 8U, desc.layer.altitude_m);
    write_f32(destination, 12U, desc.layer.wind_velocity_mps.x);
    write_f32(destination, 16U, desc.layer.wind_velocity_mps.y);
    write_f32(destination, 20U, 0.0F);
    write_f32(destination, 24U, desc.layer.sky_tint_response.x);
    write_f32(destination, 28U, desc.layer.sky_tint_response.y);
    write_f32(destination, 32U, desc.layer.sky_tint_response.z);
    write_f32(destination, 36U, desc.layer.time_of_day_response);
}

CloudLayerPolicyPlan plan_cloud_layer_policy(const CloudLayerPolicyDesc& desc) {
    const auto environment_result = validate_environment_cloud_layer(desc.layer);
    CloudLayerPolicyPlan plan{
        .status = CloudLayerPolicyStatus::planned,
        .uses_2d_cloud_layer = desc.layer.mode == EnvironmentCloudLayerMode::equirectangular_2d,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .package_evidence_ready = desc.package_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
    };

    if (!environment_result.succeeded()) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::invalid_environment_layer, "layer",
                       "cloud layer renderer policy requires a valid environment cloud layer");
    }
    if (!valid_quality_tier(desc.quality_tier)) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::unsupported_quality_tier, "quality_tier",
                       "cloud layer policy requires a supported quality tier");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready",
                       "cloud layer policy requires validated shader contract evidence");
    }
    if (!desc.package_evidence_ready) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::missing_package_evidence, "package_evidence_ready",
                       "cloud layer policy requires package evidence for referenced cloud textures");
    }
    const bool requires_execution_evidence =
        desc.request_ready_promotion || desc.request_texture_upload || desc.request_backend_execution;
    if (requires_execution_evidence && !desc.execution_evidence_ready) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::missing_execution_evidence, "execution_evidence_ready",
                       "cloud layer execution requires D3D12 readback or package execution evidence");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::unsupported_native_handle_claim, "request_native_handle_access",
                       "cloud layer policy planning must not expose native renderer or RHI handles");
    }
    if (desc.request_volumetric_clouds || desc.layer.request_volumetric_clouds ||
        desc.layer.mode == EnvironmentCloudLayerMode::volumetric) {
        add_diagnostic(plan, CloudLayerDiagnosticCode::unsupported_volumetric_clouds, "request_volumetric_clouds",
                       "cloud layer v1 does not claim volumetric cloud execution");
    }

    if (plan.succeeded()) {
        plan.uploads_textures = desc.request_texture_upload;
        plan.invokes_backend = desc.request_backend_execution;
        plan.renderer_draws = desc.request_backend_execution ? 1U : 0U;
    }

    if (!plan.succeeded()) {
        plan.status = CloudLayerPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready) {
        plan.status = CloudLayerPolicyStatus::ready;
    }

    if (environment_result.succeeded()) {
        append_rows(plan, desc);
    }

    return plan;
}

bool has_cloud_layer_diagnostic(const CloudLayerPolicyPlan& plan, CloudLayerDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
