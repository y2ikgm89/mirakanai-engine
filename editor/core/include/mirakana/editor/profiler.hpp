// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/core/diagnostics.hpp"
#include "mirakana/platform/file_dialog.hpp"
#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

class ITextStore;

struct EditorProfilerStatus {
    std::size_t log_records{0};
    std::size_t undo_stack{0};
    std::size_t redo_stack{0};
    std::size_t asset_imports{0};
    std::size_t hot_reload_events{0};
    std::size_t shader_compiles{0};
    bool dirty{false};
    std::uint64_t revision{0};
};

struct EditorProfilerKeyValueRow {
    std::string id;
    std::string label;
    std::string value;
};

struct EditorProfilerProfileRow {
    std::string id;
    std::string name;
    std::string duration;
    std::uint64_t frame_index{0};
    std::uint64_t start_time_ns{0};
    std::uint32_t depth{0};
};

struct EditorProfilerCounterRow {
    std::string id;
    std::string name;
    std::string value;
    std::uint64_t frame_index{0};
};

struct EditorProfilerEventRow {
    std::string id;
    std::string severity;
    std::string category;
    std::string message;
    std::uint64_t frame_index{0};
};

struct EditorProfilerTraceExportModel {
    bool can_export{false};
    std::string status_label;
    std::string format;
    std::string producer;
    std::string payload;
    std::size_t payload_bytes{0};
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTraceFileSaveRequest {
    std::string output_path{"diagnostics/editor-profiler-trace.json"};
    mirakana::DiagnosticsTraceExportOptions options;
};

struct EditorProfilerTraceFileSaveResult {
    bool saved{false};
    std::string status_label;
    std::string output_path;
    std::size_t payload_bytes{0};
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTraceSaveDialogModel {
    bool accepted{false};
    std::string status_label;
    std::string selected_path;
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTraceFileImportRequest {
    std::string input_path{"diagnostics/editor-profiler-trace.json"};
};

struct EditorProfilerTraceImportReviewModel {
    bool valid{false};
    bool capture_reconstructed{false};
    std::string status_label;
    std::size_t payload_bytes{0};
    mirakana::DiagnosticCapture capture;
    std::vector<EditorProfilerProfileRow> reconstructed_profile_rows;
    std::vector<EditorProfilerCounterRow> reconstructed_counter_rows;
    std::vector<EditorProfilerEventRow> reconstructed_event_rows;
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTraceFileImportResult {
    bool imported{false};
    bool capture_reconstructed{false};
    std::string status_label;
    std::string input_path;
    std::size_t payload_bytes{0};
    mirakana::DiagnosticCapture capture;
    EditorProfilerTraceImportReviewModel review;
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTraceOpenDialogModel {
    bool accepted{false};
    std::string status_label;
    std::string selected_path;
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerTelemetryHandoffModel {
    bool ready{false};
    std::string status_label;
    std::string kind;
    std::string producer;
    std::string format;
    std::string blocker;
    std::vector<EditorProfilerKeyValueRow> rows;
    std::vector<std::string> diagnostics;
};

struct EditorProfilerPanelModel {
    bool capture_empty{true};
    std::string trace_status;
    std::vector<EditorProfilerKeyValueRow> status_rows;
    std::vector<EditorProfilerKeyValueRow> summary_rows;
    std::vector<EditorProfilerProfileRow> profile_rows;
    std::vector<EditorProfilerCounterRow> counter_rows;
    std::vector<EditorProfilerEventRow> event_rows;
    EditorProfilerTraceExportModel trace_export;
    EditorProfilerTraceFileSaveResult trace_file_save;
    EditorProfilerTraceSaveDialogModel trace_save_dialog;
    EditorProfilerTraceFileImportResult trace_file_import;
    EditorProfilerTraceOpenDialogModel trace_open_dialog;
    EditorProfilerTraceImportReviewModel trace_import;
    EditorProfilerTelemetryHandoffModel telemetry;
};

[[nodiscard]] EditorProfilerTraceExportModel
make_editor_profiler_trace_export_model(const mirakana::DiagnosticCapture& capture,
                                        const mirakana::DiagnosticsTraceExportOptions& options = {});
[[nodiscard]] EditorProfilerTelemetryHandoffModel
make_editor_profiler_telemetry_handoff_model(const mirakana::DiagnosticCapture& capture,
                                             const mirakana::DiagnosticsOpsPlanOptions& options = {});
[[nodiscard]] EditorProfilerTraceImportReviewModel
make_editor_profiler_trace_import_review_model(std::string_view payload);
[[nodiscard]] EditorProfilerTraceFileSaveResult
save_editor_profiler_trace_json(ITextStore& store, const mirakana::DiagnosticCapture& capture,
                                const EditorProfilerTraceFileSaveRequest& request = {});
[[nodiscard]] mirakana::FileDialogRequest make_editor_profiler_trace_save_dialog_request(
    std::string_view default_location = "diagnostics/editor-profiler-trace.json");
[[nodiscard]] EditorProfilerTraceSaveDialogModel
make_editor_profiler_trace_save_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorProfilerTraceFileImportResult
import_editor_profiler_trace_json(ITextStore& store, const EditorProfilerTraceFileImportRequest& request = {});
[[nodiscard]] mirakana::FileDialogRequest
make_editor_profiler_trace_open_dialog_request(std::string_view default_location = "diagnostics");
[[nodiscard]] EditorProfilerTraceOpenDialogModel
make_editor_profiler_trace_open_dialog_model(const mirakana::FileDialogResult& result);
[[nodiscard]] EditorProfilerPanelModel make_editor_profiler_panel_model(const mirakana::DiagnosticCapture& capture,
                                                                        const EditorProfilerStatus& status);
[[nodiscard]] mirakana::ui::UiDocument make_profiler_ui_model(const EditorProfilerPanelModel& model);

} // namespace mirakana::editor
