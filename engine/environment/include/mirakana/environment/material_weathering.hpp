// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/environment/environment_profile.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentMaterialWeatheringPlanStatus : std::uint8_t {
    blocked = 0,
    planned,
};

enum class EnvironmentMaterialWeatheringState : std::uint8_t {
    dry = 0,
    wet,
    snow_covered,
    icy,
    mixed,
};

enum class EnvironmentMaterialWeatheringDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_environment_profile,
    invalid_wetness_intensity,
    invalid_snow_accumulation,
    invalid_ice_intensity,
    unsupported_backend_execution,
    unsupported_native_handle_claim,
    unsupported_source_material_mutation,
};

struct EnvironmentMaterialWeatheringDesc {
    EnvironmentProfileDesc environment;
    float wetness_intensity{0.0F};
    float snow_accumulation{0.0F};
    float ice_intensity{0.0F};
    bool derive_from_environment_precipitation{true};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_source_material_mutation{false};
};

struct EnvironmentMaterialWeatheringRow {
    EnvironmentMaterialWeatheringState state{EnvironmentMaterialWeatheringState::dry};
    float wetness_intensity{0.0F};
    float snow_accumulation{0.0F};
    float ice_intensity{0.0F};
    bool mutates_source_material{false};
};

struct EnvironmentMaterialWetnessRow {
    float intensity{0.0F};
    bool darkens_base_color{true};
    bool lowers_roughness{true};
    bool adds_ripple_intent{true};
    bool mutates_source_material{false};
};

struct EnvironmentMaterialSnowAccumulationRow {
    float coverage{0.0F};
    bool raises_roughness{true};
    bool lightens_base_color{true};
    bool uses_surface_up_mask{true};
    bool mutates_source_material{false};
};

struct EnvironmentMaterialIceRow {
    float intensity{0.0F};
    bool raises_specular_intent{true};
    bool uses_low_temperature_mask{true};
    bool mutates_source_material{false};
};

struct EnvironmentMaterialWeatheringDiagnostic {
    EnvironmentMaterialWeatheringDiagnosticCode code{EnvironmentMaterialWeatheringDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentMaterialWeatheringPlan {
    EnvironmentMaterialWeatheringPlanStatus status{EnvironmentMaterialWeatheringPlanStatus::blocked};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool mutates_source_materials{false};
    std::vector<EnvironmentMaterialWeatheringRow> rows;
    std::vector<EnvironmentMaterialWetnessRow> wet_rows;
    std::vector<EnvironmentMaterialSnowAccumulationRow> snow_rows;
    std::vector<EnvironmentMaterialIceRow> ice_rows;
    std::vector<EnvironmentMaterialWeatheringDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] EnvironmentMaterialWeatheringPlan
plan_environment_material_weathering(const EnvironmentMaterialWeatheringDesc& desc);

[[nodiscard]] bool
has_environment_material_weathering_diagnostic(const EnvironmentMaterialWeatheringPlan& plan,
                                               EnvironmentMaterialWeatheringDiagnosticCode code) noexcept;

} // namespace mirakana
