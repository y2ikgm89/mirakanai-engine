// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/asset_browser_production.hpp"

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/editor/content_browser.hpp"
#include "mirakana/ui/ui.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] std::string asset_row_id(AssetId asset) {
    return "asset_browser.source_pulse." + std::to_string(asset.value);
}

[[nodiscard]] bool import_plan_contains(const AssetImportPlan* plan, AssetId asset, std::string_view output_path) {
    if (plan == nullptr) {
        return false;
    }
    return std::ranges::any_of(plan->actions, [asset, output_path](const AssetImportAction& action) {
        return action.id == asset && (output_path.empty() || action.output_path == output_path);
    });
}

[[nodiscard]] EditorAssetBrowserSourcePulseRow make_source_pulse_row(const ContentBrowserItem& item,
                                                                     const ContentBrowserItem* selected,
                                                                     const AssetImportPlan* import_plan) {
    const bool import_planned = import_plan_contains(import_plan, item.id, item.path);
    EditorAssetBrowserSourcePulseRow row{
        .asset = item.id,
        .kind = item.kind,
        .row_id = asset_row_id(item.id),
        .kind_label = std::string(asset_kind_label(item.kind)),
        .asset_key_label = item.asset_key_label,
        .source_path = item.identity_source_path,
        .imported_path = item.path,
        .display_name = item.display_name,
        .scope_label = item.identity_source_path.empty() ? "cooked" : "source",
        .state_label = item.identity_backed ? "ready" : "missing_identity",
        .import_status_label = import_planned ? "planned" : "not_planned",
        .package_status_label = "not_reviewed",
        .provenance_status_label = "not_reviewed",
        .preview_status_label = "not_requested",
        .hot_reload_status_label = "not_staged",
        .selected = selected != nullptr && selected->id == item.id,
        .identity_backed = item.identity_backed,
        .source_visible = !item.identity_source_path.empty(),
        .package_visible = false,
        .blocked = !item.identity_backed,
        .host_gated = false,
    };
    return row;
}

void sort_rows(std::vector<EditorAssetBrowserSourcePulseRow>& rows) {
    std::ranges::sort(rows,
                      [](const EditorAssetBrowserSourcePulseRow& lhs, const EditorAssetBrowserSourcePulseRow& rhs) {
                          if (lhs.imported_path != rhs.imported_path) {
                              return lhs.imported_path < rhs.imported_path;
                          }
                          if (lhs.asset_key_label != rhs.asset_key_label) {
                              return lhs.asset_key_label < rhs.asset_key_label;
                          }
                          return lhs.row_id < rhs.row_id;
                      });
}

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

void require_safe_field(std::string_view field, std::string_view value) {
    if (value.empty() || contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("asset browser production ui field is invalid: ") + std::string(field));
    }
}

void require_safe_label(std::string_view field, std::string_view value) {
    if (contains_line_separator(value)) {
        throw std::invalid_argument(std::string("asset browser production ui label is invalid: ") + std::string(field));
    }
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("asset browser production ui element could not be added");
    }
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

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  std::string label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(std::move(label));
    add_or_throw(document, std::move(desc));
}

} // namespace

std::string_view editor_asset_browser_production_status_label(EditorAssetBrowserProductionStatus status) noexcept {
    switch (status) {
    case EditorAssetBrowserProductionStatus::empty:
        return "Asset browser empty";
    case EditorAssetBrowserProductionStatus::ready:
        return "Asset browser ready";
    case EditorAssetBrowserProductionStatus::attention:
        return "Asset browser attention";
    }
    return "Asset browser empty";
}

EditorAssetBrowserProductionModel
make_editor_asset_browser_production_model(const EditorAssetBrowserProductionDesc& desc) {
    EditorAssetBrowserProductionModel model;
    model.project_root = desc.project_root;
    model.asset_root = desc.asset_root;
    model.source_registry_path = desc.source_registry_path;

    if (desc.browser == nullptr) {
        model.diagnostics.push_back("content browser state is missing");
        return model;
    }

    const auto visible_items = desc.browser->visible_items();
    const auto* selected = desc.browser->selected_asset();
    model.total_row_count = desc.browser->item_count();
    model.visible_row_count = visible_items.size();
    model.rows.reserve(visible_items.size());
    for (const auto& item : visible_items) {
        model.rows.push_back(make_source_pulse_row(item, selected, desc.import_plan));
    }
    sort_rows(model.rows);

    model.status =
        model.rows.empty() ? EditorAssetBrowserProductionStatus::empty : EditorAssetBrowserProductionStatus::ready;
    model.status_label = std::string(editor_asset_browser_production_status_label(model.status));
    return model;
}

mirakana::ui::UiDocument make_editor_asset_browser_production_ui_model(const EditorAssetBrowserProductionModel& model) {
    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("asset_browser", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"asset_browser"};

    require_safe_field("status", model.status_label);
    require_safe_label("source_registry_path", model.source_registry_path);
    append_label(document, root, "asset_browser.status", model.status_label);
    append_label(document, root, "asset_browser.source_registry.path",
                 model.source_registry_path.empty() ? "-" : model.source_registry_path);
    append_label(document, root, "asset_browser.total_rows", std::to_string(model.total_row_count));
    append_label(document, root, "asset_browser.visible_rows", std::to_string(model.visible_row_count));

    add_or_throw(document, make_child("asset_browser.source_pulse", root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId list_root{"asset_browser.source_pulse"};

    for (const auto& row : model.rows) {
        require_safe_field("row_id", row.row_id);
        require_safe_label("asset_key_label", row.asset_key_label);
        require_safe_label("source_path", row.source_path);
        require_safe_label("imported_path", row.imported_path);
        require_safe_field("state_label", row.state_label);

        mirakana::ui::ElementDesc item = make_child(row.row_id, list_root, mirakana::ui::SemanticRole::list_item);
        item.text = make_text(row.display_name.empty() ? row.asset_key_label : row.display_name);
        item.enabled = !row.blocked;
        add_or_throw(document, std::move(item));

        const mirakana::ui::ElementId item_id{row.row_id};
        append_label(document, item_id, row.row_id + ".asset_key",
                     row.asset_key_label.empty() ? "-" : row.asset_key_label);
        append_label(document, item_id, row.row_id + ".scope", row.scope_label);
        append_label(document, item_id, row.row_id + ".source_path", row.source_path.empty() ? "-" : row.source_path);
        append_label(document, item_id, row.row_id + ".imported_path",
                     row.imported_path.empty() ? "-" : row.imported_path);
        append_label(document, item_id, row.row_id + ".state", row.state_label);
        append_label(document, item_id, row.row_id + ".import_status", row.import_status_label);
        append_label(document, item_id, row.row_id + ".package_status", row.package_status_label);
    }

    return document;
}

} // namespace mirakana::editor
