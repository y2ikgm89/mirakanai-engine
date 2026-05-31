// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/ai_operation_surface.hpp"

#include <array>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

struct PanelOperationInfo {
    PanelId panel{PanelId::scene};
    std::string_view token;
    std::string_view label;
};

constexpr std::array<PanelOperationInfo, 11> operation_panels{{
    PanelOperationInfo{.panel = PanelId::scene, .token = "scene", .label = "Scene"},
    PanelOperationInfo{.panel = PanelId::inspector, .token = "inspector", .label = "Inspector"},
    PanelOperationInfo{.panel = PanelId::assets, .token = "assets", .label = "Assets"},
    PanelOperationInfo{.panel = PanelId::console, .token = "console", .label = "Console"},
    PanelOperationInfo{.panel = PanelId::viewport, .token = "viewport", .label = "Viewport"},
    PanelOperationInfo{.panel = PanelId::resources, .token = "resources", .label = "Resources"},
    PanelOperationInfo{.panel = PanelId::ai_commands, .token = "ai_commands", .label = "AI Commands"},
    PanelOperationInfo{.panel = PanelId::input_rebinding, .token = "input_rebinding", .label = "Input Rebinding"},
    PanelOperationInfo{.panel = PanelId::profiler, .token = "profiler", .label = "Profiler"},
    PanelOperationInfo{.panel = PanelId::project_settings, .token = "project_settings", .label = "Project Settings"},
    PanelOperationInfo{.panel = PanelId::timeline, .token = "timeline", .label = "Timeline"},
}};

struct PanelCommandInfo {
    std::string_view id;
    std::string_view label;
    PanelId panel{PanelId::resources};
    bool target_visible{true};
};

constexpr std::array<PanelCommandInfo, 6> panel_visibility_commands{{
    PanelCommandInfo{.id = "editor.panel.resources.show",
                     .label = "Show Resources",
                     .panel = PanelId::resources,
                     .target_visible = true},
    PanelCommandInfo{.id = "editor.panel.resources.hide",
                     .label = "Hide Resources",
                     .panel = PanelId::resources,
                     .target_visible = false},
    PanelCommandInfo{.id = "editor.panel.ai_commands.show",
                     .label = "Show AI Commands",
                     .panel = PanelId::ai_commands,
                     .target_visible = true},
    PanelCommandInfo{.id = "editor.panel.ai_commands.hide",
                     .label = "Hide AI Commands",
                     .panel = PanelId::ai_commands,
                     .target_visible = false},
    PanelCommandInfo{.id = "editor.panel.profiler.show",
                     .label = "Show Profiler",
                     .panel = PanelId::profiler,
                     .target_visible = true},
    PanelCommandInfo{.id = "editor.panel.profiler.hide",
                     .label = "Hide Profiler",
                     .panel = PanelId::profiler,
                     .target_visible = false},
}};

[[nodiscard]] std::string panel_element_id(PanelId panel) {
    return "editor.panel." + std::string{panel_id_to_string(panel)};
}

[[nodiscard]] std::uint64_t workspace_operation_revision(const Workspace& workspace) noexcept {
    std::uint64_t revision = 1U;
    for (const auto& panel : workspace.panels()) {
        const auto shift = static_cast<std::uint64_t>(panel.id);
        if (shift < 63U && panel.visible) {
            revision |= (std::uint64_t{1U} << (shift + 1U));
        }
    }
    return revision;
}

[[nodiscard]] const EditorAiCommandRow* find_command(const EditorAiCommandCatalog& catalog,
                                                     std::string_view command_id) noexcept {
    for (const auto& command : catalog.commands) {
        if (command.id == command_id) {
            return &command;
        }
    }
    return nullptr;
}

[[nodiscard]] const PanelCommandInfo* find_panel_command(std::string_view command_id) noexcept {
    for (const auto& command : panel_visibility_commands) {
        if (command.id == command_id) {
            return &command;
        }
    }
    return nullptr;
}

[[nodiscard]] EditorAiOperationDiagnostic diagnostic(std::string code, std::string message) {
    return EditorAiOperationDiagnostic{.code = std::move(code), .message = std::move(message)};
}

} // namespace

EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace) {
    EditorAiOperationSnapshot snapshot;
    snapshot.revision = workspace_operation_revision(workspace);
    for (const auto& panel : operation_panels) {
        snapshot.elements.push_back(EditorAiOperationElementRow{
            .id = panel_element_id(panel.panel),
            .role = "panel",
            .label = std::string{panel.label},
            .visible = workspace.is_panel_visible(panel.panel),
            .enabled = true,
        });
    }
    return snapshot;
}

EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace) {
    EditorAiCommandCatalog catalog;
    catalog.revision = workspace_operation_revision(workspace);
    for (const auto& command : panel_visibility_commands) {
        const bool already_in_target_state = workspace.is_panel_visible(command.panel) == command.target_visible;
        catalog.commands.push_back(EditorAiCommandRow{
            .id = std::string{command.id},
            .label = std::string{command.label},
            .target_element_id = panel_element_id(command.panel),
            .enabled = !already_in_target_state,
            .mutates_state = true,
            .requires_confirmation = true,
        });
    }
    return catalog;
}

EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace, const EditorAiCommandCatalog& catalog,
                                                      const EditorAiCommandRequest& request) {
    (void)workspace;
    EditorAiCommandDryRunResult result;
    const EditorAiCommandRow* command = find_command(catalog, request.command_id);
    if (command == nullptr) {
        result.diagnostics.push_back(diagnostic("unknown_command", "AI editor command id is not in the catalog"));
        return result;
    }
    if (command->target_element_id != request.target_element_id) {
        result.diagnostics.push_back(
            diagnostic("target_mismatch", "AI editor command target does not match the catalog row"));
        return result;
    }
    if (!request.parameters.empty()) {
        result.diagnostics.push_back(
            diagnostic("unsupported_parameters", "AI editor panel visibility commands do not accept parameters"));
        return result;
    }
    if (!command->enabled) {
        result.diagnostics.push_back(diagnostic("command_disabled", "AI editor command is disabled for this revision"));
        return result;
    }

    result.accepted = true;
    result.would_mutate = command->mutates_state;
    result.requires_confirmation = command->requires_confirmation;
    return result;
}

EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace, const EditorAiCommandCatalog& catalog,
                                                   const EditorAiCommandRequest& request) {
    EditorAiCommandApplyResult result;
    result.before_revision = workspace_operation_revision(workspace);
    result.after_revision = result.before_revision;

    const auto dry_run = dry_run_editor_ai_command(workspace, catalog, request);
    if (!dry_run.accepted) {
        result.diagnostics = dry_run.diagnostics;
        return result;
    }
    if (dry_run.requires_confirmation && !request.user_confirmed) {
        result.diagnostics.push_back(
            diagnostic("confirmation_required", "AI editor command requires explicit user confirmation"));
        return result;
    }

    const PanelCommandInfo* command = find_panel_command(request.command_id);
    if (command == nullptr) {
        result.diagnostics.push_back(diagnostic("unknown_command", "AI editor command id is not implemented"));
        return result;
    }

    workspace.set_panel_visible(command->panel, command->target_visible);
    result.after_revision = workspace_operation_revision(workspace);
    result.applied = result.before_revision != result.after_revision;
    return result;
}

} // namespace mirakana::editor
