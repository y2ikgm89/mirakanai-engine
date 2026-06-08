// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/core/job_execution.hpp"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <deque>
#include <exception>
#include <limits>
#include <mutex>
#include <optional>
#include <ranges>
#include <stdexcept>
#include <thread>
#include <utility>

namespace mirakana {
namespace {

void append_execution_diagnostic(JobExecutionRunResult& result, JobExecutionDiagnosticCode code, std::string message) {
    if (std::ranges::find(result.diagnostic_codes, code) == result.diagnostic_codes.end()) {
        result.diagnostic_codes.push_back(code);
    }
    result.diagnostics.push_back(std::move(message));
}

void append_topology_policy_diagnostic(JobExecutionTopologyPolicy& policy,
                                       JobExecutionTopologyPolicyDiagnosticCode code, std::string message) {
    if (code != JobExecutionTopologyPolicyDiagnosticCode::none &&
        std::ranges::find(policy.diagnostic_codes, code) == policy.diagnostic_codes.end()) {
        policy.diagnostic_codes.push_back(code);
    }
    policy.diagnostics.push_back(std::move(message));
}

void append_placement_policy_diagnostic(JobExecutionPlacementPolicy& policy,
                                        JobExecutionPlacementPolicyDiagnosticCode code, std::string message) {
    if (code != JobExecutionPlacementPolicyDiagnosticCode::none &&
        std::ranges::find(policy.diagnostic_codes, code) == policy.diagnostic_codes.end()) {
        policy.diagnostic_codes.push_back(code);
    }
    policy.diagnostics.push_back(std::move(message));
}

void append_numa_locality_evidence_diagnostic(JobExecutionNumaLocalityEvidence& evidence,
                                              JobExecutionNumaLocalityEvidenceDiagnosticCode code,
                                              std::string message) {
    if (code != JobExecutionNumaLocalityEvidenceDiagnosticCode::none &&
        std::ranges::find(evidence.diagnostic_codes, code) == evidence.diagnostic_codes.end()) {
        evidence.diagnostic_codes.push_back(code);
    }
    evidence.diagnostics.push_back(std::move(message));
}

void append_numa_locality_evidence_diagnostic(JobExecutionNumaFirstTouchLocalityRecipe& recipe,
                                              JobExecutionNumaLocalityEvidenceDiagnosticCode code,
                                              std::string message) {
    if (code != JobExecutionNumaLocalityEvidenceDiagnosticCode::none &&
        std::ranges::find(recipe.diagnostic_codes, code) == recipe.diagnostic_codes.end()) {
        recipe.diagnostic_codes.push_back(code);
    }
    recipe.diagnostics.push_back(std::move(message));
}

void append_numa_locality_evidence_diagnostic(JobExecutionNumaMemoryPolicyComparison& comparison,
                                              JobExecutionNumaLocalityEvidenceDiagnosticCode code,
                                              std::string message) {
    if (code != JobExecutionNumaLocalityEvidenceDiagnosticCode::none &&
        std::ranges::find(comparison.diagnostic_codes, code) == comparison.diagnostic_codes.end()) {
        comparison.diagnostic_codes.push_back(code);
    }
    comparison.diagnostics.push_back(std::move(message));
}

[[nodiscard]] JobExecutionRunStatus
status_from_diagnostics(std::span<const JobExecutionDiagnosticCode> codes) noexcept {
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::invalid_configuration) != codes.end()) {
        return JobExecutionRunStatus::invalid_configuration;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::stopped) != codes.end()) {
        return JobExecutionRunStatus::stopped;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::task_exception) != codes.end()) {
        return JobExecutionRunStatus::task_exception;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::dependency_cycle) != codes.end()) {
        return JobExecutionRunStatus::dependency_cycle;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::blocked_dependency) != codes.end()) {
        return JobExecutionRunStatus::blocked_dependency;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::queue_overflow) != codes.end()) {
        return JobExecutionRunStatus::queue_overflow;
    }
    if (std::ranges::find(codes, JobExecutionDiagnosticCode::invalid_task) != codes.end()) {
        return JobExecutionRunStatus::invalid_task;
    }
    return JobExecutionRunStatus::ready;
}

[[nodiscard]] JobExecutionDiagnosticCode
execution_code_from_scheduling_code(JobSchedulingDiagnosticsCode code) noexcept {
    switch (code) {
    case JobSchedulingDiagnosticsCode::none:
        return JobExecutionDiagnosticCode::none;
    case JobSchedulingDiagnosticsCode::queue_overflow:
        return JobExecutionDiagnosticCode::queue_overflow;
    case JobSchedulingDiagnosticsCode::blocked_dependency:
        return JobExecutionDiagnosticCode::blocked_dependency;
    case JobSchedulingDiagnosticsCode::dependency_cycle:
        return JobExecutionDiagnosticCode::dependency_cycle;
    case JobSchedulingDiagnosticsCode::missing_rows:
    case JobSchedulingDiagnosticsCode::invalid_worker_topology:
    case JobSchedulingDiagnosticsCode::missing_processor_group_evidence:
    case JobSchedulingDiagnosticsCode::missing_numa_evidence:
    case JobSchedulingDiagnosticsCode::invalid_queue:
    case JobSchedulingDiagnosticsCode::scratch_misuse:
    case JobSchedulingDiagnosticsCode::nondeterministic_merge:
    case JobSchedulingDiagnosticsCode::undersized_job_batch:
    case JobSchedulingDiagnosticsCode::oversized_job_batch:
        return JobExecutionDiagnosticCode::invalid_task;
    }
    return JobExecutionDiagnosticCode::invalid_task;
}

[[nodiscard]] std::string safe_pool_name(const JobExecutionPoolDesc& desc) {
    return desc.name.empty() ? "job_execution_pool" : desc.name;
}

[[nodiscard]] std::uint32_t selected_worker_count_for_policy(const JobExecutionTopologyPolicyDesc& desc,
                                                             std::uint32_t effective_logical_processor_count,
                                                             JobExecutionTopologyPolicy& policy) noexcept {
    std::uint32_t selected_worker_count = 0;
    if (desc.requested_worker_count > 0) {
        selected_worker_count = desc.requested_worker_count;
        policy.requested_worker_count_used = true;
    } else if (effective_logical_processor_count > desc.reserved_logical_processor_count) {
        selected_worker_count = effective_logical_processor_count - desc.reserved_logical_processor_count;
    } else {
        selected_worker_count = 1;
    }

    if (desc.worker_count_limit > 0 && selected_worker_count > desc.worker_count_limit) {
        selected_worker_count = desc.worker_count_limit;
        policy.worker_count_limited_by_cap = true;
    }
    if (selected_worker_count > effective_logical_processor_count) {
        selected_worker_count = effective_logical_processor_count;
        policy.worker_count_clamped_to_logical_processors = true;
    }
    return std::max<std::uint32_t>(1, selected_worker_count);
}

} // namespace

struct JobExecutionPool::Impl {
    struct ExecutionGroupState {
        std::mutex mutex;
        std::condition_variable cv;
        std::uint64_t remaining{0};
        std::uint64_t executed{0};
        std::uint64_t failed{0};
        std::vector<JobExecutionDiagnosticCode> diagnostic_codes;
        std::vector<std::string> diagnostics;
    };

    struct WorkItem {
        std::string job_id;
        JobExecutionTaskBody body;
        std::shared_ptr<ExecutionGroupState> group;
    };

    struct WorkerState {
        WorkerState(std::uint32_t id, std::string scratch_name, std::uint64_t scratch_capacity_bytes)
            : worker_id(id), scratch(ScratchArena::make_worker(std::move(scratch_name),
                                                               static_cast<std::size_t>(scratch_capacity_bytes), id)) {}

        std::uint32_t worker_id{0};
        ScratchArena scratch;
        std::mutex mutex;
        std::condition_variable cv;
        std::deque<WorkItem> queue;
        std::shared_ptr<std::atomic_bool> stop_requested{std::make_shared<std::atomic_bool>(false)};
        std::atomic_uint64_t steal_attempt_count{0};
        std::atomic_uint64_t steal_success_count{0};
        std::atomic_uint64_t worker_wait_count{0};
        std::atomic_uint64_t worker_placement_attempt_count{0};
        std::atomic_uint64_t worker_placement_applied_count{0};
        std::atomic_uint64_t worker_placement_diagnostic_count{0};
        std::atomic_uint64_t worker_placement_selected_cpu_set_count{0};
        std::thread thread;
        bool stopping{false};

        ~WorkerState() {
            stop_requested->store(true, std::memory_order_relaxed);
            {
                std::scoped_lock lock(mutex);
                stopping = true;
            }
            cv.notify_all();
            if (thread.joinable()) {
                thread.join();
            }
        }
    };

    struct WorkerCounterSnapshot {
        std::uint64_t steal_attempt_count{0};
        std::uint64_t steal_success_count{0};
        std::uint64_t worker_wait_count{0};
    };

    explicit Impl(JobExecutionPoolDesc pool_desc) : desc(std::move(pool_desc)) {
        if (desc.name.empty() || desc.worker_count == 0 || desc.queue_capacity_per_worker == 0 ||
            desc.scratch_budget_bytes_per_worker == 0 ||
            desc.scratch_budget_bytes_per_worker >
                static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
            pool_status = JobExecutionPoolStatus::invalid_configuration;
            return;
        }

        workers.reserve(desc.worker_count);
        for (std::uint32_t worker_id = 0; worker_id < desc.worker_count; ++worker_id) {
            workers.push_back(std::make_unique<WorkerState>(
                worker_id, desc.name + ".worker." + std::to_string(worker_id) + ".scratch",
                desc.scratch_budget_bytes_per_worker));
        }
        for (auto& worker : workers) {
            auto* worker_ptr = worker.get();
            worker->thread = std::thread([this, worker_ptr] { run_worker(*worker_ptr); });
            ++started_threads;
        }
        wait_for_worker_startup_placements();
        pool_status = JobExecutionPoolStatus::ready;
    }

    ~Impl() {
        (void)stop_and_drain();
    }

    [[nodiscard]] JobExecutionRunResult execute(const JobExecutionBatchDesc& batch) {
        auto result = JobExecutionRunResult{};
        result.worker_threads_started = started_threads;

        const auto current_status = status();
        if (current_status == JobExecutionPoolStatus::invalid_configuration) {
            append_execution_diagnostic(result, JobExecutionDiagnosticCode::invalid_configuration,
                                        "job execution pool requires non-empty name, non-zero worker count, bounded "
                                        "queue capacity, and worker scratch capacity");
            result.status = JobExecutionRunStatus::invalid_configuration;
            return result;
        }
        if (current_status == JobExecutionPoolStatus::stopped) {
            append_execution_diagnostic(result, JobExecutionDiagnosticCode::stopped, "job execution pool is stopped");
            result.status = JobExecutionRunStatus::stopped;
            return result;
        }
        if (batch.tasks.empty()) {
            append_execution_diagnostic(result, JobExecutionDiagnosticCode::invalid_task,
                                        "job execution batch requires at least one task");
            result.status = JobExecutionRunStatus::invalid_task;
            return result;
        }

        std::vector<JobSchedulingWorkItemRow> work_items;
        work_items.reserve(batch.tasks.size());
        for (const auto& task : batch.tasks) {
            if (!task.body) {
                append_execution_diagnostic(result, JobExecutionDiagnosticCode::invalid_task,
                                            "job execution task '" + task.evidence.job_id + "' requires a body");
            }
            work_items.push_back(task.evidence);
        }
        if (!result.diagnostic_codes.empty()) {
            result.status = status_from_diagnostics(result.diagnostic_codes);
            return result;
        }

        const auto topology = make_topology_rows();
        auto options = batch.options;
        options.queue_capacity_per_worker = desc.queue_capacity_per_worker;
        options.scratch_budget_bytes_per_worker = desc.scratch_budget_bytes_per_worker;
        if (options.frame_index == 0) {
            options.frame_index = desc.frame_index;
        }
        result.scheduling_evidence = build_job_scheduling_execution_evidence(topology, work_items, options);
        copy_scheduling_diagnostics(result);
        if (!result.diagnostic_codes.empty()) {
            result.status = status_from_diagnostics(result.diagnostic_codes);
            return result;
        }

        const auto counter_baseline = snapshot_counters();
        std::vector<std::uint8_t> completed(batch.tasks.size(), 0);
        std::uint64_t completed_count = 0;
        while (completed_count < batch.tasks.size()) {
            if (status() == JobExecutionPoolStatus::stopped) {
                append_execution_diagnostic(result, JobExecutionDiagnosticCode::stopped,
                                            "job execution stopped before the batch drained");
                break;
            }

            std::vector<std::size_t> runnable_indices;
            for (const auto& order_row : result.scheduling_evidence.execution_order) {
                const auto task_index = find_task_index(batch.tasks, order_row.job_id);
                if (!task_index.has_value() || completed[*task_index] != 0U) {
                    continue;
                }
                if (dependencies_completed(batch.tasks[*task_index].evidence, batch.tasks, completed)) {
                    runnable_indices.push_back(*task_index);
                }
            }
            if (runnable_indices.empty()) {
                append_execution_diagnostic(result, JobExecutionDiagnosticCode::dependency_cycle,
                                            "job execution batch has no runnable task before drain");
                break;
            }

            auto group = std::make_shared<ExecutionGroupState>();
            group->remaining = static_cast<std::uint64_t>(runnable_indices.size());
            for (const auto task_index : runnable_indices) {
                if (!enqueue_task(batch.tasks[task_index], group)) {
                    append_execution_diagnostic(result, JobExecutionDiagnosticCode::stopped,
                                                "job execution pool stopped before task '" +
                                                    batch.tasks[task_index].evidence.job_id + "' was enqueued");
                    std::scoped_lock lock(group->mutex);
                    --group->remaining;
                }
            }
            wait_group(*group);
            result.tasks_executed += group->executed;
            result.tasks_failed += group->failed;
            for (auto code : group->diagnostic_codes) {
                if (std::ranges::find(result.diagnostic_codes, code) == result.diagnostic_codes.end()) {
                    result.diagnostic_codes.push_back(code);
                }
            }
            result.diagnostics.insert(result.diagnostics.end(), group->diagnostics.begin(), group->diagnostics.end());

            if (group->failed != 0 || !group->diagnostic_codes.empty()) {
                break;
            }
            for (const auto task_index : runnable_indices) {
                if (completed[task_index] == 0U) {
                    completed[task_index] = 1;
                    ++completed_count;
                }
            }
        }

        apply_runtime_queue_counters(result, topology, counter_baseline);
        copy_worker_placement_counters(result);
        result.status = status_from_diagnostics(result.diagnostic_codes);
        return result;
    }

    void request_stop() noexcept {
        {
            std::scoped_lock lock(status_mutex);
            if (pool_status == JobExecutionPoolStatus::invalid_configuration) {
                return;
            }
            pool_status = JobExecutionPoolStatus::stopped;
        }
        for (auto& worker : workers) {
            worker->stop_requested->store(true, std::memory_order_relaxed);
            {
                std::scoped_lock lock(worker->mutex);
                worker->stopping = true;
            }
            worker->cv.notify_all();
        }
    }

    [[nodiscard]] JobExecutionRunResult stop_and_drain() {
        if (status() == JobExecutionPoolStatus::invalid_configuration) {
            auto result = JobExecutionRunResult{};
            result.worker_threads_started = started_threads;
            result.status = JobExecutionRunStatus::invalid_configuration;
            append_execution_diagnostic(result, JobExecutionDiagnosticCode::invalid_configuration,
                                        "job execution pool configuration is invalid");
            return result;
        }

        request_stop();
        for (auto& worker : workers) {
            if (worker->thread.joinable()) {
                worker->thread.join();
            }
        }
        auto result = JobExecutionRunResult{};
        result.worker_threads_started = started_threads;
        result.status = JobExecutionRunStatus::stopped;
        append_execution_diagnostic(result, JobExecutionDiagnosticCode::stopped, "job execution pool stopped");
        return result;
    }

    [[nodiscard]] JobExecutionPoolStatus status() const noexcept {
        std::scoped_lock lock(status_mutex);
        return pool_status;
    }

    [[nodiscard]] std::vector<JobWorkerTopologyRow> make_topology_rows() const {
        return {JobWorkerTopologyRow{.name = safe_pool_name(desc),
                                     .logical_processor_count = desc.logical_processor_count == 0
                                                                    ? desc.worker_count
                                                                    : desc.logical_processor_count,
                                     .worker_count = desc.worker_count,
                                     .queue_count = desc.worker_count,
                                     .processor_group_count = 1,
                                     .numa_node_count = 1,
                                     .processor_groups_accounted_for = true,
                                     .numa_topology_known = true}};
    }

    void copy_scheduling_diagnostics(JobExecutionRunResult& result) const {
        const auto& summary = result.scheduling_evidence.scheduling_summary;
        for (const auto code : summary.diagnostic_codes) {
            const auto execution_code = execution_code_from_scheduling_code(code);
            if (execution_code != JobExecutionDiagnosticCode::none &&
                std::ranges::find(result.diagnostic_codes, execution_code) == result.diagnostic_codes.end()) {
                result.diagnostic_codes.push_back(execution_code);
            }
        }
        result.diagnostics.insert(result.diagnostics.end(), summary.diagnostics.begin(), summary.diagnostics.end());
    }

    [[nodiscard]] std::vector<WorkerCounterSnapshot> snapshot_counters() const {
        std::vector<WorkerCounterSnapshot> snapshots;
        snapshots.reserve(workers.size());
        for (const auto& worker : workers) {
            snapshots.push_back(WorkerCounterSnapshot{
                .steal_attempt_count = worker->steal_attempt_count.load(std::memory_order_relaxed),
                .steal_success_count = worker->steal_success_count.load(std::memory_order_relaxed),
                .worker_wait_count = worker->worker_wait_count.load(std::memory_order_relaxed),
            });
        }
        return snapshots;
    }

    void wait_for_worker_startup_placements() {
        std::unique_lock lock(worker_startup_mutex);
        worker_startup_cv.wait(lock, [this] { return worker_startup_placement_count >= started_threads; });
    }

    void apply_runtime_queue_counters(JobExecutionRunResult& result,
                                      std::span<const JobWorkerTopologyRow> topology_rows,
                                      std::span<const WorkerCounterSnapshot> baseline) const {
        for (std::size_t index = 0; index < workers.size() && index < baseline.size(); ++index) {
            const auto steal_attempt_count = workers[index]->steal_attempt_count.load(std::memory_order_relaxed) -
                                             baseline[index].steal_attempt_count;
            const auto steal_success_count = workers[index]->steal_success_count.load(std::memory_order_relaxed) -
                                             baseline[index].steal_success_count;
            const auto worker_wait_count =
                workers[index]->worker_wait_count.load(std::memory_order_relaxed) - baseline[index].worker_wait_count;

            result.steal_attempt_count += steal_attempt_count;
            result.steal_success_count += steal_success_count;
            result.worker_wait_count += worker_wait_count;
            if (index < result.scheduling_evidence.queue_rows.size()) {
                auto& row = result.scheduling_evidence.queue_rows[index];
                row.steal_attempt_count += steal_attempt_count;
                row.steal_success_count += steal_success_count;
                row.worker_wait_count += worker_wait_count;
            }
        }

        result.work_stealing_applied =
            desc.work_stealing_enabled && desc.worker_count > 1U && result.steal_success_count > 0U;
        result.scheduling_evidence.scheduling_summary =
            summarize_job_scheduling_diagnostics(topology_rows, result.scheduling_evidence.queue_rows);
    }

    void copy_worker_placement_counters(JobExecutionRunResult& result) const noexcept {
        for (const auto& worker : workers) {
            result.worker_placement_attempt_count +=
                worker->worker_placement_attempt_count.load(std::memory_order_relaxed);
            result.worker_placement_applied_count +=
                worker->worker_placement_applied_count.load(std::memory_order_relaxed);
            result.worker_placement_diagnostic_count +=
                worker->worker_placement_diagnostic_count.load(std::memory_order_relaxed);
            result.worker_placement_selected_cpu_set_count +=
                worker->worker_placement_selected_cpu_set_count.load(std::memory_order_relaxed);
        }
    }

    [[nodiscard]] static std::optional<std::size_t> find_task_index(std::span<const JobExecutionTaskDesc> tasks,
                                                                    std::string_view job_id) {
        for (std::size_t index = 0; index < tasks.size(); ++index) {
            if (tasks[index].evidence.job_id == job_id) {
                return index;
            }
        }
        return std::nullopt;
    }

    [[nodiscard]] static bool dependencies_completed(const JobSchedulingWorkItemRow& row,
                                                     std::span<const JobExecutionTaskDesc> tasks,
                                                     std::span<const std::uint8_t> completed) {
        for (const auto& dependency_id : row.dependency_job_ids) {
            const auto dependency_index = find_task_index(tasks, dependency_id);
            if (!dependency_index.has_value() || completed[*dependency_index] == 0U) {
                return false;
            }
        }
        return true;
    }

    [[nodiscard]] bool enqueue_task(const JobExecutionTaskDesc& task, std::shared_ptr<ExecutionGroupState> group) {
        if (task.evidence.worker_id >= workers.size()) {
            return false;
        }
        auto& worker = *workers[task.evidence.worker_id];
        {
            std::scoped_lock lock(worker.mutex);
            if (worker.stopping || worker.stop_requested->load(std::memory_order_relaxed) ||
                worker.queue.size() >= desc.queue_capacity_per_worker) {
                return false;
            }
            worker.queue.push_back(WorkItem{.job_id = task.evidence.job_id, .body = task.body, .group = group});
        }
        available_work_count.fetch_add(1, std::memory_order_release);
        if (desc.work_stealing_enabled && workers.size() > 1U) {
            for (auto& candidate : workers) {
                candidate->cv.notify_one();
            }
        } else {
            worker.cv.notify_one();
        }
        return true;
    }

    static void wait_group(ExecutionGroupState& group) {
        std::unique_lock lock(group.mutex);
        group.cv.wait(lock, [&group] { return group.remaining == 0; });
    }

    [[nodiscard]] bool try_pop_local_work(WorkerState& worker, WorkItem& work) {
        std::scoped_lock lock(worker.mutex);
        if (worker.queue.empty()) {
            return false;
        }
        work = std::move(worker.queue.front());
        worker.queue.pop_front();
        available_work_count.fetch_sub(1, std::memory_order_acq_rel);
        return true;
    }

    [[nodiscard]] bool try_steal_work(WorkerState& worker, WorkItem& work) {
        if (!desc.work_stealing_enabled || workers.size() <= 1U) {
            return false;
        }

        for (std::uint32_t offset = 1; offset < desc.worker_count; ++offset) {
            const auto victim_index = static_cast<std::size_t>((worker.worker_id + offset) % desc.worker_count);
            auto& victim = *workers[victim_index];
            worker.steal_attempt_count.fetch_add(1, std::memory_order_relaxed);

            std::scoped_lock lock(victim.mutex);
            if (victim.queue.empty()) {
                continue;
            }
            work = std::move(victim.queue.back());
            victim.queue.pop_back();
            available_work_count.fetch_sub(1, std::memory_order_acq_rel);
            worker.steal_success_count.fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    void apply_worker_placement(WorkerState& worker) noexcept {
        if (!desc.worker_placement_callback) {
            return;
        }

        const auto request = JobExecutionWorkerPlacementRequest{
            .pool_name = desc.name,
            .worker_id = worker.worker_id,
            .worker_count = desc.worker_count,
            .requested_mode = desc.placement_requested_mode,
            .selected_mode = desc.placement_selected_mode,
        };
        try {
            const auto placement = desc.worker_placement_callback(request);
            if (placement.attempted) {
                worker.worker_placement_attempt_count.fetch_add(1, std::memory_order_relaxed);
            }
            if (placement.applied && placement.status == JobExecutionWorkerPlacementStatus::ready) {
                worker.worker_placement_applied_count.fetch_add(1, std::memory_order_relaxed);
                worker.worker_placement_selected_cpu_set_count.fetch_add(placement.selected_cpu_set_count,
                                                                         std::memory_order_relaxed);
            }
            const auto explicit_diagnostic_count =
                std::max(placement.diagnostic_codes.size(), placement.diagnostics.size());
            if (explicit_diagnostic_count > 0) {
                worker.worker_placement_diagnostic_count.fetch_add(
                    static_cast<std::uint64_t>(explicit_diagnostic_count), std::memory_order_relaxed);
            } else if (placement.status == JobExecutionWorkerPlacementStatus::host_evidence_required ||
                       placement.status == JobExecutionWorkerPlacementStatus::failed ||
                       placement.worker_id != worker.worker_id) {
                worker.worker_placement_diagnostic_count.fetch_add(1, std::memory_order_relaxed);
            }
        } catch (...) {
            worker.worker_placement_attempt_count.fetch_add(1, std::memory_order_relaxed);
            worker.worker_placement_diagnostic_count.fetch_add(1, std::memory_order_relaxed);
        }
    }

    void run_worker(WorkerState& worker) noexcept {
        apply_worker_placement(worker);
        {
            std::scoped_lock lock(worker_startup_mutex);
            ++worker_startup_placement_count;
        }
        worker_startup_cv.notify_all();
        while (true) {
            auto work = WorkItem{};
            if (!try_pop_local_work(worker, work) && !try_steal_work(worker, work)) {
                std::unique_lock lock(worker.mutex);
                if ((worker.stop_requested->load(std::memory_order_relaxed) || worker.stopping) &&
                    worker.queue.empty() && available_work_count.load(std::memory_order_acquire) == 0U) {
                    return;
                }
                worker.worker_wait_count.fetch_add(1, std::memory_order_relaxed);
                worker.cv.wait(lock, [&worker, this] {
                    return worker.stopping || worker.stop_requested->load(std::memory_order_relaxed) ||
                           !worker.queue.empty() || available_work_count.load(std::memory_order_acquire) > 0U;
                });
                continue;
            }

            bool failed = false;
            std::string diagnostic;
            try {
                auto context = JobExecutionContext{.worker_id = worker.worker_id,
                                                   .stop_token = JobExecutionStopToken{worker.stop_requested},
                                                   .scratch = worker.scratch};
                work.body(context);
            } catch (const std::exception& error) {
                failed = true;
                diagnostic = "job execution task '" + work.job_id + "' threw exception: " + error.what();
            } catch (...) {
                failed = true;
                diagnostic = "job execution task '" + work.job_id + "' threw unknown exception";
            }
            worker.scratch.reset_at_safe_point();

            {
                std::scoped_lock lock(work.group->mutex);
                ++work.group->executed;
                if (failed) {
                    ++work.group->failed;
                    if (std::ranges::find(work.group->diagnostic_codes, JobExecutionDiagnosticCode::task_exception) ==
                        work.group->diagnostic_codes.end()) {
                        work.group->diagnostic_codes.push_back(JobExecutionDiagnosticCode::task_exception);
                    }
                    work.group->diagnostics.push_back(std::move(diagnostic));
                }
                --work.group->remaining;
            }
            work.group->cv.notify_all();
        }
    }

    JobExecutionPoolDesc desc;
    mutable std::mutex status_mutex;
    JobExecutionPoolStatus pool_status{JobExecutionPoolStatus::invalid_configuration};
    std::uint32_t started_threads{0};
    std::mutex worker_startup_mutex;
    std::condition_variable worker_startup_cv;
    std::uint32_t worker_startup_placement_count{0};
    std::vector<std::unique_ptr<WorkerState>> workers;
    std::atomic_uint64_t available_work_count{0};
};

JobExecutionStopToken::JobExecutionStopToken(std::shared_ptr<const std::atomic_bool> stop_requested) noexcept
    : stop_requested_(std::move(stop_requested)) {}

bool JobExecutionStopToken::stop_requested() const noexcept {
    return stop_requested_ != nullptr && stop_requested_->load(std::memory_order_relaxed);
}

JobExecutionPool::JobExecutionPool(JobExecutionPoolDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

JobExecutionPool::~JobExecutionPool() = default;

JobExecutionTopologyPolicy select_job_execution_topology_policy(const JobExecutionTopologyPolicyDesc& desc) {
    auto policy = JobExecutionTopologyPolicy{};
    policy.observed_logical_processor_count = desc.observed_logical_processor_count;
    policy.hardware_concurrency_fallback_used = desc.observed_logical_processor_count == 0;
    policy.effective_logical_processor_count = desc.observed_logical_processor_count == 0
                                                   ? desc.fallback_logical_processor_count
                                                   : desc.observed_logical_processor_count;
    policy.worker_count_limit = desc.worker_count_limit;
    policy.reserved_logical_processor_count = desc.reserved_logical_processor_count;

    if (desc.name.empty()) {
        append_topology_policy_diagnostic(policy, JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration,
                                          "job execution topology policy requires a non-empty name");
    }
    if (policy.effective_logical_processor_count == 0) {
        append_topology_policy_diagnostic(
            policy, JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration,
            "job execution topology policy requires observed or fallback logical processors");
    }
    if (desc.queue_capacity_per_worker == 0) {
        append_topology_policy_diagnostic(policy, JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration,
                                          "job execution topology policy requires bounded queue capacity");
    }
    if (desc.scratch_budget_bytes_per_worker == 0 ||
        desc.scratch_budget_bytes_per_worker > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        append_topology_policy_diagnostic(policy, JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration,
                                          "job execution topology policy requires bounded worker scratch capacity");
    }

    if (std::ranges::find(policy.diagnostic_codes, JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration) !=
        policy.diagnostic_codes.end()) {
        policy.status = JobExecutionTopologyPolicyStatus::invalid_configuration;
        return policy;
    }

    policy.selected_worker_count =
        selected_worker_count_for_policy(desc, policy.effective_logical_processor_count, policy);
    const auto processor_group_count = std::max<std::uint32_t>(1, desc.processor_group_count);
    policy.pool_desc =
        JobExecutionPoolDesc{.name = desc.name,
                             .logical_processor_count = policy.effective_logical_processor_count,
                             .worker_count = policy.selected_worker_count,
                             .queue_capacity_per_worker = desc.queue_capacity_per_worker,
                             .scratch_budget_bytes_per_worker = desc.scratch_budget_bytes_per_worker,
                             .frame_index = desc.frame_index,
                             .work_stealing_enabled = desc.enable_work_stealing && policy.selected_worker_count > 1U,
                             .worker_placement_callback = {}};
    policy.work_stealing_applied = policy.pool_desc.work_stealing_enabled;
    policy.topology_row = JobWorkerTopologyRow{.name = desc.name,
                                               .logical_processor_count = policy.effective_logical_processor_count,
                                               .worker_count = policy.selected_worker_count,
                                               .queue_count = policy.selected_worker_count,
                                               .processor_group_count = processor_group_count,
                                               .numa_node_count = desc.numa_node_count,
                                               .processor_groups_accounted_for = desc.processor_groups_accounted_for,
                                               .numa_topology_known = desc.numa_topology_known};

    if (processor_group_count > 1 && !desc.processor_groups_accounted_for) {
        append_topology_policy_diagnostic(
            policy, JobExecutionTopologyPolicyDiagnosticCode::missing_processor_group_evidence,
            "job execution topology policy needs processor group evidence before applying group-aware scheduling");
    }
    if (desc.numa_node_count > 1 && !desc.numa_topology_known) {
        append_topology_policy_diagnostic(
            policy, JobExecutionTopologyPolicyDiagnosticCode::missing_numa_evidence,
            "job execution topology policy needs NUMA topology evidence before applying NUMA placement");
    }

    policy.status = policy.diagnostics.empty() ? JobExecutionTopologyPolicyStatus::ready
                                               : JobExecutionTopologyPolicyStatus::host_evidence_required;
    return policy;
}

JobExecutionPlacementPolicy select_job_execution_placement_policy(const JobExecutionPlacementPolicyDesc& desc) {
    auto policy = JobExecutionPlacementPolicy{};
    policy.requested_mode = desc.requested_mode;
    policy.selected_mode = JobExecutionPlacementPolicyMode::os_default;
    policy.pool_desc = desc.topology_policy.pool_desc;
    policy.inherited_worker_count = desc.topology_policy.pool_desc.worker_count;
    policy.numa_node_count = desc.numa_node_count.value_or(desc.topology_policy.topology_row.numa_node_count);
    policy.performance_core_count = desc.performance_core_count.value_or(0U);
    policy.efficiency_core_count = desc.efficiency_core_count.value_or(0U);
    policy.smt_sibling_topology_known = desc.smt_sibling_topology_known.value_or(false);

    if (desc.name.empty()) {
        append_placement_policy_diagnostic(policy, JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration,
                                           "job execution placement policy requires a non-empty name");
    }
    if (desc.topology_policy.status == JobExecutionTopologyPolicyStatus::invalid_configuration ||
        desc.topology_policy.pool_desc.worker_count == 0) {
        append_placement_policy_diagnostic(policy, JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration,
                                           "job execution placement policy requires a ready topology policy");
    }

    if (std::ranges::find(policy.diagnostic_codes, JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration) !=
        policy.diagnostic_codes.end()) {
        policy.status = JobExecutionPlacementPolicyStatus::invalid_configuration;
        return policy;
    }

    if (std::ranges::find(desc.topology_policy.diagnostic_codes,
                          JobExecutionTopologyPolicyDiagnosticCode::missing_processor_group_evidence) !=
        desc.topology_policy.diagnostic_codes.end()) {
        append_placement_policy_diagnostic(
            policy, JobExecutionPlacementPolicyDiagnosticCode::missing_processor_group_evidence,
            "job execution placement policy needs processor group evidence before placement selection");
    }
    if (std::ranges::find(desc.topology_policy.diagnostic_codes,
                          JobExecutionTopologyPolicyDiagnosticCode::missing_numa_evidence) !=
        desc.topology_policy.diagnostic_codes.end()) {
        append_placement_policy_diagnostic(policy, JobExecutionPlacementPolicyDiagnosticCode::missing_numa_evidence,
                                           "job execution placement policy needs NUMA topology evidence");
    }

    switch (desc.requested_mode) {
    case JobExecutionPlacementPolicyMode::os_default:
        break;
    case JobExecutionPlacementPolicyMode::prefer_local_numa:
        if (!desc.numa_node_count.has_value() || policy.numa_node_count <= 1U ||
            !desc.topology_policy.topology_row.numa_topology_known) {
            append_placement_policy_diagnostic(policy, JobExecutionPlacementPolicyDiagnosticCode::missing_numa_evidence,
                                               "job execution placement policy needs explicit NUMA placement evidence");
        } else {
            policy.selected_mode = desc.requested_mode;
        }
        break;
    case JobExecutionPlacementPolicyMode::prefer_performance_cores:
    case JobExecutionPlacementPolicyMode::prefer_efficiency_cores:
        if (!desc.performance_core_count.has_value() || !desc.efficiency_core_count.has_value() ||
            (policy.performance_core_count + policy.efficiency_core_count) == 0U) {
            append_placement_policy_diagnostic(
                policy, JobExecutionPlacementPolicyDiagnosticCode::missing_hybrid_core_evidence,
                "job execution placement policy needs explicit hybrid core-type evidence");
        } else {
            policy.selected_mode = desc.requested_mode;
        }
        break;
    case JobExecutionPlacementPolicyMode::avoid_smt_siblings:
        if (!desc.smt_sibling_topology_known.value_or(false)) {
            append_placement_policy_diagnostic(policy, JobExecutionPlacementPolicyDiagnosticCode::missing_smt_evidence,
                                               "job execution placement policy needs SMT sibling evidence");
        } else {
            policy.selected_mode = desc.requested_mode;
        }
        break;
    case JobExecutionPlacementPolicyMode::manual_host_affinity:
        if (!desc.allow_host_affinity_execution) {
            append_placement_policy_diagnostic(
                policy, JobExecutionPlacementPolicyDiagnosticCode::host_execution_required,
                "job execution placement policy manual affinity requires host execution");
        } else {
            policy.selected_mode = desc.requested_mode;
        }
        break;
    }

    policy.status = policy.diagnostics.empty() ? JobExecutionPlacementPolicyStatus::ready
                                               : JobExecutionPlacementPolicyStatus::host_evidence_required;
    return policy;
}

JobExecutionNumaLocalityEvidence
summarize_job_execution_numa_locality_evidence(const JobExecutionNumaLocalityEvidenceDesc& desc) {
    auto evidence = JobExecutionNumaLocalityEvidence{};
    evidence.workload = desc.workload;
    evidence.numa_node_count = desc.numa_node_count;
    evidence.numa_topology_known = desc.numa_topology_known;
    evidence.cpu_to_node_rows = desc.cpu_to_node_rows;
    evidence.requested_memory_policy_scope = desc.memory_policy_scope;
    evidence.selected_memory_policy_scope = JobExecutionNumaLocalityMemoryPolicyScope::first_touch_locality;
    evidence.first_touch_locality_default = true;
    evidence.manual_memory_policy_applied = false;
    evidence.local_memory_bytes = desc.local_memory_bytes;
    evidence.remote_memory_bytes = desc.remote_memory_bytes;
    evidence.local_remote_memory_counters_available = desc.local_remote_memory_counters_available;
    evidence.nps_state_known = desc.nps_state_known;
    evidence.nps_state = desc.nps_state;
    evidence.cpuset_restrictions_observed = desc.cpuset_restrictions_observed;

    if (desc.name.empty()) {
        append_numa_locality_evidence_diagnostic(evidence,
                                                 JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
                                                 "job execution NUMA locality evidence requires a non-empty name");
    }
    if (desc.workload.empty()) {
        append_numa_locality_evidence_diagnostic(evidence,
                                                 JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
                                                 "job execution NUMA locality evidence requires a workload name");
    }

    if (std::ranges::find(evidence.diagnostic_codes,
                          JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration) !=
        evidence.diagnostic_codes.end()) {
        evidence.status = JobExecutionNumaLocalityEvidenceStatus::invalid_configuration;
        return evidence;
    }

    if (desc.numa_node_count > 1U && !desc.numa_topology_known) {
        append_numa_locality_evidence_diagnostic(
            evidence, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_numa_topology,
            "job execution NUMA locality evidence needs NUMA topology before recording multi-node locality");
    }
    if (desc.numa_topology_known && desc.cpu_to_node_rows.empty()) {
        append_numa_locality_evidence_diagnostic(
            evidence, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_cpu_to_node_mapping,
            "job execution NUMA locality evidence needs CPU-to-node mapping rows");
    }
    if (desc.cpuset_restrictions_observed && !desc.cpuset_restrictions_verifiable) {
        append_numa_locality_evidence_diagnostic(
            evidence, JobExecutionNumaLocalityEvidenceDiagnosticCode::cpuset_restriction_unverifiable,
            "job execution NUMA locality evidence cannot verify locality under observed cpuset restrictions");
    }

    const bool manual_policy_requested =
        desc.compare_manual_memory_policy ||
        desc.memory_policy_scope == JobExecutionNumaLocalityMemoryPolicyScope::manual_host_policy;
    if (manual_policy_requested) {
        evidence.manual_memory_policy_selected = true;
        if (!desc.local_remote_memory_counters_available || !desc.local_memory_bytes.has_value() ||
            !desc.remote_memory_bytes.has_value()) {
            append_numa_locality_evidence_diagnostic(
                evidence, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_local_remote_counters,
                "job execution NUMA locality evidence needs local and remote memory counters before comparing "
                "manual memory policy");
        }
        if (!desc.nps_state_known || desc.nps_state.empty()) {
            append_numa_locality_evidence_diagnostic(
                evidence, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_nps_state,
                "job execution NUMA locality evidence needs NPS state before comparing manual memory policy");
        }
    }

    if (evidence.diagnostic_codes.empty()) {
        evidence.status = JobExecutionNumaLocalityEvidenceStatus::ready;
        if (manual_policy_requested) {
            evidence.selected_memory_policy_scope = JobExecutionNumaLocalityMemoryPolicyScope::manual_host_policy;
        } else if (desc.memory_policy_scope == JobExecutionNumaLocalityMemoryPolicyScope::os_default) {
            evidence.selected_memory_policy_scope = JobExecutionNumaLocalityMemoryPolicyScope::os_default;
        } else {
            evidence.selected_memory_policy_scope = JobExecutionNumaLocalityMemoryPolicyScope::first_touch_locality;
        }
        return evidence;
    }

    evidence.status = JobExecutionNumaLocalityEvidenceStatus::host_evidence_required;
    evidence.manual_memory_policy_selected = false;
    evidence.selected_memory_policy_scope = JobExecutionNumaLocalityMemoryPolicyScope::first_touch_locality;
    return evidence;
}

JobExecutionNumaFirstTouchLocalityRecipe
build_job_execution_numa_first_touch_locality_recipe(const JobExecutionNumaFirstTouchLocalityRecipeDesc& desc) {
    auto recipe = JobExecutionNumaFirstTouchLocalityRecipe{};
    recipe.worker_count = desc.worker_count;
    recipe.chunk_count = desc.chunk_count;
    recipe.first_touch_locality_default = true;

    if (desc.name.empty()) {
        append_numa_locality_evidence_diagnostic(recipe,
                                                 JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
                                                 "job execution NUMA first-touch recipe requires a non-empty name");
    }
    if (desc.workload.empty()) {
        append_numa_locality_evidence_diagnostic(recipe,
                                                 JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
                                                 "job execution NUMA first-touch recipe requires a workload name");
    }
    if (desc.worker_count == 0U || desc.chunk_count == 0U) {
        append_numa_locality_evidence_diagnostic(
            recipe, JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
            "job execution NUMA first-touch recipe requires non-zero worker and chunk counts");
    }

    if (std::ranges::find(recipe.diagnostic_codes,
                          JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration) !=
        recipe.diagnostic_codes.end()) {
        recipe.status = JobExecutionNumaLocalityEvidenceStatus::invalid_configuration;
        return recipe;
    }

    recipe.chunk_rows.reserve(desc.chunk_count);
    for (std::uint32_t chunk_id = 0; chunk_id < desc.chunk_count; ++chunk_id) {
        recipe.chunk_rows.push_back(JobExecutionNumaFirstTouchChunkRow{
            .chunk_id = chunk_id,
            .assigned_worker_id = chunk_id % desc.worker_count,
            .initialize_on_assigned_worker = true,
        });
    }
    recipe.status = JobExecutionNumaLocalityEvidenceStatus::ready;
    return recipe;
}

JobExecutionNumaMemoryPolicyComparison
compare_job_execution_numa_memory_policy(const JobExecutionNumaMemoryPolicyComparisonDesc& desc) {
    auto comparison = JobExecutionNumaMemoryPolicyComparison{};
    comparison.workload = desc.workload;
    comparison.first_touch_locality_default = true;
    comparison.manual_memory_policy_applied = false;
    comparison.recommendation = JobExecutionNumaMemoryPolicyRecommendation::keep_first_touch_default;

    if (desc.name.empty()) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
            "job execution NUMA memory policy comparison requires a non-empty name");
    }
    if (desc.workload.empty()) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration,
            "job execution NUMA memory policy comparison requires a workload name");
    }

    if (std::ranges::find(comparison.diagnostic_codes,
                          JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration) !=
        comparison.diagnostic_codes.end()) {
        comparison.status = JobExecutionNumaLocalityEvidenceStatus::invalid_configuration;
        comparison.recommendation = JobExecutionNumaMemoryPolicyRecommendation::evidence_unverifiable;
        return comparison;
    }

    if (desc.numa_node_count > 1U && !desc.numa_topology_known) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_numa_topology,
            "job execution NUMA memory policy comparison needs NUMA topology before comparing placement");
    }
    if (desc.cpuset_restrictions_observed && !desc.cpuset_restrictions_verifiable) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::cpuset_restriction_unverifiable,
            "job execution NUMA memory policy comparison cannot verify locality under observed cpuset restrictions");
    }
    if (!desc.nps_state_known || desc.nps_state.empty()) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_nps_state,
            "job execution NUMA memory policy comparison needs NPS state before comparing manual memory policy");
    }

    const bool counters_present =
        desc.local_remote_memory_counters_available && desc.first_touch_local_memory_bytes.has_value() &&
        desc.first_touch_remote_memory_bytes.has_value() && desc.manual_policy_local_memory_bytes.has_value() &&
        desc.manual_policy_remote_memory_bytes.has_value();
    if (!counters_present) {
        append_numa_locality_evidence_diagnostic(
            comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_local_remote_counters,
            "job execution NUMA memory policy comparison needs first-touch and manual local/remote memory counters");
    } else {
        const std::uint64_t first_touch_total =
            desc.first_touch_local_memory_bytes.value() + desc.first_touch_remote_memory_bytes.value();
        const std::uint64_t manual_total =
            desc.manual_policy_local_memory_bytes.value() + desc.manual_policy_remote_memory_bytes.value();
        if (first_touch_total == 0ULL || manual_total == 0ULL) {
            append_numa_locality_evidence_diagnostic(
                comparison, JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_local_remote_counters,
                "job execution NUMA memory policy comparison needs non-zero memory totals to compute locality");
        } else {
            comparison.first_touch_local_fraction_per_mille =
                static_cast<std::uint32_t>((desc.first_touch_local_memory_bytes.value() * 1000ULL) / first_touch_total);
            comparison.manual_policy_local_fraction_per_mille =
                static_cast<std::uint32_t>((desc.manual_policy_local_memory_bytes.value() * 1000ULL) / manual_total);
            comparison.locality_gain_per_mille =
                static_cast<std::int32_t>(comparison.manual_policy_local_fraction_per_mille) -
                static_cast<std::int32_t>(comparison.first_touch_local_fraction_per_mille);
        }
    }

    if (!comparison.diagnostic_codes.empty()) {
        comparison.status = JobExecutionNumaLocalityEvidenceStatus::host_evidence_required;
        comparison.recommendation = JobExecutionNumaMemoryPolicyRecommendation::evidence_unverifiable;
        comparison.first_touch_local_fraction_per_mille = 0U;
        comparison.manual_policy_local_fraction_per_mille = 0U;
        comparison.locality_gain_per_mille = 0;
        return comparison;
    }

    comparison.status = JobExecutionNumaLocalityEvidenceStatus::ready;
    if (comparison.locality_gain_per_mille > 0 &&
        static_cast<std::uint32_t>(comparison.locality_gain_per_mille) >= desc.min_locality_gain_per_mille) {
        comparison.recommendation = JobExecutionNumaMemoryPolicyRecommendation::manual_policy_followup_candidate;
    } else {
        comparison.recommendation = JobExecutionNumaMemoryPolicyRecommendation::keep_first_touch_default;
    }
    return comparison;
}

std::uint32_t observe_job_execution_logical_processor_count() noexcept {
    return std::thread::hardware_concurrency();
}

JobExecutionPoolStatus JobExecutionPool::status() const noexcept {
    return impl_->status();
}

std::uint32_t JobExecutionPool::worker_threads_started() const noexcept {
    return impl_->started_threads;
}

JobExecutionRunResult JobExecutionPool::execute(const JobExecutionBatchDesc& batch) {
    return impl_->execute(batch);
}

void JobExecutionPool::request_stop() noexcept {
    impl_->request_stop();
}

JobExecutionRunResult JobExecutionPool::stop_and_drain() {
    return impl_->stop_and_drain();
}

std::string_view job_execution_topology_policy_status_label(JobExecutionTopologyPolicyStatus status) noexcept {
    switch (status) {
    case JobExecutionTopologyPolicyStatus::ready:
        return "ready";
    case JobExecutionTopologyPolicyStatus::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionTopologyPolicyStatus::host_evidence_required:
        return "host_evidence_required";
    }
    return "unknown";
}

std::string_view
job_execution_topology_policy_diagnostic_code_label(JobExecutionTopologyPolicyDiagnosticCode code) noexcept {
    switch (code) {
    case JobExecutionTopologyPolicyDiagnosticCode::none:
        return "none";
    case JobExecutionTopologyPolicyDiagnosticCode::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionTopologyPolicyDiagnosticCode::missing_processor_group_evidence:
        return "missing_processor_group_evidence";
    case JobExecutionTopologyPolicyDiagnosticCode::missing_numa_evidence:
        return "missing_numa_evidence";
    }
    return "unknown";
}

std::string_view job_execution_placement_policy_mode_label(JobExecutionPlacementPolicyMode mode) noexcept {
    switch (mode) {
    case JobExecutionPlacementPolicyMode::os_default:
        return "os_default";
    case JobExecutionPlacementPolicyMode::prefer_local_numa:
        return "prefer_local_numa";
    case JobExecutionPlacementPolicyMode::prefer_performance_cores:
        return "prefer_performance_cores";
    case JobExecutionPlacementPolicyMode::prefer_efficiency_cores:
        return "prefer_efficiency_cores";
    case JobExecutionPlacementPolicyMode::avoid_smt_siblings:
        return "avoid_smt_siblings";
    case JobExecutionPlacementPolicyMode::manual_host_affinity:
        return "manual_host_affinity";
    }
    return "unknown";
}

std::string_view job_execution_placement_policy_status_label(JobExecutionPlacementPolicyStatus status) noexcept {
    switch (status) {
    case JobExecutionPlacementPolicyStatus::ready:
        return "ready";
    case JobExecutionPlacementPolicyStatus::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionPlacementPolicyStatus::host_evidence_required:
        return "host_evidence_required";
    }
    return "unknown";
}

std::string_view
job_execution_placement_policy_diagnostic_code_label(JobExecutionPlacementPolicyDiagnosticCode code) noexcept {
    switch (code) {
    case JobExecutionPlacementPolicyDiagnosticCode::none:
        return "none";
    case JobExecutionPlacementPolicyDiagnosticCode::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionPlacementPolicyDiagnosticCode::missing_processor_group_evidence:
        return "missing_processor_group_evidence";
    case JobExecutionPlacementPolicyDiagnosticCode::missing_numa_evidence:
        return "missing_numa_evidence";
    case JobExecutionPlacementPolicyDiagnosticCode::missing_hybrid_core_evidence:
        return "missing_hybrid_core_evidence";
    case JobExecutionPlacementPolicyDiagnosticCode::missing_smt_evidence:
        return "missing_smt_evidence";
    case JobExecutionPlacementPolicyDiagnosticCode::host_execution_required:
        return "host_execution_required";
    }
    return "unknown";
}

std::string_view
job_execution_numa_locality_memory_policy_scope_label(JobExecutionNumaLocalityMemoryPolicyScope scope) noexcept {
    switch (scope) {
    case JobExecutionNumaLocalityMemoryPolicyScope::os_default:
        return "os_default";
    case JobExecutionNumaLocalityMemoryPolicyScope::first_touch_locality:
        return "first_touch_locality";
    case JobExecutionNumaLocalityMemoryPolicyScope::manual_host_policy:
        return "manual_host_policy";
    }
    return "unknown";
}

std::string_view
job_execution_numa_locality_evidence_status_label(JobExecutionNumaLocalityEvidenceStatus status) noexcept {
    switch (status) {
    case JobExecutionNumaLocalityEvidenceStatus::ready:
        return "ready";
    case JobExecutionNumaLocalityEvidenceStatus::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionNumaLocalityEvidenceStatus::host_evidence_required:
        return "host_evidence_required";
    }
    return "unknown";
}

std::string_view job_execution_numa_locality_evidence_diagnostic_code_label(
    JobExecutionNumaLocalityEvidenceDiagnosticCode code) noexcept {
    switch (code) {
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::none:
        return "none";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_numa_topology:
        return "missing_numa_topology";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_cpu_to_node_mapping:
        return "missing_cpu_to_node_mapping";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_local_remote_counters:
        return "missing_local_remote_counters";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::missing_nps_state:
        return "missing_nps_state";
    case JobExecutionNumaLocalityEvidenceDiagnosticCode::cpuset_restriction_unverifiable:
        return "cpuset_restriction_unverifiable";
    }
    return "unknown";
}

std::string_view job_execution_numa_memory_policy_recommendation_label(
    JobExecutionNumaMemoryPolicyRecommendation recommendation) noexcept {
    switch (recommendation) {
    case JobExecutionNumaMemoryPolicyRecommendation::keep_first_touch_default:
        return "keep_first_touch_default";
    case JobExecutionNumaMemoryPolicyRecommendation::manual_policy_followup_candidate:
        return "manual_policy_followup_candidate";
    case JobExecutionNumaMemoryPolicyRecommendation::evidence_unverifiable:
        return "evidence_unverifiable";
    }
    return "unknown";
}

std::string_view job_execution_pool_status_label(JobExecutionPoolStatus status) noexcept {
    switch (status) {
    case JobExecutionPoolStatus::ready:
        return "ready";
    case JobExecutionPoolStatus::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionPoolStatus::stopped:
        return "stopped";
    }
    return "unknown";
}

std::string_view job_execution_run_status_label(JobExecutionRunStatus status) noexcept {
    switch (status) {
    case JobExecutionRunStatus::ready:
        return "ready";
    case JobExecutionRunStatus::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionRunStatus::invalid_task:
        return "invalid_task";
    case JobExecutionRunStatus::queue_overflow:
        return "queue_overflow";
    case JobExecutionRunStatus::blocked_dependency:
        return "blocked_dependency";
    case JobExecutionRunStatus::dependency_cycle:
        return "dependency_cycle";
    case JobExecutionRunStatus::stopped:
        return "stopped";
    case JobExecutionRunStatus::task_exception:
        return "task_exception";
    }
    return "unknown";
}

std::string_view job_execution_diagnostic_code_label(JobExecutionDiagnosticCode code) noexcept {
    switch (code) {
    case JobExecutionDiagnosticCode::none:
        return "none";
    case JobExecutionDiagnosticCode::invalid_configuration:
        return "invalid_configuration";
    case JobExecutionDiagnosticCode::invalid_task:
        return "invalid_task";
    case JobExecutionDiagnosticCode::queue_overflow:
        return "queue_overflow";
    case JobExecutionDiagnosticCode::blocked_dependency:
        return "blocked_dependency";
    case JobExecutionDiagnosticCode::dependency_cycle:
        return "dependency_cycle";
    case JobExecutionDiagnosticCode::stopped:
        return "stopped";
    case JobExecutionDiagnosticCode::task_exception:
        return "task_exception";
    }
    return "unknown";
}

} // namespace mirakana
