// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/profiler.hpp"

#include "mirakana/editor/io.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

template <typename T> [[nodiscard]] std::string format_integer(T value) {
    return std::to_string(value);
}

[[nodiscard]] std::string format_bool(bool value) {
    return value ? "yes" : "no";
}

[[nodiscard]] std::string format_duration(std::uint64_t duration_ns) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(3) << static_cast<double>(duration_ns) / 1'000'000.0;
    return out.str() + " ms";
}

[[nodiscard]] std::string trim_fixed(std::string value) {
    while (value.size() > 1U && value.back() == '0') {
        value.pop_back();
    }
    if (!value.empty() && value.back() == '.') {
        value.pop_back();
    }
    return value;
}

[[nodiscard]] std::string format_counter(double value) {
    if (!std::isfinite(value)) {
        return "-";
    }

    constexpr double fixed_format_limit = 1'000'000'000.0;
    if (std::fabs(value) <= fixed_format_limit) {
        std::ostringstream out;
        out << std::fixed << std::setprecision(3) << value;
        return trim_fixed(out.str());
    }

    std::ostringstream out;
    out << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return out.str();
}

[[nodiscard]] bool counter_less(const mirakana::CounterSample& lhs, const mirakana::CounterSample& rhs) {
    if (lhs.frame_index != rhs.frame_index) {
        return lhs.frame_index < rhs.frame_index;
    }
    if (lhs.name != rhs.name) {
        return lhs.name < rhs.name;
    }
    if (lhs.value < rhs.value) {
        return true;
    }
    if (rhs.value < lhs.value) {
        return false;
    }
    return static_cast<int>(std::signbit(lhs.value)) < static_cast<int>(std::signbit(rhs.value));
}

[[nodiscard]] std::string row_index_id(std::string_view prefix, std::size_t index) {
    return std::string(prefix) + "." + std::to_string(index);
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor profiler ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

[[nodiscard]] mirakana::ui::ElementDesc make_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_child(std::string id, mirakana::ui::ElementId parent,
                                                   mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

void append_key_value(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                      const std::string& prefix, const EditorProfilerKeyValueRow& row) {
    mirakana::ui::ElementDesc item = make_child(prefix + "." + row.id, parent, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(row.label);
    add_or_throw(document, std::move(item));

    const mirakana::ui::ElementId item_id{prefix + "." + row.id};
    append_label(document, item_id, prefix + "." + row.id + ".label", row.label);
    append_label(document, item_id, prefix + "." + row.id + ".value", row.value);
}

void append_profile_row_list(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                             const std::string& prefix, std::span<const EditorProfilerProfileRow> rows) {
    add_or_throw(document, make_child(prefix, parent, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_id{prefix};
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& row = rows[index];
        const auto item_prefix = prefix + "." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.name);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".name", row.name);
        append_label(document, item_id, item_prefix + ".duration", row.duration);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }
}

void append_counter_row_list(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                             const std::string& prefix, std::span<const EditorProfilerCounterRow> rows) {
    add_or_throw(document, make_child(prefix, parent, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_id{prefix};
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& row = rows[index];
        const auto item_prefix = prefix + "." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.name);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".name", row.name);
        append_label(document, item_id, item_prefix + ".value", row.value);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }
}

void append_event_row_list(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                           const std::string& prefix, std::span<const EditorProfilerEventRow> rows) {
    add_or_throw(document, make_child(prefix, parent, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_id{prefix};
    for (std::size_t index = 0; index < rows.size(); ++index) {
        const auto& row = rows[index];
        const auto item_prefix = prefix + "." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, list_id, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.message);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".severity", row.severity);
        append_label(document, item_id, item_prefix + ".category", row.category);
        append_label(document, item_id, item_prefix + ".message", row.message);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }
}

[[nodiscard]] bool is_absolute_like(std::string_view path) noexcept {
    return path.starts_with('/') || path.starts_with('\\') ||
           (path.size() >= 2U && path[1] == ':' &&
            ((path[0] >= 'A' && path[0] <= 'Z') || (path[0] >= 'a' && path[0] <= 'z')));
}

[[nodiscard]] bool has_parent_segment(std::string_view path) noexcept {
    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto separator = path.find_first_of("/\\", begin);
        const auto end = separator == std::string_view::npos ? path.size() : separator;
        if (path.substr(begin, end - begin) == "..") {
            return true;
        }
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return false;
}

[[nodiscard]] std::string validate_trace_json_project_path(std::string_view path, std::string_view subject) {
    if (path.empty()) {
        return std::string(subject) + " must not be empty";
    }
    if (path.find_first_of("\r\n=") != std::string_view::npos) {
        return std::string(subject) + " must not contain newlines or '='";
    }
    if (is_absolute_like(path)) {
        return std::string(subject) + " must be project-relative";
    }
    if (has_parent_segment(path)) {
        return std::string(subject) + " must not contain '..'";
    }
    if (!path.ends_with(".json")) {
        return std::string(subject) + " must end with .json";
    }
    return {};
}

void append_trace_file_save_rows(EditorProfilerTraceFileSaveResult& result) {
    result.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = result.status_label},
        EditorProfilerKeyValueRow{.id = "output_path", .label = "Output path", .value = result.output_path},
        EditorProfilerKeyValueRow{.id = "format", .label = "Format", .value = "Chrome Trace Event JSON"},
        EditorProfilerKeyValueRow{
            .id = "payload_bytes", .label = "Payload bytes", .value = format_integer(result.payload_bytes)},
    };
}

void append_trace_import_rows(EditorProfilerTraceImportReviewModel& model,
                              const mirakana::DiagnosticsTraceImportReview& review) {
    model.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorProfilerKeyValueRow{.id = "format", .label = "Format", .value = "Chrome Trace Event JSON"},
        EditorProfilerKeyValueRow{
            .id = "payload_bytes", .label = "Payload bytes", .value = format_integer(model.payload_bytes)},
        EditorProfilerKeyValueRow{.id = "events", .label = "Events", .value = format_integer(review.trace_event_count)},
        EditorProfilerKeyValueRow{
            .id = "metadata", .label = "Metadata", .value = format_integer(review.metadata_event_count)},
        EditorProfilerKeyValueRow{
            .id = "instant_events", .label = "Instant events", .value = format_integer(review.instant_event_count)},
        EditorProfilerKeyValueRow{
            .id = "counters", .label = "Counters", .value = format_integer(review.counter_event_count)},
        EditorProfilerKeyValueRow{
            .id = "profiles", .label = "Profiles", .value = format_integer(review.profile_event_count)},
        EditorProfilerKeyValueRow{
            .id = "unknown_events", .label = "Unknown events", .value = format_integer(review.unknown_event_count)},
        EditorProfilerKeyValueRow{.id = "reconstructed_event_count",
                                  .label = "Reconstructed events",
                                  .value = format_integer(model.capture.events.size())},
        EditorProfilerKeyValueRow{.id = "reconstructed_counter_count",
                                  .label = "Reconstructed counters",
                                  .value = format_integer(model.capture.counters.size())},
        EditorProfilerKeyValueRow{.id = "reconstructed_profile_count",
                                  .label = "Reconstructed profiles",
                                  .value = format_integer(model.capture.profiles.size())},
    };
}

[[nodiscard]] std::string row_value(std::string_view value) {
    return value.empty() ? "-" : sanitize_text(value);
}

[[nodiscard]] std::string trace_import_row_value(const EditorProfilerTraceImportReviewModel& model,
                                                 std::string_view row_id) {
    const auto it =
        std::ranges::find_if(model.rows, [row_id](const EditorProfilerKeyValueRow& row) { return row.id == row_id; });
    return it == model.rows.end() ? "-" : row_value(it->value);
}

void append_trace_file_import_rows(EditorProfilerTraceFileImportResult& result) {
    result.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = result.status_label},
        EditorProfilerKeyValueRow{.id = "input_path", .label = "Input path", .value = result.input_path},
        EditorProfilerKeyValueRow{.id = "format", .label = "Format", .value = "Chrome Trace Event JSON"},
        EditorProfilerKeyValueRow{
            .id = "payload_bytes", .label = "Payload bytes", .value = format_integer(result.payload_bytes)},
        EditorProfilerKeyValueRow{
            .id = "review_status", .label = "Review status", .value = row_value(result.review.status_label)},
        EditorProfilerKeyValueRow{
            .id = "events", .label = "Events", .value = trace_import_row_value(result.review, "events")},
        EditorProfilerKeyValueRow{
            .id = "metadata", .label = "Metadata", .value = trace_import_row_value(result.review, "metadata")},
        EditorProfilerKeyValueRow{.id = "instant_events",
                                  .label = "Instant events",
                                  .value = trace_import_row_value(result.review, "instant_events")},
        EditorProfilerKeyValueRow{
            .id = "counters", .label = "Counters", .value = trace_import_row_value(result.review, "counters")},
        EditorProfilerKeyValueRow{
            .id = "profiles", .label = "Profiles", .value = trace_import_row_value(result.review, "profiles")},
        EditorProfilerKeyValueRow{.id = "unknown_events",
                                  .label = "Unknown events",
                                  .value = trace_import_row_value(result.review, "unknown_events")},
        EditorProfilerKeyValueRow{.id = "reconstructed_event_count",
                                  .label = "Reconstructed events",
                                  .value = format_integer(result.capture.events.size())},
        EditorProfilerKeyValueRow{.id = "reconstructed_counter_count",
                                  .label = "Reconstructed counters",
                                  .value = format_integer(result.capture.counters.size())},
        EditorProfilerKeyValueRow{.id = "reconstructed_profile_count",
                                  .label = "Reconstructed profiles",
                                  .value = format_integer(result.capture.profiles.size())},
    };
}

[[nodiscard]] std::vector<EditorProfilerProfileRow> make_profile_rows(const mirakana::DiagnosticCapture& capture) {
    auto profiles = capture.profiles;
    std::ranges::sort(profiles, [](const mirakana::ProfileSample& lhs, const mirakana::ProfileSample& rhs) {
        return std::tie(lhs.frame_index, lhs.start_time_ns, lhs.depth, lhs.name, lhs.duration_ns) <
               std::tie(rhs.frame_index, rhs.start_time_ns, rhs.depth, rhs.name, rhs.duration_ns);
    });

    std::vector<EditorProfilerProfileRow> rows;
    rows.reserve(profiles.size());
    for (std::size_t index = 0; index < profiles.size(); ++index) {
        const auto& profile = profiles[index];
        rows.push_back(EditorProfilerProfileRow{
            .id = row_index_id("profile", index + 1U),
            .name = sanitize_text(profile.name),
            .duration = format_duration(profile.duration_ns),
            .frame_index = profile.frame_index,
            .start_time_ns = profile.start_time_ns,
            .depth = profile.depth,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<EditorProfilerCounterRow> make_counter_rows(const mirakana::DiagnosticCapture& capture) {
    auto counters = capture.counters;
    std::ranges::sort(counters, counter_less);

    std::vector<EditorProfilerCounterRow> rows;
    rows.reserve(counters.size());
    for (std::size_t index = 0; index < counters.size(); ++index) {
        const auto& counter = counters[index];
        rows.push_back(EditorProfilerCounterRow{
            .id = row_index_id("counter", index + 1U),
            .name = sanitize_text(counter.name),
            .value = format_counter(counter.value),
            .frame_index = counter.frame_index,
        });
    }
    return rows;
}

[[nodiscard]] std::vector<EditorProfilerEventRow> make_event_rows(const mirakana::DiagnosticCapture& capture) {
    auto events = capture.events;
    std::ranges::sort(events, [](const mirakana::DiagnosticEvent& lhs, const mirakana::DiagnosticEvent& rhs) {
        return std::tie(lhs.frame_index, lhs.severity, lhs.category, lhs.message) <
               std::tie(rhs.frame_index, rhs.severity, rhs.category, rhs.message);
    });

    std::vector<EditorProfilerEventRow> rows;
    rows.reserve(events.size());
    for (std::size_t index = 0; index < events.size(); ++index) {
        const auto& event = events[index];
        rows.push_back(EditorProfilerEventRow{
            .id = row_index_id("event", index + 1U),
            .severity = std::string(mirakana::diagnostic_severity_label(event.severity)),
            .category = sanitize_text(event.category),
            .message = sanitize_text(event.message),
            .frame_index = event.frame_index,
        });
    }
    return rows;
}

[[nodiscard]] std::string selected_filter_value(int selected_filter) {
    return selected_filter < 0 ? "-" : format_integer(selected_filter);
}

void append_trace_open_dialog_rows(EditorProfilerTraceOpenDialogModel& model, std::size_t selected_count,
                                   int selected_filter) {
    model.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorProfilerKeyValueRow{
            .id = "selected_path", .label = "Selected path", .value = row_value(model.selected_path)},
        EditorProfilerKeyValueRow{
            .id = "selected_count", .label = "Selected count", .value = format_integer(selected_count)},
        EditorProfilerKeyValueRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };
}

void append_trace_save_dialog_rows(EditorProfilerTraceSaveDialogModel& model, std::size_t selected_count,
                                   int selected_filter) {
    model.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorProfilerKeyValueRow{
            .id = "selected_path", .label = "Selected path", .value = row_value(model.selected_path)},
        EditorProfilerKeyValueRow{
            .id = "selected_count", .label = "Selected count", .value = format_integer(selected_count)},
        EditorProfilerKeyValueRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };
}

void append_telemetry_rows(EditorProfilerTelemetryHandoffModel& model, std::uint64_t event_count,
                           std::uint64_t counter_count, std::uint64_t profile_count) {
    model.rows = {
        EditorProfilerKeyValueRow{.id = "kind", .label = "Kind", .value = row_value(model.kind)},
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = row_value(model.status_label)},
        EditorProfilerKeyValueRow{.id = "producer", .label = "Producer", .value = row_value(model.producer)},
        EditorProfilerKeyValueRow{.id = "format", .label = "Format", .value = row_value(model.format)},
        EditorProfilerKeyValueRow{.id = "blocker", .label = "Blocker", .value = row_value(model.blocker)},
        EditorProfilerKeyValueRow{.id = "events", .label = "Events", .value = format_integer(event_count)},
        EditorProfilerKeyValueRow{.id = "counters", .label = "Counters", .value = format_integer(counter_count)},
        EditorProfilerKeyValueRow{.id = "profiles", .label = "Profiles", .value = format_integer(profile_count)},
    };
}

[[nodiscard]] EditorProfilerTraceFileSaveResult
build_trace_file_save_result(ITextStore* store, const mirakana::DiagnosticCapture& capture,
                             const EditorProfilerTraceFileSaveRequest& request) {
    const bool capture_empty = capture.events.empty() && capture.counters.empty() && capture.profiles.empty();

    EditorProfilerTraceFileSaveResult result;
    result.status_label = capture_empty ? "Trace file save empty" : "Trace file save ready";
    result.output_path = request.output_path;

    if (capture_empty) {
        result.diagnostics.emplace_back("trace file save requires at least one diagnostic sample");
        append_trace_file_save_rows(result);
        return result;
    }

    if (auto diagnostic = validate_trace_json_project_path(request.output_path, "trace file save output path");
        !diagnostic.empty()) {
        result.status_label = "Trace file save blocked";
        result.diagnostics.push_back(std::move(diagnostic));
        append_trace_file_save_rows(result);
        return result;
    }

    const auto payload = mirakana::export_diagnostics_trace_json(capture, request.options);
    result.payload_bytes = payload.size();

    if (store != nullptr) {
        try {
            store->write_text(result.output_path, payload);
            result.saved = true;
            result.status_label = "Trace file saved";
        } catch (const std::exception& exception) {
            result.status_label = "Trace file save failed";
            result.payload_bytes = 0;
            result.diagnostics.push_back(std::string("trace file save failed: ") + exception.what());
        }
    }

    append_trace_file_save_rows(result);
    return result;
}

} // namespace

EditorProfilerTraceExportModel
make_editor_profiler_trace_export_model(const mirakana::DiagnosticCapture& capture,
                                        const mirakana::DiagnosticsTraceExportOptions& options) {
    const auto summary = mirakana::summarize_diagnostics(capture);
    const bool capture_empty = capture.events.empty() && capture.counters.empty() && capture.profiles.empty();

    EditorProfilerTraceExportModel model;
    model.format = "Chrome Trace Event JSON";
    model.producer = "mirakana::export_diagnostics_trace_json";
    model.status_label = capture_empty ? "Trace export empty" : "Trace export ready";

    if (capture_empty) {
        model.diagnostics.emplace_back("trace export requires at least one diagnostic sample");
    } else {
        model.payload = mirakana::export_diagnostics_trace_json(capture, options);
        model.payload_bytes = model.payload.size();
        model.can_export = true;
    }

    model.rows = {
        EditorProfilerKeyValueRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorProfilerKeyValueRow{.id = "format", .label = "Format", .value = model.format},
        EditorProfilerKeyValueRow{.id = "producer", .label = "Producer", .value = model.producer},
        EditorProfilerKeyValueRow{
            .id = "payload_bytes", .label = "Payload bytes", .value = format_integer(model.payload_bytes)},
        EditorProfilerKeyValueRow{.id = "events", .label = "Events", .value = format_integer(summary.event_count)},
        EditorProfilerKeyValueRow{
            .id = "counters", .label = "Counters", .value = format_integer(summary.counter_count)},
        EditorProfilerKeyValueRow{
            .id = "profiles", .label = "Profiles", .value = format_integer(summary.profile_count)},
    };

    return model;
}

EditorProfilerTraceImportReviewModel make_editor_profiler_trace_import_review_model(std::string_view payload) {
    const auto imported = mirakana::import_diagnostics_trace_json(payload);

    EditorProfilerTraceImportReviewModel model;
    model.valid = imported.valid;
    model.capture_reconstructed = imported.valid;
    model.payload_bytes = imported.review.payload_bytes;
    model.capture = imported.capture;
    if (payload.empty()) {
        model.status_label = "Trace import empty";
    } else if (imported.valid) {
        model.status_label = "Trace import ready";
    } else {
        model.status_label = "Trace import blocked";
    }
    model.diagnostics = imported.diagnostics;
    model.reconstructed_profile_rows = make_profile_rows(model.capture);
    model.reconstructed_counter_rows = make_counter_rows(model.capture);
    model.reconstructed_event_rows = make_event_rows(model.capture);
    append_trace_import_rows(model, imported.review);
    return model;
}

EditorProfilerTraceFileImportResult
import_editor_profiler_trace_json(ITextStore& store, const EditorProfilerTraceFileImportRequest& request) {
    EditorProfilerTraceFileImportResult result;
    result.status_label = "Trace file import idle";
    result.input_path = request.input_path;

    if (auto diagnostic = validate_trace_json_project_path(request.input_path, "trace file import path");
        !diagnostic.empty()) {
        result.status_label = "Trace file import blocked";
        result.diagnostics.push_back(std::move(diagnostic));
        append_trace_file_import_rows(result);
        return result;
    }

    std::string payload;
    try {
        payload = store.read_text(result.input_path);
    } catch (const std::exception& exception) {
        result.status_label = "Trace file import failed";
        result.diagnostics.push_back(std::string("trace file import failed: ") + exception.what());
        append_trace_file_import_rows(result);
        return result;
    }

    result.payload_bytes = payload.size();
    result.review = make_editor_profiler_trace_import_review_model(payload);
    result.diagnostics = result.review.diagnostics;
    result.capture_reconstructed = result.review.capture_reconstructed;
    result.capture = result.review.capture;
    if (result.review.valid) {
        result.imported = true;
        result.status_label = "Trace file import ready";
    } else if (payload.empty()) {
        result.status_label = "Trace file import empty";
    } else {
        result.status_label = "Trace file import blocked";
    }

    append_trace_file_import_rows(result);
    return result;
}

mirakana::FileDialogRequest make_editor_profiler_trace_open_dialog_request(std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = mirakana::FileDialogKind::open_file;
    request.title = "Open Trace JSON";
    request.filters = {mirakana::FileDialogFilter{.name = "Trace JSON", .pattern = "json"}};
    request.default_location = std::string(default_location);
    request.allow_many = false;
    request.accept_label = "Open";
    request.cancel_label = "Cancel";
    return request;
}

EditorProfilerTraceOpenDialogModel
make_editor_profiler_trace_open_dialog_model(const mirakana::FileDialogResult& result) {
    EditorProfilerTraceOpenDialogModel model;
    model.status_label = "Trace open dialog idle";

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = "Trace open dialog blocked";
        model.diagnostics.push_back(std::move(diagnostic));
        append_trace_open_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = "Trace open dialog canceled";
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = "Trace open dialog failed";
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        if (result.paths.size() != 1U) {
            model.status_label = "Trace open dialog blocked";
            model.diagnostics.emplace_back("trace open dialog requires exactly one selected path");
            break;
        }

        model.selected_path = result.paths.front();
        if (!model.selected_path.ends_with(".json")) {
            model.status_label = "Trace open dialog blocked";
            model.diagnostics.emplace_back("trace open dialog selection must end with .json");
            break;
        }

        model.accepted = true;
        model.status_label = "Trace open dialog accepted";
        break;
    }

    append_trace_open_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

EditorProfilerTelemetryHandoffModel
make_editor_profiler_telemetry_handoff_model(const mirakana::DiagnosticCapture& capture,
                                             const mirakana::DiagnosticsOpsPlanOptions& options) {
    const auto plan = mirakana::build_diagnostics_ops_plan(capture, options);

    EditorProfilerTelemetryHandoffModel model;
    model.kind = std::string(
        mirakana::diagnostics_ops_artifact_kind_label(mirakana::DiagnosticsOpsArtifactKind::telemetry_upload));

    const auto artifact_it = std::ranges::find_if(plan.artifacts, [](const mirakana::DiagnosticsOpsArtifact& artifact) {
        return artifact.kind == mirakana::DiagnosticsOpsArtifactKind::telemetry_upload;
    });

    if (artifact_it == plan.artifacts.end()) {
        model.status_label = std::string(
            mirakana::diagnostics_ops_artifact_status_label(mirakana::DiagnosticsOpsArtifactStatus::unsupported));
        model.blocker = "Diagnostics telemetry artifact was not present in DiagnosticsOpsPlan.";
        model.diagnostics.push_back(model.blocker);
        append_telemetry_rows(model, plan.summary.event_count, plan.summary.counter_count, plan.summary.profile_count);
        return model;
    }

    const auto& artifact = *artifact_it;
    model.kind = std::string(mirakana::diagnostics_ops_artifact_kind_label(artifact.kind));
    model.status_label = std::string(mirakana::diagnostics_ops_artifact_status_label(artifact.status));
    model.ready = artifact.status == mirakana::DiagnosticsOpsArtifactStatus::ready;
    model.producer = artifact.producer;
    model.format = artifact.format;
    model.blocker = artifact.blocker;

    if (!model.blocker.empty()) {
        model.diagnostics.push_back(model.blocker);
    }

    append_telemetry_rows(model, artifact.event_count, artifact.counter_count, artifact.profile_count);
    return model;
}

EditorProfilerTraceFileSaveResult save_editor_profiler_trace_json(ITextStore& store,
                                                                  const mirakana::DiagnosticCapture& capture,
                                                                  const EditorProfilerTraceFileSaveRequest& request) {
    return build_trace_file_save_result(&store, capture, request);
}

mirakana::FileDialogRequest make_editor_profiler_trace_save_dialog_request(std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = mirakana::FileDialogKind::save_file;
    request.title = "Save Trace JSON";
    request.filters = {mirakana::FileDialogFilter{.name = "Trace JSON", .pattern = "json"}};
    request.default_location = std::string(default_location);
    request.allow_many = false;
    request.accept_label = "Save";
    request.cancel_label = "Cancel";
    return request;
}

EditorProfilerTraceSaveDialogModel
make_editor_profiler_trace_save_dialog_model(const mirakana::FileDialogResult& result) {
    EditorProfilerTraceSaveDialogModel model;
    model.status_label = "Trace save dialog idle";

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = "Trace save dialog blocked";
        model.diagnostics.push_back(std::move(diagnostic));
        append_trace_save_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = "Trace save dialog canceled";
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = "Trace save dialog failed";
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        if (result.paths.size() != 1U) {
            model.status_label = "Trace save dialog blocked";
            model.diagnostics.emplace_back("trace save dialog requires exactly one selected path");
            break;
        }

        model.selected_path = result.paths.front();
        if (!model.selected_path.ends_with(".json")) {
            model.status_label = "Trace save dialog blocked";
            model.diagnostics.emplace_back("trace save dialog selection must end with .json");
            break;
        }

        model.accepted = true;
        model.status_label = "Trace save dialog accepted";
        break;
    }

    append_trace_save_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

EditorProfilerPanelModel make_editor_profiler_panel_model(const mirakana::DiagnosticCapture& capture,
                                                          const EditorProfilerStatus& status) {
    const auto summary = mirakana::summarize_diagnostics(capture);

    EditorProfilerPanelModel model;
    model.capture_empty = capture.events.empty() && capture.counters.empty() && capture.profiles.empty();
    model.trace_status = model.capture_empty ? "Trace export empty" : "Trace export available";
    model.trace_export = make_editor_profiler_trace_export_model(capture);
    model.trace_file_save = build_trace_file_save_result(nullptr, capture, EditorProfilerTraceFileSaveRequest{});
    model.trace_save_dialog.status_label = "Trace save dialog idle";
    append_trace_save_dialog_rows(model.trace_save_dialog, 0, -1);
    model.trace_file_import.status_label = "Trace file import idle";
    model.trace_file_import.input_path = EditorProfilerTraceFileImportRequest{}.input_path;
    append_trace_file_import_rows(model.trace_file_import);
    model.trace_open_dialog.status_label = "Trace open dialog idle";
    append_trace_open_dialog_rows(model.trace_open_dialog, 0, -1);
    model.trace_import = make_editor_profiler_trace_import_review_model({});
    model.telemetry = make_editor_profiler_telemetry_handoff_model(capture);

    model.status_rows = {
        EditorProfilerKeyValueRow{
            .id = "log_records", .label = "Log records", .value = format_integer(status.log_records)},
        EditorProfilerKeyValueRow{
            .id = "undo_stack", .label = "Undo stack", .value = format_integer(status.undo_stack)},
        EditorProfilerKeyValueRow{
            .id = "redo_stack", .label = "Redo stack", .value = format_integer(status.redo_stack)},
        EditorProfilerKeyValueRow{
            .id = "asset_imports", .label = "Asset imports", .value = format_integer(status.asset_imports)},
        EditorProfilerKeyValueRow{
            .id = "hot_reload_events", .label = "Hot reload events", .value = format_integer(status.hot_reload_events)},
        EditorProfilerKeyValueRow{
            .id = "shader_compiles", .label = "Shader compiles", .value = format_integer(status.shader_compiles)},
        EditorProfilerKeyValueRow{.id = "dirty", .label = "Dirty", .value = format_bool(status.dirty)},
        EditorProfilerKeyValueRow{.id = "revision", .label = "Revision", .value = format_integer(status.revision)},
    };

    model.summary_rows = {
        EditorProfilerKeyValueRow{.id = "events", .label = "Events", .value = format_integer(summary.event_count)},
        EditorProfilerKeyValueRow{
            .id = "warnings", .label = "Warnings", .value = format_integer(summary.warning_count)},
        EditorProfilerKeyValueRow{.id = "errors", .label = "Errors", .value = format_integer(summary.error_count)},
        EditorProfilerKeyValueRow{
            .id = "counters", .label = "Counters", .value = format_integer(summary.counter_count)},
        EditorProfilerKeyValueRow{
            .id = "profiles", .label = "Profiles", .value = format_integer(summary.profile_count)},
        EditorProfilerKeyValueRow{.id = "total_profile_time",
                                  .label = "Total profile time",
                                  .value = format_duration(summary.total_profile_time_ns)},
        EditorProfilerKeyValueRow{.id = "min_profile_time",
                                  .label = "Min profile time",
                                  .value = format_duration(summary.min_profile_time_ns)},
        EditorProfilerKeyValueRow{.id = "max_profile_time",
                                  .label = "Max profile time",
                                  .value = format_duration(summary.max_profile_time_ns)},
    };

    model.profile_rows = make_profile_rows(capture);
    model.counter_rows = make_counter_rows(capture);
    model.event_rows = make_event_rows(capture);

    return model;
}

mirakana::ui::UiDocument make_profiler_ui_model(const EditorProfilerPanelModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("profiler", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"profiler"};

    add_or_throw(document, make_child("profiler.status", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId status_root{"profiler.status"};
    for (const auto& row : model.status_rows) {
        append_key_value(document, status_root, "profiler.status", row);
    }

    add_or_throw(document, make_child("profiler.summary", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId summary_root{"profiler.summary"};
    for (const auto& row : model.summary_rows) {
        append_key_value(document, summary_root, "profiler.summary", row);
    }

    append_label(document, root, "profiler.trace_status", model.trace_status);
    if (model.capture_empty) {
        append_label(document, root, "profiler.empty", "No diagnostic samples recorded");
    }

    add_or_throw(document, make_child("profiler.trace_export", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_export_root{"profiler.trace_export"};
    for (const auto& row : model.trace_export.rows) {
        append_key_value(document, trace_export_root, "profiler.trace_export", row);
    }
    add_or_throw(document,
                 make_child("profiler.trace_export.diagnostics", trace_export_root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_export_diagnostics_root{"profiler.trace_export.diagnostics"};
    for (std::size_t index = 0; index < model.trace_export.diagnostics.size(); ++index) {
        append_label(document, trace_export_diagnostics_root,
                     "profiler.trace_export.diagnostics." + std::to_string(index),
                     model.trace_export.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.trace_file_save", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_file_save_root{"profiler.trace_file_save"};
    for (const auto& row : model.trace_file_save.rows) {
        append_key_value(document, trace_file_save_root, "profiler.trace_file_save", row);
    }
    add_or_throw(document, make_child("profiler.trace_file_save.diagnostics", trace_file_save_root,
                                      mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_file_save_diagnostics_root{"profiler.trace_file_save.diagnostics"};
    for (std::size_t index = 0; index < model.trace_file_save.diagnostics.size(); ++index) {
        append_label(document, trace_file_save_diagnostics_root,
                     "profiler.trace_file_save.diagnostics." + std::to_string(index),
                     model.trace_file_save.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.trace_save_dialog", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_save_dialog_root{"profiler.trace_save_dialog"};
    for (const auto& row : model.trace_save_dialog.rows) {
        append_key_value(document, trace_save_dialog_root, "profiler.trace_save_dialog", row);
    }
    add_or_throw(document, make_child("profiler.trace_save_dialog.diagnostics", trace_save_dialog_root,
                                      mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_save_dialog_diagnostics_root{"profiler.trace_save_dialog.diagnostics"};
    for (std::size_t index = 0; index < model.trace_save_dialog.diagnostics.size(); ++index) {
        append_label(document, trace_save_dialog_diagnostics_root,
                     "profiler.trace_save_dialog.diagnostics." + std::to_string(index),
                     model.trace_save_dialog.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.trace_file_import", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_file_import_root{"profiler.trace_file_import"};
    for (const auto& row : model.trace_file_import.rows) {
        append_key_value(document, trace_file_import_root, "profiler.trace_file_import", row);
    }
    add_or_throw(document, make_child("profiler.trace_file_import.diagnostics", trace_file_import_root,
                                      mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_file_import_diagnostics_root{"profiler.trace_file_import.diagnostics"};
    for (std::size_t index = 0; index < model.trace_file_import.diagnostics.size(); ++index) {
        append_label(document, trace_file_import_diagnostics_root,
                     "profiler.trace_file_import.diagnostics." + std::to_string(index),
                     model.trace_file_import.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.trace_open_dialog", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_open_dialog_root{"profiler.trace_open_dialog"};
    for (const auto& row : model.trace_open_dialog.rows) {
        append_key_value(document, trace_open_dialog_root, "profiler.trace_open_dialog", row);
    }
    add_or_throw(document, make_child("profiler.trace_open_dialog.diagnostics", trace_open_dialog_root,
                                      mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_open_dialog_diagnostics_root{"profiler.trace_open_dialog.diagnostics"};
    for (std::size_t index = 0; index < model.trace_open_dialog.diagnostics.size(); ++index) {
        append_label(document, trace_open_dialog_diagnostics_root,
                     "profiler.trace_open_dialog.diagnostics." + std::to_string(index),
                     model.trace_open_dialog.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.trace_import", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_import_root{"profiler.trace_import"};
    for (const auto& row : model.trace_import.rows) {
        append_key_value(document, trace_import_root, "profiler.trace_import", row);
    }
    add_or_throw(document,
                 make_child("profiler.trace_import.diagnostics", trace_import_root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId trace_import_diagnostics_root{"profiler.trace_import.diagnostics"};
    for (std::size_t index = 0; index < model.trace_import.diagnostics.size(); ++index) {
        append_label(document, trace_import_diagnostics_root,
                     "profiler.trace_import.diagnostics." + std::to_string(index),
                     model.trace_import.diagnostics[index]);
    }
    append_event_row_list(document, trace_import_root, "profiler.trace_import.reconstructed_events",
                          model.trace_import.reconstructed_event_rows);
    append_counter_row_list(document, trace_import_root, "profiler.trace_import.reconstructed_counters",
                            model.trace_import.reconstructed_counter_rows);
    append_profile_row_list(document, trace_import_root, "profiler.trace_import.reconstructed_profiles",
                            model.trace_import.reconstructed_profile_rows);

    add_or_throw(document, make_child("profiler.telemetry", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId telemetry_root{"profiler.telemetry"};
    for (const auto& row : model.telemetry.rows) {
        append_key_value(document, telemetry_root, "profiler.telemetry", row);
    }
    add_or_throw(document,
                 make_child("profiler.telemetry.diagnostics", telemetry_root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId telemetry_diagnostics_root{"profiler.telemetry.diagnostics"};
    for (std::size_t index = 0; index < model.telemetry.diagnostics.size(); ++index) {
        append_label(document, telemetry_diagnostics_root, "profiler.telemetry.diagnostics." + std::to_string(index),
                     model.telemetry.diagnostics[index]);
    }

    add_or_throw(document, make_child("profiler.profiles", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId profiles_root{"profiler.profiles"};
    for (std::size_t index = 0; index < model.profile_rows.size(); ++index) {
        const auto& row = model.profile_rows[index];
        const auto item_prefix = "profiler.profiles." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, profiles_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.name);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".name", row.name);
        append_label(document, item_id, item_prefix + ".duration", row.duration);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }

    add_or_throw(document, make_child("profiler.counters", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId counters_root{"profiler.counters"};
    for (std::size_t index = 0; index < model.counter_rows.size(); ++index) {
        const auto& row = model.counter_rows[index];
        const auto item_prefix = "profiler.counters." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, counters_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.name);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".name", row.name);
        append_label(document, item_id, item_prefix + ".value", row.value);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }

    add_or_throw(document, make_child("profiler.events", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId events_root{"profiler.events"};
    for (std::size_t index = 0; index < model.event_rows.size(); ++index) {
        const auto& row = model.event_rows[index];
        const auto item_prefix = "profiler.events." + std::to_string(index + 1U);
        mirakana::ui::ElementDesc item = make_child(item_prefix, events_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.message);
        add_or_throw(document, std::move(item));
        const mirakana::ui::ElementId item_id{item_prefix};
        append_label(document, item_id, item_prefix + ".severity", row.severity);
        append_label(document, item_id, item_prefix + ".category", row.category);
        append_label(document, item_id, item_prefix + ".message", row.message);
        append_label(document, item_id, item_prefix + ".frame", format_integer(row.frame_index));
    }

    return document;
}

} // namespace mirakana::editor
