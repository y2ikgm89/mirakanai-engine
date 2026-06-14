// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class EnvironmentWeatherSimulationStatus : std::uint8_t {
    blocked = 0,
    stepped,
};

enum class EnvironmentWeatherSimulationDeterminism : std::uint8_t {
    deterministic = 0,
    deterministic_per_host,
    nondeterministic,
};

enum class EnvironmentWeatherSimulationDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_grid,
    invalid_timestep,
    invalid_pressure,
    invalid_mixing_height,
    invalid_cell_area,
    invalid_cell_state,
    invalid_forcing,
    unsupported_gpu_acceleration,
    unsupported_backend_execution,
    unsupported_native_handle_access,
    unsupported_physical_weather_ready_claim,
};

struct EnvironmentWeatherSimulationCellState {
    float temperature_celsius{15.0F};
    float vapor_water_kg_per_m2{0.0F};
    float cloud_water_kg_per_m2{0.0F};
    float surface_water_kg_per_m2{0.0F};
};

struct EnvironmentWeatherSimulationCellForcing {
    float surface_evaporation_kg_per_m2_s{0.0F};
    float temperature_delta_celsius_per_s{0.0F};
    float cloud_precipitation_rate_per_s{0.0F};
};

struct EnvironmentWeatherSimulationDesc {
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    float cell_area_m2{1.0F};
    float mixing_height_m{1000.0F};
    float air_pressure_hpa{1013.25F};
    float requested_timestep_s{1.0F};
    float max_timestep_s{1.0F};
    std::uint64_t deterministic_seed{0U};
    EnvironmentWeatherSimulationDeterminism determinism{EnvironmentWeatherSimulationDeterminism::deterministic};
    std::vector<EnvironmentWeatherSimulationCellState> initial_cells;
    std::vector<EnvironmentWeatherSimulationCellForcing> forcing_rows;
    bool request_gpu_acceleration{false};
    bool request_backend_execution{false};
    bool request_native_handle_access{false};
    bool request_physical_weather_ready_claim{false};
};

struct EnvironmentWeatherSimulationCellRow {
    std::size_t cell_index{0U};
    EnvironmentWeatherSimulationCellState state{};
    float saturation_vapor_kg_per_m2{0.0F};
    float evaporated_kg_per_m2{0.0F};
    float condensed_kg_per_m2{0.0F};
    float precipitated_kg_per_m2{0.0F};
    float water_conservation_error_kg_per_m2{0.0F};
};

struct EnvironmentWeatherSimulationDiagnostic {
    EnvironmentWeatherSimulationDiagnosticCode code{EnvironmentWeatherSimulationDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentWeatherSimulationPlan {
    EnvironmentWeatherSimulationStatus status{EnvironmentWeatherSimulationStatus::blocked};
    EnvironmentWeatherSimulationDeterminism determinism{EnvironmentWeatherSimulationDeterminism::deterministic};
    float effective_timestep_s{0.0F};
    bool timestep_clamped{false};
    std::uint32_t cell_count{0U};
    bool fallback_cpu_reference_used{false};
    bool invokes_gpu{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool physical_weather_ready{false};
    double total_water_before_kg{0.0};
    double total_water_after_kg{0.0};
    double water_conservation_error_kg{0.0};
    double max_cell_water_conservation_error_kg_per_m2{0.0};
    double total_evaporated_kg{0.0};
    double total_condensed_kg{0.0};
    double total_precipitated_kg{0.0};
    std::uint64_t replay_hash{0U};
    std::vector<EnvironmentWeatherSimulationCellRow> cell_rows;
    std::vector<EnvironmentWeatherSimulationDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] float environment_weather_saturation_vapor_kg_per_m2(float temperature_celsius, float air_pressure_hpa,
                                                                   float mixing_height_m) noexcept;

[[nodiscard]] EnvironmentWeatherSimulationPlan
simulate_environment_weather_cpu_reference(const EnvironmentWeatherSimulationDesc& desc);

[[nodiscard]] bool
has_environment_weather_simulation_diagnostic(const EnvironmentWeatherSimulationPlan& plan,
                                              EnvironmentWeatherSimulationDiagnosticCode code) noexcept;

} // namespace mirakana
