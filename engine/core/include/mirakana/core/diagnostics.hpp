// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
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
[[nodiscard]] std::string_view diagnostics_ops_artifact_kind_label(DiagnosticsOpsArtifactKind kind) noexcept;
[[nodiscard]] std::string_view diagnostics_ops_artifact_status_label(DiagnosticsOpsArtifactStatus status) noexcept;
[[nodiscard]] DiagnosticsOpsPlan build_diagnostics_ops_plan(const DiagnosticCapture& capture,
                                                            const DiagnosticsOpsPlanOptions& options = {});
[[nodiscard]] std::string export_diagnostics_trace_json(const DiagnosticCapture& capture,
                                                        const DiagnosticsTraceExportOptions& options = {});
[[nodiscard]] DiagnosticsTraceImportReview review_diagnostics_trace_json(std::string_view json);
[[nodiscard]] DiagnosticsTraceImportResult import_diagnostics_trace_json(std::string_view json);

} // namespace mirakana
