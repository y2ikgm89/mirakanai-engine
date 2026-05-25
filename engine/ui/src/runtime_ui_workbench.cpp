// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_workbench.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

void append_diagnostic(std::vector<RuntimeUiWorkbenchDiagnostic>& diagnostics, RuntimeUiWorkbenchDiagnosticCode code,
                       std::string owner_id, std::string message) {
    diagnostics.push_back(RuntimeUiWorkbenchDiagnostic{
        .code = code,
        .owner_id = std::move(owner_id),
        .message = std::move(message),
    });
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool contains_panel_id(const std::vector<RuntimeUiWorkbenchPanelRow>& panels,
                                     std::string_view id) noexcept {
    return std::ranges::any_of(panels, [id](const RuntimeUiWorkbenchPanelRow& panel) { return panel.id == id; });
}

[[nodiscard]] bool contains_table_column(const std::vector<RuntimeUiWorkbenchTableColumn>& columns,
                                         std::string_view panel_id, std::string_view column_id) noexcept {
    return std::ranges::any_of(columns, [panel_id, column_id](const RuntimeUiWorkbenchTableColumn& column) {
        return column.panel_id == panel_id && column.id == column_id;
    });
}

[[nodiscard]] bool is_valid_panel_kind(RuntimeUiWorkbenchPanelKind kind) noexcept {
    switch (kind) {
    case RuntimeUiWorkbenchPanelKind::menu:
    case RuntimeUiWorkbenchPanelKind::inventory:
    case RuntimeUiWorkbenchPanelKind::equipment:
    case RuntimeUiWorkbenchPanelKind::shop:
    case RuntimeUiWorkbenchPanelKind::simulation_dashboard:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_valid_item_kind(RuntimeUiWorkbenchItemRowKind kind) noexcept {
    switch (kind) {
    case RuntimeUiWorkbenchItemRowKind::inventory:
    case RuntimeUiWorkbenchItemRowKind::equipment:
    case RuntimeUiWorkbenchItemRowKind::shop:
        return true;
    }
    return false;
}

[[nodiscard]] bool is_positive_runtime_ui_rect(Rect rect) noexcept {
    return is_valid_rect(rect) && rect.width > 0.0F && rect.height > 0.0F;
}

[[nodiscard]] std::string lowercase_ascii(std::string_view value) {
    std::string result;
    result.reserve(value.size());
    for (const char character : value) {
        result.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }
    return result;
}

[[nodiscard]] bool is_runtime_ui_workbench_token_character(char character) noexcept {
    return std::isalnum(static_cast<unsigned char>(character)) != 0;
}

[[nodiscard]] bool is_forbidden_backend_token(std::string_view token) noexcept {
    constexpr std::array<std::string_view, 11> forbidden_tokens{
        "backend", "native", "renderer", "rhi", "d3d12", "vulkan", "sdl", "sdl3", "imgui", "gpu", "middleware",
    };
    return std::ranges::find(forbidden_tokens, token) != forbidden_tokens.end();
}

[[nodiscard]] bool has_backend_reference(std::string_view value) {
    const auto lower = lowercase_ascii(value);
    std::size_t token_begin = 0U;
    while (token_begin < lower.size()) {
        while (token_begin < lower.size() && !is_runtime_ui_workbench_token_character(lower[token_begin])) {
            ++token_begin;
        }
        auto token_end = token_begin;
        while (token_end < lower.size() && is_runtime_ui_workbench_token_character(lower[token_end])) {
            ++token_end;
        }
        if (token_begin < token_end &&
            is_forbidden_backend_token(std::string_view{lower}.substr(token_begin, token_end - token_begin))) {
            return true;
        }
        token_begin = token_end;
    }
    return false;
}

[[nodiscard]] std::string table_column_owner_id(std::string_view panel_id, std::string_view column_id) {
    std::string owner_id{panel_id};
    owner_id += ":";
    owner_id += column_id;
    return owner_id;
}

[[nodiscard]] std::string table_cell_owner_id(std::string_view row_id, std::string_view column_id) {
    std::string owner_id{row_id};
    owner_id += ":";
    owner_id += column_id;
    return owner_id;
}

void validate_localization_key(std::vector<RuntimeUiWorkbenchDiagnostic>& diagnostics, std::string_view owner_id,
                               std::string_view key) {
    if (key.empty()) {
        append_diagnostic(diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_required_localization_key,
                          std::string{owner_id}, "runtime ui workbench localization key must not be empty");
        return;
    }
    if (has_backend_reference(key)) {
        append_diagnostic(diagnostics, RuntimeUiWorkbenchDiagnosticCode::unsupported_backend_reference,
                          std::string{owner_id},
                          "runtime ui workbench localization key must not reference backend or native services");
    }
}

void validate_accessibility_label(std::vector<RuntimeUiWorkbenchDiagnostic>& diagnostics, std::string_view owner_id,
                                  std::string_view label) {
    if (label.empty()) {
        append_diagnostic(diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_accessibility_label,
                          std::string{owner_id}, "runtime ui workbench accessibility label must not be empty");
        return;
    }
    if (has_backend_reference(label)) {
        append_diagnostic(diagnostics, RuntimeUiWorkbenchDiagnosticCode::unsupported_backend_reference,
                          std::string{owner_id},
                          "runtime ui workbench accessibility label must not reference backend or native services");
    }
}

void append_localization_ref(std::vector<RuntimeUiWorkbenchLocalizationRef>& refs, std::string_view owner_id,
                             std::string_view key) {
    if (!key.empty()) {
        refs.push_back(RuntimeUiWorkbenchLocalizationRef{.owner_id = std::string{owner_id}, .key = std::string{key}});
    }
}

void append_accessibility_ref(std::vector<RuntimeUiWorkbenchAccessibilityRef>& refs, std::string_view owner_id,
                              std::string_view label, bool focusable) {
    if (!label.empty()) {
        refs.push_back(RuntimeUiWorkbenchAccessibilityRef{
            .owner_id = std::string{owner_id},
            .label = std::string{label},
            .focusable = focusable,
        });
    }
}

void validate_focus_ref(std::vector<RuntimeUiWorkbenchDiagnostic>& diagnostics, const std::vector<std::string>& targets,
                        std::string_view edge_id, std::string_view ref) {
    if (ref.empty()) {
        return;
    }
    if (!contains_id(targets, ref)) {
        append_diagnostic(diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_focus_target, std::string{edge_id},
                          "runtime ui workbench focus edge references an unknown focus target");
    }
}

} // namespace

bool RuntimeUiWorkbenchPlan::succeeded() const noexcept {
    return status == RuntimeUiWorkbenchStatus::ready && diagnostics.empty();
}

RuntimeUiWorkbenchPlan plan_runtime_ui_workbench(const RuntimeUiWorkbenchDocument& document) {
    RuntimeUiWorkbenchPlan plan;

    if (document.id.empty()) {
        append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_document_id, {},
                          "runtime ui workbench document id must not be empty");
    } else if (has_backend_reference(document.id)) {
        append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unsupported_backend_reference,
                          document.id, "runtime ui workbench document id must not reference backend services");
    }
    validate_localization_key(plan.diagnostics, document.id, document.title_localization_key);

    std::vector<std::string> panel_ids;
    panel_ids.reserve(document.panels.size());
    for (const auto& panel : document.panels) {
        if (panel.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_panel_id, {},
                              "runtime ui workbench panel id must not be empty");
        } else if (contains_id(panel_ids, panel.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_panel_id, panel.id,
                              "runtime ui workbench panel ids must be unique");
        } else {
            panel_ids.push_back(panel.id);
        }
        if (!is_valid_panel_kind(panel.kind)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unsupported_panel_kind, panel.id,
                              "runtime ui workbench panel kind is unsupported");
        }
        validate_localization_key(plan.diagnostics, panel.id, panel.title_localization_key);
        validate_accessibility_label(plan.diagnostics, panel.id, panel.accessibility_label);
    }

    std::vector<std::string> table_column_ids;
    table_column_ids.reserve(document.table_columns.size());
    for (const auto& column : document.table_columns) {
        const auto column_key = table_column_owner_id(column.panel_id, column.id);
        if (!contains_panel_id(document.panels, column.panel_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_panel_id, column.id,
                              "runtime ui workbench table column references an unknown panel");
        }
        if (column.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_table_column_id,
                              column.panel_id, "runtime ui workbench table column id must not be empty");
        } else if (contains_id(table_column_ids, column_key)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_table_column_id, column.id,
                              "runtime ui workbench table column ids must be unique per panel");
        } else {
            table_column_ids.push_back(column_key);
        }
        validate_localization_key(plan.diagnostics, column_key, column.localization_key);
    }

    std::vector<std::string> table_row_ids;
    table_row_ids.reserve(document.table_rows.size());
    for (const auto& row : document.table_rows) {
        if (!contains_panel_id(document.panels, row.panel_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_panel_id, row.id,
                              "runtime ui workbench table row references an unknown panel");
        }
        if (row.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_table_row_id, {},
                              "runtime ui workbench table row id must not be empty");
        } else if (contains_id(table_row_ids, row.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_table_row_id, row.id,
                              "runtime ui workbench table row ids must be unique");
        } else {
            table_row_ids.push_back(row.id);
        }
        for (const auto& cell : row.cells) {
            if (!contains_table_column(document.table_columns, row.panel_id, cell.column_id)) {
                append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_table_column, row.id,
                                  "runtime ui workbench table cell references an unknown column");
            }
            if (!cell.localization_key.empty()) {
                validate_localization_key(plan.diagnostics, table_cell_owner_id(row.id, cell.column_id),
                                          cell.localization_key);
            }
        }
    }

    std::vector<std::string> graph_ids;
    graph_ids.reserve(document.graph_series.size());
    for (const auto& series : document.graph_series) {
        if (!contains_panel_id(document.panels, series.panel_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_panel_id, series.id,
                              "runtime ui workbench graph series references an unknown panel");
        }
        if (series.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_graph_series_id, {},
                              "runtime ui workbench graph series id must not be empty");
        } else if (contains_id(graph_ids, series.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_graph_series_id, series.id,
                              "runtime ui workbench graph series ids must be unique");
        } else {
            graph_ids.push_back(series.id);
        }
        if (series.points.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::empty_graph_series, series.id,
                              "runtime ui workbench graph series must contain at least one point");
        }
        for (const auto& point : series.points) {
            if (!std::isfinite(point.x) || !std::isfinite(point.y)) {
                append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_graph_point, series.id,
                                  "runtime ui workbench graph points must be finite");
            }
        }
        validate_localization_key(plan.diagnostics, series.id, series.localization_key);
        validate_accessibility_label(plan.diagnostics, series.id, series.accessibility_label);
    }

    std::vector<std::string> item_ids;
    item_ids.reserve(document.item_rows.size());
    std::vector<std::string> equipment_slots;
    equipment_slots.reserve(document.item_rows.size());
    for (const auto& item : document.item_rows) {
        if (!contains_panel_id(document.panels, item.panel_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_panel_id, item.id,
                              "runtime ui workbench item row references an unknown panel");
        }
        if (item.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_item_row_id, {},
                              "runtime ui workbench item row id must not be empty");
        } else if (contains_id(item_ids, item.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_item_row_id, item.id,
                              "runtime ui workbench item row ids must be unique");
        } else {
            item_ids.push_back(item.id);
        }
        if (!is_valid_item_kind(item.kind)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unsupported_item_row_kind, item.id,
                              "runtime ui workbench item row kind is unsupported");
        }
        if (item.item_id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_item_id, item.id,
                              "runtime ui workbench item row requires an item id");
        }
        if (item.quantity == 0U) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_inventory_quantity, item.id,
                              "runtime ui workbench item row quantity must be positive");
        }
        if (item.kind == RuntimeUiWorkbenchItemRowKind::equipment) {
            if (item.slot_id.empty()) {
                append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_equipment_slot, item.id,
                                  "runtime ui workbench equipment rows require a slot id");
            } else if (contains_id(equipment_slots, item.slot_id)) {
                append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_item_row_id, item.id,
                                  "runtime ui workbench equipment slot rows must be unique");
            } else {
                equipment_slots.push_back(item.slot_id);
            }
        }
        if (item.kind == RuntimeUiWorkbenchItemRowKind::shop && item.price == 0U) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_shop_price, item.id,
                              "runtime ui workbench shop rows require a positive price");
        }
        validate_localization_key(plan.diagnostics, item.id, item.localization_key);
        validate_accessibility_label(plan.diagnostics, item.id, item.accessibility_label);
    }

    std::vector<std::string> text_input_ids;
    text_input_ids.reserve(document.text_inputs.size());
    for (const auto& input : document.text_inputs) {
        if (!contains_panel_id(document.panels, input.panel_id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::unknown_panel_id, input.id,
                              "runtime ui workbench text input references an unknown panel");
        }
        if (input.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_text_input_id, {},
                              "runtime ui workbench text input id must not be empty");
        } else if (contains_id(text_input_ids, input.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_text_input_id, input.id,
                              "runtime ui workbench text input ids must be unique");
        } else {
            text_input_ids.push_back(input.id);
        }
        if (input.target.value.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_text_input_target, input.id,
                              "runtime ui workbench text input target must not be empty");
        }
        if (!is_positive_runtime_ui_rect(input.text_bounds)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_text_input_bounds, input.id,
                              "runtime ui workbench text input bounds must be positive");
        }
        if (input.max_code_units == 0U) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::invalid_text_input_limit, input.id,
                              "runtime ui workbench text input limit must be positive");
        }
        validate_localization_key(plan.diagnostics, input.id, input.placeholder_localization_key);
        validate_accessibility_label(plan.diagnostics, input.id, input.accessibility_label);
    }

    std::vector<std::string> focus_targets;
    focus_targets.reserve(document.item_rows.size() + document.text_inputs.size());
    const auto add_focus_target = [&plan, &focus_targets](std::string_view id) {
        if (id.empty()) {
            return;
        }
        if (contains_id(focus_targets, id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_focus_target,
                              std::string{id}, "runtime ui workbench focus target ids must be globally unique");
            return;
        }
        focus_targets.push_back(std::string{id});
    };
    for (const auto& item : document.item_rows) {
        add_focus_target(item.id);
    }
    for (const auto& input : document.text_inputs) {
        add_focus_target(input.id);
    }

    validate_focus_ref(plan.diagnostics, focus_targets, document.id, document.initial_focus_id);
    std::vector<std::string> focus_edge_ids;
    focus_edge_ids.reserve(document.focus_edges.size());
    for (const auto& edge : document.focus_edges) {
        if (edge.id.empty()) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::missing_focus_target_id, {},
                              "runtime ui workbench focus edge target id must not be empty");
        } else if (contains_id(focus_edge_ids, edge.id)) {
            append_diagnostic(plan.diagnostics, RuntimeUiWorkbenchDiagnosticCode::duplicate_focus_target, edge.id,
                              "runtime ui workbench focus edges must be unique per target");
        } else {
            focus_edge_ids.push_back(edge.id);
        }
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.id);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.next);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.previous);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.up);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.down);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.left);
        validate_focus_ref(plan.diagnostics, focus_targets, edge.id, edge.right);
    }

    if (!plan.diagnostics.empty()) {
        return plan;
    }

    plan.status = RuntimeUiWorkbenchStatus::ready;
    plan.panels = document.panels;
    plan.table_columns = document.table_columns;
    plan.table_rows = document.table_rows;
    plan.graph_series = document.graph_series;
    plan.item_rows = document.item_rows;
    plan.text_inputs = document.text_inputs;
    plan.focus_plan = RuntimeUiWorkbenchFocusPlan{
        .initial_focus_id = document.initial_focus_id,
        .edges = document.focus_edges,
    };

    append_localization_ref(plan.localization_references, document.id, document.title_localization_key);
    for (const auto& panel : document.panels) {
        append_localization_ref(plan.localization_references, panel.id, panel.title_localization_key);
        append_accessibility_ref(plan.accessibility_references, panel.id, panel.accessibility_label, false);
    }
    for (const auto& column : document.table_columns) {
        append_localization_ref(plan.localization_references, table_column_owner_id(column.panel_id, column.id),
                                column.localization_key);
    }
    for (const auto& row : document.table_rows) {
        for (const auto& cell : row.cells) {
            append_localization_ref(plan.localization_references, table_cell_owner_id(row.id, cell.column_id),
                                    cell.localization_key);
        }
    }
    for (const auto& series : document.graph_series) {
        append_localization_ref(plan.localization_references, series.id, series.localization_key);
        append_accessibility_ref(plan.accessibility_references, series.id, series.accessibility_label, false);
    }
    for (const auto& item : document.item_rows) {
        append_localization_ref(plan.localization_references, item.id, item.localization_key);
        append_accessibility_ref(plan.accessibility_references, item.id, item.accessibility_label, true);
    }
    for (const auto& input : document.text_inputs) {
        append_localization_ref(plan.localization_references, input.id, input.placeholder_localization_key);
        append_accessibility_ref(plan.accessibility_references, input.id, input.accessibility_label, true);
        plan.platform_text_input_requests.push_back(PlatformTextInputRequest{
            .target = input.target,
            .text_bounds = input.text_bounds,
        });
    }

    return plan;
}

} // namespace mirakana::ui
