// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/genre_simulation_management.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

[[nodiscard]] bool is_valid_id(std::string_view id) {
    return !id.empty() && std::ranges::none_of(id, [](char ch) {
        const auto value = static_cast<unsigned char>(ch);
        return value < 0x20U || ch == '\\' || ch == '/' || ch == ':';
    });
}

[[nodiscard]] bool is_token_char(char ch) noexcept {
    const auto value = static_cast<unsigned char>(ch);
    return (value >= static_cast<unsigned char>('a') && value <= static_cast<unsigned char>('z')) ||
           (value >= static_cast<unsigned char>('A') && value <= static_cast<unsigned char>('Z')) ||
           (value >= static_cast<unsigned char>('0') && value <= static_cast<unsigned char>('9')) || ch == '_';
}

[[nodiscard]] char lower_ascii(char ch) noexcept {
    if (ch >= 'A' && ch <= 'Z') {
        return static_cast<char>(ch - 'A' + 'a');
    }
    return ch;
}

[[nodiscard]] bool is_forbidden_backend_token(std::string_view token) {
    constexpr auto forbidden = std::array<std::string_view, 11U>{
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "sdl", "sdl3", "imgui", "gpu", "package",
    };
    return std::ranges::find(forbidden, token) != forbidden.end();
}

[[nodiscard]] bool has_backend_reference(std::string_view value) {
    std::string token;
    for (const auto ch : value) {
        if (is_token_char(ch)) {
            token.push_back(lower_ascii(ch));
            continue;
        }
        if (is_forbidden_backend_token(token)) {
            return true;
        }
        token.clear();
    }
    return is_forbidden_backend_token(token);
}

[[nodiscard]] std::string make_pair_key(std::string_view first, std::string_view second) {
    std::string key;
    key.reserve(first.size() + second.size() + 1U);
    key.append(first);
    key.push_back('\n');
    key.append(second);
    return key;
}

[[nodiscard]] bool contains_value(const std::vector<std::string>& values, std::string_view value) {
    return std::ranges::any_of(values, [value](const auto& candidate) { return candidate == value; });
}

void add_diagnostic(RuntimeSimulationManagementPlan& plan, RuntimeSimulationDiagnosticCode code,
                    const RuntimeSimulationManagementRequest& request, std::string row_id, std::string secondary_id,
                    std::string message, std::uint32_t source_index) {
    plan.diagnostics.push_back(RuntimeSimulationDiagnostic{
        .code = code,
        .simulation_id = request.simulation_id,
        .row_id = std::move(row_id),
        .secondary_id = std::move(secondary_id),
        .message = std::move(message),
        .source_index = source_index,
    });
}

void sort_diagnostics(RuntimeSimulationManagementPlan& plan) {
    std::ranges::sort(plan.diagnostics, [](const auto& lhs, const auto& rhs) {
        if (lhs.code != rhs.code) {
            return static_cast<std::uint8_t>(lhs.code) < static_cast<std::uint8_t>(rhs.code);
        }
        if (lhs.simulation_id != rhs.simulation_id) {
            return lhs.simulation_id < rhs.simulation_id;
        }
        if (lhs.row_id != rhs.row_id) {
            return lhs.row_id < rhs.row_id;
        }
        if (lhs.secondary_id != rhs.secondary_id) {
            return lhs.secondary_id < rhs.secondary_id;
        }
        if (lhs.source_index != rhs.source_index) {
            return lhs.source_index < rhs.source_index;
        }
        return lhs.message < rhs.message;
    });
}

[[nodiscard]] std::size_t request_row_count(const RuntimeSimulationManagementRequest& request) {
    return request.resource_rows.size() + request.job_rows.size() + request.logistics_links.size() +
           request.economy_summaries.size() + request.population_need_rows.size() + request.schedule_rows.size() +
           request.save_review_rows.size() + request.game_balance_rule_ids.size();
}

[[nodiscard]] std::size_t output_row_count(const RuntimeSimulationManagementPlan& plan) {
    return plan.resource_rows.size() + plan.resource_balance_rows.size() + plan.job_rows.size() +
           plan.logistics_links.size() + plan.logistics_transfer_rows.size() + plan.economy_summaries.size() +
           plan.population_need_rows.size() + plan.schedule_rows.size() + plan.save_review_rows.size() +
           plan.dashboard_rows.size();
}

void clear_output_rows(RuntimeSimulationManagementPlan& plan) {
    plan.resource_rows.clear();
    plan.resource_balance_rows.clear();
    plan.job_rows.clear();
    plan.logistics_links.clear();
    plan.logistics_transfer_rows.clear();
    plan.economy_summaries.clear();
    plan.population_need_rows.clear();
    plan.schedule_rows.clear();
    plan.save_review_rows.clear();
    plan.dashboard_rows.clear();
    plan.tick_count = 0U;
    plan.resource_balance_count = 0U;
    plan.job_assignment_count = 0U;
    plan.logistics_transfer_count = 0U;
    plan.scheduled_logistics_transfer_count = 0U;
    plan.need_deficit_count = 0U;
    plan.save_review_count = 0U;
    plan.repairable_save_review_count = 0U;
    plan.dashboard_row_count = 0U;
    plan.replay_hash = 0U;
}

[[nodiscard]] bool resource_row_has_backend_reference(const RuntimeSimulationResourceRow& row) {
    return has_backend_reference(row.resource_id) || has_backend_reference(row.storage_id);
}

[[nodiscard]] bool valid_resource_ids(const RuntimeSimulationResourceRow& row) {
    return is_valid_id(row.resource_id) && is_valid_id(row.storage_id) && !resource_row_has_backend_reference(row);
}

[[nodiscard]] std::string resource_key(std::string_view resource_id, std::string_view storage_id) {
    return make_pair_key(resource_id, storage_id);
}

[[nodiscard]] bool resource_exists(const std::vector<std::string>& resource_keys, std::string_view resource_id,
                                   std::string_view storage_id) {
    return contains_value(resource_keys, resource_key(resource_id, storage_id));
}

[[nodiscard]] bool resource_id_exists(const std::vector<std::string>& resource_keys, std::string_view resource_id) {
    std::string prefix;
    prefix.reserve(resource_id.size() + 1U);
    prefix.append(resource_id);
    prefix.push_back('\n');
    return std::ranges::any_of(resource_keys, [&prefix](const auto& key) { return key.starts_with(prefix); });
}

[[nodiscard]] bool diagnostics_contain(const RuntimeSimulationManagementPlan& plan,
                                       RuntimeSimulationDiagnosticCode code) {
    return std::ranges::any_of(plan.diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

void enforce_diagnostic_row_budget(RuntimeSimulationManagementPlan& plan,
                                   const RuntimeSimulationManagementRequest& request) {
    if (plan.diagnostics.size() > request.row_budget &&
        !diagnostics_contain(plan, RuntimeSimulationDiagnosticCode::row_budget_exceeded)) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::row_budget_exceeded, request, request.simulation_id, {},
                       "runtime simulation management diagnostics exceed the review row budget", 0U);
    }
}

[[nodiscard]] const RuntimeSimulationResourceRow* find_resource(const std::vector<RuntimeSimulationResourceRow>& rows,
                                                                std::string_view resource_id,
                                                                std::string_view storage_id) {
    const auto iter = std::ranges::find_if(rows, [resource_id, storage_id](const auto& row) {
        return row.resource_id == resource_id && row.storage_id == storage_id;
    });
    if (iter == rows.end()) {
        return nullptr;
    }
    return &(*iter);
}

[[nodiscard]] bool can_add_without_exceeding(std::int64_t base, std::int64_t delta, std::int64_t capacity) noexcept {
    if (base < 0 || delta < 0 || capacity < 0) {
        return false;
    }
    return base <= capacity && delta <= capacity - base;
}

[[nodiscard]] RuntimeSimulationResourceBalanceRow* find_balance(std::vector<RuntimeSimulationResourceBalanceRow>& rows,
                                                                std::string_view resource_id,
                                                                std::string_view storage_id) {
    const auto iter = std::ranges::find_if(rows, [resource_id, storage_id](const auto& row) {
        return row.resource_id == resource_id && row.storage_id == storage_id;
    });
    if (iter == rows.end()) {
        return nullptr;
    }
    return &(*iter);
}

void validate_top_level(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request) {
    if (!is_valid_id(request.simulation_id)) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::missing_simulation_id, request, request.simulation_id, {},
                       "runtime simulation id must be non-empty and path-safe", 0U);
    }
    if (has_backend_reference(request.simulation_id)) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unsupported_backend_reference, request,
                       request.simulation_id, {},
                       "runtime simulation management rows must not encode renderer/platform/backend references", 0U);
    }
    if (request.long_run_tick_count == 0U) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_tick_count, request, request.simulation_id, {},
                       "runtime simulation management long-run tick count must be positive", 0U);
    }
    if (request_row_count(request) > request.row_budget) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::row_budget_exceeded, request, request.simulation_id, {},
                       "runtime simulation management request exceeds its review row budget", 0U);
    }
    for (std::size_t index = 0U; index < request.game_balance_rule_ids.size(); ++index) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unsupported_game_balance_rule, request,
                       request.game_balance_rule_ids[index], {},
                       "economy balance, population content, production recipes, and schedules remain game-owned",
                       static_cast<std::uint32_t>(index));
    }
}

[[nodiscard]] std::vector<std::string> validate_resources(RuntimeSimulationManagementPlan& plan,
                                                          const RuntimeSimulationManagementRequest& request) {
    std::vector<std::string> keys;
    keys.reserve(request.resource_rows.size());
    for (const auto& row : request.resource_rows) {
        auto valid = true;
        if (!valid_resource_ids(row)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_resource_row, request, row.resource_id,
                           row.storage_id, "simulation resource and storage ids must be path-safe and backend-neutral",
                           row.source_index);
            valid = false;
        }
        if (row.capacity <= 0 || row.quantity < 0 || row.quantity > row.capacity) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_resource_row, request, row.resource_id,
                           row.storage_id, "simulation resource quantities must be non-negative and fit capacity",
                           row.source_index);
            valid = false;
        }
        const auto key = resource_key(row.resource_id, row.storage_id);
        if (contains_value(keys, key)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_resource, request, row.resource_id,
                           row.storage_id, "simulation resource rows must be unique per resource and storage",
                           row.source_index);
        } else if (valid) {
            keys.push_back(key);
        }
    }
    return keys;
}

void validate_jobs(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request,
                   const std::vector<std::string>& resource_keys) {
    std::vector<std::string> job_ids;
    job_ids.reserve(request.job_rows.size());
    for (const auto& row : request.job_rows) {
        auto syntactically_valid = true;
        if (!is_valid_id(row.job_id) || !is_valid_id(row.worker_id) || !is_valid_id(row.input_resource_id) ||
            !is_valid_id(row.input_storage_id) || !is_valid_id(row.output_resource_id) ||
            !is_valid_id(row.output_storage_id) || has_backend_reference(row.job_id) ||
            has_backend_reference(row.worker_id) || has_backend_reference(row.input_resource_id) ||
            has_backend_reference(row.input_storage_id) || has_backend_reference(row.output_resource_id) ||
            has_backend_reference(row.output_storage_id) || row.input_quantity <= 0 || row.output_quantity <= 0 ||
            row.duration_ticks == 0U) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_job_row, request, row.job_id, row.worker_id,
                           "simulation jobs require backend-neutral ids and positive input/output/duration",
                           row.source_index);
            syntactically_valid = false;
        }
        if (contains_value(job_ids, row.job_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_job, request, row.job_id, row.worker_id,
                           "simulation job ids must be unique", row.source_index);
        } else if (syntactically_valid) {
            job_ids.push_back(row.job_id);
        }
        if (!syntactically_valid) {
            continue;
        }
        if (!resource_exists(resource_keys, row.input_resource_id, row.input_storage_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_job_resource, request, row.job_id,
                           row.input_resource_id, "simulation jobs must reference known input resource/storage rows",
                           row.source_index);
        } else if (!resource_exists(resource_keys, row.output_resource_id, row.output_storage_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_job_resource, request, row.job_id,
                           row.output_resource_id, "simulation jobs must reference known output resource/storage rows",
                           row.source_index);
        }
    }
}

void validate_logistics(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request,
                        const std::vector<std::string>& resource_keys) {
    std::vector<std::string> link_ids;
    link_ids.reserve(request.logistics_links.size());
    for (const auto& row : request.logistics_links) {
        auto syntactically_valid = true;
        const auto travel_overflows = static_cast<std::uint64_t>(row.travel_ticks) >
                                      std::numeric_limits<std::uint64_t>::max() - request.world_tick;
        if (!is_valid_id(row.link_id) || !is_valid_id(row.resource_id) || !is_valid_id(row.source_storage_id) ||
            !is_valid_id(row.destination_storage_id) || has_backend_reference(row.link_id) ||
            has_backend_reference(row.resource_id) || has_backend_reference(row.source_storage_id) ||
            has_backend_reference(row.destination_storage_id) || row.transfer_quantity <= 0 || row.travel_ticks == 0U ||
            travel_overflows) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_logistics_link, request, row.link_id,
                           row.resource_id,
                           "simulation logistics links require backend-neutral ids and positive quantity/travel ticks",
                           row.source_index);
            syntactically_valid = false;
        }
        if (contains_value(link_ids, row.link_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_logistics_link, request, row.link_id,
                           row.resource_id, "simulation logistics link ids must be unique", row.source_index);
        } else if (syntactically_valid) {
            link_ids.push_back(row.link_id);
        }
        if (!syntactically_valid) {
            continue;
        }
        const auto source_known = resource_exists(resource_keys, row.resource_id, row.source_storage_id);
        const auto destination_known = resource_exists(resource_keys, row.resource_id, row.destination_storage_id);
        if (!source_known) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_logistics_resource, request, row.link_id,
                           row.source_storage_id,
                           "simulation logistics links must reference a known source resource/storage row",
                           row.source_index);
        }
        if (!destination_known) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_logistics_resource, request, row.link_id,
                           row.destination_storage_id,
                           "simulation logistics links must reference a known destination resource/storage row",
                           row.source_index);
        }
    }
}

void validate_economy(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request,
                      const std::vector<std::string>& resource_keys) {
    std::vector<std::string> summary_ids;
    summary_ids.reserve(request.economy_summaries.size());
    for (const auto& row : request.economy_summaries) {
        auto syntactically_valid = true;
        if (!is_valid_id(row.summary_id) || !is_valid_id(row.resource_id) || has_backend_reference(row.summary_id) ||
            has_backend_reference(row.resource_id) || row.produced_quantity < 0 || row.consumed_quantity < 0 ||
            row.traded_quantity < 0) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_economy_summary, request, row.summary_id,
                           row.resource_id,
                           "simulation economy summary rows require backend-neutral ids and non-negative quantities",
                           row.source_index);
            syntactically_valid = false;
        }
        if (contains_value(summary_ids, row.summary_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_economy_summary, request, row.summary_id,
                           row.resource_id, "simulation economy summary ids must be unique", row.source_index);
        } else if (syntactically_valid) {
            summary_ids.push_back(row.summary_id);
        }
        if (!syntactically_valid) {
            continue;
        }
        if (!resource_id_exists(resource_keys, row.resource_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_economy_resource, request, row.summary_id,
                           row.resource_id, "simulation economy summaries must reference a known resource id",
                           row.source_index);
        }
    }
}

void validate_population_needs(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request,
                               const std::vector<std::string>& resource_keys) {
    std::vector<std::string> keys;
    keys.reserve(request.population_need_rows.size());
    for (const auto& row : request.population_need_rows) {
        auto syntactically_valid = true;
        if (!is_valid_id(row.population_id) || !is_valid_id(row.need_id) || !is_valid_id(row.resource_id) ||
            !is_valid_id(row.storage_id) || has_backend_reference(row.population_id) ||
            has_backend_reference(row.need_id) || has_backend_reference(row.resource_id) ||
            has_backend_reference(row.storage_id) || row.required_quantity <= 0) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_population_need_row, request,
                           row.population_id, row.need_id,
                           "simulation population needs require backend-neutral ids and positive required quantity",
                           row.source_index);
            syntactically_valid = false;
        }
        const auto key = make_pair_key(row.population_id, row.need_id);
        if (contains_value(keys, key)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_population_need, request, row.population_id,
                           row.need_id, "simulation population need rows must be unique per population and need",
                           row.source_index);
        } else if (syntactically_valid) {
            keys.push_back(key);
        }
        if (!syntactically_valid) {
            continue;
        }
        if (!resource_exists(resource_keys, row.resource_id, row.storage_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::unknown_population_need_resource, request,
                           row.population_id, row.need_id,
                           "simulation population needs must reference a known resource/storage row", row.source_index);
        }
    }
}

void validate_schedules(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request) {
    std::vector<std::string> ids;
    ids.reserve(request.schedule_rows.size());
    for (const auto& row : request.schedule_rows) {
        if (!is_valid_id(row.schedule_id) || !is_valid_id(row.target_id) || has_backend_reference(row.schedule_id) ||
            has_backend_reference(row.target_id) || row.end_tick <= row.start_tick) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_schedule_row, request, row.schedule_id,
                           row.target_id, "simulation schedule rows require backend-neutral ids and increasing ticks",
                           row.source_index);
        }
        if (contains_value(ids, row.schedule_id)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_schedule, request, row.schedule_id,
                           row.target_id, "simulation schedule ids must be unique", row.source_index);
        } else {
            ids.push_back(row.schedule_id);
        }
    }
}

void validate_save_reviews(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request) {
    std::vector<std::string> keys;
    keys.reserve(request.save_review_rows.size());
    for (const auto& row : request.save_review_rows) {
        if (!is_valid_id(row.domain) || !is_valid_id(row.key) || has_backend_reference(row.domain) ||
            has_backend_reference(row.key) || row.expected_schema_version == 0U || row.observed_schema_version == 0U) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::invalid_save_review_row, request, row.domain, row.key,
                           "simulation save review rows require path-safe ids and schema versions", row.source_index);
        }
        const auto key = make_pair_key(row.domain, row.key);
        if (contains_value(keys, key)) {
            add_diagnostic(plan, RuntimeSimulationDiagnosticCode::duplicate_save_review_key, request, row.domain,
                           row.key, "simulation save review rows must be unique per domain and key", row.source_index);
        } else {
            keys.push_back(key);
        }
    }
}

[[nodiscard]] bool target_enabled_by_schedule(const std::vector<RuntimeSimulationScheduleRow>& schedules,
                                              std::string_view target_id, std::uint64_t world_tick) {
    const auto iter = std::ranges::find_if(schedules, [target_id, world_tick](const auto& row) {
        return row.target_id == target_id && row.enabled && row.start_tick <= world_tick && row.end_tick >= world_tick;
    });
    return iter != schedules.end();
}

[[nodiscard]] RuntimeSimulationJobStatus job_status_for(const RuntimeSimulationJobRow& row,
                                                        const RuntimeSimulationManagementRequest& request,
                                                        std::vector<RuntimeSimulationResourceBalanceRow>& balances) {
    auto* input = find_balance(balances, row.input_resource_id, row.input_storage_id);
    if (input == nullptr || input->projected_quantity < row.input_quantity) {
        return RuntimeSimulationJobStatus::blocked_missing_input;
    }
    auto* output = find_balance(balances, row.output_resource_id, row.output_storage_id);
    if (output == nullptr) {
        return RuntimeSimulationJobStatus::blocked_missing_output;
    }
    const auto output_base =
        input == output ? output->projected_quantity - row.input_quantity : output->projected_quantity;
    if (!can_add_without_exceeding(output_base, row.output_quantity, output->capacity)) {
        return RuntimeSimulationJobStatus::blocked_missing_output;
    }
    if (!target_enabled_by_schedule(request.schedule_rows, row.job_id, request.world_tick)) {
        return RuntimeSimulationJobStatus::inactive_schedule;
    }
    return RuntimeSimulationJobStatus::assigned;
}

[[nodiscard]] RuntimeSimulationLogisticsTransferStatus
transfer_status_for(const RuntimeSimulationLogisticsLink& row, const RuntimeSimulationManagementRequest& request,
                    std::vector<RuntimeSimulationResourceBalanceRow>& balances) {
    auto* source = find_balance(balances, row.resource_id, row.source_storage_id);
    if (source == nullptr) {
        return RuntimeSimulationLogisticsTransferStatus::blocked_missing_source;
    }
    auto* destination = find_balance(balances, row.resource_id, row.destination_storage_id);
    if (destination == nullptr) {
        return RuntimeSimulationLogisticsTransferStatus::blocked_missing_destination;
    }
    if (source->projected_quantity < row.transfer_quantity) {
        return RuntimeSimulationLogisticsTransferStatus::blocked_missing_source;
    }
    const auto destination_base = source == destination ? destination->projected_quantity - row.transfer_quantity
                                                        : destination->projected_quantity;
    if (!can_add_without_exceeding(destination_base, row.transfer_quantity, destination->capacity)) {
        return RuntimeSimulationLogisticsTransferStatus::blocked_missing_destination;
    }
    if (!row.enabled || !target_enabled_by_schedule(request.schedule_rows, row.link_id, request.world_tick)) {
        return RuntimeSimulationLogisticsTransferStatus::disabled;
    }
    return RuntimeSimulationLogisticsTransferStatus::scheduled;
}

void append_output_rows(RuntimeSimulationManagementPlan& plan, const RuntimeSimulationManagementRequest& request) {
    plan.tick_count = request.long_run_tick_count;
    plan.resource_rows = request.resource_rows;
    plan.job_rows = request.job_rows;
    plan.logistics_links = request.logistics_links;
    plan.economy_summaries = request.economy_summaries;
    plan.population_need_rows = request.population_need_rows;
    plan.schedule_rows = request.schedule_rows;
    plan.save_review_rows = request.save_review_rows;

    std::ranges::sort(plan.resource_rows, [](const auto& lhs, const auto& rhs) {
        if (lhs.resource_id != rhs.resource_id) {
            return lhs.resource_id < rhs.resource_id;
        }
        return lhs.storage_id < rhs.storage_id;
    });
    for (const auto& row : plan.resource_rows) {
        plan.resource_balance_rows.push_back(RuntimeSimulationResourceBalanceRow{
            .resource_id = row.resource_id,
            .storage_id = row.storage_id,
            .starting_quantity = row.quantity,
            .projected_quantity = row.quantity,
            .capacity = row.capacity,
            .job_input_quantity = 0,
            .job_output_quantity = 0,
            .logistics_outgoing_quantity = 0,
            .logistics_incoming_quantity = 0,
            .source_index = row.source_index,
        });
    }

    for (auto& row : plan.job_rows) {
        row.status = job_status_for(row, request, plan.resource_balance_rows);
        if (row.status == RuntimeSimulationJobStatus::assigned) {
            if (auto* input = find_balance(plan.resource_balance_rows, row.input_resource_id, row.input_storage_id);
                input != nullptr) {
                input->projected_quantity -= row.input_quantity;
                input->job_input_quantity += row.input_quantity;
            }
            if (auto* output = find_balance(plan.resource_balance_rows, row.output_resource_id, row.output_storage_id);
                output != nullptr) {
                output->projected_quantity += row.output_quantity;
                output->job_output_quantity += row.output_quantity;
            }
        }
    }

    for (const auto& row : plan.logistics_links) {
        const auto status = transfer_status_for(row, request, plan.resource_balance_rows);
        plan.logistics_transfer_rows.push_back(RuntimeSimulationLogisticsTransferRow{
            .link_id = row.link_id,
            .resource_id = row.resource_id,
            .source_storage_id = row.source_storage_id,
            .destination_storage_id = row.destination_storage_id,
            .transfer_quantity = row.transfer_quantity,
            .arrival_tick = request.world_tick + row.travel_ticks,
            .status = status,
            .source_index = row.source_index,
        });
        if (status == RuntimeSimulationLogisticsTransferStatus::scheduled) {
            if (auto* source = find_balance(plan.resource_balance_rows, row.resource_id, row.source_storage_id);
                source != nullptr) {
                source->projected_quantity -= row.transfer_quantity;
                source->logistics_outgoing_quantity += row.transfer_quantity;
            }
            if (auto* destination =
                    find_balance(plan.resource_balance_rows, row.resource_id, row.destination_storage_id);
                destination != nullptr) {
                destination->projected_quantity += row.transfer_quantity;
                destination->logistics_incoming_quantity += row.transfer_quantity;
            }
        }
    }

    for (auto& row : plan.population_need_rows) {
        const auto* resource = find_resource(request.resource_rows, row.resource_id, row.storage_id);
        row.available_quantity = resource != nullptr ? resource->quantity : 0;
        row.status = row.available_quantity >= row.required_quantity ? RuntimeSimulationNeedStatus::satisfied
                                                                     : RuntimeSimulationNeedStatus::deficit;
    }

    for (auto& row : plan.save_review_rows) {
        if (row.observed_schema_version == row.expected_schema_version) {
            row.status = RuntimeSimulationSaveReviewStatus::accepted;
        } else if (row.observed_schema_version < row.expected_schema_version) {
            row.status = RuntimeSimulationSaveReviewStatus::repairable;
        } else {
            row.status = RuntimeSimulationSaveReviewStatus::rejected;
        }
    }
}

void update_counts(RuntimeSimulationManagementPlan& plan) {
    plan.resource_balance_count = plan.resource_balance_rows.size();
    plan.logistics_transfer_count = plan.logistics_transfer_rows.size();
    plan.save_review_count = plan.save_review_rows.size();
    for (const auto& row : plan.job_rows) {
        if (row.status == RuntimeSimulationJobStatus::assigned) {
            ++plan.job_assignment_count;
        }
    }
    for (const auto& row : plan.logistics_transfer_rows) {
        if (row.status == RuntimeSimulationLogisticsTransferStatus::scheduled) {
            ++plan.scheduled_logistics_transfer_count;
        }
    }
    for (const auto& row : plan.population_need_rows) {
        if (row.status == RuntimeSimulationNeedStatus::deficit) {
            ++plan.need_deficit_count;
        }
    }
    for (const auto& row : plan.save_review_rows) {
        if (row.status == RuntimeSimulationSaveReviewStatus::repairable) {
            ++plan.repairable_save_review_count;
        }
    }
}

void append_dashboard_rows(RuntimeSimulationManagementPlan& plan) {
    plan.dashboard_rows = {
        RuntimeSimulationDashboardRow{
            .metric_id = "simulation_management.tick_count", .value = plan.tick_count, .source_index = 1U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.resource_balance_rows",
                                      .value = static_cast<std::uint64_t>(plan.resource_balance_count),
                                      .source_index = 2U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.job_assignment_rows",
                                      .value = static_cast<std::uint64_t>(plan.job_assignment_count),
                                      .source_index = 3U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.logistics_transfer_rows",
                                      .value = static_cast<std::uint64_t>(plan.logistics_transfer_count),
                                      .source_index = 4U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.need_deficit_rows",
                                      .value = static_cast<std::uint64_t>(plan.need_deficit_count),
                                      .source_index = 5U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.save_review_rows",
                                      .value = static_cast<std::uint64_t>(plan.save_review_count),
                                      .source_index = 6U},
        RuntimeSimulationDashboardRow{.metric_id = "simulation_management.repairable_save_review_rows",
                                      .value = static_cast<std::uint64_t>(plan.repairable_save_review_count),
                                      .source_index = 7U},
    };
    plan.dashboard_row_count = plan.dashboard_rows.size();
}

void mix_hash(std::uint64_t& hash, std::uint64_t value) noexcept {
    hash ^= value + 0x9e3779b97f4a7c15ULL + (hash << 6U) + (hash >> 2U);
}

void mix_hash(std::uint64_t& hash, std::uint32_t value) noexcept {
    mix_hash(hash, static_cast<std::uint64_t>(value));
}

void mix_hash(std::uint64_t& hash, std::string_view value) noexcept {
    for (const auto ch : value) {
        mix_hash(hash, static_cast<std::uint64_t>(static_cast<unsigned char>(ch)));
    }
    mix_hash(hash, 0xffU);
}

void mix_hash(std::uint64_t& hash, std::int64_t value) noexcept {
    mix_hash(hash, static_cast<std::uint64_t>(value));
}

[[nodiscard]] std::uint64_t compute_replay_hash(const RuntimeSimulationManagementRequest& request,
                                                const RuntimeSimulationManagementPlan& plan) {
    auto hash = std::uint64_t{1469598103934665603ULL};
    mix_hash(hash, request.simulation_id);
    mix_hash(hash, request.world_tick);
    mix_hash(hash, request.long_run_tick_count);
    mix_hash(hash, request.seed);
    mix_hash(hash, static_cast<std::uint64_t>(plan.status));
    mix_hash(hash, plan.tick_count);
    mix_hash(hash, static_cast<std::uint64_t>(plan.resource_balance_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.job_assignment_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.logistics_transfer_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.scheduled_logistics_transfer_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.need_deficit_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.save_review_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.repairable_save_review_count));
    mix_hash(hash, static_cast<std::uint64_t>(plan.dashboard_row_count));
    for (const auto& row : plan.resource_rows) {
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.storage_id);
        mix_hash(hash, row.quantity);
        mix_hash(hash, row.capacity);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.resource_balance_rows) {
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.storage_id);
        mix_hash(hash, row.starting_quantity);
        mix_hash(hash, row.projected_quantity);
        mix_hash(hash, row.capacity);
        mix_hash(hash, row.job_input_quantity);
        mix_hash(hash, row.job_output_quantity);
        mix_hash(hash, row.logistics_outgoing_quantity);
        mix_hash(hash, row.logistics_incoming_quantity);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.job_rows) {
        mix_hash(hash, row.job_id);
        mix_hash(hash, row.worker_id);
        mix_hash(hash, row.input_resource_id);
        mix_hash(hash, row.input_storage_id);
        mix_hash(hash, row.output_resource_id);
        mix_hash(hash, row.output_storage_id);
        mix_hash(hash, row.input_quantity);
        mix_hash(hash, row.output_quantity);
        mix_hash(hash, row.duration_ticks);
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.logistics_links) {
        mix_hash(hash, row.link_id);
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.source_storage_id);
        mix_hash(hash, row.destination_storage_id);
        mix_hash(hash, row.transfer_quantity);
        mix_hash(hash, row.travel_ticks);
        mix_hash(hash, row.enabled ? 1U : 0U);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.logistics_transfer_rows) {
        mix_hash(hash, row.link_id);
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.source_storage_id);
        mix_hash(hash, row.destination_storage_id);
        mix_hash(hash, row.transfer_quantity);
        mix_hash(hash, row.arrival_tick);
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.economy_summaries) {
        mix_hash(hash, row.summary_id);
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.produced_quantity);
        mix_hash(hash, row.consumed_quantity);
        mix_hash(hash, row.traded_quantity);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.population_need_rows) {
        mix_hash(hash, row.population_id);
        mix_hash(hash, row.need_id);
        mix_hash(hash, row.resource_id);
        mix_hash(hash, row.storage_id);
        mix_hash(hash, row.required_quantity);
        mix_hash(hash, row.available_quantity);
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.schedule_rows) {
        mix_hash(hash, row.schedule_id);
        mix_hash(hash, row.target_id);
        mix_hash(hash, row.start_tick);
        mix_hash(hash, row.end_tick);
        mix_hash(hash, row.enabled ? 1U : 0U);
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.save_review_rows) {
        mix_hash(hash, row.domain);
        mix_hash(hash, row.key);
        mix_hash(hash, row.expected_schema_version);
        mix_hash(hash, row.observed_schema_version);
        mix_hash(hash, static_cast<std::uint64_t>(row.status));
        mix_hash(hash, row.source_index);
    }
    for (const auto& row : plan.dashboard_rows) {
        mix_hash(hash, row.metric_id);
        mix_hash(hash, row.value);
        mix_hash(hash, row.source_index);
    }
    mix_hash(hash, plan.invoked_economy_execution ? 1U : 0U);
    mix_hash(hash, plan.invoked_save_io ? 1U : 0U);
    mix_hash(hash, plan.invoked_runtime_ui ? 1U : 0U);
    mix_hash(hash, plan.invoked_package_io ? 1U : 0U);
    return hash == 0U ? 1U : hash;
}

} // namespace

bool RuntimeSimulationManagementPlan::succeeded() const noexcept {
    return status == RuntimeSimulationManagementStatus::ready || status == RuntimeSimulationManagementStatus::no_rows;
}

RuntimeSimulationManagementPlan plan_runtime_simulation_management(const RuntimeSimulationManagementRequest& request) {
    RuntimeSimulationManagementPlan plan;

    validate_top_level(plan, request);
    const auto resource_keys = validate_resources(plan, request);
    validate_jobs(plan, request, resource_keys);
    validate_logistics(plan, request, resource_keys);
    validate_economy(plan, request, resource_keys);
    validate_population_needs(plan, request, resource_keys);
    validate_schedules(plan, request);
    validate_save_reviews(plan, request);
    enforce_diagnostic_row_budget(plan, request);

    if (!plan.diagnostics.empty()) {
        sort_diagnostics(plan);
        plan.status = RuntimeSimulationManagementStatus::invalid_request;
        clear_output_rows(plan);
        return plan;
    }

    if (request_row_count(request) == 0U) {
        plan.status = RuntimeSimulationManagementStatus::no_rows;
        plan.tick_count = request.long_run_tick_count;
        plan.replay_hash = compute_replay_hash(request, plan);
        return plan;
    }

    append_output_rows(plan, request);
    update_counts(plan);
    append_dashboard_rows(plan);
    plan.status = RuntimeSimulationManagementStatus::ready;

    if (output_row_count(plan) > request.row_budget) {
        add_diagnostic(plan, RuntimeSimulationDiagnosticCode::row_budget_exceeded, request, request.simulation_id, {},
                       "runtime simulation management output rows exceed the review row budget", 0U);
        sort_diagnostics(plan);
        plan.status = RuntimeSimulationManagementStatus::invalid_request;
        clear_output_rows(plan);
        return plan;
    }

    plan.replay_hash = compute_replay_hash(request, plan);
    return plan;
}

} // namespace mirakana::runtime
