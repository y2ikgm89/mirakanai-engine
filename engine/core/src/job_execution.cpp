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
            auto worker = std::make_unique<WorkerState>(worker_id,
                                                        desc.name + ".worker." + std::to_string(worker_id) + ".scratch",
                                                        desc.scratch_budget_bytes_per_worker);
            auto* worker_ptr = worker.get();
            worker->thread = std::thread([this, worker_ptr] { run_worker(*worker_ptr); });
            workers.push_back(std::move(worker));
            ++started_threads;
        }
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
        worker.cv.notify_one();
        return true;
    }

    static void wait_group(ExecutionGroupState& group) {
        std::unique_lock lock(group.mutex);
        group.cv.wait(lock, [&group] { return group.remaining == 0; });
    }

    void run_worker(WorkerState& worker) noexcept {
        while (true) {
            auto work = WorkItem{};
            {
                std::unique_lock lock(worker.mutex);
                worker.cv.wait(lock, [&worker] {
                    return worker.stopping || worker.stop_requested->load(std::memory_order_relaxed) ||
                           !worker.queue.empty();
                });
                if ((worker.stop_requested->load(std::memory_order_relaxed) || worker.stopping) &&
                    worker.queue.empty()) {
                    return;
                }
                work = std::move(worker.queue.front());
                worker.queue.pop_front();
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
    std::vector<std::unique_ptr<WorkerState>> workers;
};

JobExecutionStopToken::JobExecutionStopToken(std::shared_ptr<const std::atomic_bool> stop_requested) noexcept
    : stop_requested_(std::move(stop_requested)) {}

bool JobExecutionStopToken::stop_requested() const noexcept {
    return stop_requested_ != nullptr && stop_requested_->load(std::memory_order_relaxed);
}

JobExecutionPool::JobExecutionPool(JobExecutionPoolDesc desc) : impl_(std::make_unique<Impl>(std::move(desc))) {}

JobExecutionPool::~JobExecutionPool() = default;

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
