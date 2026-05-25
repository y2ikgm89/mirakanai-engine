// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/ui/runtime_ui_workbench.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>
#include <string_view>
#include <vector>

namespace {

[[nodiscard]] bool has_diagnostic(const std::vector<mirakana::ui::RuntimeUiWorkbenchDiagnostic>& diagnostics,
                                  mirakana::ui::RuntimeUiWorkbenchDiagnosticCode code) {
    return std::ranges::any_of(diagnostics, [code](const auto& diagnostic) { return diagnostic.code == code; });
}

[[nodiscard]] bool has_localization_ref(const std::vector<mirakana::ui::RuntimeUiWorkbenchLocalizationRef>& refs,
                                        std::string_view owner_id, std::string_view key) {
    return std::ranges::any_of(refs,
                               [owner_id, key](const auto& ref) { return ref.owner_id == owner_id && ref.key == key; });
}

[[nodiscard]] bool has_accessibility_ref(const std::vector<mirakana::ui::RuntimeUiWorkbenchAccessibilityRef>& refs,
                                         std::string_view owner_id, std::string_view label) {
    return std::ranges::any_of(
        refs, [owner_id, label](const auto& ref) { return ref.owner_id == owner_id && ref.label == label; });
}

[[nodiscard]] mirakana::ui::RuntimeUiWorkbenchDocument make_dense_workbench_document() {
    using namespace mirakana::ui;

    RuntimeUiWorkbenchDocument document;
    document.id = "sample.production.runtime_ui_workbench";
    document.title_localization_key = "ui.workbench.title";
    document.panels = {
        {.id = "menu.pause",
         .kind = RuntimeUiWorkbenchPanelKind::menu,
         .title_localization_key = "ui.menu.pause.title",
         .accessibility_label = "Pause menu"},
        {.id = "inventory.pack",
         .kind = RuntimeUiWorkbenchPanelKind::inventory,
         .title_localization_key = "ui.inventory.title",
         .accessibility_label = "Inventory"},
        {.id = "equipment.paperdoll",
         .kind = RuntimeUiWorkbenchPanelKind::equipment,
         .title_localization_key = "ui.equipment.title",
         .accessibility_label = "Equipment"},
        {.id = "shop.vendor",
         .kind = RuntimeUiWorkbenchPanelKind::shop,
         .title_localization_key = "ui.shop.title",
         .accessibility_label = "Shop"},
        {.id = "dashboard.colony",
         .kind = RuntimeUiWorkbenchPanelKind::simulation_dashboard,
         .title_localization_key = "ui.dashboard.title",
         .accessibility_label = "Simulation dashboard"},
    };
    document.table_columns = {
        {.panel_id = "dashboard.colony", .id = "resource", .localization_key = "ui.table.resource"},
        {.panel_id = "dashboard.colony", .id = "stored", .localization_key = "ui.table.stored"},
        {.panel_id = "dashboard.colony", .id = "delta", .localization_key = "ui.table.delta"},
    };
    document.table_rows = {
        {.id = "dashboard.food",
         .panel_id = "dashboard.colony",
         .cells = {{.column_id = "resource", .localization_key = "ui.resource.food"},
                   {.column_id = "stored", .text = "124"},
                   {.column_id = "delta", .text = "+8"}}},
        {.id = "dashboard.power",
         .panel_id = "dashboard.colony",
         .cells = {{.column_id = "resource", .localization_key = "ui.resource.power"},
                   {.column_id = "stored", .text = "72"},
                   {.column_id = "delta", .text = "-3"}}},
    };
    document.graph_series = {
        {.id = "graph.food",
         .panel_id = "dashboard.colony",
         .localization_key = "ui.graph.food",
         .accessibility_label = "Food trend",
         .points = {{.x = 0.0, .y = 100.0}, {.x = 1.0, .y = 108.0}}},
        {.id = "graph.power",
         .panel_id = "dashboard.colony",
         .localization_key = "ui.graph.power",
         .accessibility_label = "Power trend",
         .points = {{.x = 0.0, .y = 75.0}, {.x = 1.0, .y = 72.0}}},
    };
    document.item_rows = {
        {.id = "inventory.potion",
         .panel_id = "inventory.pack",
         .kind = RuntimeUiWorkbenchItemRowKind::inventory,
         .item_id = "item.potion",
         .quantity = 3,
         .localization_key = "ui.item.potion",
         .accessibility_label = "Potion"},
        {.id = "equipment.weapon",
         .panel_id = "equipment.paperdoll",
         .kind = RuntimeUiWorkbenchItemRowKind::equipment,
         .item_id = "item.sword",
         .slot_id = "slot.weapon",
         .quantity = 1,
         .localization_key = "ui.item.sword",
         .accessibility_label = "Equipped weapon"},
        {.id = "shop.elixir",
         .panel_id = "shop.vendor",
         .kind = RuntimeUiWorkbenchItemRowKind::shop,
         .item_id = "item.elixir",
         .quantity = 4,
         .price = 25,
         .localization_key = "ui.item.elixir",
         .accessibility_label = "Elixir for sale"},
    };
    document.text_inputs = {
        {.id = "input.search",
         .panel_id = "inventory.pack",
         .target = ElementId{"inventory.search"},
         .text_bounds = Rect{.x = 8.0F, .y = 8.0F, .width = 220.0F, .height = 32.0F},
         .placeholder_localization_key = "ui.inventory.search.placeholder",
         .accessibility_label = "Search inventory",
         .max_code_units = 64},
    };
    document.initial_focus_id = "inventory.potion";
    document.focus_edges = {
        {.id = "inventory.potion",
         .next = "equipment.weapon",
         .previous = {},
         .up = {},
         .down = "input.search",
         .left = {},
         .right = "equipment.weapon"},
        {.id = "equipment.weapon",
         .next = "shop.elixir",
         .previous = "inventory.potion",
         .up = {},
         .down = {},
         .left = {},
         .right = "shop.elixir"},
        {.id = "shop.elixir",
         .next = "input.search",
         .previous = "equipment.weapon",
         .up = {},
         .down = {},
         .left = "equipment.weapon",
         .right = {}},
        {.id = "input.search",
         .next = "inventory.potion",
         .previous = "shop.elixir",
         .up = "inventory.potion",
         .down = {},
         .left = {},
         .right = {}},
    };

    return document;
}

} // namespace

MK_TEST("runtime ui workbench plans dense runtime rows without adapter invocation") {
    const auto plan = mirakana::ui::plan_runtime_ui_workbench(make_dense_workbench_document());

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiWorkbenchStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.panels.size() == 5U);
    MK_REQUIRE(plan.table_columns.size() == 3U);
    MK_REQUIRE(plan.table_rows.size() == 2U);
    MK_REQUIRE(plan.graph_series.size() == 2U);
    MK_REQUIRE(plan.item_rows.size() == 3U);
    MK_REQUIRE(plan.text_inputs.size() == 1U);
    MK_REQUIRE(plan.platform_text_input_requests.size() == 1U);
    MK_REQUIRE(plan.platform_text_input_requests[0].target == mirakana::ui::ElementId{"inventory.search"});
    MK_REQUIRE(plan.focus_plan.initial_focus_id == "inventory.potion");
    MK_REQUIRE(plan.focus_plan.edges.size() == 4U);
    MK_REQUIRE(plan.localization_references.size() == 17U);
    MK_REQUIRE(plan.accessibility_references.size() == 11U);
    MK_REQUIRE(has_localization_ref(plan.localization_references, "dashboard.colony:resource", "ui.table.resource"));
    MK_REQUIRE(has_localization_ref(plan.localization_references, "dashboard.food:resource", "ui.resource.food"));
    MK_REQUIRE(has_accessibility_ref(plan.accessibility_references, "inventory.potion", "Potion"));
    MK_REQUIRE(!plan.invoked_renderer_submission);
    MK_REQUIRE(!plan.invoked_text_shaping);
    MK_REQUIRE(!plan.invoked_font_rasterization);
    MK_REQUIRE(!plan.invoked_ime);
    MK_REQUIRE(!plan.invoked_accessibility_bridge);
    MK_REQUIRE(!plan.invoked_image_decoding);
    MK_REQUIRE(!plan.invoked_native_platform);
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui workbench accepts bounded natural language and localization tokens") {
    auto document = make_dense_workbench_document();
    document.id = "sample.production.runtime_ui_alternative";
    document.panels[0].title_localization_key = "ui.menu.alternative.title";
    document.item_rows[1].accessibility_label = "Metal sword";

    const auto plan = mirakana::ui::plan_runtime_ui_workbench(document);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiWorkbenchStatus::ready);
    MK_REQUIRE(plan.succeeded());
    MK_REQUIRE(plan.diagnostics.empty());
}

MK_TEST("runtime ui workbench rejects empty focus target ids") {
    auto document = make_dense_workbench_document();
    document.graph_series[0].id.clear();
    document.item_rows[0].id.clear();
    document.text_inputs[0].id.clear();
    document.focus_edges[0].id.clear();

    const auto plan = mirakana::ui::plan_runtime_ui_workbench(document);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiWorkbenchStatus::invalid_request);
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::missing_graph_series_id));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::missing_item_row_id));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::missing_text_input_id));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::missing_focus_target_id));
}

MK_TEST("runtime ui workbench rejects colliding focus target ids") {
    auto document = make_dense_workbench_document();
    document.item_rows[0].id = "input.search";

    const auto plan = mirakana::ui::plan_runtime_ui_workbench(document);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiWorkbenchStatus::invalid_request);
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::duplicate_focus_target));
}

MK_TEST("runtime ui workbench fails closed for invalid dense ui and backend references") {
    auto document = make_dense_workbench_document();
    document.panels.push_back(document.panels.front());
    document.table_rows[0].cells.push_back({.column_id = "missing", .text = "broken"});
    document.graph_series[0].points.push_back({.x = 2.0, .y = std::numeric_limits<double>::quiet_NaN()});
    document.item_rows[1].slot_id.clear();
    document.item_rows[2].price = 0;
    document.text_inputs[0].text_bounds.width = 0.0F;
    document.text_inputs[0].placeholder_localization_key = "ui.renderer.native.handle";
    document.initial_focus_id = "missing.focus";
    document.focus_edges[0].next = "missing.focus";

    const auto plan = mirakana::ui::plan_runtime_ui_workbench(document);

    MK_REQUIRE(plan.status == mirakana::ui::RuntimeUiWorkbenchStatus::invalid_request);
    MK_REQUIRE(!plan.succeeded());
    MK_REQUIRE(plan.panels.empty());
    MK_REQUIRE(plan.table_rows.empty());
    MK_REQUIRE(plan.graph_series.empty());
    MK_REQUIRE(plan.item_rows.empty());
    MK_REQUIRE(plan.text_inputs.empty());
    MK_REQUIRE(plan.platform_text_input_requests.empty());
    MK_REQUIRE(plan.localization_references.empty());
    MK_REQUIRE(plan.accessibility_references.empty());
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::duplicate_panel_id));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::unknown_table_column));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::invalid_graph_point));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::missing_equipment_slot));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::invalid_shop_price));
    MK_REQUIRE(
        has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::invalid_text_input_bounds));
    MK_REQUIRE(has_diagnostic(plan.diagnostics,
                              mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::unsupported_backend_reference));
    MK_REQUIRE(has_diagnostic(plan.diagnostics, mirakana::ui::RuntimeUiWorkbenchDiagnosticCode::unknown_focus_target));
}

int main() {
    return mirakana::test::run_all();
}
