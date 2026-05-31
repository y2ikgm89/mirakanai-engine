// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "first_party_editor_document.hpp"

#include "native_editor_app.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

struct EditorPanelDocumentToken {
    std::string_view id;
    std::string_view label;
};

constexpr std::array<EditorPanelDocumentToken, 11> editor_panel_tokens{{
    EditorPanelDocumentToken{.id = "main_menu", .label = "Main Menu"},
    EditorPanelDocumentToken{.id = "scene", .label = "Scene"},
    EditorPanelDocumentToken{.id = "inspector", .label = "Inspector"},
    EditorPanelDocumentToken{.id = "assets", .label = "Assets"},
    EditorPanelDocumentToken{.id = "console", .label = "Console"},
    EditorPanelDocumentToken{.id = "viewport", .label = "Viewport"},
    EditorPanelDocumentToken{.id = "resources", .label = "Resources"},
    EditorPanelDocumentToken{.id = "ai_commands", .label = "AI Commands"},
    EditorPanelDocumentToken{.id = "profiler", .label = "Profiler"},
    EditorPanelDocumentToken{.id = "timeline", .label = "Timeline"},
    EditorPanelDocumentToken{.id = "project_settings", .label = "Project Settings"},
}};

[[nodiscard]] ui::ElementId element_id(std::string value) {
    return ui::ElementId{.value = std::move(value)};
}

[[nodiscard]] ui::TextContent text(std::string label) {
    return ui::TextContent{.label = std::move(label), .font_family = "editor-ui"};
}

[[nodiscard]] ui::ElementDesc element(std::string id, ui::SemanticRole role) {
    ui::ElementDesc desc;
    desc.id = element_id(std::move(id));
    desc.role = role;
    desc.accessibility_label = desc.id.value;
    desc.style.layout = ui::LayoutMode::column;
    desc.style.anchor = ui::AnchorMode::fill;
    desc.style.gap = 4.0F;
    desc.style.padding = ui::EdgeInsets{.top = 4.0F, .right = 4.0F, .bottom = 4.0F, .left = 4.0F};
    desc.style.background_token = "editor.panel.background";
    desc.style.foreground_token = "editor.text";
    return desc;
}

[[nodiscard]] ui::ElementDesc child(std::string id, const ui::ElementId& parent, ui::SemanticRole role) {
    ui::ElementDesc desc = element(std::move(id), role);
    desc.parent = parent;
    return desc;
}

void add_or_throw(ui::UiDocument& document, ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("first-party editor document element is invalid or duplicated");
    }
}

void append_label(ui::UiDocument& document, const ui::ElementId& parent, std::string id, std::string label) {
    ui::ElementDesc desc = child(std::move(id), parent, ui::SemanticRole::label);
    desc.text = text(std::move(label));
    desc.accessibility_label = desc.text.label;
    add_or_throw(document, std::move(desc));
}

void append_panel_status(ui::UiDocument& document, const NativeEditorApp& app, std::string_view panel_id,
                         const ui::ElementId& panel_root) {
    const std::string prefix = "editor.panel." + std::string{panel_id};
    if (panel_id == "viewport") {
        append_label(document, panel_root, prefix + ".status", "viewport " + app.viewport_display().status_id);
    } else if (panel_id == "resources") {
        append_label(document, panel_root, prefix + ".status", "resources " + app.resources().status);
    } else if (panel_id == "ai_commands") {
        append_label(document, panel_root, prefix + ".status",
                     "commands " + std::to_string(app.ai_commands().command_rows.size()));
    } else if (panel_id == "project_settings") {
        append_label(document, panel_root, prefix + ".status",
                     "project settings diagnostics " + std::to_string(app.project_settings_errors().size()));
    }
}

} // namespace

FirstPartyEditorDocument make_first_party_editor_document(const NativeEditorApp& app) {
    FirstPartyEditorDocument result;
    ui::UiDocument& document = result.document;

    const ui::Rect root_bounds{.x = 0.0F,
                               .y = 0.0F,
                               .width = static_cast<float>(app.options().width),
                               .height = static_cast<float>(app.options().height)};
    ui::ElementDesc root = element("editor.root", ui::SemanticRole::root);
    root.accessibility_label = "MIRAIKANAI Editor";
    root.bounds = root_bounds;
    add_or_throw(document, std::move(root));

    const ui::ElementId root_id = element_id("editor.root");
    ui::ElementDesc dock = child("editor.dock", root_id, ui::SemanticRole::panel);
    dock.accessibility_label = "Editor Dock";
    add_or_throw(document, std::move(dock));

    const ui::ElementId dock_id = element_id("editor.dock");
    for (const auto& panel : editor_panel_tokens) {
        if (!app.has_native_panel(panel.id)) {
            continue;
        }

        const std::string panel_root_id = "editor.panel." + std::string{panel.id};
        ui::ElementDesc panel_root = child(panel_root_id, dock_id, ui::SemanticRole::panel);
        panel_root.accessibility_label = std::string{panel.label};
        add_or_throw(document, std::move(panel_root));
        ++result.panel_root_count;

        const ui::ElementId panel_id = element_id(panel_root_id);
        append_label(document, panel_id, panel_root_id + ".title", std::string{panel.label});
        append_panel_status(document, app, panel.id, panel_id);
    }

    result.layout = ui::solve_layout(document, root_id, root_bounds);
    result.renderer_submission = ui::build_renderer_submission(document, result.layout);
    return result;
}

FirstPartyEditorShellSmokeCounters
make_first_party_editor_shell_smoke_counters(const NativeEditorApp& app, const FirstPartyEditorDocument& document) {
    return FirstPartyEditorShellSmokeCounters{
        .ui = "first_party",
        .backend = "d3d12",
        .panel_count = document.panel_root_count,
        .imgui_enabled = false,
        .sdl3_enabled = false,
        .viewport_native_handles_exposed = app.viewport_display().native_texture_handles_exposed,
        .material_preview_native_handles_exposed = app.material_preview_display().native_texture_handles_exposed,
    };
}

} // namespace mirakana::editor
