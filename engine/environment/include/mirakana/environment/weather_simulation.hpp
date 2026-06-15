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

enum class EnvironmentWeatherSimulationSolverBudgetStatus : std::uint8_t {
    blocked = 0,
    host_evidence_required,
    ready,
    budget_exceeded,
};

enum class EnvironmentWeatherSimulationSolverBudgetDiagnosticCode : std::uint8_t {
    none = 0,
    missing_cpu_reference_package,
    invalid_cpu_budget,
    cpu_budget_exceeded,
    unsupported_gpu_solver,
    unsupported_native_handle_access,
    unsupported_production_solver_ready_claim,
};

enum class EnvironmentWeatherSimulationValidationDatasetStatus : std::uint8_t {
    blocked = 0,
    ready,
};

enum class EnvironmentWeatherSimulationValidationCaseKind : std::uint8_t {
    supersaturated_condensation = 0,
    forced_evaporation_precipitation,
    clamped_mixed_grid,
};

enum class EnvironmentWeatherSimulationValidationDatasetDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_case_id,
    duplicate_case,
    missing_required_case,
    simulation_failed,
    water_error_exceeded,
    missing_expected_condensation,
    missing_expected_evaporation,
    missing_expected_precipitation,
    missing_expected_timestep_clamp,
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

struct EnvironmentWeatherSimulationSolverBudgetDesc {
    bool cpu_reference_package_ready{false};
    std::uint64_t cpu_elapsed_us{0U};
    std::uint64_t cpu_budget_us{0U};
    std::uint64_t gpu_elapsed_us{0U};
    std::uint64_t gpu_budget_us{0U};
    bool gpu_solver_package_ready{false};
    bool profiler_artifact_ready{false};
    bool request_native_handle_access{false};
    bool request_production_solver_ready_claim{false};
};

struct EnvironmentWeatherSimulationSolverBudgetDiagnostic {
    EnvironmentWeatherSimulationSolverBudgetDiagnosticCode code{
        EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::none};
    std::string field;
    std::string message;
};

struct EnvironmentWeatherSimulationSolverBudgetPlan {
    EnvironmentWeatherSimulationSolverBudgetStatus status{EnvironmentWeatherSimulationSolverBudgetStatus::blocked};
    std::uint64_t cpu_elapsed_us{0U};
    std::uint64_t cpu_budget_us{0U};
    std::uint64_t gpu_elapsed_us{0U};
    std::uint64_t gpu_budget_us{0U};
    bool cpu_budget_ready{false};
    bool cpu_budget_over{false};
    bool gpu_budget_ready{false};
    bool profiler_artifact_ready{false};
    bool profiler_budget_ready{false};
    bool production_solver_ready{false};
    bool invokes_gpu{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::vector<EnvironmentWeatherSimulationSolverBudgetDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
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

struct EnvironmentWeatherSimulationValidationCase {
    std::string case_id;
    EnvironmentWeatherSimulationValidationCaseKind kind{
        EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation};
    EnvironmentWeatherSimulationPlan plan{};
    std::uint64_t water_error_bound_mg{1U};
    bool expect_condensation{false};
    bool expect_evaporation{false};
    bool expect_precipitation{false};
    bool expect_timestep_clamped{false};
    bool request_native_handle_access{false};
    bool request_physical_weather_ready_claim{false};
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationValidationDatasetDesc {
    std::vector<EnvironmentWeatherSimulationValidationCase> cases;
};

struct EnvironmentWeatherSimulationValidationDatasetDiagnostic {
    EnvironmentWeatherSimulationValidationDatasetDiagnosticCode code{
        EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::none};
    EnvironmentWeatherSimulationValidationCaseKind kind{
        EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation};
    std::string case_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationValidationDatasetPlan {
    EnvironmentWeatherSimulationValidationDatasetStatus status{
        EnvironmentWeatherSimulationValidationDatasetStatus::blocked};
    std::vector<EnvironmentWeatherSimulationValidationCase> cases;
    std::vector<EnvironmentWeatherSimulationValidationDatasetDiagnostic> diagnostics;
    std::uint32_t case_count{0U};
    std::uint32_t required_case_count{3U};
    std::uint32_t ready_case_count{0U};
    bool supersaturated_condensation_ready{false};
    bool forced_evaporation_precipitation_ready{false};
    bool clamped_mixed_grid_ready{false};
    bool validation_images_ready{false};
    bool physical_weather_ready{false};
    bool invokes_gpu{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::uint64_t max_water_conservation_error_mg{0U};
    std::uint64_t water_conservation_error_bound_mg{1U};
    std::uint64_t dataset_hash{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] float environment_weather_saturation_vapor_kg_per_m2(float temperature_celsius, float air_pressure_hpa,
                                                                   float mixing_height_m) noexcept;

[[nodiscard]] EnvironmentWeatherSimulationPlan
simulate_environment_weather_cpu_reference(const EnvironmentWeatherSimulationDesc& desc);

[[nodiscard]] EnvironmentWeatherSimulationSolverBudgetPlan
plan_environment_weather_simulation_solver_budget(const EnvironmentWeatherSimulationSolverBudgetDesc& desc);

[[nodiscard]] EnvironmentWeatherSimulationValidationDatasetPlan
plan_environment_weather_simulation_validation_dataset(const EnvironmentWeatherSimulationValidationDatasetDesc& desc);

[[nodiscard]] bool
has_environment_weather_simulation_diagnostic(const EnvironmentWeatherSimulationPlan& plan,
                                              EnvironmentWeatherSimulationDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_solver_budget_diagnostic(
    const EnvironmentWeatherSimulationSolverBudgetPlan& plan,
    EnvironmentWeatherSimulationSolverBudgetDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_validation_dataset_diagnostic(
    const EnvironmentWeatherSimulationValidationDatasetPlan& plan,
    EnvironmentWeatherSimulationValidationDatasetDiagnosticCode code) noexcept;

} // namespace mirakana
