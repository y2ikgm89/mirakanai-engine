// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/runtime/genre_simulation_management.hpp"

#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace {

using mirakana::runtime::RuntimeSimulationJobStatus;
using mirakana::runtime::RuntimeSimulationLogisticsTransferStatus;
using mirakana::runtime::RuntimeSimulationManagementStatus;
using mirakana::runtime::RuntimeSimulationNeedStatus;
using mirakana::runtime::RuntimeSimulationSaveReviewStatus;

[[nodiscard]] mirakana::runtime::RuntimeSimulationResourceRow
make_resource(std::string resource_id, std::string storage_id, std::int64_t quantity, std::int64_t capacity,
              std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationResourceRow{
        .resource_id = std::move(resource_id),
        .storage_id = std::move(storage_id),
        .quantity = quantity,
        .capacity = capacity,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationJobRow
make_job(std::string job_id, std::string worker_id, std::string input_resource_id, std::string input_storage_id,
         std::string output_resource_id, std::string output_storage_id, std::int64_t input_quantity,
         std::int64_t output_quantity, std::uint32_t duration_ticks, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationJobRow{
        .job_id = std::move(job_id),
        .worker_id = std::move(worker_id),
        .input_resource_id = std::move(input_resource_id),
        .input_storage_id = std::move(input_storage_id),
        .output_resource_id = std::move(output_resource_id),
        .output_storage_id = std::move(output_storage_id),
        .input_quantity = input_quantity,
        .output_quantity = output_quantity,
        .duration_ticks = duration_ticks,
        .status = RuntimeSimulationJobStatus::invalid,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationLogisticsLink
make_link(std::string link_id, std::string resource_id, std::string source_storage_id,
          std::string destination_storage_id, std::int64_t transfer_quantity, std::uint32_t travel_ticks, bool enabled,
          std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationLogisticsLink{
        .link_id = std::move(link_id),
        .resource_id = std::move(resource_id),
        .source_storage_id = std::move(source_storage_id),
        .destination_storage_id = std::move(destination_storage_id),
        .transfer_quantity = transfer_quantity,
        .travel_ticks = travel_ticks,
        .enabled = enabled,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationEconomySummary
make_economy(std::string summary_id, std::string resource_id, std::int64_t produced_quantity,
             std::int64_t consumed_quantity, std::int64_t traded_quantity, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationEconomySummary{
        .summary_id = std::move(summary_id),
        .resource_id = std::move(resource_id),
        .produced_quantity = produced_quantity,
        .consumed_quantity = consumed_quantity,
        .traded_quantity = traded_quantity,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationPopulationNeedRow
make_need(std::string population_id, std::string need_id, std::string resource_id, std::string storage_id,
          std::int64_t required_quantity, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationPopulationNeedRow{
        .population_id = std::move(population_id),
        .need_id = std::move(need_id),
        .resource_id = std::move(resource_id),
        .storage_id = std::move(storage_id),
        .required_quantity = required_quantity,
        .available_quantity = 0,
        .status = RuntimeSimulationNeedStatus::invalid,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationScheduleRow
make_schedule(std::string schedule_id, std::string target_id, std::uint64_t start_tick, std::uint64_t end_tick,
              bool enabled, std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationScheduleRow{
        .schedule_id = std::move(schedule_id),
        .target_id = std::move(target_id),
        .start_tick = start_tick,
        .end_tick = end_tick,
        .enabled = enabled,
        .source_index = source_index,
    };
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationSaveReviewRow make_save(std::string domain, std::string key,
                                                                          std::uint32_t expected_schema_version,
                                                                          std::uint32_t observed_schema_version,
                                                                          std::uint32_t source_index) {
    return mirakana::runtime::RuntimeSimulationSaveReviewRow{
        .domain = std::move(domain),
        .key = std::move(key),
        .expected_schema_version = expected_schema_version,
        .observed_schema_version = observed_schema_version,
        .status = RuntimeSimulationSaveReviewStatus::rejected,
        .source_index = source_index,
    };
}

[[nodiscard]] std::size_t diagnostic_count(const mirakana::runtime::RuntimeSimulationManagementPlan& plan,
                                           mirakana::runtime::RuntimeSimulationDiagnosticCode code) {
    std::size_t count{0U};
    for (const auto& diagnostic : plan.diagnostics) {
        if (diagnostic.code == code) {
            ++count;
        }
    }
    return count;
}

[[nodiscard]] std::int64_t projected_quantity_for(const mirakana::runtime::RuntimeSimulationManagementPlan& plan,
                                                  const char* resource_id, const char* storage_id) {
    for (const auto& row : plan.resource_balance_rows) {
        if (row.resource_id == resource_id && row.storage_id == storage_id) {
            return row.projected_quantity;
        }
    }
    return -1;
}

[[nodiscard]] mirakana::runtime::RuntimeSimulationManagementRequest make_hash_sensitive_request() {
    return mirakana::runtime::RuntimeSimulationManagementRequest{
        .simulation_id = "colony.hash",
        .world_tick = 120U,
        .long_run_tick_count = 240U,
        .resource_rows =
            {
                make_resource("food", "colony", 25, 100, 1U),
                make_resource("meal", "colony", 2, 30, 2U),
                make_resource("ore", "mine", 12, 40, 3U),
                make_resource("ore", "colony", 0, 40, 4U),
            },
        .job_rows = {make_job("job.cook", "worker.0", "food", "colony", "meal", "colony", 5, 3, 4U, 1U)},
        .logistics_links = {make_link("route.ore", "ore", "mine", "colony", 4, 6U, true, 1U)},
        .economy_summaries = {make_economy("economy.food", "food", 6, 5, 0, 1U)},
        .population_need_rows =
            {
                make_need("population.colony", "need.food", "food", "colony", 20, 1U),
                make_need("population.colony", "need.comfort", "meal", "colony", 5, 2U),
            },
        .schedule_rows =
            {
                make_schedule("schedule.job.cook", "job.cook", 100U, 200U, true, 1U),
                make_schedule("schedule.route.ore", "route.ore", 100U, 200U, true, 2U),
            },
        .save_review_rows =
            {
                make_save("simulation", "state", 2U, 2U, 1U),
                make_save("simulation", "balances", 3U, 2U, 2U),
            },
        .seed = 42U,
    };
}

} // namespace

MK_TEST("runtime simulation management plans resources jobs logistics needs saves and dashboard rows") {
    const auto
        plan =
            mirakana::runtime::plan_runtime_simulation_management(
                mirakana::runtime::RuntimeSimulationManagementRequest{
                    .simulation_id = "colony.sim",
                    .world_tick = 100U,
                    .long_run_tick_count = 240U,
                    .resource_rows =
                        {
                            make_resource("food", "colony", 25, 100, 1U),
                            make_resource("meal", "colony", 2, 30, 2U),
                            make_resource("ore", "mine", 12, 40, 3U),
                            make_resource("ore", "colony", 0, 40, 4U),
                        },
                    .job_rows =
                        {
                            make_job("job.cook", "worker.0", "food", "colony", "meal", "colony", 5, 3, 4U, 1U),
                            make_job("job.shortage", "worker.1", "ore", "mine", "meal", "colony", 99, 1, 4U, 2U),
                        },
                    .logistics_links =
                        {
                            make_link("route.ore", "ore", "mine", "colony", 4, 6U, true, 1U),
                            make_link("route.ore.back", "ore", "colony", "mine", 99, 6U, true, 2U),
                        },
                    .economy_summaries = {make_economy("economy.food", "food", 6, 5, 0, 1U)},
                    .population_need_rows =
                        {
                            make_need("population.colony", "need.food", "food", "colony", 20, 1U),
                            make_need("population.colony", "need.comfort", "meal", "colony", 5, 2U),
                        },
                    .schedule_rows =
                        {
                            make_schedule("schedule.job.cook", "job.cook", 90U, 200U, true, 1U),
                            make_schedule("schedule.route.ore", "route.ore", 90U, 200U, true, 2U),
                        },
                    .save_review_rows =
                        {
                            make_save("simulation", "state", 2U, 2U, 1U),
                            make_save("simulation", "balances", 3U, 2U, 2U),
                        },
                });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.tick_count == 240U);
    MK_REQUIRE(plan.resource_rows.size() == 4U);
    MK_REQUIRE(plan.resource_balance_rows.size() == 4U);
    MK_REQUIRE(plan.job_rows.size() == 2U);
    MK_REQUIRE(plan.job_assignment_count == 1U);
    MK_REQUIRE(plan.logistics_links.size() == 2U);
    MK_REQUIRE(plan.logistics_transfer_rows.size() == 2U);
    MK_REQUIRE(plan.scheduled_logistics_transfer_count == 1U);
    MK_REQUIRE(plan.economy_summaries.size() == 1U);
    MK_REQUIRE(plan.population_need_rows.size() == 2U);
    MK_REQUIRE(plan.need_deficit_count == 1U);
    MK_REQUIRE(plan.schedule_rows.size() == 2U);
    MK_REQUIRE(plan.save_review_rows.size() == 2U);
    MK_REQUIRE(plan.repairable_save_review_count == 1U);
    MK_REQUIRE(plan.dashboard_rows.size() == 7U);
    MK_REQUIRE(plan.replay_hash != 0U);
    MK_REQUIRE(!plan.invoked_economy_execution);
    MK_REQUIRE(!plan.invoked_save_io);
    MK_REQUIRE(!plan.invoked_runtime_ui);
    MK_REQUIRE(!plan.invoked_package_io);
    MK_REQUIRE(plan.job_rows[0].status == RuntimeSimulationJobStatus::assigned);
    MK_REQUIRE(plan.job_rows[1].status == RuntimeSimulationJobStatus::blocked_missing_input);
    MK_REQUIRE(plan.logistics_transfer_rows[0].status == RuntimeSimulationLogisticsTransferStatus::scheduled);
    MK_REQUIRE(plan.logistics_transfer_rows[1].status ==
               RuntimeSimulationLogisticsTransferStatus::blocked_missing_source);
    MK_REQUIRE(plan.population_need_rows[0].status == RuntimeSimulationNeedStatus::satisfied);
    MK_REQUIRE(plan.population_need_rows[1].status == RuntimeSimulationNeedStatus::deficit);
    MK_REQUIRE(plan.save_review_rows[0].status == RuntimeSimulationSaveReviewStatus::accepted);
    MK_REQUIRE(plan.save_review_rows[1].status == RuntimeSimulationSaveReviewStatus::repairable);
}

MK_TEST("runtime simulation management rejects unsafe or inconsistent rows before output rows") {
    const auto
        plan =
            mirakana::runtime::plan_runtime_simulation_management(
                mirakana::runtime::RuntimeSimulationManagementRequest{
                    .simulation_id = "renderer.sim",
                    .world_tick = 1U,
                    .long_run_tick_count = 0U,
                    .resource_rows =
                        {
                            make_resource("food", "colony", 25, 100, 1U),
                            make_resource("food", "colony", 25, 100, 2U),
                            make_resource("bad_resource", "colony", 1, 0, 3U),
                        },
                    .job_rows =
                        {
                            make_job("job.dupe", "worker.0", "food", "colony", "meal", "colony", 1, 1, 1U, 1U),
                            make_job("job.dupe", "worker.1", "missing", "colony", "meal", "colony", 1, 1, 1U, 2U),
                            make_job("renderer.job", "", "food", "colony", "meal", "colony", 0, 1, 1U, 3U),
                        },
                    .logistics_links =
                        {
                            make_link("route.dupe", "missing", "colony", "mine", 1, 1U, true, 1U),
                            make_link("route.dupe", "missing", "colony", "mine", 1, 1U, true, 2U),
                            make_link("route.bad", "missing", "colony", "mine", 0, 0U, true, 3U),
                        },
                    .economy_summaries =
                        {
                            make_economy("economy.food", "food", 1, 1, 0, 1U),
                            make_economy("economy.food", "food", -1, 1, 0, 2U),
                        },
                    .population_need_rows =
                        {
                            make_need("population.colony", "need.food", "food", "colony", 1, 1U),
                            make_need("population.colony", "need.food", "missing", "colony", 0, 2U),
                        },
                    .schedule_rows =
                        {
                            make_schedule("schedule.dupe", "job.dupe", 1U, 2U, true, 1U),
                            make_schedule("schedule.dupe", "job.dupe", 2U, 1U, true, 2U),
                        },
                    .save_review_rows =
                        {
                            make_save("simulation", "state", 1U, 1U, 1U),
                            make_save("simulation", "state", 0U, 1U, 2U),
                        },
                    .game_balance_rule_ids = {"balance.tax", "content.worker-names"},
                });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unsupported_backend_reference) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_tick_count) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_resource_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_resource) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_job_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_job) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unknown_job_resource) == 2U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_logistics_link) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_logistics_link) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unknown_logistics_resource) ==
               4U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_economy_summary) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_economy_summary) ==
               1U);
    MK_REQUIRE(
        diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_population_need_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_population_need) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_schedule_row) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_schedule) == 1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_save_review_row) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::duplicate_save_review_key) ==
               1U);
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unsupported_game_balance_rule) == 2U);
    MK_REQUIRE(plan.resource_rows.empty());
    MK_REQUIRE(plan.resource_balance_rows.empty());
    MK_REQUIRE(plan.job_rows.empty());
    MK_REQUIRE(plan.logistics_transfer_rows.empty());
    MK_REQUIRE(plan.dashboard_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("runtime simulation management replay hash changes when normalized output fields change") {
    auto request = make_hash_sensitive_request();
    const auto first = mirakana::runtime::plan_runtime_simulation_management(request);
    MK_REQUIRE(first.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(first.replay_hash != 0U);

    request.population_need_rows[1].required_quantity = 6;
    const auto changed_need = mirakana::runtime::plan_runtime_simulation_management(request);
    MK_REQUIRE(changed_need.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(changed_need.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.save_review_rows[1].observed_schema_version = 3U;
    const auto changed_save_status = mirakana::runtime::plan_runtime_simulation_management(request);
    MK_REQUIRE(changed_save_status.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(changed_save_status.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.logistics_links[0].transfer_quantity = 5;
    const auto changed_transfer = mirakana::runtime::plan_runtime_simulation_management(request);
    MK_REQUIRE(changed_transfer.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(changed_transfer.replay_hash != first.replay_hash);

    request = make_hash_sensitive_request();
    request.job_rows[0].input_quantity = 6;
    const auto changed_job = mirakana::runtime::plan_runtime_simulation_management(request);
    MK_REQUIRE(changed_job.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(changed_job.replay_hash != first.replay_hash);
}

MK_TEST("runtime simulation management projects competing rows without signed overflow") {
    const auto max_quantity = std::numeric_limits<std::int64_t>::max();
    const auto
        plan =
            mirakana::runtime::plan_runtime_simulation_management(
                mirakana::runtime::RuntimeSimulationManagementRequest{
                    .simulation_id = "colony.competition",
                    .world_tick = 100U,
                    .long_run_tick_count = 240U,
                    .resource_rows =
                        {
                            make_resource("ore", "mine", 10, 20, 1U),
                            make_resource("ore", "depot", 0, 20, 2U),
                            make_resource("ingot", "forge", 0, 5, 3U),
                            make_resource("credits", "vault", max_quantity - 1, max_quantity, 4U),
                            make_resource("credits", "warehouse", 10, 10, 5U),
                        },
                    .job_rows =
                        {
                            make_job("job.smelt.a", "worker.0", "ore", "mine", "ingot", "forge", 7, 3, 4U, 1U),
                            make_job("job.smelt.b", "worker.1", "ore", "mine", "ingot", "forge", 7, 3, 4U, 2U),
                            make_job("job.mint", "worker.2", "ore", "mine", "credits", "vault", 1, 8, 4U, 3U),
                        },
                    .logistics_links =
                        {
                            make_link("route.ore.a", "ore", "mine", "depot", 2, 6U, true, 1U),
                            make_link("route.ore.b", "ore", "mine", "depot", 2, 6U, true, 2U),
                            make_link("route.credits", "credits", "warehouse", "vault", 8, 6U, true, 3U),
                        },
                    .schedule_rows =
                        {
                            make_schedule("schedule.job.smelt.a", "job.smelt.a", 90U, 200U, true, 1U),
                            make_schedule("schedule.job.smelt.b", "job.smelt.b", 90U, 200U, true, 2U),
                            make_schedule("schedule.job.mint", "job.mint", 90U, 200U, true, 3U),
                            make_schedule("schedule.route.ore.a", "route.ore.a", 90U, 200U, true, 4U),
                            make_schedule("schedule.route.ore.b", "route.ore.b", 90U, 200U, true, 5U),
                            make_schedule("schedule.route.credits", "route.credits", 90U, 200U, true, 6U),
                        },
                });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::ready);
    MK_REQUIRE(plan.diagnostics.empty());
    MK_REQUIRE(plan.job_assignment_count == 1U);
    MK_REQUIRE(plan.scheduled_logistics_transfer_count == 1U);
    MK_REQUIRE(plan.job_rows[0].status == RuntimeSimulationJobStatus::assigned);
    MK_REQUIRE(plan.job_rows[1].status == RuntimeSimulationJobStatus::blocked_missing_input);
    MK_REQUIRE(plan.job_rows[2].status == RuntimeSimulationJobStatus::blocked_missing_output);
    MK_REQUIRE(plan.logistics_transfer_rows[0].status == RuntimeSimulationLogisticsTransferStatus::scheduled);
    MK_REQUIRE(plan.logistics_transfer_rows[1].status ==
               RuntimeSimulationLogisticsTransferStatus::blocked_missing_source);
    MK_REQUIRE(plan.logistics_transfer_rows[2].status ==
               RuntimeSimulationLogisticsTransferStatus::blocked_missing_destination);
    MK_REQUIRE(projected_quantity_for(plan, "ore", "mine") == 1);
    MK_REQUIRE(projected_quantity_for(plan, "credits", "vault") == max_quantity - 1);
}

MK_TEST("runtime simulation management rejects missing cross referenced resources") {
    const auto plan =
        mirakana::runtime::plan_runtime_simulation_management(mirakana::runtime::RuntimeSimulationManagementRequest{
            .simulation_id = "colony.cross_refs",
            .world_tick = 100U,
            .long_run_tick_count = 240U,
            .resource_rows = {make_resource("food", "colony", 10, 20, 1U)},
            .logistics_links = {make_link("route.food", "food", "colony", "warehouse", 1, 1U, true, 1U)},
            .economy_summaries = {make_economy("economy.ore", "ore", 1, 0, 0, 1U)},
            .population_need_rows = {make_need("population.colony", "need.ore", "ore", "colony", 1, 1U)},
        });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unknown_logistics_resource) ==
               1U);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unknown_economy_resource) ==
               1U);
    MK_REQUIRE(diagnostic_count(
                   plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::unknown_population_need_resource) == 1U);
    MK_REQUIRE(plan.resource_rows.empty());
    MK_REQUIRE(plan.logistics_transfer_rows.empty());
    MK_REQUIRE(plan.economy_summaries.empty());
    MK_REQUIRE(plan.population_need_rows.empty());
}

MK_TEST("runtime simulation management diagnostics are ordered by stable public fields") {
    const auto plan =
        mirakana::runtime::plan_runtime_simulation_management(mirakana::runtime::RuntimeSimulationManagementRequest{
            .simulation_id = "colony.diagnostics",
            .world_tick = 1U,
            .long_run_tick_count = 1U,
            .resource_rows = {make_resource("renderer", "native", 2, 1, 3U)},
        });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::invalid_resource_row) == 2U);
    MK_REQUIRE(plan.diagnostics[0].message < plan.diagnostics[1].message);
}

MK_TEST("runtime simulation management diagnostic rows exceeding row budget add budget diagnostic") {
    const auto plan =
        mirakana::runtime::plan_runtime_simulation_management(mirakana::runtime::RuntimeSimulationManagementRequest{
            .simulation_id = "colony.diagnostic_budget",
            .world_tick = 1U,
            .long_run_tick_count = 1U,
            .resource_rows = {make_resource("renderer", "native", 2, 1, 1U)},
            .game_balance_rule_ids = {"balance.worker_names"},
            .row_budget = 2U,
        });

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::invalid_request);
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.resource_rows.empty());
    MK_REQUIRE(plan.dashboard_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

MK_TEST("runtime simulation management generated rows exceeding row budget fail closed") {
    auto request = make_hash_sensitive_request();
    request.row_budget = 5U;

    const auto plan = mirakana::runtime::plan_runtime_simulation_management(request);

    MK_REQUIRE(plan.status == RuntimeSimulationManagementStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(diagnostic_count(plan, mirakana::runtime::RuntimeSimulationDiagnosticCode::row_budget_exceeded) == 1U);
    MK_REQUIRE(plan.resource_rows.empty());
    MK_REQUIRE(plan.resource_balance_rows.empty());
    MK_REQUIRE(plan.job_rows.empty());
    MK_REQUIRE(plan.logistics_links.empty());
    MK_REQUIRE(plan.logistics_transfer_rows.empty());
    MK_REQUIRE(plan.economy_summaries.empty());
    MK_REQUIRE(plan.population_need_rows.empty());
    MK_REQUIRE(plan.schedule_rows.empty());
    MK_REQUIRE(plan.save_review_rows.empty());
    MK_REQUIRE(plan.dashboard_rows.empty());
    MK_REQUIRE(plan.replay_hash == 0U);
}

int main() {
    return mirakana::test::run_all();
}
