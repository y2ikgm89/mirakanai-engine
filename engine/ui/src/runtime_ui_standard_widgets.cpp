// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/ui/runtime_ui_standard_widgets.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::ui {

namespace {

void append_provenance_diagnostic(std::vector<RuntimeUiProvenanceDiagnostic>& diagnostics,
                                  RuntimeUiProvenanceDiagnosticCode code, std::string subject) {
    diagnostics.push_back(RuntimeUiProvenanceDiagnostic{.code = code, .subject = std::move(subject)});
}

void append_text_diagnostic(std::vector<std::string>& diagnostics, std::string value) {
    if (std::ranges::find(diagnostics, value) == diagnostics.end()) {
        diagnostics.push_back(std::move(value));
    }
}

[[nodiscard]] bool contains_id(const std::vector<std::string>& ids, std::string_view id) noexcept {
    return std::ranges::find(ids, id) != ids.end();
}

[[nodiscard]] bool is_ascii_identifier_char(char value) noexcept {
    return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') ||
           value == '_';
}

[[nodiscard]] bool contains_bounded_token(std::string_view value, std::string_view token) noexcept {
    for (std::size_t position = value.find(token); position != std::string_view::npos;
         position = value.find(token, position + token.size())) {
        const auto end = position + token.size();
        const auto left_boundary = position == 0U || !is_ascii_identifier_char(value[position - 1U]);
        const auto right_boundary = end == value.size() || !is_ascii_identifier_char(value[end]);
        if (left_boundary && right_boundary) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] bool contains_external_engine_public_token(std::string_view value) noexcept {
    constexpr std::array<std::string_view, 8> tokens{
        "uGUI", "UMG", "Slate", "Widget Blueprint", "Control", "CanvasLayer", "UIDocument", "VisualElement",
    };
    return std::ranges::any_of(tokens,
                               [value](std::string_view token) { return contains_bounded_token(value, token); });
}

[[nodiscard]] bool is_valid_float(float value) noexcept {
    return std::isfinite(value);
}

[[nodiscard]] bool has_screen(const std::vector<RuntimeUiMenuScreenDesc>& screens, std::string_view id) noexcept {
    return std::ranges::any_of(screens, [id](const RuntimeUiMenuScreenDesc& screen) { return screen.id == id; });
}

[[nodiscard]] Rect row_bounds(Rect viewport, std::size_t index) noexcept {
    const auto y = viewport.y + 12.0F + (static_cast<float>(index) * 36.0F);
    return Rect{.x = viewport.x + 12.0F, .y = y, .width = 240.0F, .height = 28.0F};
}

[[nodiscard]] Rect fill_bounds(Rect track, RuntimeUiMeterFillDirection direction, float normalized) noexcept {
    const auto fill_width = track.width * normalized;
    const auto fill_height = track.height * normalized;
    switch (direction) {
    case RuntimeUiMeterFillDirection::left_to_right:
        return Rect{.x = track.x, .y = track.y, .width = fill_width, .height = track.height};
    case RuntimeUiMeterFillDirection::right_to_left:
        return Rect{.x = track.x + track.width - fill_width, .y = track.y, .width = fill_width, .height = track.height};
    case RuntimeUiMeterFillDirection::bottom_to_top:
        return Rect{
            .x = track.x, .y = track.y + track.height - fill_height, .width = track.width, .height = fill_height};
    case RuntimeUiMeterFillDirection::top_to_bottom:
        return Rect{.x = track.x, .y = track.y, .width = track.width, .height = fill_height};
    }
    return track;
}

[[nodiscard]] std::string fallback_string(const std::string& value, std::string_view fallback) {
    return value.empty() ? std::string{fallback} : value;
}

void append_meter_elements(UiDocument& document, const RuntimeUiMeterRow& row, std::size_t index, Rect viewport,
                           std::string_view parent_id) {
    const auto base = std::string{parent_id} + ".meter." + row.id;
    const auto panel_bounds = row_bounds(viewport, index);
    const auto track_bounds =
        Rect{.x = panel_bounds.x + 52.0F, .y = panel_bounds.y + 6.0F, .width = 176.0F, .height = 12.0F};
    const auto fill = fill_bounds(track_bounds, row.fill_direction, row.normalized_value);
    const auto label_bounds = Rect{.x = panel_bounds.x, .y = panel_bounds.y, .width = 48.0F, .height = 24.0F};
    const auto fill_token = row.warning && !row.warning_token.empty() ? row.warning_token : row.fill_token;

    const auto panel_id = ElementId{base};
    const auto track_id = ElementId{base + ".track"};
    const auto fill_id = ElementId{base + ".fill"};
    const auto label_id = ElementId{base + ".label"};
    const auto parent = ElementId{std::string{parent_id}};

    (void)document.try_add_element(ElementDesc{
        .id = panel_id,
        .parent = parent,
        .role = SemanticRole::meter,
        .bounds = panel_bounds,
        .visible = row.visible,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = fallback_string(row.accessibility_label, row.id),
        .style = {},
    });
    (void)document.try_add_element(ElementDesc{
        .id = track_id,
        .parent = panel_id,
        .role = SemanticRole::panel,
        .bounds = track_bounds,
        .visible = row.visible,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = Style{.background_token = fallback_string(row.track_token, "runtime_ui.meter.track")},
    });
    (void)document.try_add_element(ElementDesc{
        .id = fill_id,
        .parent = track_id,
        .role = SemanticRole::panel,
        .bounds = fill,
        .visible = row.visible,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = Style{.background_token = fallback_string(fill_token, "runtime_ui.meter.fill")},
    });
    (void)document.try_add_element(ElementDesc{
        .id = label_id,
        .parent = panel_id,
        .role = SemanticRole::label,
        .bounds = label_bounds,
        .visible = row.visible,
        .enabled = false,
        .text =
            TextContent{
                .label = fallback_string(row.label, row.id),
                .localization_key = row.localization_key,
            },
        .image = {},
        .accessibility_label = {},
        .style = {},
    });
}

} // namespace

RuntimeUiStandardWidgetProvenancePlan
review_runtime_ui_standard_widget_provenance(const RuntimeUiStandardWidgetProvenanceDesc& desc) {
    RuntimeUiStandardWidgetProvenancePlan plan;

    if (desc.uses_external_engine_code) {
        append_provenance_diagnostic(
            plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::third_party_code_without_notice, desc.feature_id);
    }
    if (desc.uses_external_engine_assets) {
        append_provenance_diagnostic(
            plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::third_party_asset_without_notice, desc.feature_id);
    }
    if (desc.uses_external_engine_public_names) {
        append_provenance_diagnostic(plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::external_engine_public_name,
                                     desc.feature_id);
    }
    if (desc.uses_ui_middleware) {
        append_provenance_diagnostic(
            plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::unsupported_ui_middleware_reference, desc.feature_id);
    }

    for (const auto& reference : desc.source_references) {
        if (reference.contains_copied_expression) {
            append_provenance_diagnostic(plan.diagnostics,
                                         RuntimeUiProvenanceDiagnosticCode::copied_external_expression, reference.name);
        }
        if (contains_external_engine_public_token(reference.name)) {
            append_provenance_diagnostic(
                plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::external_engine_public_name, reference.name);
        }

        switch (reference.kind) {
        case RuntimeUiSourceReferenceKind::official_documentation:
            ++plan.official_documentation_rows;
            break;
        case RuntimeUiSourceReferenceKind::first_party_design:
            ++plan.first_party_design_rows;
            break;
        case RuntimeUiSourceReferenceKind::third_party_code:
            ++plan.third_party_rows;
            append_provenance_diagnostic(
                plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::third_party_code_without_notice, reference.name);
            break;
        case RuntimeUiSourceReferenceKind::third_party_asset:
        case RuntimeUiSourceReferenceKind::ai_generated_asset:
            ++plan.third_party_rows;
            append_provenance_diagnostic(
                plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::third_party_asset_without_notice, reference.name);
            break;
        case RuntimeUiSourceReferenceKind::marketplace_asset:
            ++plan.third_party_rows;
            append_provenance_diagnostic(
                plan.diagnostics, RuntimeUiProvenanceDiagnosticCode::marketplace_asset_without_review, reference.name);
            break;
        }
    }

    plan.ready = plan.diagnostics.empty();
    return plan;
}

RuntimeUiMeterPlan plan_runtime_ui_meters(std::vector<RuntimeUiMeterDesc> meters) {
    RuntimeUiMeterPlan plan;
    std::vector<std::string> ids;
    ids.reserve(meters.size());

    for (const auto& meter : meters) {
        if (meter.id.empty()) {
            append_text_diagnostic(plan.diagnostics, "missing_meter_id");
            continue;
        }
        if (contains_id(ids, meter.id)) {
            append_text_diagnostic(plan.diagnostics, "duplicate_meter_id");
            continue;
        }
        ids.push_back(meter.id);

        if (!is_valid_float(meter.maximum) || meter.maximum <= 0.0F) {
            append_text_diagnostic(plan.diagnostics, "invalid_meter_maximum");
            continue;
        }
        if (!is_valid_float(meter.value) || !is_valid_float(meter.warning_threshold)) {
            append_text_diagnostic(plan.diagnostics, "invalid_meter_value");
            continue;
        }

        const auto normalized = std::clamp(meter.value / meter.maximum, 0.0F, 1.0F);
        plan.rows.push_back(RuntimeUiMeterRow{
            .id = meter.id,
            .kind = meter.kind,
            .label = meter.label,
            .localization_key = meter.localization_key,
            .accessibility_label = meter.accessibility_label,
            .value = meter.value,
            .maximum = meter.maximum,
            .normalized_value = normalized,
            .fill_direction = meter.fill_direction,
            .track_token = meter.track_token,
            .fill_token = meter.fill_token,
            .warning_token = meter.warning_token,
            .warning = normalized <= meter.warning_threshold,
            .depleted = normalized <= 0.0F,
            .visible = meter.visible,
        });
    }

    plan.ready = plan.diagnostics.empty();
    return plan;
}

UiDocument build_runtime_ui_meter_document(const RuntimeUiMeterPlan& plan, Rect viewport) {
    UiDocument document;
    if (!plan.ready || !is_valid_rect(viewport)) {
        return document;
    }

    constexpr std::string_view root_id{"runtime_ui.meters.root"};
    (void)document.try_add_element(ElementDesc{
        .id = ElementId{std::string{root_id}},
        .parent = {},
        .role = SemanticRole::root,
        .bounds = viewport,
        .visible = true,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = Style{.layout = LayoutMode::column, .anchor = AnchorMode::top_left},
    });

    for (std::size_t index = 0; index < plan.rows.size(); ++index) {
        append_meter_elements(document, plan.rows[index], index, viewport, root_id);
    }
    return document;
}

RuntimeUiMenuStackPlan plan_runtime_ui_menu_stack(const RuntimeUiMenuStackDesc& desc) {
    RuntimeUiMenuStackPlan plan;
    plan.active_screen_id = desc.active_screen_id;
    plan.screen_count = desc.screens.size();

    std::vector<std::string> screen_ids;
    screen_ids.reserve(desc.screens.size());
    for (const auto& screen : desc.screens) {
        if (screen.id.empty()) {
            append_text_diagnostic(plan.diagnostics, "missing_screen_id");
        } else if (contains_id(screen_ids, screen.id)) {
            append_text_diagnostic(plan.diagnostics, "duplicate_screen_id");
        } else {
            screen_ids.push_back(screen.id);
        }
        if (contains_external_engine_public_token(screen.id)) {
            append_text_diagnostic(plan.diagnostics, "external_engine_public_name");
        }

        std::vector<std::string> action_ids;
        action_ids.reserve(screen.actions.size());
        for (const auto& action : screen.actions) {
            ++plan.action_count;
            if (action.id.empty()) {
                append_text_diagnostic(plan.diagnostics, "missing_action_id");
            } else if (contains_id(action_ids, action.id)) {
                append_text_diagnostic(plan.diagnostics, "duplicate_action_id");
            } else {
                action_ids.push_back(action.id);
            }
            if (contains_external_engine_public_token(action.id)) {
                append_text_diagnostic(plan.diagnostics, "external_engine_public_name");
            }
        }
    }

    if (!has_screen(desc.screens, desc.active_screen_id)) {
        append_text_diagnostic(plan.diagnostics, "unknown_active_screen");
    }

    for (const auto& screen : desc.screens) {
        for (const auto& action : screen.actions) {
            if (!action.target_screen_id.empty() && !has_screen(desc.screens, action.target_screen_id)) {
                append_text_diagnostic(plan.diagnostics, "unknown_target_screen");
            }
            if (screen.id == desc.active_screen_id && action.enabled) {
                plan.focus_order.push_back(action.id);
            }
        }
    }

    plan.ready = plan.diagnostics.empty();
    return plan;
}

UiDocument build_runtime_ui_menu_stack_document(const RuntimeUiMenuStackDesc& desc, const RuntimeUiMenuStackPlan& plan,
                                                Rect viewport) {
    UiDocument document;
    if (!plan.ready || !is_valid_rect(viewport)) {
        return document;
    }

    constexpr std::string_view root_id{"runtime_ui.menu_stack.root"};
    (void)document.try_add_element(ElementDesc{
        .id = ElementId{std::string{root_id}},
        .parent = {},
        .role = SemanticRole::root,
        .bounds = viewport,
        .visible = true,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = Style{.layout = LayoutMode::column, .anchor = AnchorMode::center},
    });

    const auto screen_it = std::ranges::find_if(
        desc.screens, [&plan](const RuntimeUiMenuScreenDesc& screen) { return screen.id == plan.active_screen_id; });
    if (screen_it == desc.screens.end()) {
        return document;
    }

    const auto screen_id = ElementId{"runtime_ui.menu_stack." + screen_it->id};
    (void)document.try_add_element(ElementDesc{
        .id = screen_id,
        .parent = ElementId{std::string{root_id}},
        .role = SemanticRole::dialog,
        .bounds = Rect{.x = viewport.x + 32.0F, .y = viewport.y + 32.0F, .width = 320.0F, .height = 240.0F},
        .visible = true,
        .enabled = true,
        .text = TextContent{.localization_key = screen_it->title_localization_key},
        .image = {},
        .accessibility_label = screen_it->accessibility_label,
        .style = {},
    });

    for (std::size_t index = 0; index < screen_it->actions.size(); ++index) {
        const auto& action = screen_it->actions[index];
        (void)document.try_add_element(ElementDesc{
            .id = ElementId{"runtime_ui.menu_stack." + screen_it->id + ".action." + action.id},
            .parent = screen_id,
            .role = SemanticRole::button,
            .bounds = Rect{.x = viewport.x + 48.0F,
                           .y = viewport.y + 72.0F + (static_cast<float>(index) * 36.0F),
                           .width = 220.0F,
                           .height = 28.0F},
            .visible = true,
            .enabled = action.enabled,
            .text = TextContent{.label = action.label, .localization_key = action.localization_key},
            .image = {},
            .accessibility_label = action.label,
            .style = {},
        });
    }

    return document;
}

RuntimeUiStandardHudPlan plan_runtime_ui_standard_hud(const RuntimeUiStandardHudDesc& desc) {
    RuntimeUiStandardHudPlan plan;
    plan.provenance_plan = review_runtime_ui_standard_widget_provenance(desc.provenance);
    plan.meter_plan = plan_runtime_ui_meters(desc.meters);
    plan.menu_plan = plan_runtime_ui_menu_stack(desc.menu_stack);

    if (!plan.provenance_plan.ready) {
        append_text_diagnostic(plan.diagnostics, "invalid_provenance");
    }
    if (!plan.meter_plan.ready) {
        append_text_diagnostic(plan.diagnostics, "invalid_meters");
    }
    if (!plan.menu_plan.ready) {
        append_text_diagnostic(plan.diagnostics, "invalid_menu_stack");
    }

    plan.ready = plan.diagnostics.empty();
    if (!plan.ready) {
        return plan;
    }

    (void)plan.document.try_add_element(ElementDesc{
        .id = ElementId{"runtime_ui.standard_hud." + desc.id + ".root"},
        .parent = {},
        .role = SemanticRole::root,
        .bounds = desc.viewport,
        .visible = true,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = Style{.layout = LayoutMode::stack, .anchor = AnchorMode::fill},
    });
    const auto meter_root = ElementId{"runtime_ui.standard_hud." + desc.id + ".meters"};
    (void)plan.document.try_add_element(ElementDesc{
        .id = meter_root,
        .parent = ElementId{"runtime_ui.standard_hud." + desc.id + ".root"},
        .role = SemanticRole::panel,
        .bounds = desc.viewport,
        .visible = true,
        .enabled = false,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = {},
    });
    for (std::size_t index = 0; index < plan.meter_plan.rows.size(); ++index) {
        append_meter_elements(plan.document, plan.meter_plan.rows[index], index, desc.viewport, meter_root.value);
    }

    const auto menu_root = ElementId{"runtime_ui.standard_hud." + desc.id + ".menu"};
    (void)plan.document.try_add_element(ElementDesc{
        .id = menu_root,
        .parent = ElementId{"runtime_ui.standard_hud." + desc.id + ".root"},
        .role = SemanticRole::panel,
        .bounds = desc.viewport,
        .visible = true,
        .enabled = true,
        .text = {},
        .image = {},
        .accessibility_label = {},
        .style = {},
    });
    const auto menu_doc = build_runtime_ui_menu_stack_document(desc.menu_stack, plan.menu_plan, desc.viewport);
    for (const auto& element : menu_doc.traverse()) {
        if (element.role == SemanticRole::root) {
            continue;
        }
        auto parent = element.parent;
        if (parent.value == "runtime_ui.menu_stack.root") {
            parent = menu_root;
        }
        (void)plan.document.try_add_element(ElementDesc{
            .id = ElementId{"runtime_ui.standard_hud." + desc.id + "." + element.id.value},
            .parent =
                parent == menu_root ? menu_root : ElementId{"runtime_ui.standard_hud." + desc.id + "." + parent.value},
            .role = element.role,
            .bounds = element.bounds,
            .visible = element.visible,
            .enabled = element.enabled,
            .text = element.text,
            .image = element.image,
            .accessibility_label = element.accessibility_label,
            .style = element.style,
        });
    }

    return plan;
}

} // namespace mirakana::ui
