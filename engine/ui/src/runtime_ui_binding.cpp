// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_binding.hpp"

#include <algorithm>
#include <iterator>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

void append_diagnostic(std::vector<RuntimeUiBindingDiagnostic>& diagnostics, RuntimeUiBindingDiagnosticCode code,
                       std::string subject, std::string message) {
    diagnostics.push_back(
        RuntimeUiBindingDiagnostic{.code = code, .subject = std::move(subject), .message = std::move(message)});
}

[[nodiscard]] bool contains_string(const std::vector<std::string>& values, std::string_view value) noexcept {
    return std::ranges::find(values, value) != values.end();
}

[[nodiscard]] const RuntimeUiBindingValueRow* find_value(const std::vector<RuntimeUiBindingValueRow>& values,
                                                         std::string_view key) noexcept {
    const auto it = std::ranges::find_if(values, [key](const RuntimeUiBindingValueRow& row) { return row.key == key; });
    return it == values.end() ? nullptr : &(*it);
}

[[nodiscard]] const RuntimeUiCommandAvailabilityRow*
find_command(const std::vector<RuntimeUiCommandAvailabilityRow>& commands, std::string_view command_id) noexcept {
    const auto it = std::ranges::find_if(
        commands, [command_id](const RuntimeUiCommandAvailabilityRow& row) { return row.command_id == command_id; });
    return it == commands.end() ? nullptr : &(*it);
}

[[nodiscard]] const RuntimeUiFocusScope* find_scope(const std::vector<RuntimeUiFocusScope>& scopes,
                                                    std::string_view id) noexcept {
    const auto it = std::ranges::find_if(scopes, [id](const RuntimeUiFocusScope& scope) { return scope.id == id; });
    return it == scopes.end() ? nullptr : &(*it);
}

[[nodiscard]] RuntimeUiBindingValueType expected_type_for_target(RuntimeUiBindingTarget target) noexcept {
    switch (target) {
    case RuntimeUiBindingTarget::text:
        return RuntimeUiBindingValueType::text;
    case RuntimeUiBindingTarget::enabled:
    case RuntimeUiBindingTarget::visible:
        return RuntimeUiBindingValueType::boolean;
    }
    return RuntimeUiBindingValueType::text;
}

[[nodiscard]] bool element_exists(const UiDocument& document, const ElementId& id) noexcept {
    return document.find(id) != nullptr;
}

[[nodiscard]] bool modal_scope_contains(const UiDocument& document, const RuntimeUiFocusScope& scope,
                                        const ElementId& id) noexcept {
    return document.is_descendant_or_same(scope.root, id);
}

void validate_command_rows(const RuntimeUiBindingDocument& document, RuntimeUiInputRoutingPlan& plan) {
    std::vector<std::string> command_ids;
    command_ids.reserve(document.command_rows.size());

    for (const auto& row : document.command_rows) {
        const auto subject = row.id.empty() ? row.command_id : row.id;
        if (row.command_id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_command_id, subject,
                              "runtime UI command availability row requires a command id");
            continue;
        }
        if (contains_string(command_ids, row.command_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::duplicate_command_id, row.command_id,
                              "runtime UI command availability rows must use unique command ids");
        } else {
            command_ids.push_back(row.command_id);
        }
        if (!element_exists(document.document, row.element)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_element, subject,
                              "runtime UI command availability row must target an existing element");
        }
    }

    for (const auto& invocation : document.command_invocations) {
        const auto* command = find_command(document.command_rows, invocation.command_id);
        if (command == nullptr) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_command_id,
                              invocation.command_id,
                              "runtime UI command invocation must reference a declared command row");
            continue;
        }
        if (!command->available) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::disabled_command_invocation,
                              invocation.command_id,
                              "runtime UI input routing must not invoke unavailable gameplay commands");
        }
    }
}

void validate_focus_scopes(const RuntimeUiBindingDocument& document, RuntimeUiInputRoutingPlan& plan) {
    std::vector<std::string> scope_ids;
    scope_ids.reserve(document.focus_scopes.size());

    for (const auto& scope : document.focus_scopes) {
        if (scope.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_focus_scope, scope.root.value,
                              "runtime UI focus scope id must not be empty");
        } else if (contains_string(scope_ids, scope.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_focus_scope, scope.id,
                              "runtime UI focus scope ids must be unique");
        } else {
            scope_ids.push_back(scope.id);
        }

        if (!element_exists(document.document, scope.root) || !element_exists(document.document, scope.initial_focus)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_element, scope.id,
                              "runtime UI focus scope must reference existing root and initial focus elements");
            continue;
        }
        if (scope.modal && !modal_scope_contains(document.document, scope, scope.initial_focus)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::modal_focus_escape, scope.id,
                              "runtime UI modal focus scope initial focus must remain inside the modal root");
        }
    }
}

void validate_modal_navigation_edge(const RuntimeUiBindingDocument& document, const RuntimeUiNavigationEdge& edge,
                                    const RuntimeUiFocusScope& scope, RuntimeUiInputRoutingPlan& plan) {
    if (!scope.modal) {
        return;
    }
    if (!modal_scope_contains(document.document, scope, edge.from) ||
        !modal_scope_contains(document.document, scope, edge.to)) {
        append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::modal_focus_escape, edge.id,
                          "runtime UI modal navigation edges must stay inside the modal focus scope");
    }
}

[[nodiscard]] bool reaches_navigation_origin(const std::vector<RuntimeUiNavigationEdge>& edges,
                                             const RuntimeUiNavigationEdge& origin,
                                             const RuntimeUiNavigationEdge& first) {
    if (first.to == first.from) {
        return false;
    }

    std::vector<std::string> visited;
    visited.push_back(first.from.value);
    auto cursor = first.to.value;

    while (!cursor.empty()) {
        if (cursor == origin.from.value) {
            return true;
        }
        if (contains_string(visited, cursor)) {
            return false;
        }
        visited.push_back(cursor);

        const auto next = std::ranges::find_if(edges, [&origin, &cursor](const RuntimeUiNavigationEdge& edge) {
            return edge.scope_id == origin.scope_id && edge.direction == origin.direction &&
                   edge.from.value == cursor && edge.to.value != edge.from.value;
        });
        if (next == edges.end()) {
            return false;
        }
        cursor = next->to.value;
    }
    return false;
}

void validate_navigation_edges(const RuntimeUiBindingDocument& document, RuntimeUiInputRoutingPlan& plan) {
    bool reported_cycle = false;

    for (const auto& edge : document.navigation_edges) {
        const auto* scope = find_scope(document.focus_scopes, edge.scope_id);
        if (scope == nullptr) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_focus_scope, edge.id,
                              "runtime UI navigation edge must reference a declared focus scope");
            continue;
        }
        if (!element_exists(document.document, edge.from) || !element_exists(document.document, edge.to)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_navigation_target, edge.id,
                              "runtime UI navigation edge must reference existing elements");
            continue;
        }

        validate_modal_navigation_edge(document, edge, *scope, plan);
        if (!reported_cycle && reaches_navigation_origin(document.navigation_edges, edge, edge)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::navigation_cycle, edge.id,
                              "runtime UI explicit navigation edges must not form non-trivial cycles");
            reported_cycle = true;
        }
    }
}

void validate_controller_glyph_refs(const RuntimeUiBindingDocument& document, RuntimeUiInputRoutingPlan& plan) {
    for (const auto& ref : document.controller_glyph_refs) {
        if (!element_exists(document.document, ref.element)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_element, ref.id,
                              "runtime UI controller glyph ref must target an existing element");
        }
        if (ref.glyph_id.empty() || !contains_string(document.known_controller_glyph_ids, ref.glyph_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::unknown_controller_glyph_ref, ref.id,
                              "runtime UI controller glyph ref must use a declared glyph id");
        }
    }
}

void validate_pointer_captures(const RuntimeUiBindingDocument& document, RuntimeUiInputRoutingPlan& plan) {
    for (auto current = document.pointer_captures.begin(); current != document.pointer_captures.end(); ++current) {
        if (!element_exists(document.document, current->element)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_element, current->id,
                              "runtime UI pointer capture must target an existing element");
        }
        const auto conflict =
            std::find_if(std::next(current), document.pointer_captures.end(), [current](const auto& row) {
                return row.pointer_id == current->pointer_id && row.element != current->element;
            });
        if (conflict != document.pointer_captures.end()) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::pointer_capture_conflict, current->id,
                              "runtime UI pointer id cannot be captured by multiple elements");
        }
    }
}

void append_diagnostics(std::vector<RuntimeUiBindingDiagnostic>& diagnostics,
                        const std::vector<RuntimeUiBindingDiagnostic>& source) {
    diagnostics.insert(diagnostics.end(), source.begin(), source.end());
}

void apply_binding_row(RuntimeUiBindingDocument& document, const RuntimeUiBindingRow& row,
                       const RuntimeUiBindingValueRow& value) {
    switch (row.target) {
    case RuntimeUiBindingTarget::text:
        static_cast<void>(document.document.set_text(row.element, TextContent{.label = value.text,
                                                                              .localization_key = {},
                                                                              .font_family = "default",
                                                                              .direction = TextDirection::automatic,
                                                                              .wrap = TextWrapMode::clip}));
        return;
    case RuntimeUiBindingTarget::enabled:
        static_cast<void>(document.document.set_enabled(row.element, value.boolean));
        return;
    case RuntimeUiBindingTarget::visible:
        static_cast<void>(document.document.set_visible(row.element, value.boolean));
        return;
    }
}

} // namespace

std::string_view runtime_ui_binding_value_type_name(RuntimeUiBindingValueType type) noexcept {
    switch (type) {
    case RuntimeUiBindingValueType::text:
        return "text";
    case RuntimeUiBindingValueType::boolean:
        return "boolean";
    }
    return "unknown";
}

std::string_view runtime_ui_binding_target_name(RuntimeUiBindingTarget target) noexcept {
    switch (target) {
    case RuntimeUiBindingTarget::text:
        return "text";
    case RuntimeUiBindingTarget::enabled:
        return "enabled";
    case RuntimeUiBindingTarget::visible:
        return "visible";
    }
    return "unknown";
}

std::string_view runtime_ui_binding_plan_status_name(RuntimeUiBindingPlanStatus status) noexcept {
    switch (status) {
    case RuntimeUiBindingPlanStatus::invalid_request:
        return "invalid_request";
    case RuntimeUiBindingPlanStatus::ready:
        return "ready";
    case RuntimeUiBindingPlanStatus::diagnostics:
        return "diagnostics";
    }
    return "unknown";
}

RuntimeUiInputRoutingPlan plan_runtime_ui_input_routing(const RuntimeUiBindingDocument& document) {
    RuntimeUiInputRoutingPlan plan{
        .status = RuntimeUiBindingPlanStatus::invalid_request,
        .ready = false,
        .input_routing_ready = false,
        .command_rows = document.command_rows.size(),
        .focus_scopes = document.focus_scopes.size(),
        .navigation_edges = document.navigation_edges.size(),
        .controller_glyph_refs = document.controller_glyph_refs.size(),
        .pointer_captures = document.pointer_captures.size(),
        .gameplay_commands_executed = 0U,
        .diagnostics = {},
    };

    validate_command_rows(document, plan);
    validate_focus_scopes(document, plan);
    validate_navigation_edges(document, plan);
    validate_controller_glyph_refs(document, plan);
    validate_pointer_captures(document, plan);

    plan.ready = plan.diagnostics.empty();
    plan.input_routing_ready = plan.ready;
    plan.status = plan.ready ? RuntimeUiBindingPlanStatus::ready : RuntimeUiBindingPlanStatus::diagnostics;
    return plan;
}

RuntimeUiBindingPlan plan_runtime_ui_binding(RuntimeUiBindingDocument document) {
    RuntimeUiBindingPlan plan{
        .status = RuntimeUiBindingPlanStatus::invalid_request,
        .ready = false,
        .input_routing_ready = false,
        .document = {},
        .binding_rows = document.binding_rows.size(),
        .command_rows = document.command_rows.size(),
        .focus_scopes = document.focus_scopes.size(),
        .navigation_edges = document.navigation_edges.size(),
        .controller_glyph_refs = 0U,
        .gameplay_commands_executed = 0U,
        .diagnostics = {},
    };

    std::vector<std::string> binding_ids;
    binding_ids.reserve(document.binding_rows.size());

    for (const auto& row : document.binding_rows) {
        bool row_valid = true;
        if (row.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_binding_id, row.source_key,
                              "runtime UI binding row id must not be empty");
            row_valid = false;
        } else if (contains_string(binding_ids, row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::duplicate_binding_id, row.id,
                              "runtime UI binding row ids must be unique");
            row_valid = false;
        } else {
            binding_ids.push_back(row.id);
        }

        if (!element_exists(document.document, row.element)) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_element, row.id,
                              "runtime UI binding row must target an existing element");
            row_valid = false;
        }

        const auto* value = find_value(document.values, row.source_key);
        if (value == nullptr) {
            append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::missing_binding_key, row.id,
                              "runtime UI binding source key must be present in the binding values");
            row_valid = false;
        } else {
            const auto target_type = expected_type_for_target(row.target);
            if (row.expected_type != target_type || value->type != row.expected_type) {
                append_diagnostic(plan.diagnostics, RuntimeUiBindingDiagnosticCode::type_mismatch, row.id,
                                  "runtime UI binding value type must match the binding target");
                row_valid = false;
            }
        }

        if (row_valid && value != nullptr) {
            apply_binding_row(document, row, *value);
        }
    }

    const auto routing_plan = plan_runtime_ui_input_routing(document);
    append_diagnostics(plan.diagnostics, routing_plan.diagnostics);
    plan.input_routing_ready = routing_plan.input_routing_ready;
    plan.controller_glyph_refs = routing_plan.controller_glyph_refs;
    plan.gameplay_commands_executed = routing_plan.gameplay_commands_executed;
    plan.document = std::move(document.document);
    plan.ready = plan.diagnostics.empty();
    plan.status = plan.ready ? RuntimeUiBindingPlanStatus::ready : RuntimeUiBindingPlanStatus::diagnostics;
    return plan;
}

} // namespace mirakana::ui
