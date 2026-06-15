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
    invalid_gpu_budget,
    gpu_budget_exceeded,
    unsupported_gpu_solver,
    missing_profiler_artifacts,
    invalid_profiler_artifact,
    missing_production_solver_package_evidence,
    missing_production_solver_core_evidence,
    unsupported_native_handle_access,
    unsupported_production_solver_ready_claim,
};

enum class EnvironmentWeatherSimulationValidationDatasetStatus : std::uint8_t {
    blocked = 0,
    ready,
};

enum class EnvironmentWeatherSimulationValidationImageStatus : std::uint8_t {
    blocked = 0,
    ready,
};

enum class EnvironmentWeatherSimulationArtistControlStatus : std::uint8_t {
    blocked = 0,
    ready,
};

enum class EnvironmentWeatherSimulationValidationCaseKind : std::uint8_t {
    supersaturated_condensation = 0,
    forced_evaporation_precipitation,
    clamped_mixed_grid,
};

enum class EnvironmentWeatherSimulationValidationImageKind : std::uint8_t {
    vapor_water_after = 0,
    cloud_water_after,
    surface_water_after,
    water_transfer,
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

enum class EnvironmentWeatherSimulationValidationImageDiagnosticCode : std::uint8_t {
    none = 0,
    missing_ready_dataset,
    invalid_image_dimensions,
    missing_case_rows,
    unsupported_native_handle_access,
    unsupported_physical_weather_ready_claim,
};

enum class EnvironmentWeatherSimulationArtistControlDiagnosticCode : std::uint8_t {
    none = 0,
    invalid_grid,
    invalid_environment_value,
    invalid_control_id,
    invalid_control_value,
    unsupported_raw_solver_internal_access,
    unsupported_native_handle_access,
    unsupported_backend_execution,
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

struct EnvironmentWeatherSimulationSolverProfilerArtifactRow {
    std::string artifact_id;
    std::string artifact_kind;
    std::string artifact_path;
    std::string capture_tool;
    std::string backend;
    std::uint64_t cpu_duration_us{0U};
    std::uint64_t gpu_duration_us{0U};
    std::uint64_t cpu_budget_us{0U};
    std::uint64_t gpu_budget_us{0U};
    std::uint64_t source_index{0U};
};

struct EnvironmentWeatherSimulationSolverBudgetDesc {
    bool cpu_reference_package_ready{false};
    std::uint64_t cpu_elapsed_us{0U};
    std::uint64_t cpu_budget_us{0U};
    std::uint64_t gpu_elapsed_us{0U};
    std::uint64_t gpu_budget_us{0U};
    bool gpu_solver_package_ready{false};
    std::vector<EnvironmentWeatherSimulationSolverProfilerArtifactRow> profiler_artifacts;
    bool validation_dataset_ready{false};
    bool validation_images_ready{false};
    bool artist_controls_ready{false};
    bool production_solver_package_counter_reviewed{false};
    bool production_solver_core_reviewed{false};
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
    bool gpu_budget_over{false};
    bool profiler_artifact_ready{false};
    bool profiler_budget_ready{false};
    std::uint32_t profiler_artifact_count{0U};
    std::uint32_t profiler_tool_rows{0U};
    std::uint32_t profiler_backend_rows{0U};
    std::uint64_t profiler_artifact_hash{0U};
    bool production_solver_package_counter_review_ready{false};
    std::uint32_t production_solver_package_counter_rows{0U};
    bool production_solver_core_review_ready{false};
    std::uint32_t production_solver_core_rows{0U};
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
    std::uint32_t grid_width{0U};
    std::uint32_t grid_height{0U};
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

struct EnvironmentWeatherSimulationValidationImageDesc {
    EnvironmentWeatherSimulationValidationDatasetPlan dataset{};
    bool request_native_handle_access{false};
    bool request_physical_weather_ready_claim{false};
};

struct EnvironmentWeatherSimulationValidationImageRow {
    std::string case_id;
    EnvironmentWeatherSimulationValidationCaseKind case_kind{
        EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation};
    EnvironmentWeatherSimulationValidationImageKind image_kind{
        EnvironmentWeatherSimulationValidationImageKind::vapor_water_after};
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    std::uint32_t pixel_count{0U};
    std::uint64_t max_sample_mg_per_m2{0U};
    std::uint64_t checksum{0U};
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationValidationImageDiagnostic {
    EnvironmentWeatherSimulationValidationImageDiagnosticCode code{
        EnvironmentWeatherSimulationValidationImageDiagnosticCode::none};
    EnvironmentWeatherSimulationValidationCaseKind kind{
        EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation};
    std::string case_id;
    std::string message;
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationValidationImagePlan {
    EnvironmentWeatherSimulationValidationImageStatus status{
        EnvironmentWeatherSimulationValidationImageStatus::blocked};
    std::vector<EnvironmentWeatherSimulationValidationImageRow> rows;
    std::vector<EnvironmentWeatherSimulationValidationImageDiagnostic> diagnostics;
    std::uint32_t image_count{0U};
    std::uint32_t required_image_count{12U};
    bool supersaturated_condensation_images_ready{false};
    bool forced_evaporation_precipitation_images_ready{false};
    bool clamped_mixed_grid_images_ready{false};
    bool validation_images_ready{false};
    bool physical_weather_ready{false};
    bool invokes_gpu{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    std::uint64_t image_hash{0U};

    [[nodiscard]] bool succeeded() const noexcept;
};

struct EnvironmentWeatherSimulationArtistControlCell {
    std::string control_id;
    float temperature_celsius{15.0F};
    float relative_humidity_percent{50.0F};
    float cloud_cover_percent{0.0F};
    float surface_wetness_percent{0.0F};
    float evaporation_intensity_percent{0.0F};
    float precipitation_intensity_percent{0.0F};
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationArtistControlDesc {
    std::uint32_t width{0U};
    std::uint32_t height{0U};
    float cell_area_m2{1.0F};
    float mixing_height_m{1000.0F};
    float air_pressure_hpa{1013.25F};
    float requested_timestep_s{1.0F};
    float max_timestep_s{1.0F};
    std::uint64_t deterministic_seed{0U};
    std::vector<EnvironmentWeatherSimulationArtistControlCell> cells;
    bool request_raw_solver_internal_access{false};
    bool request_native_handle_access{false};
    bool request_backend_execution{false};
    bool request_physical_weather_ready_claim{false};
};

struct EnvironmentWeatherSimulationArtistControlDiagnostic {
    EnvironmentWeatherSimulationArtistControlDiagnosticCode code{
        EnvironmentWeatherSimulationArtistControlDiagnosticCode::none};
    std::string field;
    std::string message;
    std::uint32_t source_index{0U};
};

struct EnvironmentWeatherSimulationArtistControlPlan {
    EnvironmentWeatherSimulationArtistControlStatus status{EnvironmentWeatherSimulationArtistControlStatus::blocked};
    EnvironmentWeatherSimulationDesc preview_desc{};
    std::vector<EnvironmentWeatherSimulationArtistControlDiagnostic> diagnostics;
    std::uint32_t control_row_count{0U};
    std::uint32_t generated_cell_count{0U};
    bool artist_controls_ready{false};
    bool physical_weather_ready{false};
    bool invokes_gpu{false};
    bool invokes_backend{false};
    bool exposes_native_handles{false};
    bool raw_solver_internal_access{false};
    std::uint64_t control_hash{0U};

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

[[nodiscard]] EnvironmentWeatherSimulationValidationImagePlan
plan_environment_weather_simulation_validation_images(const EnvironmentWeatherSimulationValidationImageDesc& desc);

[[nodiscard]] EnvironmentWeatherSimulationArtistControlPlan
plan_environment_weather_simulation_artist_controls(const EnvironmentWeatherSimulationArtistControlDesc& desc);

[[nodiscard]] bool
has_environment_weather_simulation_diagnostic(const EnvironmentWeatherSimulationPlan& plan,
                                              EnvironmentWeatherSimulationDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_solver_budget_diagnostic(
    const EnvironmentWeatherSimulationSolverBudgetPlan& plan,
    EnvironmentWeatherSimulationSolverBudgetDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_validation_dataset_diagnostic(
    const EnvironmentWeatherSimulationValidationDatasetPlan& plan,
    EnvironmentWeatherSimulationValidationDatasetDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_validation_image_diagnostic(
    const EnvironmentWeatherSimulationValidationImagePlan& plan,
    EnvironmentWeatherSimulationValidationImageDiagnosticCode code) noexcept;

[[nodiscard]] bool has_environment_weather_simulation_artist_control_diagnostic(
    const EnvironmentWeatherSimulationArtistControlPlan& plan,
    EnvironmentWeatherSimulationArtistControlDiagnosticCode code) noexcept;

} // namespace mirakana
