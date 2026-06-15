// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/environment/weather_simulation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <string_view>
#include <utility>

namespace mirakana {
namespace {

constexpr double dry_air_gas_constant_j_kg_k = 287.05;
constexpr double minimum_temperature_celsius = -90.0;
constexpr double maximum_temperature_celsius = 60.0;
constexpr double maximum_timestep_s = 60.0;
constexpr std::uint64_t fnv_offset_basis = 14695981039346656037ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

void add_diagnostic(EnvironmentWeatherSimulationPlan& plan, EnvironmentWeatherSimulationDiagnosticCode code,
                    std::string field, std::string message) {
    plan.diagnostics.push_back(EnvironmentWeatherSimulationDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

void add_diagnostic(EnvironmentWeatherSimulationSolverBudgetPlan& plan,
                    EnvironmentWeatherSimulationSolverBudgetDiagnosticCode code, std::string field,
                    std::string message) {
    plan.diagnostics.push_back(EnvironmentWeatherSimulationSolverBudgetDiagnostic{
        .code = code,
        .field = std::move(field),
        .message = std::move(message),
    });
}

void add_diagnostic(EnvironmentWeatherSimulationValidationDatasetPlan& plan,
                    EnvironmentWeatherSimulationValidationDatasetDiagnosticCode code,
                    EnvironmentWeatherSimulationValidationCaseKind kind, std::string case_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(EnvironmentWeatherSimulationValidationDatasetDiagnostic{
        .code = code,
        .kind = kind,
        .case_id = std::move(case_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void add_diagnostic(EnvironmentWeatherSimulationValidationImagePlan& plan,
                    EnvironmentWeatherSimulationValidationImageDiagnosticCode code,
                    EnvironmentWeatherSimulationValidationCaseKind kind, std::string case_id, std::string message,
                    std::uint32_t source_index) {
    plan.diagnostics.push_back(EnvironmentWeatherSimulationValidationImageDiagnostic{
        .code = code,
        .kind = kind,
        .case_id = std::move(case_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

[[nodiscard]] bool finite_positive(const float value) noexcept {
    return std::isfinite(value) && value > 0.0F;
}

[[nodiscard]] bool finite_nonnegative(const float value) noexcept {
    return std::isfinite(value) && value >= 0.0F;
}

[[nodiscard]] bool valid_temperature(const float value) noexcept {
    return std::isfinite(value) && value >= static_cast<float>(minimum_temperature_celsius) &&
           value <= static_cast<float>(maximum_temperature_celsius);
}

[[nodiscard]] std::uint64_t quantize_for_hash(const double value) noexcept {
    if (!std::isfinite(value)) {
        return 0ULL;
    }
    const auto scaled = std::llround(value * 1000000.0);
    return static_cast<std::uint64_t>(scaled);
}

void hash_combine(std::uint64_t& hash, const std::uint64_t value) noexcept {
    hash ^= value;
    hash *= fnv_prime;
}

void hash_combine_float(std::uint64_t& hash, const double value) noexcept {
    hash_combine(hash, quantize_for_hash(value));
}

[[nodiscard]] std::uint64_t build_replay_hash(const EnvironmentWeatherSimulationDesc& desc,
                                              const EnvironmentWeatherSimulationPlan& plan) noexcept {
    std::uint64_t hash = fnv_offset_basis;
    hash_combine(hash, desc.deterministic_seed);
    hash_combine(hash, desc.width);
    hash_combine(hash, desc.height);
    hash_combine(hash, static_cast<std::uint64_t>(desc.determinism));
    hash_combine_float(hash, desc.cell_area_m2);
    hash_combine_float(hash, desc.mixing_height_m);
    hash_combine_float(hash, desc.air_pressure_hpa);
    hash_combine_float(hash, plan.effective_timestep_s);
    hash_combine(hash, plan.cell_count);
    hash_combine_float(hash, plan.total_water_before_kg);
    hash_combine_float(hash, plan.total_water_after_kg);
    hash_combine_float(hash, plan.water_conservation_error_kg);
    for (const auto& row : plan.cell_rows) {
        hash_combine(hash, row.cell_index);
        hash_combine_float(hash, row.state.temperature_celsius);
        hash_combine_float(hash, row.state.vapor_water_kg_per_m2);
        hash_combine_float(hash, row.state.cloud_water_kg_per_m2);
        hash_combine_float(hash, row.state.surface_water_kg_per_m2);
        hash_combine_float(hash, row.saturation_vapor_kg_per_m2);
        hash_combine_float(hash, row.evaporated_kg_per_m2);
        hash_combine_float(hash, row.condensed_kg_per_m2);
        hash_combine_float(hash, row.precipitated_kg_per_m2);
    }
    hash_combine(hash, plan.diagnostics.size());
    return hash == 0ULL ? fnv_offset_basis : hash;
}

[[nodiscard]] std::uint64_t kilograms_to_milligrams(const double kilograms) noexcept {
    if (!std::isfinite(kilograms) || kilograms <= 0.0) {
        return 0ULL;
    }
    return static_cast<std::uint64_t>(std::llround(kilograms * 1000000.0));
}

[[nodiscard]] std::uint64_t kilograms_per_m2_to_milligrams_per_m2(const double kilograms_per_m2) noexcept {
    if (!std::isfinite(kilograms_per_m2) || kilograms_per_m2 <= 0.0) {
        return 0ULL;
    }
    return static_cast<std::uint64_t>(std::llround(kilograms_per_m2 * 1000000.0));
}

[[nodiscard]] constexpr std::array required_validation_cases{
    EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation,
    EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation,
    EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid,
};

[[nodiscard]] constexpr std::array required_validation_image_kinds{
    EnvironmentWeatherSimulationValidationImageKind::vapor_water_after,
    EnvironmentWeatherSimulationValidationImageKind::cloud_water_after,
    EnvironmentWeatherSimulationValidationImageKind::surface_water_after,
    EnvironmentWeatherSimulationValidationImageKind::water_transfer,
};

[[nodiscard]] std::string_view
canonical_validation_case_id(const EnvironmentWeatherSimulationValidationCaseKind kind) noexcept {
    switch (kind) {
    case EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation:
        return "supersaturated_condensation";
    case EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation:
        return "forced_evaporation_precipitation";
    case EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid:
        return "clamped_mixed_grid";
    }
    return {};
}

[[nodiscard]] bool is_case_id_char(char ch) noexcept {
    return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '_';
}

[[nodiscard]] bool valid_validation_case_id(const EnvironmentWeatherSimulationValidationCase& row) {
    return !row.case_id.empty() && std::ranges::all_of(row.case_id, is_case_id_char) &&
           row.case_id == canonical_validation_case_id(row.kind);
}

[[nodiscard]] bool has_validation_case(const EnvironmentWeatherSimulationValidationDatasetPlan& plan,
                                       const EnvironmentWeatherSimulationValidationCaseKind kind) {
    return std::ranges::any_of(plan.cases, [kind](const auto& row) { return row.kind == kind; });
}

[[nodiscard]] bool validation_case_ready(const EnvironmentWeatherSimulationValidationCase& row,
                                         const std::uint64_t water_error_mg) noexcept {
    return row.plan.succeeded() && row.plan.status == EnvironmentWeatherSimulationStatus::stepped &&
           row.plan.replay_hash != 0U && water_error_mg <= row.water_error_bound_mg &&
           (!row.expect_condensation || row.plan.total_condensed_kg > 0.0) &&
           (!row.expect_evaporation || row.plan.total_evaporated_kg > 0.0) &&
           (!row.expect_precipitation || row.plan.total_precipitated_kg > 0.0) &&
           (!row.expect_timestep_clamped || row.plan.timestep_clamped) && !row.plan.invokes_gpu &&
           !row.plan.invokes_backend && !row.plan.exposes_native_handles && !row.plan.physical_weather_ready &&
           !row.request_native_handle_access && !row.request_physical_weather_ready_claim;
}

[[nodiscard]] std::uint64_t build_dataset_hash(const EnvironmentWeatherSimulationValidationDatasetPlan& plan) noexcept {
    std::uint64_t hash = fnv_offset_basis;
    hash_combine(hash, plan.case_count);
    hash_combine(hash, plan.ready_case_count);
    hash_combine(hash, plan.max_water_conservation_error_mg);
    for (const auto& row : plan.cases) {
        hash_combine(hash, static_cast<std::uint64_t>(row.kind));
        hash_combine(hash, row.plan.replay_hash);
        hash_combine(hash, kilograms_to_milligrams(row.plan.water_conservation_error_kg));
    }
    hash_combine(hash, plan.diagnostics.size());
    return hash == 0ULL ? fnv_offset_basis : hash;
}

[[nodiscard]] std::uint64_t build_image_hash(const EnvironmentWeatherSimulationValidationImagePlan& plan) noexcept {
    std::uint64_t hash = fnv_offset_basis;
    hash_combine(hash, plan.image_count);
    hash_combine(hash, plan.required_image_count);
    for (const auto& row : plan.rows) {
        hash_combine(hash, static_cast<std::uint64_t>(row.case_kind));
        hash_combine(hash, static_cast<std::uint64_t>(row.image_kind));
        hash_combine(hash, row.width);
        hash_combine(hash, row.height);
        hash_combine(hash, row.pixel_count);
        hash_combine(hash, row.max_sample_mg_per_m2);
        hash_combine(hash, row.checksum);
    }
    hash_combine(hash, plan.diagnostics.size());
    return hash == 0ULL ? fnv_offset_basis : hash;
}

[[nodiscard]] std::size_t expected_cell_count(const EnvironmentWeatherSimulationDesc& desc) noexcept {
    return static_cast<std::size_t>(desc.width) * static_cast<std::size_t>(desc.height);
}

[[nodiscard]] double active_water_kg_per_m2(const EnvironmentWeatherSimulationCellState& state) noexcept {
    return static_cast<double>(state.vapor_water_kg_per_m2) + static_cast<double>(state.cloud_water_kg_per_m2) +
           static_cast<double>(state.surface_water_kg_per_m2);
}

void validate_desc(EnvironmentWeatherSimulationPlan& plan, const EnvironmentWeatherSimulationDesc& desc) {
    const auto cells = expected_cell_count(desc);
    if (desc.width == 0U || desc.height == 0U || cells == 0U || desc.initial_cells.size() != cells ||
        desc.forcing_rows.size() != cells) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_grid, "width/height/cells",
                       "weather simulation requires a non-empty grid and one state/forcing row per cell");
    }
    if (!finite_positive(desc.cell_area_m2)) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_cell_area, "cell_area_m2",
                       "weather simulation cell area must be finite and positive");
    }
    if (!finite_positive(desc.mixing_height_m)) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_mixing_height, "mixing_height_m",
                       "weather simulation mixing height must be finite and positive");
    }
    if (!finite_positive(desc.air_pressure_hpa) || desc.air_pressure_hpa <= 10.0F) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_pressure, "air_pressure_hpa",
                       "weather simulation air pressure must be finite and above the selected saturation range");
    }
    if (!finite_positive(desc.requested_timestep_s) || !finite_positive(desc.max_timestep_s) ||
        desc.max_timestep_s > static_cast<float>(maximum_timestep_s)) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_timestep,
                       "requested_timestep_s/max_timestep_s",
                       "weather simulation timestep values must be finite, positive, and within the stable cap");
    }
    if (desc.determinism != EnvironmentWeatherSimulationDeterminism::deterministic) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_forcing, "determinism",
                       "this CPU reference weather simulation slice supports only deterministic execution");
    }
    if (desc.request_gpu_acceleration) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::unsupported_gpu_acceleration,
                       "request_gpu_acceleration",
                       "weather simulation CPU reference must not invoke GPU acceleration in this slice");
    }
    if (desc.request_backend_execution) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::unsupported_backend_execution,
                       "request_backend_execution",
                       "weather simulation CPU reference must not invoke renderer, physics, or platform backends");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::unsupported_native_handle_access,
                       "request_native_handle_access",
                       "weather simulation CPU reference must not expose native handles");
    }
    if (desc.request_physical_weather_ready_claim) {
        add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::unsupported_physical_weather_ready_claim,
                       "request_physical_weather_ready_claim",
                       "this foundation slice cannot claim complete physical weather simulation readiness");
    }

    const float effective_timestep = finite_positive(desc.requested_timestep_s) && finite_positive(desc.max_timestep_s)
                                         ? std::min(desc.requested_timestep_s, desc.max_timestep_s)
                                         : 0.0F;

    for (const auto& state : desc.initial_cells) {
        if (!valid_temperature(state.temperature_celsius) || !finite_nonnegative(state.vapor_water_kg_per_m2) ||
            !finite_nonnegative(state.cloud_water_kg_per_m2) || !finite_nonnegative(state.surface_water_kg_per_m2)) {
            add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_cell_state, "initial_cells",
                           "weather simulation cells require finite temperature and non-negative water pools");
            break;
        }
    }
    for (const auto& forcing : desc.forcing_rows) {
        if (!finite_nonnegative(forcing.surface_evaporation_kg_per_m2_s) ||
            !std::isfinite(forcing.temperature_delta_celsius_per_s) ||
            !finite_nonnegative(forcing.cloud_precipitation_rate_per_s) ||
            (effective_timestep > 0.0F && forcing.cloud_precipitation_rate_per_s * effective_timestep > 1.0F)) {
            add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_forcing, "forcing_rows",
                           "weather simulation forcing rows require finite stable evaporation, temperature, and "
                           "precipitation rates");
            break;
        }
    }
    if (effective_timestep > 0.0F && desc.initial_cells.size() == cells && desc.forcing_rows.size() == cells) {
        for (std::size_t index = 0; index < cells; ++index) {
            const auto& state = desc.initial_cells[index];
            const auto& forcing = desc.forcing_rows[index];
            if (valid_temperature(state.temperature_celsius) &&
                std::isfinite(forcing.temperature_delta_celsius_per_s) &&
                !valid_temperature(state.temperature_celsius +
                                   (forcing.temperature_delta_celsius_per_s * effective_timestep))) {
                add_diagnostic(plan, EnvironmentWeatherSimulationDiagnosticCode::invalid_forcing,
                               "forcing_rows.temperature_delta_celsius_per_s",
                               "weather simulation forcing must keep stepped temperature within the stable range");
                break;
            }
        }
    }
}

[[nodiscard]] EnvironmentWeatherSimulationCellRow step_cell(const EnvironmentWeatherSimulationCellState& initial,
                                                            const EnvironmentWeatherSimulationCellForcing& forcing,
                                                            const float effective_timestep_s,
                                                            const float air_pressure_hpa, const float mixing_height_m,
                                                            const std::size_t cell_index) noexcept {
    auto state = initial;
    const double water_before = active_water_kg_per_m2(state);

    const float evaporated =
        std::min(state.surface_water_kg_per_m2, forcing.surface_evaporation_kg_per_m2_s * effective_timestep_s);
    state.surface_water_kg_per_m2 -= evaporated;
    state.vapor_water_kg_per_m2 += evaporated;
    state.temperature_celsius += forcing.temperature_delta_celsius_per_s * effective_timestep_s;

    const float saturation =
        environment_weather_saturation_vapor_kg_per_m2(state.temperature_celsius, air_pressure_hpa, mixing_height_m);
    const float condensed = std::max(0.0F, state.vapor_water_kg_per_m2 - saturation);
    state.vapor_water_kg_per_m2 -= condensed;
    state.cloud_water_kg_per_m2 += condensed;

    const float precipitated =
        std::min(state.cloud_water_kg_per_m2,
                 state.cloud_water_kg_per_m2 * forcing.cloud_precipitation_rate_per_s * effective_timestep_s);
    state.cloud_water_kg_per_m2 -= precipitated;
    state.surface_water_kg_per_m2 += precipitated;

    const double water_after = active_water_kg_per_m2(state);
    return EnvironmentWeatherSimulationCellRow{
        .cell_index = cell_index,
        .state = state,
        .saturation_vapor_kg_per_m2 = saturation,
        .evaporated_kg_per_m2 = evaporated,
        .condensed_kg_per_m2 = condensed,
        .precipitated_kg_per_m2 = precipitated,
        .water_conservation_error_kg_per_m2 = static_cast<float>(std::fabs(water_after - water_before)),
    };
}

[[nodiscard]] double
validation_image_sample_kg_per_m2(const EnvironmentWeatherSimulationCellRow& cell,
                                  const EnvironmentWeatherSimulationValidationImageKind kind) noexcept {
    switch (kind) {
    case EnvironmentWeatherSimulationValidationImageKind::vapor_water_after:
        return cell.state.vapor_water_kg_per_m2;
    case EnvironmentWeatherSimulationValidationImageKind::cloud_water_after:
        return cell.state.cloud_water_kg_per_m2;
    case EnvironmentWeatherSimulationValidationImageKind::surface_water_after:
        return cell.state.surface_water_kg_per_m2;
    case EnvironmentWeatherSimulationValidationImageKind::water_transfer:
        return static_cast<double>(cell.evaporated_kg_per_m2) + static_cast<double>(cell.condensed_kg_per_m2) +
               static_cast<double>(cell.precipitated_kg_per_m2);
    }
    return 0.0;
}

[[nodiscard]] EnvironmentWeatherSimulationValidationImageRow
build_validation_image_row(const EnvironmentWeatherSimulationValidationCase& row,
                           const EnvironmentWeatherSimulationValidationImageKind image_kind) noexcept {
    std::uint64_t checksum = fnv_offset_basis;
    std::uint64_t max_sample_mg_per_m2 = 0U;
    hash_combine(checksum, static_cast<std::uint64_t>(row.kind));
    hash_combine(checksum, static_cast<std::uint64_t>(image_kind));
    hash_combine(checksum, row.grid_width);
    hash_combine(checksum, row.grid_height);
    for (const auto& cell : row.plan.cell_rows) {
        const auto sample_mg_per_m2 =
            kilograms_per_m2_to_milligrams_per_m2(validation_image_sample_kg_per_m2(cell, image_kind));
        max_sample_mg_per_m2 = std::max(max_sample_mg_per_m2, sample_mg_per_m2);
        hash_combine(checksum, cell.cell_index);
        hash_combine(checksum, sample_mg_per_m2);
    }
    return EnvironmentWeatherSimulationValidationImageRow{
        .case_id = row.case_id,
        .case_kind = row.kind,
        .image_kind = image_kind,
        .width = row.grid_width,
        .height = row.grid_height,
        .pixel_count = static_cast<std::uint32_t>(row.plan.cell_rows.size()),
        .max_sample_mg_per_m2 = max_sample_mg_per_m2,
        .checksum = checksum == 0ULL ? fnv_offset_basis : checksum,
        .source_index = row.source_index,
    };
}

} // namespace

bool EnvironmentWeatherSimulationPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool EnvironmentWeatherSimulationSolverBudgetPlan::succeeded() const noexcept {
    return status == EnvironmentWeatherSimulationSolverBudgetStatus::ready && diagnostics.empty() &&
           production_solver_ready;
}

bool EnvironmentWeatherSimulationValidationDatasetPlan::succeeded() const noexcept {
    return status == EnvironmentWeatherSimulationValidationDatasetStatus::ready && diagnostics.empty();
}

bool EnvironmentWeatherSimulationValidationImagePlan::succeeded() const noexcept {
    return status == EnvironmentWeatherSimulationValidationImageStatus::ready && diagnostics.empty() &&
           validation_images_ready;
}

float environment_weather_saturation_vapor_kg_per_m2(const float temperature_celsius, const float air_pressure_hpa,
                                                     const float mixing_height_m) noexcept {
    if (!valid_temperature(temperature_celsius) || !finite_positive(air_pressure_hpa) ||
        !finite_positive(mixing_height_m)) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const double temperature_kelvin = static_cast<double>(temperature_celsius) + 273.15;
    const double pressure_pa = static_cast<double>(air_pressure_hpa) * 100.0;
    const double saturation_vapor_pressure_hpa =
        6.11 * std::pow(10.0, (7.5 * static_cast<double>(temperature_celsius)) /
                                  (237.3 + static_cast<double>(temperature_celsius)));
    const double saturation_vapor_pressure_pa = saturation_vapor_pressure_hpa * 100.0;
    if (saturation_vapor_pressure_pa <= 0.0 || saturation_vapor_pressure_pa >= pressure_pa) {
        return std::numeric_limits<float>::quiet_NaN();
    }

    const double air_density_kg_per_m3 = pressure_pa / (dry_air_gas_constant_j_kg_k * temperature_kelvin);
    const double air_mass_kg_per_m2 = air_density_kg_per_m3 * static_cast<double>(mixing_height_m);
    const double saturation_specific_humidity =
        (0.622 * saturation_vapor_pressure_pa) / (pressure_pa - (0.378 * saturation_vapor_pressure_pa));
    return static_cast<float>(saturation_specific_humidity * air_mass_kg_per_m2);
}

EnvironmentWeatherSimulationPlan
simulate_environment_weather_cpu_reference(const EnvironmentWeatherSimulationDesc& desc) {
    EnvironmentWeatherSimulationPlan plan{
        .status = EnvironmentWeatherSimulationStatus::stepped,
        .determinism = desc.determinism,
        .effective_timestep_s = finite_positive(desc.requested_timestep_s) && finite_positive(desc.max_timestep_s)
                                    ? std::min(desc.requested_timestep_s, desc.max_timestep_s)
                                    : 0.0F,
        .timestep_clamped = finite_positive(desc.requested_timestep_s) && finite_positive(desc.max_timestep_s) &&
                            desc.requested_timestep_s > desc.max_timestep_s,
        .cell_count = static_cast<std::uint32_t>(expected_cell_count(desc)),
        .fallback_cpu_reference_used = true,
    };

    validate_desc(plan, desc);
    if (!plan.succeeded()) {
        plan.status = EnvironmentWeatherSimulationStatus::blocked;
        plan.cell_rows.clear();
        plan.replay_hash = build_replay_hash(desc, plan);
        return plan;
    }

    plan.cell_rows.reserve(desc.initial_cells.size());
    for (std::size_t index = 0; index < desc.initial_cells.size(); ++index) {
        const auto row = step_cell(desc.initial_cells[index], desc.forcing_rows[index], plan.effective_timestep_s,
                                   desc.air_pressure_hpa, desc.mixing_height_m, index);
        plan.cell_rows.push_back(row);

        const double water_before_kg = active_water_kg_per_m2(desc.initial_cells[index]) * desc.cell_area_m2;
        const double water_after_kg = active_water_kg_per_m2(row.state) * desc.cell_area_m2;
        plan.total_water_before_kg += water_before_kg;
        plan.total_water_after_kg += water_after_kg;
        plan.total_evaporated_kg += static_cast<double>(row.evaporated_kg_per_m2) * desc.cell_area_m2;
        plan.total_condensed_kg += static_cast<double>(row.condensed_kg_per_m2) * desc.cell_area_m2;
        plan.total_precipitated_kg += static_cast<double>(row.precipitated_kg_per_m2) * desc.cell_area_m2;
        plan.max_cell_water_conservation_error_kg_per_m2 =
            std::max(plan.max_cell_water_conservation_error_kg_per_m2,
                     static_cast<double>(row.water_conservation_error_kg_per_m2));
    }

    plan.water_conservation_error_kg = std::fabs(plan.total_water_after_kg - plan.total_water_before_kg);
    plan.replay_hash = build_replay_hash(desc, plan);
    return plan;
}

EnvironmentWeatherSimulationSolverBudgetPlan
plan_environment_weather_simulation_solver_budget(const EnvironmentWeatherSimulationSolverBudgetDesc& desc) {
    EnvironmentWeatherSimulationSolverBudgetPlan plan{
        .cpu_elapsed_us = desc.cpu_elapsed_us,
        .cpu_budget_us = desc.cpu_budget_us,
        .gpu_elapsed_us = desc.gpu_elapsed_us,
        .gpu_budget_us = desc.gpu_budget_us,
        .profiler_artifact_ready = desc.profiler_artifact_ready,
        .exposes_native_handles = desc.request_native_handle_access,
    };

    if (!desc.cpu_reference_package_ready) {
        add_diagnostic(plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::missing_cpu_reference_package,
                       "cpu_reference_package_ready",
                       "weather simulation budget counters require a ready CPU reference package row");
    }
    if (desc.cpu_budget_us == 0U) {
        add_diagnostic(plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::invalid_cpu_budget,
                       "cpu_budget_us", "weather simulation CPU budget must be non-zero");
    }
    if (desc.cpu_budget_us > 0U && desc.cpu_elapsed_us > desc.cpu_budget_us) {
        plan.cpu_budget_over = true;
        add_diagnostic(plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::cpu_budget_exceeded,
                       "cpu_elapsed_us",
                       "weather simulation CPU reference package elapsed time exceeds the selected budget");
    }
    if (desc.gpu_solver_package_ready || desc.gpu_elapsed_us != 0U || desc.gpu_budget_us != 0U) {
        add_diagnostic(plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::unsupported_gpu_solver,
                       "gpu_solver_package_ready",
                       "weather simulation GPU solver budget counters require a reviewed GPU solver package row");
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::unsupported_native_handle_access,
                       "request_native_handle_access",
                       "weather simulation budget counters must not expose native handles");
    }
    if (desc.request_production_solver_ready_claim) {
        add_diagnostic(
            plan, EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::unsupported_production_solver_ready_claim,
            "request_production_solver_ready_claim",
            "CPU package timing counters alone cannot claim production weather solver readiness");
    }

    plan.cpu_budget_ready = desc.cpu_reference_package_ready && desc.cpu_budget_us > 0U &&
                            desc.cpu_elapsed_us <= desc.cpu_budget_us && !plan.cpu_budget_over;
    plan.profiler_budget_ready = plan.cpu_budget_ready && desc.profiler_artifact_ready;
    plan.gpu_budget_ready = false;
    plan.production_solver_ready = false;
    plan.invokes_gpu = false;
    plan.invokes_backend = false;

    if (plan.cpu_budget_over) {
        plan.status = EnvironmentWeatherSimulationSolverBudgetStatus::budget_exceeded;
    } else if (!plan.diagnostics.empty()) {
        plan.status = EnvironmentWeatherSimulationSolverBudgetStatus::blocked;
    } else if (plan.production_solver_ready) {
        plan.status = EnvironmentWeatherSimulationSolverBudgetStatus::ready;
    } else {
        plan.status = EnvironmentWeatherSimulationSolverBudgetStatus::host_evidence_required;
    }

    return plan;
}

EnvironmentWeatherSimulationValidationDatasetPlan
plan_environment_weather_simulation_validation_dataset(const EnvironmentWeatherSimulationValidationDatasetDesc& desc) {
    EnvironmentWeatherSimulationValidationDatasetPlan plan;
    plan.cases = desc.cases;
    plan.case_count = static_cast<std::uint32_t>(desc.cases.size());
    plan.required_case_count = static_cast<std::uint32_t>(required_validation_cases.size());

    for (const auto& required_case : required_validation_cases) {
        if (!has_validation_case(plan, required_case)) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_required_case,
                           required_case, std::string{canonical_validation_case_id(required_case)},
                           "weather simulation validation dataset is missing a required canonical case", 0U);
        }
    }

    std::vector<EnvironmentWeatherSimulationValidationCaseKind> seen;
    seen.reserve(desc.cases.size());
    for (const auto& row : desc.cases) {
        plan.exposes_native_handles =
            plan.exposes_native_handles || row.request_native_handle_access || row.plan.exposes_native_handles;
        if (!valid_validation_case_id(row)) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::invalid_case_id, row.kind,
                           row.case_id, "weather simulation validation cases require canonical lower_snake_case ids",
                           row.source_index);
        }
        if (std::ranges::find(seen, row.kind) != seen.end()) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::duplicate_case, row.kind,
                           row.case_id, "weather simulation validation dataset allows one row per canonical case",
                           row.source_index);
        } else {
            seen.push_back(row.kind);
        }
        if (!row.plan.succeeded() || row.plan.status != EnvironmentWeatherSimulationStatus::stepped ||
            row.plan.replay_hash == 0U) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::simulation_failed,
                           row.kind, row.case_id,
                           "weather simulation validation cases require a successful deterministic CPU plan",
                           row.source_index);
        }

        const auto water_error_mg = kilograms_to_milligrams(row.plan.water_conservation_error_kg);
        plan.max_water_conservation_error_mg = std::max(plan.max_water_conservation_error_mg, water_error_mg);
        plan.water_conservation_error_bound_mg =
            std::max(plan.water_conservation_error_bound_mg, row.water_error_bound_mg);
        if (water_error_mg > row.water_error_bound_mg) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::water_error_exceeded,
                           row.kind, row.case_id,
                           "weather simulation validation case exceeded its water-conservation error bound",
                           row.source_index);
        }
        if (row.expect_condensation && row.plan.total_condensed_kg <= 0.0) {
            add_diagnostic(plan,
                           EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_expected_condensation,
                           row.kind, row.case_id, "weather simulation validation case expected condensation transfer",
                           row.source_index);
        }
        if (row.expect_evaporation && row.plan.total_evaporated_kg <= 0.0) {
            add_diagnostic(plan,
                           EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_expected_evaporation,
                           row.kind, row.case_id, "weather simulation validation case expected evaporation transfer",
                           row.source_index);
        }
        if (row.expect_precipitation && row.plan.total_precipitated_kg <= 0.0) {
            add_diagnostic(plan,
                           EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_expected_precipitation,
                           row.kind, row.case_id, "weather simulation validation case expected precipitation transfer",
                           row.source_index);
        }
        if (row.expect_timestep_clamped && !row.plan.timestep_clamped) {
            add_diagnostic(plan,
                           EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_expected_timestep_clamp,
                           row.kind, row.case_id, "weather simulation validation case expected timestep clamping",
                           row.source_index);
        }
        if (row.request_native_handle_access) {
            add_diagnostic(
                plan, EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::unsupported_native_handle_access,
                row.kind, row.case_id, "weather simulation validation datasets must not expose native handles",
                row.source_index);
        }
        if (row.request_physical_weather_ready_claim) {
            add_diagnostic(
                plan,
                EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::unsupported_physical_weather_ready_claim,
                row.kind, row.case_id,
                "canonical validation datasets alone cannot claim complete physical weather simulation readiness",
                row.source_index);
        }

        const bool ready = validation_case_ready(row, water_error_mg);
        if (ready) {
            ++plan.ready_case_count;
            switch (row.kind) {
            case EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation:
                plan.supersaturated_condensation_ready = true;
                break;
            case EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation:
                plan.forced_evaporation_precipitation_ready = true;
                break;
            case EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid:
                plan.clamped_mixed_grid_ready = true;
                break;
            }
        }
    }

    plan.validation_images_ready = false;
    plan.physical_weather_ready = false;
    plan.invokes_gpu = false;
    plan.invokes_backend = false;
    plan.dataset_hash = build_dataset_hash(plan);
    plan.status = plan.diagnostics.empty() && plan.ready_case_count == plan.required_case_count
                      ? EnvironmentWeatherSimulationValidationDatasetStatus::ready
                      : EnvironmentWeatherSimulationValidationDatasetStatus::blocked;
    return plan;
}

EnvironmentWeatherSimulationValidationImagePlan
plan_environment_weather_simulation_validation_images(const EnvironmentWeatherSimulationValidationImageDesc& desc) {
    EnvironmentWeatherSimulationValidationImagePlan plan;
    plan.exposes_native_handles = desc.request_native_handle_access || desc.dataset.exposes_native_handles;
    plan.required_image_count =
        static_cast<std::uint32_t>(required_validation_cases.size() * required_validation_image_kinds.size());

    if (!desc.dataset.succeeded()) {
        add_diagnostic(plan, EnvironmentWeatherSimulationValidationImageDiagnosticCode::missing_ready_dataset,
                       EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation, {},
                       "weather simulation validation images require a ready canonical validation dataset", 0U);
    }
    if (desc.request_native_handle_access) {
        add_diagnostic(plan,
                       EnvironmentWeatherSimulationValidationImageDiagnosticCode::unsupported_native_handle_access,
                       EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation, {},
                       "weather simulation validation image counters must not expose native handles", 0U);
    }
    if (desc.request_physical_weather_ready_claim) {
        add_diagnostic(
            plan, EnvironmentWeatherSimulationValidationImageDiagnosticCode::unsupported_physical_weather_ready_claim,
            EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation, {},
            "selected validation image counters cannot claim complete physical weather simulation readiness", 0U);
    }

    for (const auto& row : desc.dataset.cases) {
        const auto expected_pixels =
            static_cast<std::uint64_t>(row.grid_width) * static_cast<std::uint64_t>(row.grid_height);
        if (row.grid_width == 0U || row.grid_height == 0U ||
            expected_pixels > static_cast<std::uint64_t>(std::numeric_limits<std::uint32_t>::max()) ||
            expected_pixels != row.plan.cell_rows.size() || expected_pixels != row.plan.cell_count) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationImageDiagnosticCode::invalid_image_dimensions,
                           row.kind, row.case_id,
                           "weather simulation validation image dimensions must match the reviewed CPU grid",
                           row.source_index);
            continue;
        }
        if (row.plan.cell_rows.empty()) {
            add_diagnostic(plan, EnvironmentWeatherSimulationValidationImageDiagnosticCode::missing_case_rows, row.kind,
                           row.case_id, "weather simulation validation images require reviewed CPU reference cell rows",
                           row.source_index);
            continue;
        }

        const auto row_count_before = plan.rows.size();
        for (const auto image_kind : required_validation_image_kinds) {
            plan.rows.push_back(build_validation_image_row(row, image_kind));
        }
        const auto added_rows = plan.rows.size() - row_count_before;
        if (added_rows == required_validation_image_kinds.size()) {
            switch (row.kind) {
            case EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation:
                plan.supersaturated_condensation_images_ready = true;
                break;
            case EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation:
                plan.forced_evaporation_precipitation_images_ready = true;
                break;
            case EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid:
                plan.clamped_mixed_grid_images_ready = true;
                break;
            }
        }
    }

    plan.image_count = static_cast<std::uint32_t>(plan.rows.size());
    plan.validation_images_ready = plan.diagnostics.empty() && plan.image_count == plan.required_image_count &&
                                   plan.supersaturated_condensation_images_ready &&
                                   plan.forced_evaporation_precipitation_images_ready &&
                                   plan.clamped_mixed_grid_images_ready;
    plan.physical_weather_ready = false;
    plan.invokes_gpu = false;
    plan.invokes_backend = false;
    plan.image_hash = build_image_hash(plan);
    plan.status = plan.validation_images_ready ? EnvironmentWeatherSimulationValidationImageStatus::ready
                                               : EnvironmentWeatherSimulationValidationImageStatus::blocked;
    return plan;
}

bool has_environment_weather_simulation_diagnostic(const EnvironmentWeatherSimulationPlan& plan,
                                                   EnvironmentWeatherSimulationDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_environment_weather_simulation_solver_budget_diagnostic(
    const EnvironmentWeatherSimulationSolverBudgetPlan& plan,
    EnvironmentWeatherSimulationSolverBudgetDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_environment_weather_simulation_validation_dataset_diagnostic(
    const EnvironmentWeatherSimulationValidationDatasetPlan& plan,
    EnvironmentWeatherSimulationValidationDatasetDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

bool has_environment_weather_simulation_validation_image_diagnostic(
    const EnvironmentWeatherSimulationValidationImagePlan& plan,
    EnvironmentWeatherSimulationValidationImageDiagnosticCode code) noexcept {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

} // namespace mirakana
