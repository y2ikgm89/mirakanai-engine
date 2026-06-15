// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/environment/weather_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

namespace {

[[nodiscard]] bool nearly_equal(const double lhs, const double rhs, const double epsilon = 0.000001) noexcept {
    return std::fabs(lhs - rhs) <= epsilon;
}

[[nodiscard]] mirakana::EnvironmentWeatherSimulationDesc make_single_cell_desc() {
    const auto saturation = mirakana::environment_weather_saturation_vapor_kg_per_m2(10.0F, 1000.0F, 1000.0F);

    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 1U;
    desc.height = 1U;
    desc.cell_area_m2 = 25.0F;
    desc.mixing_height_m = 1000.0F;
    desc.air_pressure_hpa = 1000.0F;
    desc.requested_timestep_s = 1.0F;
    desc.max_timestep_s = 1.0F;
    desc.deterministic_seed = 42U;
    desc.initial_cells.push_back(mirakana::EnvironmentWeatherSimulationCellState{
        .temperature_celsius = 10.0F,
        .vapor_water_kg_per_m2 = saturation + 0.2F,
        .cloud_water_kg_per_m2 = 0.1F,
        .surface_water_kg_per_m2 = 2.0F,
    });
    desc.forcing_rows.push_back(mirakana::EnvironmentWeatherSimulationCellForcing{});
    return desc;
}

[[nodiscard]] mirakana::EnvironmentWeatherSimulationDesc make_transfer_desc() {
    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 1U;
    desc.height = 1U;
    desc.cell_area_m2 = 4.0F;
    desc.mixing_height_m = 800.0F;
    desc.air_pressure_hpa = 1000.0F;
    desc.requested_timestep_s = 1.0F;
    desc.max_timestep_s = 1.0F;
    desc.initial_cells.push_back(mirakana::EnvironmentWeatherSimulationCellState{
        .temperature_celsius = 4.0F,
        .vapor_water_kg_per_m2 = 0.1F,
        .cloud_water_kg_per_m2 = 1.0F,
        .surface_water_kg_per_m2 = 2.0F,
    });
    desc.forcing_rows.push_back(mirakana::EnvironmentWeatherSimulationCellForcing{
        .surface_evaporation_kg_per_m2_s = 0.2F,
        .temperature_delta_celsius_per_s = 0.0F,
        .cloud_precipitation_rate_per_s = 0.5F,
    });
    return desc;
}

[[nodiscard]] mirakana::EnvironmentWeatherSimulationDesc make_clamped_mixed_grid_desc() {
    const auto saturation = mirakana::environment_weather_saturation_vapor_kg_per_m2(8.0F, 1000.0F, 900.0F);

    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 2U;
    desc.height = 2U;
    desc.cell_area_m2 = 9.0F;
    desc.mixing_height_m = 900.0F;
    desc.air_pressure_hpa = 1000.0F;
    desc.requested_timestep_s = 10.0F;
    desc.max_timestep_s = 0.25F;
    desc.deterministic_seed = 7U;
    desc.initial_cells = {
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 8.0F,
            .vapor_water_kg_per_m2 = saturation + 0.1F,
            .cloud_water_kg_per_m2 = 0.2F,
            .surface_water_kg_per_m2 = 1.0F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 2.0F,
            .vapor_water_kg_per_m2 = 0.2F,
            .cloud_water_kg_per_m2 = 0.4F,
            .surface_water_kg_per_m2 = 1.2F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = -2.0F,
            .vapor_water_kg_per_m2 = 0.15F,
            .cloud_water_kg_per_m2 = 0.3F,
            .surface_water_kg_per_m2 = 0.8F,
        },
        mirakana::EnvironmentWeatherSimulationCellState{
            .temperature_celsius = 12.0F,
            .vapor_water_kg_per_m2 = 0.3F,
            .cloud_water_kg_per_m2 = 0.1F,
            .surface_water_kg_per_m2 = 1.5F,
        },
    };
    desc.forcing_rows = {
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .cloud_precipitation_rate_per_s = 0.25F,
        },
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .surface_evaporation_kg_per_m2_s = 0.1F,
            .temperature_delta_celsius_per_s = 0.0F,
            .cloud_precipitation_rate_per_s = 0.1F,
        },
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .surface_evaporation_kg_per_m2_s = 0.05F,
        },
        mirakana::EnvironmentWeatherSimulationCellForcing{
            .temperature_delta_celsius_per_s = -1.0F,
            .cloud_precipitation_rate_per_s = 0.15F,
        },
    };
    return desc;
}

[[nodiscard]] mirakana::EnvironmentWeatherSimulationValidationDatasetDesc make_validation_dataset_desc() {
    const auto condensation_plan = mirakana::simulate_environment_weather_cpu_reference(make_single_cell_desc());
    const auto transfer_plan = mirakana::simulate_environment_weather_cpu_reference(make_transfer_desc());
    const auto clamped_plan = mirakana::simulate_environment_weather_cpu_reference(make_clamped_mixed_grid_desc());

    return mirakana::EnvironmentWeatherSimulationValidationDatasetDesc{
        .cases =
            {
                mirakana::EnvironmentWeatherSimulationValidationCase{
                    .case_id = "supersaturated_condensation",
                    .kind = mirakana::EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation,
                    .plan = condensation_plan,
                    .grid_width = 1U,
                    .grid_height = 1U,
                    .water_error_bound_mg = 1U,
                    .expect_condensation = true,
                    .source_index = 1U,
                },
                mirakana::EnvironmentWeatherSimulationValidationCase{
                    .case_id = "forced_evaporation_precipitation",
                    .kind = mirakana::EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation,
                    .plan = transfer_plan,
                    .grid_width = 1U,
                    .grid_height = 1U,
                    .water_error_bound_mg = 1U,
                    .expect_evaporation = true,
                    .expect_precipitation = true,
                    .source_index = 2U,
                },
                mirakana::EnvironmentWeatherSimulationValidationCase{
                    .case_id = "clamped_mixed_grid",
                    .kind = mirakana::EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid,
                    .plan = clamped_plan,
                    .grid_width = 2U,
                    .grid_height = 2U,
                    .water_error_bound_mg = 1U,
                    .expect_condensation = true,
                    .expect_timestep_clamped = true,
                    .source_index = 3U,
                },
            },
    };
}

} // namespace

MK_TEST("environment weather simulation condenses supersaturated vapor and conserves water") {
    const auto desc = make_single_cell_desc();
    const auto saturation = mirakana::environment_weather_saturation_vapor_kg_per_m2(10.0F, 1000.0F, 1000.0F);

    const auto plan = mirakana::simulate_environment_weather_cpu_reference(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentWeatherSimulationStatus::stepped);
    MK_REQUIRE(plan.determinism == mirakana::EnvironmentWeatherSimulationDeterminism::deterministic);
    MK_REQUIRE(plan.fallback_cpu_reference_used);
    MK_REQUIRE(!plan.invokes_gpu);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.physical_weather_ready);
    MK_REQUIRE(plan.cell_rows.size() == 1U);
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].saturation_vapor_kg_per_m2, saturation));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].condensed_kg_per_m2, 0.2));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].state.vapor_water_kg_per_m2, saturation));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].state.cloud_water_kg_per_m2, 0.3));
    MK_REQUIRE(plan.total_condensed_kg > 0.0);
    MK_REQUIRE(plan.total_precipitated_kg == 0.0);
    MK_REQUIRE(plan.water_conservation_error_kg <= 0.00001);
    MK_REQUIRE(plan.max_cell_water_conservation_error_kg_per_m2 <= 0.000001);
    MK_REQUIRE(plan.replay_hash != 0U);
}

MK_TEST("environment weather simulation moves surface evaporation and precipitation without creating water") {
    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 1U;
    desc.height = 1U;
    desc.cell_area_m2 = 4.0F;
    desc.mixing_height_m = 800.0F;
    desc.air_pressure_hpa = 1000.0F;
    desc.requested_timestep_s = 1.0F;
    desc.max_timestep_s = 1.0F;
    desc.initial_cells.push_back(mirakana::EnvironmentWeatherSimulationCellState{
        .temperature_celsius = 4.0F,
        .vapor_water_kg_per_m2 = 0.1F,
        .cloud_water_kg_per_m2 = 1.0F,
        .surface_water_kg_per_m2 = 2.0F,
    });
    desc.forcing_rows.push_back(mirakana::EnvironmentWeatherSimulationCellForcing{
        .surface_evaporation_kg_per_m2_s = 0.2F,
        .temperature_delta_celsius_per_s = 0.0F,
        .cloud_precipitation_rate_per_s = 0.5F,
    });

    const auto plan = mirakana::simulate_environment_weather_cpu_reference(desc);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.cell_rows.size() == 1U);
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].evaporated_kg_per_m2, 0.2));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].precipitated_kg_per_m2, 0.5));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].state.vapor_water_kg_per_m2, 0.3));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].state.cloud_water_kg_per_m2, 0.5));
    MK_REQUIRE(nearly_equal(plan.cell_rows[0].state.surface_water_kg_per_m2, 2.3));
    MK_REQUIRE(nearly_equal(plan.total_evaporated_kg, 0.8));
    MK_REQUIRE(nearly_equal(plan.total_precipitated_kg, 2.0));
    MK_REQUIRE(plan.water_conservation_error_kg <= 0.00001);
}

MK_TEST("environment weather simulation clamps timestep and replays deterministically") {
    auto desc = make_single_cell_desc();
    desc.requested_timestep_s = 10.0F;
    desc.max_timestep_s = 0.25F;
    desc.forcing_rows[0].cloud_precipitation_rate_per_s = 1.0F;

    const auto first = mirakana::simulate_environment_weather_cpu_reference(desc);
    const auto second = mirakana::simulate_environment_weather_cpu_reference(desc);

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.timestep_clamped);
    MK_REQUIRE(first.effective_timestep_s == 0.25F);
    MK_REQUIRE(first.replay_hash == second.replay_hash);
    MK_REQUIRE(first.total_water_before_kg == second.total_water_before_kg);
    MK_REQUIRE(first.total_water_after_kg == second.total_water_after_kg);
}

MK_TEST("environment weather simulation fails closed for invalid input and unsupported side effects") {
    mirakana::EnvironmentWeatherSimulationDesc desc{};
    desc.width = 0U;
    desc.height = 1U;
    desc.cell_area_m2 = 0.0F;
    desc.mixing_height_m = std::numeric_limits<float>::quiet_NaN();
    desc.air_pressure_hpa = 2.0F;
    desc.requested_timestep_s = -1.0F;
    desc.max_timestep_s = 0.0F;
    desc.initial_cells.push_back(mirakana::EnvironmentWeatherSimulationCellState{
        .temperature_celsius = std::numeric_limits<float>::quiet_NaN(),
        .vapor_water_kg_per_m2 = -1.0F,
        .cloud_water_kg_per_m2 = -1.0F,
        .surface_water_kg_per_m2 = -1.0F,
    });
    desc.forcing_rows.push_back(mirakana::EnvironmentWeatherSimulationCellForcing{
        .surface_evaporation_kg_per_m2_s = -0.1F,
        .temperature_delta_celsius_per_s = std::numeric_limits<float>::quiet_NaN(),
        .cloud_precipitation_rate_per_s = 1000.0F,
    });
    desc.request_gpu_acceleration = true;
    desc.request_backend_execution = true;
    desc.request_native_handle_access = true;
    desc.request_physical_weather_ready_claim = true;

    const auto plan = mirakana::simulate_environment_weather_cpu_reference(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentWeatherSimulationStatus::blocked);
    MK_REQUIRE(!plan.invokes_gpu);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(!plan.physical_weather_ready);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::invalid_grid));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::invalid_timestep));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::invalid_cell_state));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::invalid_forcing));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::unsupported_gpu_acceleration));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::unsupported_backend_execution));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::unsupported_native_handle_access));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationDiagnosticCode::unsupported_physical_weather_ready_claim));
}

MK_TEST("environment weather simulation package budget reports cpu reference timing without production solver claim") {
    const mirakana::EnvironmentWeatherSimulationSolverBudgetDesc desc{
        .cpu_reference_package_ready = true,
        .cpu_elapsed_us = 750U,
        .cpu_budget_us = 5000U,
    };

    const auto plan = mirakana::plan_environment_weather_simulation_solver_budget(desc);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentWeatherSimulationSolverBudgetStatus::host_evidence_required);
    MK_REQUIRE(plan.cpu_elapsed_us == 750U);
    MK_REQUIRE(plan.cpu_budget_us == 5000U);
    MK_REQUIRE(plan.cpu_budget_ready);
    MK_REQUIRE(!plan.cpu_budget_over);
    MK_REQUIRE(plan.gpu_elapsed_us == 0U);
    MK_REQUIRE(plan.gpu_budget_us == 0U);
    MK_REQUIRE(!plan.gpu_budget_ready);
    MK_REQUIRE(!plan.profiler_artifact_ready);
    MK_REQUIRE(!plan.profiler_budget_ready);
    MK_REQUIRE(!plan.production_solver_ready);
    MK_REQUIRE(!plan.invokes_gpu);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("environment weather simulation package budget fails closed for invalid or unsupported readiness claims") {
    mirakana::EnvironmentWeatherSimulationSolverBudgetDesc missing_package{};
    missing_package.cpu_budget_us = 5000U;

    const auto missing_plan = mirakana::plan_environment_weather_simulation_solver_budget(missing_package);

    MK_REQUIRE(!missing_plan.succeeded());
    MK_REQUIRE(mirakana::has_environment_weather_simulation_solver_budget_diagnostic(
        missing_plan, mirakana::EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::missing_cpu_reference_package));

    const mirakana::EnvironmentWeatherSimulationSolverBudgetDesc over_budget{
        .cpu_reference_package_ready = true,
        .cpu_elapsed_us = 6000U,
        .cpu_budget_us = 5000U,
    };

    const auto over_budget_plan = mirakana::plan_environment_weather_simulation_solver_budget(over_budget);

    MK_REQUIRE(!over_budget_plan.succeeded());
    MK_REQUIRE(over_budget_plan.status == mirakana::EnvironmentWeatherSimulationSolverBudgetStatus::budget_exceeded);
    MK_REQUIRE(over_budget_plan.cpu_budget_over);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_solver_budget_diagnostic(
        over_budget_plan, mirakana::EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::cpu_budget_exceeded));

    const mirakana::EnvironmentWeatherSimulationSolverBudgetDesc unsupported_ready{
        .cpu_reference_package_ready = true,
        .cpu_elapsed_us = 750U,
        .cpu_budget_us = 5000U,
        .gpu_solver_package_ready = true,
        .request_production_solver_ready_claim = true,
    };

    const auto unsupported_plan = mirakana::plan_environment_weather_simulation_solver_budget(unsupported_ready);

    MK_REQUIRE(!unsupported_plan.succeeded());
    MK_REQUIRE(!unsupported_plan.production_solver_ready);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_solver_budget_diagnostic(
        unsupported_plan, mirakana::EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::unsupported_gpu_solver));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_solver_budget_diagnostic(
        unsupported_plan,
        mirakana::EnvironmentWeatherSimulationSolverBudgetDiagnosticCode::unsupported_production_solver_ready_claim));
}

MK_TEST("environment weather simulation validation dataset accepts selected canonical water-cycle cases") {
    const auto dataset = make_validation_dataset_desc();

    const auto plan = mirakana::plan_environment_weather_simulation_validation_dataset(dataset);

    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentWeatherSimulationValidationDatasetStatus::ready);
    MK_REQUIRE(plan.case_count == 3U);
    MK_REQUIRE(plan.required_case_count == 3U);
    MK_REQUIRE(plan.ready_case_count == 3U);
    MK_REQUIRE(plan.supersaturated_condensation_ready);
    MK_REQUIRE(plan.forced_evaporation_precipitation_ready);
    MK_REQUIRE(plan.clamped_mixed_grid_ready);
    MK_REQUIRE(plan.validation_images_ready == false);
    MK_REQUIRE(plan.physical_weather_ready == false);
    MK_REQUIRE(!plan.invokes_gpu);
    MK_REQUIRE(!plan.invokes_backend);
    MK_REQUIRE(!plan.exposes_native_handles);
    MK_REQUIRE(plan.max_water_conservation_error_mg <= 1U);
    MK_REQUIRE(plan.dataset_hash != 0U);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("environment weather simulation validation images produce deterministic selected scalar fields") {
    const auto dataset_plan =
        mirakana::plan_environment_weather_simulation_validation_dataset(make_validation_dataset_desc());

    const auto first = mirakana::plan_environment_weather_simulation_validation_images(
        mirakana::EnvironmentWeatherSimulationValidationImageDesc{.dataset = dataset_plan});
    const auto second = mirakana::plan_environment_weather_simulation_validation_images(
        mirakana::EnvironmentWeatherSimulationValidationImageDesc{.dataset = dataset_plan});

    MK_REQUIRE(first.succeeded());
    MK_REQUIRE(first.status == mirakana::EnvironmentWeatherSimulationValidationImageStatus::ready);
    MK_REQUIRE(first.validation_images_ready);
    MK_REQUIRE(!first.physical_weather_ready);
    MK_REQUIRE(!first.invokes_gpu);
    MK_REQUIRE(!first.invokes_backend);
    MK_REQUIRE(!first.exposes_native_handles);
    MK_REQUIRE(first.image_count == 12U);
    MK_REQUIRE(first.required_image_count == 12U);
    MK_REQUIRE(first.supersaturated_condensation_images_ready);
    MK_REQUIRE(first.forced_evaporation_precipitation_images_ready);
    MK_REQUIRE(first.clamped_mixed_grid_images_ready);
    MK_REQUIRE(first.image_hash != 0U);
    MK_REQUIRE(first.image_hash == second.image_hash);
    MK_REQUIRE(first.rows.size() == 12U);
    MK_REQUIRE(first.diagnostics.empty());

    const auto find_row = [&first](mirakana::EnvironmentWeatherSimulationValidationCaseKind case_kind,
                                   mirakana::EnvironmentWeatherSimulationValidationImageKind image_kind) {
        return std::ranges::find_if(first.rows, [case_kind, image_kind](const auto& row) {
            return row.case_kind == case_kind && row.image_kind == image_kind;
        });
    };

    const auto transfer_row =
        find_row(mirakana::EnvironmentWeatherSimulationValidationCaseKind::forced_evaporation_precipitation,
                 mirakana::EnvironmentWeatherSimulationValidationImageKind::water_transfer);
    MK_REQUIRE(transfer_row != first.rows.end());
    MK_REQUIRE(transfer_row->width == 1U);
    MK_REQUIRE(transfer_row->height == 1U);
    MK_REQUIRE(transfer_row->pixel_count == 1U);
    MK_REQUIRE(transfer_row->max_sample_mg_per_m2 > 0U);
    MK_REQUIRE(transfer_row->checksum != 0U);

    const auto clamped_cloud_row =
        find_row(mirakana::EnvironmentWeatherSimulationValidationCaseKind::clamped_mixed_grid,
                 mirakana::EnvironmentWeatherSimulationValidationImageKind::cloud_water_after);
    MK_REQUIRE(clamped_cloud_row != first.rows.end());
    MK_REQUIRE(clamped_cloud_row->width == 2U);
    MK_REQUIRE(clamped_cloud_row->height == 2U);
    MK_REQUIRE(clamped_cloud_row->pixel_count == 4U);
    MK_REQUIRE(clamped_cloud_row->checksum != 0U);
}

MK_TEST("environment weather simulation validation images fail closed without dataset or for ready claims") {
    const auto missing = mirakana::plan_environment_weather_simulation_validation_images(
        mirakana::EnvironmentWeatherSimulationValidationImageDesc{});

    MK_REQUIRE(!missing.succeeded());
    MK_REQUIRE(missing.status == mirakana::EnvironmentWeatherSimulationValidationImageStatus::blocked);
    MK_REQUIRE(!missing.validation_images_ready);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_image_diagnostic(
        missing, mirakana::EnvironmentWeatherSimulationValidationImageDiagnosticCode::missing_ready_dataset));

    auto dataset_plan =
        mirakana::plan_environment_weather_simulation_validation_dataset(make_validation_dataset_desc());
    dataset_plan.cases[2].grid_width = 0U;
    const auto unsupported = mirakana::plan_environment_weather_simulation_validation_images(
        mirakana::EnvironmentWeatherSimulationValidationImageDesc{
            .dataset = dataset_plan,
            .request_native_handle_access = true,
            .request_physical_weather_ready_claim = true,
        });

    MK_REQUIRE(!unsupported.succeeded());
    MK_REQUIRE(!unsupported.validation_images_ready);
    MK_REQUIRE(!unsupported.physical_weather_ready);
    MK_REQUIRE(unsupported.exposes_native_handles);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_image_diagnostic(
        unsupported, mirakana::EnvironmentWeatherSimulationValidationImageDiagnosticCode::invalid_image_dimensions));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_image_diagnostic(
        unsupported,
        mirakana::EnvironmentWeatherSimulationValidationImageDiagnosticCode::unsupported_native_handle_access));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_image_diagnostic(
        unsupported,
        mirakana::EnvironmentWeatherSimulationValidationImageDiagnosticCode::unsupported_physical_weather_ready_claim));
}

MK_TEST("environment weather simulation validation dataset fails closed for missing or unsupported cases") {
    const auto condensation_plan = mirakana::simulate_environment_weather_cpu_reference(make_single_cell_desc());

    mirakana::EnvironmentWeatherSimulationValidationDatasetDesc dataset{};
    dataset.cases.push_back(mirakana::EnvironmentWeatherSimulationValidationCase{
        .case_id = "supersaturated_condensation",
        .kind = mirakana::EnvironmentWeatherSimulationValidationCaseKind::supersaturated_condensation,
        .plan = condensation_plan,
        .water_error_bound_mg = 1U,
        .expect_evaporation = true,
        .request_physical_weather_ready_claim = true,
        .source_index = 1U,
    });

    const auto plan = mirakana::plan_environment_weather_simulation_validation_dataset(dataset);

    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.status == mirakana::EnvironmentWeatherSimulationValidationDatasetStatus::blocked);
    MK_REQUIRE(!plan.physical_weather_ready);
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_dataset_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_required_case));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_dataset_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::missing_expected_evaporation));
    MK_REQUIRE(mirakana::has_environment_weather_simulation_validation_dataset_diagnostic(
        plan, mirakana::EnvironmentWeatherSimulationValidationDatasetDiagnosticCode::
                  unsupported_physical_weather_ready_claim));
}

int main() {
    return mirakana::test::run_all();
}
