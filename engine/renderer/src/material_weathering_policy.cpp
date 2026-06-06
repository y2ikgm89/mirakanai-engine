// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/renderer/material_weathering_policy.hpp"

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <utility>

namespace mirakana {
namespace {

void add_diagnostic(MaterialWeatheringPolicyPlan& plan, MaterialWeatheringPolicyDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(MaterialWeatheringPolicyDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

[[nodiscard]] bool valid_quality_tier(MaterialWeatheringQualityTier tier) noexcept {
    switch (tier) {
    case MaterialWeatheringQualityTier::low:
    case MaterialWeatheringQualityTier::balanced:
    case MaterialWeatheringQualityTier::high:
    case MaterialWeatheringQualityTier::custom:
        return true;
    case MaterialWeatheringQualityTier::unknown:
        return false;
    }
    return false;
}

[[nodiscard]] bool valid_environment_plan(const EnvironmentMaterialWeatheringPlan& plan) noexcept {
    return plan.succeeded() && plan.status == EnvironmentMaterialWeatheringPlanStatus::planned && !plan.rows.empty() &&
           !plan.invokes_backend && !plan.exposes_native_handles && !plan.mutates_source_materials;
}

[[nodiscard]] float state_id(EnvironmentMaterialWeatheringState state) noexcept {
    switch (state) {
    case EnvironmentMaterialWeatheringState::dry:
        return 0.0F;
    case EnvironmentMaterialWeatheringState::wet:
        return 1.0F;
    case EnvironmentMaterialWeatheringState::snow_covered:
        return 2.0F;
    case EnvironmentMaterialWeatheringState::icy:
        return 3.0F;
    case EnvironmentMaterialWeatheringState::mixed:
        return 4.0F;
    }
    return 0.0F;
}

void write_f32(std::span<std::uint8_t> destination, std::size_t offset, float value) {
    std::memcpy(destination.data() + offset, &value, sizeof(float));
}

void append_constant_row(MaterialWeatheringPolicyPlan& plan, const EnvironmentMaterialWeatheringPlan& environment) {
    const auto& row = environment.rows.front();
    plan.constant_rows.push_back(MaterialWeatheringConstantRow{
        .state = row.state,
        .constants_binding_slot = material_weathering_constants_binding(),
        .constant_bytes = material_weathering_constants_byte_size(),
        .wetness_intensity = row.wetness_intensity,
        .snow_accumulation = row.snow_accumulation,
        .ice_intensity = row.ice_intensity,
    });
}

void append_quality_row(MaterialWeatheringPolicyPlan& plan, const MaterialWeatheringPolicyDesc& desc) {
    plan.quality_rows.push_back(MaterialWeatheringQualityRow{
        .tier = desc.quality_tier,
        .material_parameter_path = !plan.constant_rows.empty(),
        .ready = plan.ready(),
    });
}

} // namespace

bool MaterialWeatheringPolicyPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool MaterialWeatheringPolicyPlan::ready() const noexcept {
    return status == MaterialWeatheringPolicyStatus::ready;
}

void pack_material_weathering_constants(std::span<std::uint8_t> destination, const MaterialWeatheringPolicyDesc& desc) {
    if (destination.size() < material_weathering_constants_byte_size()) {
        throw std::invalid_argument("material weathering constants destination is too small");
    }

    std::ranges::fill(destination, std::uint8_t{0});
    if (desc.environment_plan.rows.empty()) {
        return;
    }

    const auto& row = desc.environment_plan.rows.front();
    write_f32(destination, 0U, row.wetness_intensity);
    write_f32(destination, 4U, row.snow_accumulation);
    write_f32(destination, 8U, row.ice_intensity);
    write_f32(destination, 12U, state_id(row.state));
    write_f32(destination, 16U, row.mutates_source_material ? 1.0F : 0.0F);
}

MaterialWeatheringPolicyPlan plan_material_weathering_policy(const MaterialWeatheringPolicyDesc& desc) {
    MaterialWeatheringPolicyPlan plan{
        .status = MaterialWeatheringPolicyStatus::planned,
        .shader_contract_evidence_ready = desc.shader_contract_evidence_ready,
        .package_evidence_ready = desc.package_evidence_ready,
        .execution_evidence_ready = desc.execution_evidence_ready,
    };

    if (!valid_environment_plan(desc.environment_plan)) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::invalid_environment_plan, "environment_plan",
                       "material weathering renderer policy requires a valid value-only environment plan");
    }
    if (!valid_quality_tier(desc.quality_tier)) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::unsupported_quality_tier, "quality_tier",
                       "material weathering renderer policy requires a supported quality tier");
    }
    if (!desc.shader_contract_evidence_ready) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::missing_shader_contract_evidence,
                       "shader_contract_evidence_ready",
                       "material weathering requires reviewed shader contract evidence");
    }
    if (!desc.package_evidence_ready) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::missing_package_evidence, "package_evidence_ready",
                       "material weathering requires package-visible material evidence");
    }
    if (desc.request_ready_promotion && !desc.execution_evidence_ready) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::missing_execution_evidence,
                       "execution_evidence_ready",
                       "material weathering ready promotion requires selected runtime execution evidence");
    }
    if (desc.request_material_parameter_binding &&
        (!desc.execution_evidence_ready || desc.material_parameter_binding_count == 0U ||
         desc.material_constant_bytes_uploaded == 0U)) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::missing_material_parameter_binding_evidence,
                       "material_parameter_binding_count",
                       "material weathering requires positive material parameter binding and constant upload evidence");
    }
    if (desc.request_backend_execution && (!desc.execution_evidence_ready || desc.backend_invocation_count == 0U)) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::unsupported_backend_execution,
                       "backend_invocation_count",
                       "material weathering backend execution requires a positive selected backend invocation counter");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::unsupported_native_handle_claim,
                       "request_native_handle_access",
                       "material weathering policy planning must not expose native renderer or RHI handles");
    }
    if (desc.request_source_material_mutation) {
        add_diagnostic(plan, MaterialWeatheringPolicyDiagnosticCode::unsupported_source_material_mutation,
                       "request_source_material_mutation",
                       "runtime material weathering must not rewrite source material documents");
    }

    if (!plan.succeeded()) {
        plan.status = MaterialWeatheringPolicyStatus::blocked;
    } else if (desc.request_ready_promotion && desc.execution_evidence_ready) {
        plan.status = MaterialWeatheringPolicyStatus::ready;
        if (desc.request_material_parameter_binding) {
            plan.material_parameter_bindings = desc.material_parameter_binding_count;
            plan.material_constant_bytes_uploaded = desc.material_constant_bytes_uploaded;
        }
        if (desc.request_backend_execution) {
            plan.invokes_backend = true;
            plan.backend_invocations = desc.backend_invocation_count;
        }
    }

    if (valid_environment_plan(desc.environment_plan)) {
        append_constant_row(plan, desc.environment_plan);
        plan.wet_rows = desc.environment_plan.wet_rows;
        plan.snow_rows = desc.environment_plan.snow_rows;
        plan.ice_rows = desc.environment_plan.ice_rows;
    }
    append_quality_row(plan, desc);

    return plan;
}

bool has_material_weathering_policy_diagnostic(const MaterialWeatheringPolicyPlan& plan,
                                               MaterialWeatheringPolicyDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
