// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/diagnostics.hpp"
#include "mirakana/core/memory.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class JobExecutionPoolStatus : std::uint8_t { ready, invalid_configuration, stopped };

enum class JobExecutionRunStatus : std::uint8_t {
    ready,
    invalid_configuration,
    invalid_task,
    queue_overflow,
    blocked_dependency,
    dependency_cycle,
    stopped,
    task_exception
};

enum class JobExecutionDiagnosticCode : std::uint8_t {
    none,
    invalid_configuration,
    invalid_task,
    queue_overflow,
    blocked_dependency,
    dependency_cycle,
    stopped,
    task_exception
};

enum class JobExecutionTopologyPolicyStatus : std::uint8_t { ready, invalid_configuration, host_evidence_required };

enum class JobExecutionTopologyPolicyDiagnosticCode : std::uint8_t {
    none,
    invalid_configuration,
    missing_processor_group_evidence,
    missing_numa_evidence
};

struct JobExecutionTopologyPolicyDesc {
    std::string name;
    std::uint32_t observed_logical_processor_count{0};
    std::uint32_t fallback_logical_processor_count{1};
    std::uint32_t requested_worker_count{0};
    std::uint32_t worker_count_limit{0};
    std::uint32_t reserved_logical_processor_count{1};
    std::uint64_t queue_capacity_per_worker{256};
    std::uint64_t scratch_budget_bytes_per_worker{4096};
    std::uint64_t frame_index{0};
    std::uint32_t processor_group_count{1};
    std::uint32_t numa_node_count{0};
    bool processor_groups_accounted_for{false};
    bool numa_topology_known{false};
};

struct JobExecutionPoolDesc {
    std::string name;
    std::uint32_t logical_processor_count{0};
    std::uint32_t worker_count{0};
    std::uint64_t queue_capacity_per_worker{256};
    std::uint64_t scratch_budget_bytes_per_worker{4096};
    std::uint64_t frame_index{0};
};

struct JobExecutionTopologyPolicy {
    JobExecutionTopologyPolicyStatus status{JobExecutionTopologyPolicyStatus::invalid_configuration};
    std::uint32_t observed_logical_processor_count{0};
    std::uint32_t effective_logical_processor_count{0};
    std::uint32_t selected_worker_count{0};
    std::uint32_t worker_count_limit{0};
    std::uint32_t reserved_logical_processor_count{0};
    bool hardware_concurrency_fallback_used{false};
    bool requested_worker_count_used{false};
    bool worker_count_limited_by_cap{false};
    bool worker_count_clamped_to_logical_processors{false};
    bool processor_group_policy_applied{false};
    bool numa_policy_applied{false};
    bool affinity_policy_applied{false};
    bool simd_dispatch_applied{false};
    bool gpu_async_overlap_applied{false};
    JobExecutionPoolDesc pool_desc;
    JobWorkerTopologyRow topology_row;
    std::vector<JobExecutionTopologyPolicyDiagnosticCode> diagnostic_codes;
    std::vector<std::string> diagnostics;

    [[nodiscard]] bool ready() const noexcept {
        return status == JobExecutionTopologyPolicyStatus::ready;
    }
};

class JobExecutionStopToken final {
  public:
    JobExecutionStopToken() noexcept = default;
    explicit JobExecutionStopToken(std::shared_ptr<const std::atomic_bool> stop_requested) noexcept;

    [[nodiscard]] bool stop_requested() const noexcept;

  private:
    std::shared_ptr<const std::atomic_bool> stop_requested_;
};

struct JobExecutionContext {
    std::uint32_t worker_id{0};
    JobExecutionStopToken stop_token;
    ScratchArena& scratch;
};

using JobExecutionTaskBody = std::function<void(JobExecutionContext&)>;

struct JobExecutionTaskDesc {
    JobSchedulingWorkItemRow evidence;
    JobExecutionTaskBody body;
};

struct JobExecutionBatchDesc {
    std::vector<JobExecutionTaskDesc> tasks;
    JobSchedulingExecutionOptions options;
};

struct JobExecutionRunResult {
    JobExecutionRunStatus status{JobExecutionRunStatus::invalid_configuration};
    std::uint32_t worker_threads_started{0};
    std::uint64_t tasks_executed{0};
    std::uint64_t tasks_failed{0};
    std::vector<JobExecutionDiagnosticCode> diagnostic_codes;
    std::vector<std::string> diagnostics;
    JobSchedulingExecutionEvidence scheduling_evidence;

    [[nodiscard]] bool ready() const noexcept {
        return status == JobExecutionRunStatus::ready;
    }
};

class JobExecutionPool final {
  public:
    explicit JobExecutionPool(JobExecutionPoolDesc desc);
    ~JobExecutionPool();

    JobExecutionPool(const JobExecutionPool&) = delete;
    JobExecutionPool& operator=(const JobExecutionPool&) = delete;
    JobExecutionPool(JobExecutionPool&&) noexcept = delete;
    JobExecutionPool& operator=(JobExecutionPool&&) noexcept = delete;

    [[nodiscard]] JobExecutionPoolStatus status() const noexcept;
    [[nodiscard]] std::uint32_t worker_threads_started() const noexcept;
    [[nodiscard]] JobExecutionRunResult execute(const JobExecutionBatchDesc& batch);

    void request_stop() noexcept;
    [[nodiscard]] JobExecutionRunResult stop_and_drain();

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

[[nodiscard]] JobExecutionTopologyPolicy
select_job_execution_topology_policy(const JobExecutionTopologyPolicyDesc& desc);
[[nodiscard]] std::uint32_t observe_job_execution_logical_processor_count() noexcept;
[[nodiscard]] std::string_view
job_execution_topology_policy_status_label(JobExecutionTopologyPolicyStatus status) noexcept;
[[nodiscard]] std::string_view
job_execution_topology_policy_diagnostic_code_label(JobExecutionTopologyPolicyDiagnosticCode code) noexcept;
[[nodiscard]] std::string_view job_execution_pool_status_label(JobExecutionPoolStatus status) noexcept;
[[nodiscard]] std::string_view job_execution_run_status_label(JobExecutionRunStatus status) noexcept;
[[nodiscard]] std::string_view job_execution_diagnostic_code_label(JobExecutionDiagnosticCode code) noexcept;

} // namespace mirakana
