// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/ui/ui.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana::ui {

enum class RuntimeUiWorkbenchStatus : std::uint8_t {
    ready,
    invalid_request,
};

enum class RuntimeUiWorkbenchPanelKind : std::uint8_t {
    menu,
    inventory,
    equipment,
    shop,
    simulation_dashboard,
};

enum class RuntimeUiWorkbenchItemRowKind : std::uint8_t {
    inventory,
    equipment,
    shop,
};

enum class RuntimeUiWorkbenchDiagnosticCode : std::uint8_t {
    missing_document_id,
    missing_panel_id,
    duplicate_panel_id,
    unsupported_panel_kind,
    unknown_panel_id,
    missing_table_column_id,
    duplicate_table_column_id,
    duplicate_table_row_id,
    unknown_table_column,
    missing_graph_series_id,
    duplicate_graph_series_id,
    empty_graph_series,
    invalid_graph_point,
    missing_item_row_id,
    duplicate_item_row_id,
    unsupported_item_row_kind,
    missing_item_id,
    invalid_inventory_quantity,
    missing_equipment_slot,
    invalid_shop_price,
    missing_text_input_id,
    duplicate_text_input_id,
    invalid_text_input_target,
    invalid_text_input_bounds,
    invalid_text_input_limit,
    missing_focus_target_id,
    duplicate_focus_target,
    unknown_focus_target,
    unsupported_backend_reference,
    missing_required_localization_key,
    missing_accessibility_label,
};

struct RuntimeUiWorkbenchPanelRow {
    std::string id;
    RuntimeUiWorkbenchPanelKind kind{RuntimeUiWorkbenchPanelKind::menu};
    std::string title_localization_key;
    std::string accessibility_label;
    bool enabled{true};
};

struct RuntimeUiWorkbenchTableColumn {
    std::string panel_id;
    std::string id;
    std::string localization_key;
};

struct RuntimeUiWorkbenchTableCell {
    std::string column_id;
    std::string text;
    std::string localization_key;
};

struct RuntimeUiWorkbenchTableRow {
    std::string id;
    std::string panel_id;
    std::vector<RuntimeUiWorkbenchTableCell> cells;
};

struct RuntimeUiWorkbenchGraphPoint {
    double x{0.0};
    double y{0.0};
};

struct RuntimeUiWorkbenchGraphSeries {
    std::string id;
    std::string panel_id;
    std::string localization_key;
    std::string accessibility_label;
    std::vector<RuntimeUiWorkbenchGraphPoint> points;
};

struct RuntimeUiWorkbenchItemRow {
    std::string id;
    std::string panel_id;
    RuntimeUiWorkbenchItemRowKind kind{RuntimeUiWorkbenchItemRowKind::inventory};
    std::string item_id;
    std::string slot_id;
    std::uint32_t quantity{0};
    std::uint32_t price{0};
    std::string localization_key;
    std::string accessibility_label;
};

struct RuntimeUiWorkbenchTextInputFieldRow {
    std::string id;
    std::string panel_id;
    ElementId target;
    Rect text_bounds;
    std::string placeholder_localization_key;
    std::string accessibility_label;
    std::size_t max_code_units{0};
};

struct RuntimeUiWorkbenchFocusEdge {
    std::string id;
    std::string next;
    std::string previous;
    std::string up;
    std::string down;
    std::string left;
    std::string right;
};

struct RuntimeUiWorkbenchFocusPlan {
    std::string initial_focus_id;
    std::vector<RuntimeUiWorkbenchFocusEdge> edges;
};

struct RuntimeUiWorkbenchLocalizationRef {
    std::string owner_id;
    std::string key;
};

struct RuntimeUiWorkbenchAccessibilityRef {
    std::string owner_id;
    std::string label;
    bool focusable{false};
};

struct RuntimeUiWorkbenchDocument {
    std::string id;
    std::string title_localization_key;
    std::vector<RuntimeUiWorkbenchPanelRow> panels;
    std::vector<RuntimeUiWorkbenchTableColumn> table_columns;
    std::vector<RuntimeUiWorkbenchTableRow> table_rows;
    std::vector<RuntimeUiWorkbenchGraphSeries> graph_series;
    std::vector<RuntimeUiWorkbenchItemRow> item_rows;
    std::vector<RuntimeUiWorkbenchTextInputFieldRow> text_inputs;
    std::string initial_focus_id;
    std::vector<RuntimeUiWorkbenchFocusEdge> focus_edges;
};

struct RuntimeUiWorkbenchDiagnostic {
    RuntimeUiWorkbenchDiagnosticCode code{RuntimeUiWorkbenchDiagnosticCode::missing_document_id};
    std::string owner_id;
    std::string message;
};

struct RuntimeUiWorkbenchPlan {
    RuntimeUiWorkbenchStatus status{RuntimeUiWorkbenchStatus::invalid_request};
    std::vector<RuntimeUiWorkbenchPanelRow> panels;
    std::vector<RuntimeUiWorkbenchTableColumn> table_columns;
    std::vector<RuntimeUiWorkbenchTableRow> table_rows;
    std::vector<RuntimeUiWorkbenchGraphSeries> graph_series;
    std::vector<RuntimeUiWorkbenchItemRow> item_rows;
    std::vector<RuntimeUiWorkbenchTextInputFieldRow> text_inputs;
    RuntimeUiWorkbenchFocusPlan focus_plan;
    std::vector<PlatformTextInputRequest> platform_text_input_requests;
    std::vector<RuntimeUiWorkbenchLocalizationRef> localization_references;
    std::vector<RuntimeUiWorkbenchAccessibilityRef> accessibility_references;
    std::vector<RuntimeUiWorkbenchDiagnostic> diagnostics;
    bool invoked_renderer_submission{false};
    bool invoked_text_shaping{false};
    bool invoked_font_rasterization{false};
    bool invoked_ime{false};
    bool invoked_accessibility_bridge{false};
    bool invoked_image_decoding{false};
    bool invoked_native_platform{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeUiWorkbenchPlan plan_runtime_ui_workbench(const RuntimeUiWorkbenchDocument& document);

} // namespace mirakana::ui
