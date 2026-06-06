// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/material_weathering.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

enum class MaterialWeatheringQualityTier : std::uint8_t {
    unknown = 0,
    low,
    balanced,
    high,
    custom,
};

enum class MaterialWeatheringPolicyStatus : std::uint8_t {
    blocked = 0,
    planned,
    ready,
};

enum class MaterialWeatheringPolicyDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_environment_plan,
    unsupported_quality_tier,
    missing_shader_contract_evidence,
    missing_package_evidence,
    missing_execution_evidence,
    missing_material_parameter_binding_evidence,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_source_material_mutation,
};

[[nodiscard]] constexpr std::uint32_t material_weathering_constants_binding() noexcept {
    return 14;
}

[[nodiscard]] constexpr std::size_t material_weathering_constants_byte_size() noexcept {
    return 256;
}

struct MaterialWeatheringPolicyDesc {
    EnvironmentMaterialWeatheringPlan environment_plan;
    MaterialWeatheringQualityTier quality_tier{MaterialWeatheringQualityTier::balanced};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    bool request_ready_promotion{false};
    bool request_material_parameter_binding{false};
    bool request_backend_execution{false};
    std::uint64_t material_parameter_binding_count{0};
    std::uint64_t material_constant_bytes_uploaded{0};
    std::uint64_t backend_invocation_count{0};
    bool request_native_handle_access{false};
    bool request_source_material_mutation{false};
};

struct MaterialWeatheringConstantRow {
    EnvironmentMaterialWeatheringState state{EnvironmentMaterialWeatheringState::dry};
    std::uint32_t constants_binding_slot{0};
    std::size_t constant_bytes{0};
    float wetness_intensity{0.0F};
    float snow_accumulation{0.0F};
    float ice_intensity{0.0F};
};

struct MaterialWeatheringQualityRow {
    MaterialWeatheringQualityTier tier{MaterialWeatheringQualityTier::unknown};
    bool material_parameter_path{false};
    bool ready{false};
};

struct MaterialWeatheringPolicyDiagnostic {
    MaterialWeatheringPolicyDiagnosticCode code{MaterialWeatheringPolicyDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct MaterialWeatheringPolicyPlan {
    MaterialWeatheringPolicyStatus status{MaterialWeatheringPolicyStatus::blocked};
    bool shader_contract_evidence_ready{false};
    bool package_evidence_ready{false};
    bool execution_evidence_ready{false};
    std::uint64_t material_parameter_bindings{0};
    std::uint64_t material_constant_bytes_uploaded{0};
    bool invokes_backend{false};
    std::uint64_t backend_invocations{0};
    bool exposes_native_handles{false};
    bool mutates_source_materials{false};
    std::vector<MaterialWeatheringConstantRow> constant_rows;
    std::vector<EnvironmentMaterialWetnessRow> wet_rows;
    std::vector<EnvironmentMaterialSnowAccumulationRow> snow_rows;
    std::vector<EnvironmentMaterialIceRow> ice_rows;
    std::vector<MaterialWeatheringQualityRow> quality_rows;
    std::vector<MaterialWeatheringPolicyDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
    [[nodiscard]] bool ready() const noexcept;
};

[[nodiscard]] MaterialWeatheringPolicyPlan plan_material_weathering_policy(const MaterialWeatheringPolicyDesc& desc);

void pack_material_weathering_constants(std::span<std::uint8_t> destination, const MaterialWeatheringPolicyDesc& desc);

[[nodiscard]] bool has_material_weathering_policy_diagnostic(const MaterialWeatheringPolicyPlan& plan,
                                                             MaterialWeatheringPolicyDiagnosticCode code) noexcept;

} // namespace mirakana
