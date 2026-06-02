// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/job_execution.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::win32 {

enum class Win32CpuSetWorkerPlacementStatus : std::uint8_t {
    ready,
    invalid_configuration,
    host_evidence_required,
    failed
};

enum class Win32CpuSetWorkerPlacementDiagnosticCode : std::uint8_t {
    none,
    invalid_configuration,
    no_available_cpu_sets,
    missing_worker_row,
    apply_failed,
    verification_failed
};

struct Win32CpuSetRow {
    std::uint32_t id{0};
    std::uint16_t group{0};
    std::uint8_t logical_processor_index{0};
    std::uint8_t core_index{0};
    std::uint8_t numa_node_index{0};
    std::uint8_t efficiency_class{0};
    bool parked{false};
    bool allocated{false};
    bool allocated_to_target_process{false};
};

struct Win32CpuSetWorkerPlacementDesc {
    std::vector<Win32CpuSetRow> cpu_sets;
    std::uint32_t worker_count{0};
    JobExecutionPlacementPolicyMode mode{JobExecutionPlacementPolicyMode::os_default};
};

struct Win32CpuSetWorkerPlacementRow {
    std::uint32_t worker_id{0};
    std::uint32_t cpu_set_id{0};
    std::uint8_t efficiency_class{0};
    std::uint8_t core_index{0};
    std::uint8_t numa_node_index{0};
};

struct Win32CpuSetWorkerPlacementPlan {
    Win32CpuSetWorkerPlacementStatus status{Win32CpuSetWorkerPlacementStatus::invalid_configuration};
    std::vector<Win32CpuSetWorkerPlacementRow> worker_rows;
    std::uint32_t selected_cpu_set_count{0};
    std::vector<Win32CpuSetWorkerPlacementDiagnosticCode> diagnostic_codes;
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool ready() const noexcept {
        return status == Win32CpuSetWorkerPlacementStatus::ready;
    }
};

[[nodiscard]] Win32CpuSetWorkerPlacementPlan
select_win32_cpu_set_worker_placement(const Win32CpuSetWorkerPlacementDesc& desc);

[[nodiscard]] std::vector<Win32CpuSetRow> query_win32_cpu_sets();

[[nodiscard]] JobExecutionWorkerPlacementCallback
make_win32_cpu_set_worker_placement_callback(Win32CpuSetWorkerPlacementPlan plan);

} // namespace mirakana::win32
