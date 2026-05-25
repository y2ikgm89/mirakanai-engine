// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::runtime {

enum class RuntimeSimulationManagementStatus : std::uint8_t {
    ready = 0,
    no_rows,
    invalid_request,
};

enum class RuntimeSimulationJobStatus : std::uint8_t {
    assigned = 0,
    blocked_missing_input,
    blocked_missing_output,
    inactive_schedule,
    invalid,
};

enum class RuntimeSimulationLogisticsTransferStatus : std::uint8_t {
    scheduled = 0,
    disabled,
    blocked_missing_source,
    blocked_missing_destination,
    invalid,
};

enum class RuntimeSimulationNeedStatus : std::uint8_t {
    satisfied = 0,
    deficit,
    invalid,
};

enum class RuntimeSimulationSaveReviewStatus : std::uint8_t {
    accepted = 0,
    repairable,
    rejected,
};

enum class RuntimeSimulationDiagnosticCode : std::uint8_t {
    missing_simulation_id = 0,
    unsupported_backend_reference,
    invalid_tick_count,
    invalid_resource_row,
    duplicate_resource,
    invalid_job_row,
    duplicate_job,
    unknown_job_resource,
    invalid_logistics_link,
    duplicate_logistics_link,
    unknown_logistics_resource,
    invalid_economy_summary,
    duplicate_economy_summary,
    unknown_economy_resource,
    invalid_population_need_row,
    duplicate_population_need,
    unknown_population_need_resource,
    invalid_schedule_row,
    duplicate_schedule,
    invalid_save_review_row,
    duplicate_save_review_key,
    unsupported_game_balance_rule,
    row_budget_exceeded,
};

struct RuntimeSimulationResourceRow {
    std::string resource_id;
    std::string storage_id;
    std::int64_t quantity{0};
    std::int64_t capacity{0};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationResourceRow&) const = default;
};

struct RuntimeSimulationResourceBalanceRow {
    std::string resource_id;
    std::string storage_id;
    std::int64_t starting_quantity{0};
    std::int64_t projected_quantity{0};
    std::int64_t capacity{0};
    std::int64_t job_input_quantity{0};
    std::int64_t job_output_quantity{0};
    std::int64_t logistics_outgoing_quantity{0};
    std::int64_t logistics_incoming_quantity{0};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationResourceBalanceRow&) const = default;
};

struct RuntimeSimulationJobRow {
    std::string job_id;
    std::string worker_id;
    std::string input_resource_id;
    std::string input_storage_id;
    std::string output_resource_id;
    std::string output_storage_id;
    std::int64_t input_quantity{0};
    std::int64_t output_quantity{0};
    std::uint32_t duration_ticks{0U};
    RuntimeSimulationJobStatus status{RuntimeSimulationJobStatus::invalid};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationJobRow&) const = default;
};

struct RuntimeSimulationLogisticsLink {
    std::string link_id;
    std::string resource_id;
    std::string source_storage_id;
    std::string destination_storage_id;
    std::int64_t transfer_quantity{0};
    std::uint32_t travel_ticks{0U};
    bool enabled{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationLogisticsLink&) const = default;
};

struct RuntimeSimulationLogisticsTransferRow {
    std::string link_id;
    std::string resource_id;
    std::string source_storage_id;
    std::string destination_storage_id;
    std::int64_t transfer_quantity{0};
    std::uint64_t arrival_tick{0U};
    RuntimeSimulationLogisticsTransferStatus status{RuntimeSimulationLogisticsTransferStatus::invalid};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationLogisticsTransferRow&) const = default;
};

struct RuntimeSimulationEconomySummary {
    std::string summary_id;
    std::string resource_id;
    std::int64_t produced_quantity{0};
    std::int64_t consumed_quantity{0};
    std::int64_t traded_quantity{0};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationEconomySummary&) const = default;
};

struct RuntimeSimulationPopulationNeedRow {
    std::string population_id;
    std::string need_id;
    std::string resource_id;
    std::string storage_id;
    std::int64_t required_quantity{0};
    std::int64_t available_quantity{0};
    RuntimeSimulationNeedStatus status{RuntimeSimulationNeedStatus::invalid};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationPopulationNeedRow&) const = default;
};

struct RuntimeSimulationScheduleRow {
    std::string schedule_id;
    std::string target_id;
    std::uint64_t start_tick{0U};
    std::uint64_t end_tick{0U};
    bool enabled{false};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationScheduleRow&) const = default;
};

struct RuntimeSimulationSaveReviewRow {
    std::string domain;
    std::string key;
    std::uint32_t expected_schema_version{1U};
    std::uint32_t observed_schema_version{1U};
    RuntimeSimulationSaveReviewStatus status{RuntimeSimulationSaveReviewStatus::rejected};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationSaveReviewRow&) const = default;
};

struct RuntimeSimulationDashboardRow {
    std::string metric_id;
    std::uint64_t value{0U};
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationDashboardRow&) const = default;
};

struct RuntimeSimulationManagementRequest {
    std::string simulation_id;
    std::uint64_t world_tick{0U};
    std::uint64_t long_run_tick_count{0U};
    std::vector<RuntimeSimulationResourceRow> resource_rows;
    std::vector<RuntimeSimulationJobRow> job_rows;
    std::vector<RuntimeSimulationLogisticsLink> logistics_links;
    std::vector<RuntimeSimulationEconomySummary> economy_summaries;
    std::vector<RuntimeSimulationPopulationNeedRow> population_need_rows;
    std::vector<RuntimeSimulationScheduleRow> schedule_rows;
    std::vector<RuntimeSimulationSaveReviewRow> save_review_rows;
    std::vector<std::string> game_balance_rule_ids;
    std::size_t row_budget{512U};
    std::uint64_t seed{0U};
};

struct RuntimeSimulationDiagnostic {
    RuntimeSimulationDiagnosticCode code{RuntimeSimulationDiagnosticCode::missing_simulation_id};
    std::string simulation_id;
    std::string row_id;
    std::string secondary_id;
    std::string message;
    std::uint32_t source_index{0U};

    [[nodiscard]] bool operator==(const RuntimeSimulationDiagnostic&) const = default;
};

struct RuntimeSimulationManagementPlan {
    RuntimeSimulationManagementStatus status{RuntimeSimulationManagementStatus::invalid_request};
    std::vector<RuntimeSimulationDiagnostic> diagnostics;
    std::vector<RuntimeSimulationResourceRow> resource_rows;
    std::vector<RuntimeSimulationResourceBalanceRow> resource_balance_rows;
    std::vector<RuntimeSimulationJobRow> job_rows;
    std::vector<RuntimeSimulationLogisticsLink> logistics_links;
    std::vector<RuntimeSimulationLogisticsTransferRow> logistics_transfer_rows;
    std::vector<RuntimeSimulationEconomySummary> economy_summaries;
    std::vector<RuntimeSimulationPopulationNeedRow> population_need_rows;
    std::vector<RuntimeSimulationScheduleRow> schedule_rows;
    std::vector<RuntimeSimulationSaveReviewRow> save_review_rows;
    std::vector<RuntimeSimulationDashboardRow> dashboard_rows;
    std::uint64_t tick_count{0U};
    std::size_t resource_balance_count{0U};
    std::size_t job_assignment_count{0U};
    std::size_t logistics_transfer_count{0U};
    std::size_t scheduled_logistics_transfer_count{0U};
    std::size_t need_deficit_count{0U};
    std::size_t save_review_count{0U};
    std::size_t repairable_save_review_count{0U};
    std::size_t dashboard_row_count{0U};
    std::uint64_t replay_hash{0U};
    bool invoked_economy_execution{false};
    bool invoked_save_io{false};
    bool invoked_runtime_ui{false};
    bool invoked_package_io{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Reviews reusable simulation-management value rows and emits deterministic long-run package-evidence rows.
/// This value-only planner does not execute economy/jobs/logistics, read or write saves, render UI dashboards,
/// mutate packages/worlds/scenes, create threads, call renderer/platform/editor APIs, or own game balance/content.
[[nodiscard]] RuntimeSimulationManagementPlan
plan_runtime_simulation_management(const RuntimeSimulationManagementRequest& request);

} // namespace mirakana::runtime
