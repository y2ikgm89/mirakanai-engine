// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/platform/win32/win32_cpu_sets.hpp"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::win32 {
namespace {

struct OrderedCpuSetRow {
    Win32CpuSetRow row;
    bool smt_sibling_deferred{false};
};

[[nodiscard]] bool cpu_set_available(const Win32CpuSetRow& row) noexcept {
    return !row.parked && (!row.allocated || row.allocated_to_target_process);
}

void append_diagnostic(Win32CpuSetWorkerPlacementPlan& plan, Win32CpuSetWorkerPlacementDiagnosticCode code,
                       std::string message) {
    plan.diagnostic_codes.push_back(code);
    plan.diagnostics.push_back(std::move(message));
}

void append_core_diagnostic(JobExecutionWorkerPlacementResult& result, JobExecutionPlacementPolicyDiagnosticCode code,
                            std::string message) {
    result.diagnostic_codes.push_back(code);
    result.diagnostics.push_back(std::move(message));
}

[[nodiscard]] std::string win32_last_error_message(std::string_view prefix, DWORD error = GetLastError()) {
    LPSTR buffer = nullptr;
    const auto chars = FormatMessageA(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), reinterpret_cast<LPSTR>(&buffer), 0, nullptr);

    std::string message(prefix);
    message.append(" failed with error ");
    message.append(std::to_string(error));
    if (chars != 0 && buffer != nullptr) {
        message.append(": ");
        message.append(buffer, chars);
        LocalFree(buffer);
    }
    return message;
}

void sort_cpu_sets_for_mode(std::vector<Win32CpuSetRow>& rows, JobExecutionPlacementPolicyMode mode) {
    std::ranges::sort(rows, [mode](const Win32CpuSetRow& lhs, const Win32CpuSetRow& rhs) {
        if (mode == JobExecutionPlacementPolicyMode::prefer_performance_cores &&
            lhs.efficiency_class != rhs.efficiency_class) {
            return lhs.efficiency_class > rhs.efficiency_class;
        }
        if (mode == JobExecutionPlacementPolicyMode::prefer_efficiency_cores &&
            lhs.efficiency_class != rhs.efficiency_class) {
            return lhs.efficiency_class < rhs.efficiency_class;
        }
        if (lhs.group != rhs.group) {
            return lhs.group < rhs.group;
        }
        if (lhs.core_index != rhs.core_index) {
            return lhs.core_index < rhs.core_index;
        }
        if (lhs.logical_processor_index != rhs.logical_processor_index) {
            return lhs.logical_processor_index < rhs.logical_processor_index;
        }
        return lhs.id < rhs.id;
    });
}

[[nodiscard]] bool same_core(const Win32CpuSetRow& lhs, const Win32CpuSetRow& rhs) noexcept {
    return lhs.group == rhs.group && lhs.core_index == rhs.core_index;
}

[[nodiscard]] bool core_already_selected(std::span<const OrderedCpuSetRow> rows,
                                         const Win32CpuSetRow& candidate) noexcept {
    return std::ranges::any_of(rows,
                               [&candidate](const OrderedCpuSetRow& row) { return same_core(row.row, candidate); });
}

[[nodiscard]] auto build_ordered_cpu_set_rows(std::vector<Win32CpuSetRow> rows, JobExecutionPlacementPolicyMode mode)
    -> std::vector<OrderedCpuSetRow> {
    sort_cpu_sets_for_mode(rows, mode);
    std::vector<OrderedCpuSetRow> ordered_rows;
    ordered_rows.reserve(rows.size());

    if (mode != JobExecutionPlacementPolicyMode::avoid_smt_siblings) {
        for (const auto& row : rows) {
            ordered_rows.push_back(OrderedCpuSetRow{.row = row});
        }
        return ordered_rows;
    }

    std::vector<OrderedCpuSetRow> sibling_rows;
    sibling_rows.reserve(rows.size());
    for (const auto& row : rows) {
        if (core_already_selected(ordered_rows, row)) {
            sibling_rows.push_back(OrderedCpuSetRow{.row = row, .smt_sibling_deferred = true});
        } else {
            ordered_rows.push_back(OrderedCpuSetRow{.row = row});
        }
    }
    ordered_rows.insert(ordered_rows.end(), sibling_rows.begin(), sibling_rows.end());
    return ordered_rows;
}

[[nodiscard]] std::uint32_t count_distinct_cores(std::span<const OrderedCpuSetRow> rows) noexcept {
    std::vector<OrderedCpuSetRow> distinct_rows;
    distinct_rows.reserve(rows.size());
    for (const auto& row : rows) {
        if (!core_already_selected(distinct_rows, row.row)) {
            distinct_rows.push_back(row);
        }
    }
    return static_cast<std::uint32_t>(distinct_rows.size());
}

[[nodiscard]] auto find_worker_row(const Win32CpuSetWorkerPlacementPlan& plan, std::uint32_t worker_id) noexcept
    -> const Win32CpuSetWorkerPlacementRow* {
    const auto row =
        std::ranges::find_if(plan.worker_rows, [worker_id](const Win32CpuSetWorkerPlacementRow& candidate) {
            return candidate.worker_id == worker_id;
        });
    if (row == plan.worker_rows.end()) {
        return nullptr;
    }
    return &*row;
}

[[nodiscard]] JobExecutionWorkerPlacementResult
make_host_evidence_result(const JobExecutionWorkerPlacementRequest& request, std::string message) {
    JobExecutionWorkerPlacementResult result;
    result.status = JobExecutionWorkerPlacementStatus::host_evidence_required;
    result.worker_id = request.worker_id;
    append_core_diagnostic(result, JobExecutionPlacementPolicyDiagnosticCode::host_execution_required,
                           std::move(message));
    return result;
}

[[nodiscard]] JobExecutionWorkerPlacementResult make_failed_result(const JobExecutionWorkerPlacementRequest& request,
                                                                   JobExecutionPlacementPolicyDiagnosticCode code,
                                                                   std::string message) {
    JobExecutionWorkerPlacementResult result;
    result.status = JobExecutionWorkerPlacementStatus::failed;
    result.worker_id = request.worker_id;
    result.attempted = true;
    append_core_diagnostic(result, code, std::move(message));
    return result;
}

} // namespace

Win32CpuSetWorkerPlacementPlan select_win32_cpu_set_worker_placement(const Win32CpuSetWorkerPlacementDesc& desc) {
    Win32CpuSetWorkerPlacementPlan plan;
    if (desc.worker_count == 0) {
        append_diagnostic(plan, Win32CpuSetWorkerPlacementDiagnosticCode::invalid_configuration,
                          "win32 CPU set worker placement requires at least one worker");
        return plan;
    }

    std::vector<Win32CpuSetRow> available_rows;
    available_rows.reserve(desc.cpu_sets.size());
    for (const auto& row : desc.cpu_sets) {
        if (cpu_set_available(row)) {
            available_rows.push_back(row);
        }
    }

    if (available_rows.empty()) {
        plan.status = Win32CpuSetWorkerPlacementStatus::host_evidence_required;
        append_diagnostic(plan, Win32CpuSetWorkerPlacementDiagnosticCode::no_available_cpu_sets,
                          "win32 CPU set worker placement found no available CPU sets");
        return plan;
    }

    const auto ordered_rows = build_ordered_cpu_set_rows(std::move(available_rows), desc.mode);
    plan.status = Win32CpuSetWorkerPlacementStatus::ready;
    plan.selected_cpu_set_count = static_cast<std::uint32_t>(ordered_rows.size());
    plan.distinct_core_count = count_distinct_cores(ordered_rows);
    plan.smt_sibling_cpu_set_count = plan.selected_cpu_set_count - plan.distinct_core_count;
    plan.smt_sibling_topology_known = plan.smt_sibling_cpu_set_count > 0U;
    plan.smt_sibling_policy_applied = desc.mode == JobExecutionPlacementPolicyMode::avoid_smt_siblings;
    plan.worker_rows.reserve(desc.worker_count);

    for (std::uint32_t worker_id = 0; worker_id < desc.worker_count; ++worker_id) {
        const auto& row = ordered_rows[static_cast<std::size_t>(worker_id) % ordered_rows.size()];
        plan.worker_rows.push_back(Win32CpuSetWorkerPlacementRow{
            .worker_id = worker_id,
            .cpu_set_id = row.row.id,
            .efficiency_class = row.row.efficiency_class,
            .core_index = row.row.core_index,
            .numa_node_index = row.row.numa_node_index,
            .smt_sibling_deferred = row.smt_sibling_deferred,
        });
    }

    return plan;
}

std::vector<Win32CpuSetRow> query_win32_cpu_sets() {
    ULONG required_bytes = 0;
    if (GetSystemCpuSetInformation(nullptr, 0, &required_bytes, GetCurrentProcess(), 0) == FALSE) {
        const auto error = GetLastError();
        if (error != ERROR_INSUFFICIENT_BUFFER || required_bytes == 0) {
            return {};
        }
    }

    std::vector<std::byte> buffer(required_bytes);
    if (GetSystemCpuSetInformation(reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data()), required_bytes,
                                   &required_bytes, GetCurrentProcess(), 0) == FALSE) {
        return {};
    }

    std::vector<Win32CpuSetRow> rows;
    auto* current = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(buffer.data());
    auto* const end = buffer.data() + buffer.size();
    while (reinterpret_cast<std::byte*>(current) < end && current->Size != 0) {
        if (current->Type == CpuSetInformation) {
            const auto& cpu_set = current->CpuSet;
            rows.push_back(Win32CpuSetRow{
                .id = cpu_set.Id,
                .group = cpu_set.Group,
                .logical_processor_index = cpu_set.LogicalProcessorIndex,
                .core_index = cpu_set.CoreIndex,
                .numa_node_index = cpu_set.NumaNodeIndex,
                .efficiency_class = cpu_set.EfficiencyClass,
                .parked = cpu_set.Parked != 0,
                .allocated = cpu_set.Allocated != 0,
                .allocated_to_target_process = cpu_set.AllocatedToTargetProcess != 0,
            });
        }
        current = reinterpret_cast<PSYSTEM_CPU_SET_INFORMATION>(reinterpret_cast<std::byte*>(current) + current->Size);
    }

    return rows;
}

JobExecutionWorkerPlacementCallback make_win32_cpu_set_worker_placement_callback(Win32CpuSetWorkerPlacementPlan plan) {
    return [plan = std::move(plan)](const JobExecutionWorkerPlacementRequest& request) {
        if (plan.status == Win32CpuSetWorkerPlacementStatus::host_evidence_required) {
            return make_host_evidence_result(request, "win32 CPU set worker placement requires host CPU set evidence");
        }
        if (!plan.ready()) {
            return make_failed_result(request, JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration,
                                      "win32 CPU set worker placement plan is not ready");
        }

        const auto* const row = find_worker_row(plan, request.worker_id);
        if (row == nullptr) {
            return make_failed_result(request, JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration,
                                      "win32 CPU set worker placement missing row for worker");
        }

        ULONG cpu_set_id = row->cpu_set_id;
        if (SetThreadSelectedCpuSets(GetCurrentThread(), &cpu_set_id, 1) == FALSE) {
            return make_failed_result(request, JobExecutionPlacementPolicyDiagnosticCode::host_execution_required,
                                      win32_last_error_message("SetThreadSelectedCpuSets"));
        }

        ULONG required_cpu_set_count = 0;
        ULONG selected_cpu_set_id = 0;
        if (GetThreadSelectedCpuSets(GetCurrentThread(), &selected_cpu_set_id, 1, &required_cpu_set_count) == FALSE) {
            return make_failed_result(request, JobExecutionPlacementPolicyDiagnosticCode::host_execution_required,
                                      win32_last_error_message("GetThreadSelectedCpuSets"));
        }
        if (required_cpu_set_count != 1 || selected_cpu_set_id != cpu_set_id) {
            return make_failed_result(request, JobExecutionPlacementPolicyDiagnosticCode::host_execution_required,
                                      "win32 CPU set worker placement verification failed");
        }

        JobExecutionWorkerPlacementResult result;
        result.status = JobExecutionWorkerPlacementStatus::ready;
        result.worker_id = request.worker_id;
        result.attempted = true;
        result.applied = true;
        result.selected_cpu_set_count = 1;
        return result;
    };
}

} // namespace mirakana::win32
