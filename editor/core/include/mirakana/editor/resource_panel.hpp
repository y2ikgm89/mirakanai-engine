// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

struct EditorResourceCounterInput {
    std::string id;
    std::string label;
    std::uint64_t value{0};
};

struct EditorResourceMemoryInput {
    bool os_video_memory_budget_available{false};
    std::uint64_t local_video_memory_budget_bytes{0};
    std::uint64_t local_video_memory_usage_bytes{0};
    std::uint64_t non_local_video_memory_budget_bytes{0};
    std::uint64_t non_local_video_memory_usage_bytes{0};
    bool committed_resources_byte_estimate_available{false};
    std::uint64_t committed_resources_byte_estimate{0};
};

struct EditorResourceLifetimeInput {
    std::uint64_t live_resources{0};
    std::uint64_t deferred_release_resources{0};
    std::uint64_t lifetime_events{0};
    std::vector<EditorResourceCounterInput> resources_by_kind;
};

struct EditorResourceCaptureRequestInput {
    std::string id;
    std::string label;
    std::string tool_label;
    std::string action_label;
    std::vector<std::string> host_gates;
    bool available{true};
    bool acknowledged{false};
    std::string diagnostic;
};

struct EditorResourceCaptureExecutionInput {
    std::string id;
    std::string label;
    std::string tool_label;
    bool requested{false};
    bool host_gated{false};
    bool capture_started{false};
    bool capture_completed{false};
    bool capture_failed{false};
    bool editor_core_executed{false};
    bool exposes_native_handles{false};
    /// Host gates mirrored from the paired capture request for MK_ui symmetry (bounded when materialized).
    std::vector<std::string> host_gates;
    std::string artifact_path;
    std::string diagnostic;
};

struct EditorResourcePanelInput {
    bool device_available{false};
    std::string backend_id;
    std::string backend_label;
    std::uint64_t frame_index{0};
    std::vector<EditorResourceCounterInput> rhi_counters;
    EditorResourceMemoryInput memory;
    EditorResourceLifetimeInput lifetime;
    std::vector<EditorResourceCaptureRequestInput> capture_requests;
    std::vector<EditorResourceCaptureExecutionInput> capture_execution_snapshots;
};

struct EditorResourceRow {
    std::string id;
    std::string label;
    std::string value;
    bool available{true};
};

struct EditorResourceCaptureRequestRow {
    std::string id;
    std::string label;
    std::string tool_label;
    std::string action_label;
    std::vector<std::string> host_gates;
    std::string host_gates_label;
    bool request_available{false};
    bool request_acknowledged{false};
    bool host_gate_acknowledgement_required{false};
    std::string acknowledgement_label;
    std::string status_label;
    std::string diagnostic;
};

struct EditorResourceCaptureExecutionRow {
    std::string id;
    std::string label;
    std::string tool_label;
    /// Stable machine slug aligned with `status_label` (`blocked`, `failed`, `captured`, `running`,
    /// `host_gated`, `requested`, `not_requested`).
    std::string phase_code;
    std::string status_label;
    std::vector<std::string> host_gates;
    std::string host_gates_label;
    /// Sanitized paths/diagnostics truncated for retained MK_ui and agent contracts.
    std::string artifact_path;
    std::string diagnostic;
};

struct EditorResourcePanelModel {
    bool device_available{false};
    std::string status;
    std::vector<EditorResourceRow> status_rows;
    std::vector<EditorResourceRow> counter_rows;
    std::vector<EditorResourceRow> memory_rows;
    std::vector<EditorResourceRow> lifetime_rows;
    std::vector<EditorResourceCaptureRequestRow> capture_request_rows;
    std::vector<EditorResourceCaptureExecutionRow> capture_execution_rows;
};

[[nodiscard]] EditorResourcePanelModel make_editor_resource_panel_model(const EditorResourcePanelInput& input);
[[nodiscard]] mirakana::ui::UiDocument make_resource_panel_ui_model(const EditorResourcePanelModel& model);

/// Retained MK_ui / agent contract root for `resources.capture_execution` (Editor Resource Capture Execution Evidence
/// v1).
[[nodiscard]] constexpr std::string_view editor_resources_capture_execution_contract_v1() noexcept {
    return std::string_view{"ge.editor.resources_capture_execution.v1"};
}

/// Retained MK_ui contract for operator-confirmed PIX helper launch (Resources capture execution stream).
[[nodiscard]] constexpr std::string_view
editor_resources_capture_operator_validated_launch_workflow_contract_v1() noexcept {
    return std::string_view{"ge.editor.resources_capture_operator_validated_launch_workflow.v1"};
}

} // namespace mirakana::editor
