// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/resource_panel.hpp"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string_view>
#include <tuple>
#include <utility>

namespace mirakana::editor {
namespace {

constexpr std::size_t k_max_capture_execution_host_gate_entries = 24;
constexpr std::size_t k_max_capture_execution_artifact_chars = 260;
constexpr std::size_t k_max_capture_execution_diagnostic_chars = 512;

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

[[nodiscard]] std::string bound_sanitized_field(std::string_view value, std::size_t max_chars) {
    auto text = sanitize_text(value);
    if (text.size() > max_chars) {
        text.resize(max_chars);
    }
    return text.empty() ? "-" : std::move(text);
}

[[nodiscard]] std::string format_integer(std::uint64_t value) {
    return std::to_string(value);
}

[[nodiscard]] std::string format_bytes(std::uint64_t value) {
    constexpr double kib = 1024.0;
    constexpr double mib = kib * 1024.0;

    std::ostringstream out;
    out << std::fixed << std::setprecision(3);
    if (value >= static_cast<std::uint64_t>(mib)) {
        out << static_cast<double>(value) / mib << " MiB";
        return out.str();
    }
    if (value >= static_cast<std::uint64_t>(kib)) {
        out << static_cast<double>(value) / kib << " KiB";
        return out.str();
    }
    return std::to_string(value) + " B";
}

[[nodiscard]] std::string format_budget_usage(std::uint64_t usage, std::uint64_t budget) {
    std::ostringstream out;
    out << format_bytes(usage) << " / " << format_bytes(budget) << " (" << std::fixed << std::setprecision(1)
        << (static_cast<double>(usage) * 100.0 / static_cast<double>(budget)) << "%)";
    return out.str();
}

[[nodiscard]] EditorResourceRow unavailable_row(std::string id, std::string label) {
    return EditorResourceRow{
        .id = std::move(id), .label = std::move(label), .value = "unavailable", .available = false};
}

[[nodiscard]] bool counter_input_less(const EditorResourceCounterInput& lhs, const EditorResourceCounterInput& rhs) {
    return std::tie(lhs.id, lhs.label, lhs.value) < std::tie(rhs.id, rhs.label, rhs.value);
}

[[nodiscard]] bool capture_request_input_less(const EditorResourceCaptureRequestInput& lhs,
                                              const EditorResourceCaptureRequestInput& rhs) {
    return std::tie(lhs.id, lhs.label, lhs.tool_label, lhs.action_label) <
           std::tie(rhs.id, rhs.label, rhs.tool_label, rhs.action_label);
}

[[nodiscard]] bool capture_execution_input_less(const EditorResourceCaptureExecutionInput& lhs,
                                                const EditorResourceCaptureExecutionInput& rhs) {
    return std::tie(lhs.id, lhs.label, lhs.tool_label) < std::tie(rhs.id, rhs.label, rhs.tool_label);
}

[[nodiscard]] std::vector<std::string> sanitize_and_sort_values(const std::vector<std::string>& values) {
    std::vector<std::string> sanitized;
    sanitized.reserve(values.size());
    for (const auto& value : values) {
        auto text = sanitize_text(value);
        if (text != "-") {
            sanitized.push_back(std::move(text));
        }
    }
    std::ranges::sort(sanitized);
    sanitized.erase(std::ranges::unique(sanitized).begin(), sanitized.end());
    return sanitized;
}

[[nodiscard]] std::string join_values(const std::vector<std::string>& values) {
    if (values.empty()) {
        return "-";
    }

    std::string joined;
    for (const auto& value : values) {
        if (!joined.empty()) {
            joined += ",";
        }
        joined += value;
    }
    return joined;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor resource panel ui element could not be added");
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
                      const std::string& prefix, const EditorResourceRow& row) {
    mirakana::ui::ElementDesc item = make_child(prefix + "." + row.id, parent, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(row.label);
    add_or_throw(document, std::move(item));

    const mirakana::ui::ElementId item_id{prefix + "." + row.id};
    append_label(document, item_id, prefix + "." + row.id + ".label", row.label);
    append_label(document, item_id, prefix + "." + row.id + ".value", row.value);
    append_label(document, item_id, prefix + "." + row.id + ".status", row.available ? "available" : "unavailable");
}

void append_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root, const std::string& section,
                 const std::vector<EditorResourceRow>& rows) {
    const std::string section_id = "resources." + section;
    add_or_throw(document, make_child(section_id, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId section_root{section_id};
    for (const auto& row : rows) {
        append_key_value(document, section_root, section_id, row);
    }
}

void append_capture_request(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                            const std::string& prefix, const EditorResourceCaptureRequestRow& row) {
    const std::string row_id = prefix + "." + row.id;
    mirakana::ui::ElementDesc item = make_child(row_id, parent, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(row.label);
    add_or_throw(document, std::move(item));

    const mirakana::ui::ElementId item_id{row_id};
    append_label(document, item_id, row_id + ".label", row.label);
    append_label(document, item_id, row_id + ".tool", row.tool_label);
    append_label(document, item_id, row_id + ".action", row.action_label);
    append_label(document, item_id, row_id + ".status", row.status_label);
    append_label(document, item_id, row_id + ".host_gates", row.host_gates_label);
    append_label(document, item_id, row_id + ".acknowledgement", row.acknowledgement_label);
    append_label(document, item_id, row_id + ".diagnostic", row.diagnostic);
}

void append_capture_requests(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                             const std::vector<EditorResourceCaptureRequestRow>& rows) {
    const std::string section_id = "resources.capture_requests";
    add_or_throw(document, make_child(section_id, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId section_root{section_id};
    for (const auto& row : rows) {
        append_capture_request(document, section_root, section_id, row);
    }
}

void append_capture_execution(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent,
                              const std::string& prefix, const EditorResourceCaptureExecutionRow& row) {
    const std::string row_id = prefix + "." + row.id;
    mirakana::ui::ElementDesc item = make_child(row_id, parent, mirakana::ui::SemanticRole::list_item);
    item.text = make_text(row.label);
    add_or_throw(document, std::move(item));

    const mirakana::ui::ElementId item_id{row_id};
    append_label(document, item_id, row_id + ".label", row.label);
    append_label(document, item_id, row_id + ".tool", row.tool_label);
    append_label(document, item_id, row_id + ".phase", row.phase_code);
    append_label(document, item_id, row_id + ".status", row.status_label);
    append_label(document, item_id, row_id + ".host_gates", row.host_gates_label);
    append_label(document, item_id, row_id + ".artifact", row.artifact_path);
    append_label(document, item_id, row_id + ".diagnostic", row.diagnostic);
    if (row.id == "pix_gpu_capture") {
        // Retained MK_ui element id: resources.capture_execution.pix_gpu_capture.host_helper_hint
        append_label(document, item_id, row_id + ".host_helper_hint",
                     "Windows: after acknowledging the PIX request, use the Host helper column in MK_editor "
                     "Resources for reviewed pwsh tools/launch-pix-host-helper.ps1: Run helper (-SkipLaunch) for "
                     "path smoke, or Run helper (launch PIX) after confirming the modal when PIX installation is "
                     "intended; repository script must be discoverable from the process working directory walk.");
    }
}

void append_capture_execution_rows(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& root,
                                   const std::vector<EditorResourceCaptureExecutionRow>& rows) {
    const std::string section_id = "resources.capture_execution";
    add_or_throw(document, make_child(section_id, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId section_root{section_id};
    // Retained agent needle (check-ai-integration): ge.editor.resources_capture_execution.v1
    append_label(document, section_root, "resources.capture_execution.contract_label",
                 std::string{editor_resources_capture_execution_contract_v1()});
    // Retained agent needle: ge.editor.resources_capture_operator_validated_launch_workflow.v1
    append_label(document, section_root,
                 "resources.capture_execution.operator_validated_launch_workflow_contract_label",
                 std::string{editor_resources_capture_operator_validated_launch_workflow_contract_v1()});
    for (const auto& row : rows) {
        append_capture_execution(document, section_root, section_id, row);
    }
}

[[nodiscard]] std::string capture_execution_status_label(const EditorResourceCaptureExecutionInput& input) {
    if (input.editor_core_executed || input.exposes_native_handles) {
        return "Blocked";
    }
    if (input.capture_failed) {
        return "Failed";
    }
    if (input.capture_completed) {
        return "Captured";
    }
    if (input.capture_started) {
        return "Running";
    }
    if (input.host_gated) {
        return "Host-gated";
    }
    if (input.requested) {
        return "Requested";
    }
    return "Not requested";
}

[[nodiscard]] std::string capture_execution_phase_code(const EditorResourceCaptureExecutionInput& input) {
    if (input.editor_core_executed || input.exposes_native_handles) {
        return "blocked";
    }
    if (input.capture_failed) {
        return "failed";
    }
    if (input.capture_completed) {
        return "captured";
    }
    if (input.capture_started) {
        return "running";
    }
    if (input.host_gated) {
        return "host_gated";
    }
    if (input.requested) {
        return "requested";
    }
    return "not_requested";
}

[[nodiscard]] std::string capture_execution_diagnostic(const EditorResourceCaptureExecutionInput& input,
                                                       std::string_view status_label) {
    if (input.editor_core_executed || input.exposes_native_handles) {
        return "resource capture execution evidence blocked: editor-core execution and native handle exposure are "
               "unsupported; external host workflow only";
    }
    if (!input.diagnostic.empty()) {
        return sanitize_text(input.diagnostic);
    }
    if (status_label == "Captured") {
        return "capture completed by external host workflow";
    }
    if (status_label == "Failed") {
        return "capture failed in external host workflow";
    }
    if (status_label == "Running") {
        return "capture running in external host workflow";
    }
    if (status_label == "Host-gated") {
        return "capture evidence host-gated for external host workflow";
    }
    if (status_label == "Requested") {
        return "capture requested for external host workflow";
    }
    return "no capture execution evidence reported";
}

} // namespace

EditorResourcePanelModel make_editor_resource_panel_model(const EditorResourcePanelInput& input) {
    EditorResourcePanelModel model;
    model.device_available = input.device_available;
    model.status = input.device_available ? "Ready" : "No RHI device";

    model.status_rows.push_back(EditorResourceRow{.id = "device",
                                                  .label = "Device",
                                                  .value = input.device_available ? "available" : "unavailable",
                                                  .available = input.device_available});
    if (input.device_available) {
        model.status_rows.push_back(EditorResourceRow{
            .id = "backend",
            .label = "Backend",
            .value = sanitize_text(input.backend_label.empty() ? input.backend_id : input.backend_label),
            .available = !input.backend_id.empty() || !input.backend_label.empty(),
        });
        model.status_rows.push_back(EditorResourceRow{
            .id = "frame", .label = "Frame", .value = format_integer(input.frame_index), .available = true});
    } else {
        model.status_rows.push_back(unavailable_row("backend", "Backend"));
        model.status_rows.push_back(unavailable_row("frame", "Frame"));
    }

    if (input.device_available) {
        auto counters = input.rhi_counters;
        std::ranges::sort(counters, counter_input_less);
        model.counter_rows.reserve(counters.size());
        for (const auto& counter : counters) {
            model.counter_rows.push_back(EditorResourceRow{
                .id = sanitize_text(counter.id),
                .label = sanitize_text(counter.label),
                .value = format_integer(counter.value),
                .available = true,
            });
        }
    }

    if (input.device_available && input.memory.os_video_memory_budget_available &&
        input.memory.local_video_memory_budget_bytes > 0U) {
        model.memory_rows.push_back(EditorResourceRow{
            .id = "local_video_memory",
            .label = "Local video memory",
            .value = format_budget_usage(input.memory.local_video_memory_usage_bytes,
                                         input.memory.local_video_memory_budget_bytes),
            .available = true,
        });
    } else {
        model.memory_rows.push_back(unavailable_row("local_video_memory", "Local video memory"));
    }

    if (input.device_available && input.memory.os_video_memory_budget_available &&
        input.memory.non_local_video_memory_budget_bytes > 0U) {
        model.memory_rows.push_back(
            EditorResourceRow{.id = "non_local_video_memory",
                              .label = "Non-local video memory",
                              .value = format_budget_usage(input.memory.non_local_video_memory_usage_bytes,
                                                           input.memory.non_local_video_memory_budget_bytes),
                              .available = true});
    } else {
        model.memory_rows.push_back(unavailable_row("non_local_video_memory", "Non-local video memory"));
    }

    if (input.device_available && input.memory.committed_resources_byte_estimate_available) {
        model.memory_rows.push_back(EditorResourceRow{
            .id = "committed_resources",
            .label = "Committed resource estimate",
            .value = format_bytes(input.memory.committed_resources_byte_estimate),
            .available = true,
        });
    } else {
        model.memory_rows.push_back(unavailable_row("committed_resources", "Committed resource estimate"));
    }

    if (input.device_available) {
        model.lifetime_rows.push_back(EditorResourceRow{
            .id = "live_resources",
            .label = "Live resources",
            .value = format_integer(input.lifetime.live_resources),
            .available = true,
        });
        model.lifetime_rows.push_back(EditorResourceRow{
            .id = "deferred_release_resources",
            .label = "Deferred-release resources",
            .value = format_integer(input.lifetime.deferred_release_resources),
            .available = true,
        });
        model.lifetime_rows.push_back(EditorResourceRow{
            .id = "lifetime_events",
            .label = "Lifetime events",
            .value = format_integer(input.lifetime.lifetime_events),
            .available = true,
        });

        auto by_kind = input.lifetime.resources_by_kind;
        std::ranges::sort(by_kind, counter_input_less);
        for (const auto& row : by_kind) {
            model.lifetime_rows.push_back(EditorResourceRow{
                .id = "kind." + sanitize_text(row.id),
                .label = sanitize_text(row.label),
                .value = format_integer(row.value),
                .available = true,
            });
        }
    }

    auto capture_requests = input.capture_requests;
    std::ranges::sort(capture_requests, capture_request_input_less);
    model.capture_request_rows.reserve(capture_requests.size());
    for (const auto& request : capture_requests) {
        EditorResourceCaptureRequestRow row;
        row.id = sanitize_text(request.id);
        row.label = sanitize_text(request.label);
        row.tool_label = sanitize_text(request.tool_label);
        row.action_label = sanitize_text(request.action_label);
        row.host_gates = sanitize_and_sort_values(request.host_gates);
        row.host_gates_label = join_values(row.host_gates);
        row.request_available = input.device_available && request.available;
        row.request_acknowledged = row.request_available && request.acknowledged;
        row.host_gate_acknowledgement_required =
            row.request_available && !row.request_acknowledged && !row.host_gates.empty();

        if (!input.device_available) {
            row.status_label = "Blocked";
            row.acknowledgement_label = "blocked";
            row.diagnostic =
                "capture request blocked: no RHI device diagnostics available; host-gated external workflow only";
        } else if (!request.available) {
            row.status_label = "Blocked";
            row.acknowledgement_label = "blocked";
            row.diagnostic = sanitize_text(request.diagnostic.empty()
                                               ? "capture request unavailable; host-gated external workflow only"
                                               : request.diagnostic + "; host-gated external workflow only");
        } else if (row.request_acknowledged) {
            row.status_label = "Acknowledged";
            row.acknowledgement_label = "acknowledged";
            row.diagnostic =
                sanitize_text(request.diagnostic.empty() ? "capture request acknowledged for external host workflow"
                                                         : request.diagnostic + "; external host workflow only");
        } else {
            row.status_label = "Ready";
            row.acknowledgement_label = row.host_gates.empty() ? "not required" : "required";
            row.diagnostic =
                sanitize_text(request.diagnostic.empty() ? "capture request ready for external host workflow"
                                                         : request.diagnostic + "; external host workflow only");
        }

        model.capture_request_rows.push_back(std::move(row));
    }

    auto capture_execution = input.capture_execution_snapshots;
    std::ranges::sort(capture_execution, capture_execution_input_less);
    model.capture_execution_rows.reserve(capture_execution.size());
    for (const auto& snapshot : capture_execution) {
        EditorResourceCaptureExecutionRow row;
        row.id = sanitize_text(snapshot.id);
        row.label = sanitize_text(snapshot.label);
        row.tool_label = sanitize_text(snapshot.tool_label);
        row.status_label = capture_execution_status_label(snapshot);
        row.phase_code = capture_execution_phase_code(snapshot);
        row.host_gates = sanitize_and_sort_values(snapshot.host_gates);
        if (row.host_gates.size() > k_max_capture_execution_host_gate_entries) {
            row.host_gates.resize(k_max_capture_execution_host_gate_entries);
        }
        row.host_gates_label = join_values(row.host_gates);
        if (snapshot.editor_core_executed || snapshot.exposes_native_handles) {
            row.artifact_path = "-";
        } else {
            row.artifact_path = bound_sanitized_field(snapshot.artifact_path, k_max_capture_execution_artifact_chars);
        }
        row.diagnostic = bound_sanitized_field(capture_execution_diagnostic(snapshot, row.status_label),
                                               k_max_capture_execution_diagnostic_chars);
        model.capture_execution_rows.push_back(std::move(row));
    }

    return model;
}

mirakana::ui::UiDocument make_resource_panel_ui_model(const EditorResourcePanelModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("resources", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"resources"};

    append_rows(document, root, "status", model.status_rows);
    append_label(document, root, "resources.status_text", model.status);
    if (!model.device_available) {
        append_label(document, root, "resources.empty", "No RHI device diagnostics available");
    }

    append_rows(document, root, "counters", model.counter_rows);
    append_rows(document, root, "memory", model.memory_rows);
    append_rows(document, root, "lifetime", model.lifetime_rows);
    append_capture_requests(document, root, model.capture_request_rows);
    append_capture_execution_rows(document, root, model.capture_execution_rows);

    return document;
}

} // namespace mirakana::editor
