// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

enum class DiagnosticSeverity : std::uint8_t { trace, debug, info, warning, error, fatal };

struct DiagnosticEvent {
    DiagnosticSeverity severity{DiagnosticSeverity::info};
    std::string category;
    std::string message;
    std::uint64_t frame_index{0};
};

struct CounterSample {
    std::string name;
    double value{0.0};
    std::uint64_t frame_index{0};
};

struct ProfileSample {
    std::string name;
    std::uint64_t frame_index{0};
    std::uint64_t start_time_ns{0};
    std::uint64_t duration_ns{0};
    std::uint32_t depth{0};
};

struct DiagnosticCapture {
    std::vector<DiagnosticEvent> events;
    std::vector<CounterSample> counters;
    std::vector<ProfileSample> profiles;
};

struct DiagnosticSummary {
    std::uint64_t event_count{0};
    std::uint64_t warning_count{0};
    std::uint64_t error_count{0};
    std::uint64_t counter_count{0};
    std::uint64_t profile_count{0};
    std::uint64_t total_profile_time_ns{0};
    std::uint64_t min_profile_time_ns{0};
    std::uint64_t max_profile_time_ns{0};
};

enum class DiagnosticsBudgetStatus : std::uint8_t {
    ready,
    missing_samples,
    invalid_samples,
    invalid_thresholds,
    threshold_exceeded
};

struct DiagnosticsBudgetThresholds {
    std::uint64_t minimum_sample_count{1};
    double maximum_average{std::numeric_limits<double>::infinity()};
    double maximum_p95{std::numeric_limits<double>::infinity()};
    double maximum_p99{std::numeric_limits<double>::infinity()};
    double maximum_sample{std::numeric_limits<double>::infinity()};
};

struct DiagnosticsBudgetSummary {
    std::string sample_name;
    std::uint64_t count{0};
    std::uint64_t non_finite_sample_count{0};
    double min{0.0};
    double average{0.0};
    // p95 and p99 use nearest-rank percentiles over sorted finite samples.
    double p95{0.0};
    double p99{0.0};
    double max{0.0};
    DiagnosticsBudgetStatus status{DiagnosticsBudgetStatus::missing_samples};
    std::vector<std::string> diagnostics;
};

enum class MemoryLifetimeClass : std::uint8_t {
    frame_temporary,
    worker_scratch,
    persistent_cpu,
    package_resident_cpu,
    upload_staging,
    resident_gpu,
    transient_gpu,
    readback,
    editor_tooling
};

enum class MemoryBudgetPressure : std::uint8_t { none, nominal, warning, exceeded };

enum class MemoryDiagnosticsCode : std::uint8_t {
    none,
    invalid_counter,
    stale_generation,
    use_after_safe_point,
    cross_thread_free,
    false_sharing,
    budget_pressure,
    budget_exceeded
};

enum class MemoryDiagnosticsStatus : std::uint8_t {
    ready,
    missing_rows,
    invalid_rows,
    budget_pressure,
    budget_exceeded
};

struct MemoryCounterRow {
    MemoryLifetimeClass lifetime_class{MemoryLifetimeClass::frame_temporary};
    std::string name;
    std::uint64_t bytes{0};
    std::uint64_t allocation_count{0};
    std::uint64_t high_water_bytes{0};
    std::uint64_t budget_bytes{0};
    std::uint64_t generation{0};
    std::uint64_t safe_point_generation{0};
    std::uint64_t frame_index{0};
    std::uint64_t reuse_count{0};
    std::uint64_t reset_count{0};
    std::uint64_t cross_thread_free_count{0};
    std::uint64_t use_after_safe_point_count{0};
    std::uint64_t false_sharing_count{0};
    bool use_after_safe_point{false};
};

struct MemoryClassDiagnosticsSummary {
    MemoryLifetimeClass lifetime_class{MemoryLifetimeClass::frame_temporary};
    std::uint64_t row_count{0};
    std::uint64_t bytes{0};
    std::uint64_t allocation_count{0};
    std::uint64_t reuse_count{0};
    std::uint64_t reset_count{0};
    std::uint64_t high_water_bytes{0};
    std::uint64_t budget_bytes{0};
    std::uint64_t cross_thread_free_count{0};
    std::uint64_t use_after_safe_point_count{0};
    std::uint64_t false_sharing_count{0};
    double budget_pressure_ratio{0.0};
    MemoryBudgetPressure pressure{MemoryBudgetPressure::none};
    std::vector<MemoryDiagnosticsCode> diagnostic_codes;
};

struct MemoryDiagnosticsOptions {
    double budget_pressure_warning_ratio{0.9};
};

struct MemoryDiagnosticsSummary {
    std::uint64_t row_count{0};
    std::uint64_t total_bytes{0};
    std::uint64_t total_allocation_count{0};
    std::uint64_t total_reuse_count{0};
    std::uint64_t total_reset_count{0};
    std::uint64_t high_water_bytes{0};
    std::uint64_t total_cross_thread_free_count{0};
    std::uint64_t total_use_after_safe_point_count{0};
    std::uint64_t total_false_sharing_count{0};
    MemoryDiagnosticsStatus status{MemoryDiagnosticsStatus::missing_rows};
    std::vector<MemoryClassDiagnosticsSummary> class_summaries;
    std::vector<MemoryDiagnosticsCode> diagnostic_codes;
    std::vector<std::string> diagnostics;
};

enum class JobSchedulingDiagnosticsCode : std::uint8_t {
    none,
    missing_rows,
    invalid_worker_topology,
    missing_processor_group_evidence,
    missing_numa_evidence,
    invalid_queue,
    queue_overflow,
    blocked_dependency,
    dependency_cycle,
    scratch_misuse,
    nondeterministic_merge,
    undersized_job_batch,
    oversized_job_batch
};

enum class JobSchedulingDiagnosticsStatus : std::uint8_t { ready, missing_rows, invalid_rows };

struct JobWorkerTopologyRow {
    std::string name;
    std::uint32_t logical_processor_count{0};
    std::uint32_t worker_count{0};
    std::uint32_t queue_count{0};
    std::uint32_t processor_group_count{1};
    std::uint32_t numa_node_count{0};
    bool processor_groups_accounted_for{false};
    bool numa_topology_known{false};
};

struct JobQueueCounterRow {
    std::string name;
    std::uint32_t worker_id{0};
    std::uint64_t submitted_jobs{0};
    std::uint64_t completed_jobs{0};
    std::uint64_t queue_capacity{0};
    std::uint64_t queue_depth_high_water{0};
    std::uint64_t queue_overflow_count{0};
    std::uint64_t steal_attempt_count{0};
    std::uint64_t steal_success_count{0};
    std::uint64_t worker_wait_count{0};
    std::uint64_t blocked_dependency_count{0};
    std::uint64_t dependency_cycle_count{0};
    std::uint64_t deterministic_merge_count{0};
    std::uint64_t nondeterministic_merge_count{0};
    std::uint64_t scratch_bytes{0};
    std::uint64_t scratch_high_water_bytes{0};
    std::uint64_t scratch_misuse_count{0};
    std::uint64_t undersized_job_batch_count{0};
    std::uint64_t oversized_job_batch_count{0};
    std::uint64_t frame_index{0};
};

struct JobSchedulingDiagnosticsSummary {
    std::uint64_t worker_topology_row_count{0};
    std::uint64_t queue_row_count{0};
    std::uint32_t logical_processor_count{0};
    std::uint32_t worker_count{0};
    std::uint32_t queue_count{0};
    std::uint32_t processor_group_count{0};
    std::uint32_t numa_node_count{0};
    std::uint64_t total_submitted_jobs{0};
    std::uint64_t total_completed_jobs{0};
    std::uint64_t total_queue_capacity{0};
    std::uint64_t queue_depth_high_water{0};
    std::uint64_t total_queue_overflow_count{0};
    std::uint64_t total_steal_attempt_count{0};
    std::uint64_t total_steal_success_count{0};
    std::uint64_t total_worker_wait_count{0};
    std::uint64_t total_blocked_dependency_count{0};
    std::uint64_t total_dependency_cycle_count{0};
    std::uint64_t total_deterministic_merge_count{0};
    std::uint64_t total_nondeterministic_merge_count{0};
    std::uint64_t total_scratch_bytes{0};
    std::uint64_t total_scratch_high_water_bytes{0};
    std::uint64_t total_scratch_misuse_count{0};
    std::uint64_t total_undersized_job_batch_count{0};
    std::uint64_t total_oversized_job_batch_count{0};
    JobSchedulingDiagnosticsStatus status{JobSchedulingDiagnosticsStatus::missing_rows};
    std::vector<JobSchedulingDiagnosticsCode> diagnostic_codes;
    std::vector<std::string> diagnostics;
};

struct DiagnosticsTraceExportOptions {
    std::string trace_name{"GameEngine Diagnostics"};
    std::string thread_name{"main"};
    std::uint32_t process_id{1};
    std::uint32_t thread_id{0};
    bool include_metadata{true};
};

struct DiagnosticsTraceImportReview {
    bool valid{false};
    std::size_t payload_bytes{0};
    std::uint64_t trace_event_count{0};
    std::uint64_t metadata_event_count{0};
    std::uint64_t instant_event_count{0};
    std::uint64_t counter_event_count{0};
    std::uint64_t profile_event_count{0};
    std::uint64_t unknown_event_count{0};
    std::vector<std::string> diagnostics;
};

struct DiagnosticsTraceImportResult {
    bool valid{false};
    DiagnosticsTraceImportReview review;
    DiagnosticCapture capture;
    std::vector<std::string> diagnostics;
};

enum class DiagnosticsOpsArtifactKind : std::uint8_t { summary, trace_event_json, crash_dump_review, telemetry_upload };

enum class DiagnosticsOpsArtifactStatus : std::uint8_t { ready, host_gated, unsupported };

struct DiagnosticsOpsHostStatus {
    bool debugging_tools_for_windows_available{false};
    bool telemetry_backend_configured{false};
};

struct DiagnosticsOpsPlanOptions {
    DiagnosticsOpsHostStatus host_status;
};

struct DiagnosticsOpsArtifact {
    DiagnosticsOpsArtifactKind kind{DiagnosticsOpsArtifactKind::summary};
    DiagnosticsOpsArtifactStatus status{DiagnosticsOpsArtifactStatus::unsupported};
    std::string id;
    std::string label;
    std::string producer;
    std::string format;
    std::string blocker;
    std::uint64_t event_count{0};
    std::uint64_t counter_count{0};
    std::uint64_t profile_count{0};
};

struct DiagnosticsOpsPlan {
    DiagnosticSummary summary;
    std::vector<DiagnosticsOpsArtifact> artifacts;
};

class IProfileClock {
  public:
    virtual ~IProfileClock() = default;

    [[nodiscard]] virtual std::uint64_t now_ns() const noexcept = 0;
};

class ManualProfileClock final : public IProfileClock {
  public:
    explicit ManualProfileClock(std::uint64_t initial_time_ns = 0) noexcept;

    [[nodiscard]] std::uint64_t now_ns() const noexcept override;
    void set(std::uint64_t time_ns) noexcept;
    void advance(std::uint64_t delta_ns) noexcept;

  private:
    std::uint64_t time_ns_{0};
};

class SteadyProfileClock final : public IProfileClock {
  public:
    [[nodiscard]] std::uint64_t now_ns() const noexcept override;
};

class DiagnosticsRecorder {
  public:
    explicit DiagnosticsRecorder(std::size_t capacity = 1024);

    void record_event(DiagnosticEvent event);
    void record_counter(CounterSample sample);
    void record_profile_sample(ProfileSample sample);

    [[nodiscard]] DiagnosticCapture snapshot() const;
    [[nodiscard]] std::size_t capacity() const noexcept;
    [[nodiscard]] std::uint32_t current_profile_depth() const noexcept;

    std::uint32_t begin_profile_zone() noexcept;
    void end_profile_zone() noexcept;
    void clear() noexcept;

  private:
    [[nodiscard]] static bool valid_label(std::string_view label) noexcept;
    void record_internal_warning(std::string message);

    std::size_t capacity_{0};
    std::vector<DiagnosticEvent> events_;
    std::vector<CounterSample> counters_;
    std::vector<ProfileSample> profiles_;
    std::uint32_t profile_depth_{0};
};

class ScopedProfileZone {
  public:
    ScopedProfileZone(DiagnosticsRecorder& recorder, const IProfileClock& clock, std::string name,
                      std::uint64_t frame_index);
    ScopedProfileZone(const ScopedProfileZone&) = delete;
    ScopedProfileZone& operator=(const ScopedProfileZone&) = delete;
    ScopedProfileZone(ScopedProfileZone&& other) noexcept;
    ScopedProfileZone& operator=(ScopedProfileZone&& other) noexcept;
    ~ScopedProfileZone() noexcept;

  private:
    void close() noexcept;

    DiagnosticsRecorder* recorder_{nullptr};
    const IProfileClock* clock_{nullptr};
    std::string name_;
    std::uint64_t frame_index_{0};
    std::uint64_t start_time_ns_{0};
    std::uint32_t depth_{0};
    bool active_{false};
};

[[nodiscard]] DiagnosticSummary summarize_diagnostics(const DiagnosticCapture& capture) noexcept;
[[nodiscard]] std::string_view diagnostic_severity_label(DiagnosticSeverity severity) noexcept;
[[nodiscard]] std::string_view diagnostics_budget_status_label(DiagnosticsBudgetStatus status) noexcept;
[[nodiscard]] std::string_view memory_lifetime_class_label(MemoryLifetimeClass lifetime_class) noexcept;
[[nodiscard]] std::string_view memory_budget_pressure_label(MemoryBudgetPressure pressure) noexcept;
[[nodiscard]] std::string_view memory_diagnostics_code_label(MemoryDiagnosticsCode code) noexcept;
[[nodiscard]] std::string_view memory_diagnostics_status_label(MemoryDiagnosticsStatus status) noexcept;
[[nodiscard]] std::string_view job_scheduling_diagnostics_code_label(JobSchedulingDiagnosticsCode code) noexcept;
[[nodiscard]] std::string_view job_scheduling_diagnostics_status_label(JobSchedulingDiagnosticsStatus status) noexcept;
[[nodiscard]] DiagnosticsBudgetSummary summarize_counter_budget(std::span<const CounterSample> counters,
                                                                std::string_view sample_name,
                                                                const DiagnosticsBudgetThresholds& thresholds = {});
[[nodiscard]] DiagnosticsBudgetSummary summarize_profile_budget(std::span<const ProfileSample> profiles,
                                                                std::string_view sample_name,
                                                                const DiagnosticsBudgetThresholds& thresholds = {});
[[nodiscard]] MemoryDiagnosticsSummary summarize_memory_diagnostics(std::span<const MemoryCounterRow> rows,
                                                                    const MemoryDiagnosticsOptions& options = {});
[[nodiscard]] JobSchedulingDiagnosticsSummary
summarize_job_scheduling_diagnostics(std::span<const JobWorkerTopologyRow> topology_rows,
                                     std::span<const JobQueueCounterRow> queue_rows);
[[nodiscard]] std::string_view diagnostics_ops_artifact_kind_label(DiagnosticsOpsArtifactKind kind) noexcept;
[[nodiscard]] std::string_view diagnostics_ops_artifact_status_label(DiagnosticsOpsArtifactStatus status) noexcept;
[[nodiscard]] DiagnosticsOpsPlan build_diagnostics_ops_plan(const DiagnosticCapture& capture,
                                                            const DiagnosticsOpsPlanOptions& options = {});
[[nodiscard]] std::string export_diagnostics_trace_json(const DiagnosticCapture& capture,
                                                        const DiagnosticsTraceExportOptions& options = {});
[[nodiscard]] DiagnosticsTraceImportReview review_diagnostics_trace_json(std::string_view json);
[[nodiscard]] DiagnosticsTraceImportResult import_diagnostics_trace_json(std::string_view json);

} // namespace mirakana
