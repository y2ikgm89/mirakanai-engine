// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/ai_operation_surface.hpp"

#include <algorithm>
#include <array>
#include <optional>
#include <stdexcept>
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

struct ParsedDockCommand {
    bool recognized{false};
    EditorDockCommandKind kind{EditorDockCommandKind::show_panel};
    std::string panel_id;
};

struct ParsedDockParameters {
    bool accepted{false};
    std::string target_stack_id;
    std::string source_stack_id;
    std::string new_stack_id;
    EditorDockSplitAxis split_axis{EditorDockSplitAxis::horizontal};
    float split_ratio{0.5F};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

constexpr std::string_view dock_command_prefix = "editor.dock.";
constexpr std::string_view dock_panel_command_prefix = "editor.dock.panel.";
constexpr std::string_view dock_layout_reset_command = "editor.dock.layout.reset";
constexpr std::string_view target_stack_parameter = "target_stack_id";
constexpr std::string_view source_stack_parameter = "source_stack_id";
constexpr std::string_view new_stack_parameter = "new_stack_id";
constexpr std::string_view split_axis_parameter = "split_axis";
constexpr std::string_view split_ratio_parameter = "split_ratio";

[[nodiscard]] std::string panel_element_id(PanelId panel) {
    return "editor.panel." + std::string{panel_id_to_string(panel)};
}

[[nodiscard]] std::string dock_layout_element_id() {
    return "editor.dock.layout";
}

[[nodiscard]] std::string dock_stack_element_id(std::string_view stack_id) {
    return "editor.dock.stack." + std::string{stack_id};
}

[[nodiscard]] std::string dock_panel_element_id(std::string_view panel_id) {
    return "editor.dock.panel." + std::string{panel_id};
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

[[nodiscard]] std::uint64_t combined_operation_revision(const Workspace& workspace,
                                                        const EditorDockLayout& dock_layout) noexcept {
    return (workspace_operation_revision(workspace) * 131U) + dock_layout.layout_revision;
}

[[nodiscard]] bool is_dock_ai_command(std::string_view command_id) noexcept {
    return command_id.starts_with(dock_command_prefix);
}

[[nodiscard]] bool contains_tab(const EditorDockNode& node, std::string_view panel_id) noexcept {
    return std::ranges::any_of(node.tabs, [panel_id](const std::string& tab) { return tab == panel_id; });
}

[[nodiscard]] const EditorDockNode* find_dock_stack_containing_panel(const EditorDockLayout& layout,
                                                                     std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_tab(node, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] bool dock_panel_is_visible(const EditorDockLayout& layout, std::string_view panel_id) noexcept {
    return find_dock_stack_containing_panel(layout, panel_id) != nullptr;
}

[[nodiscard]] std::optional<std::string_view> find_parameter(const EditorAiCommandRequest& request,
                                                             std::string_view key) noexcept {
    const auto it = std::ranges::find_if(
        request.parameters, [key](const EditorAiCommandParameter& parameter) { return parameter.key == key; });
    if (it == request.parameters.end()) {
        return std::nullopt;
    }
    return std::string_view{it->value};
}

[[nodiscard]] bool has_unsupported_parameter(const EditorAiCommandRequest& request,
                                             std::string_view allowed_key) noexcept {
    return std::ranges::any_of(request.parameters, [allowed_key](const EditorAiCommandParameter& parameter) {
        return parameter.key != allowed_key;
    });
}

[[nodiscard]] bool has_unsupported_split_parameter(const EditorAiCommandRequest& request) noexcept {
    return std::ranges::any_of(request.parameters, [](const EditorAiCommandParameter& parameter) {
        return parameter.key != source_stack_parameter && parameter.key != new_stack_parameter &&
               parameter.key != split_axis_parameter && parameter.key != split_ratio_parameter;
    });
}

[[nodiscard]] EditorAiOperationDiagnostic diagnostic(std::string code, std::string message);

[[nodiscard]] EditorDockSplitAxis parse_dock_split_axis(std::string_view value) {
    if (value == "horizontal") {
        return EditorDockSplitAxis::horizontal;
    }
    if (value == "vertical") {
        return EditorDockSplitAxis::vertical;
    }
    throw std::invalid_argument("AI editor dock split axis must be horizontal or vertical");
}

[[nodiscard]] float parse_dock_split_ratio(std::string_view value) {
    std::size_t consumed = 0;
    const auto parsed = std::stof(std::string{value}, &consumed);
    if (consumed != value.size()) {
        throw std::invalid_argument("AI editor dock split ratio must be a float");
    }
    return parsed;
}

[[nodiscard]] ParsedDockCommand parse_dock_command(std::string_view command_id) {
    if (command_id == dock_layout_reset_command) {
        return ParsedDockCommand{.recognized = true, .kind = EditorDockCommandKind::reset_layout, .panel_id = {}};
    }

    if (!command_id.starts_with(dock_panel_command_prefix)) {
        return {};
    }
    const auto suffix = command_id.substr(dock_panel_command_prefix.size());
    const auto separator = suffix.rfind('.');
    if (separator == std::string_view::npos) {
        return {};
    }

    const auto panel_id = suffix.substr(0U, separator);
    const auto action = suffix.substr(separator + 1U);
    if (action == "show") {
        return ParsedDockCommand{
            .recognized = true, .kind = EditorDockCommandKind::show_panel, .panel_id = std::string{panel_id}};
    }
    if (action == "hide") {
        return ParsedDockCommand{
            .recognized = true, .kind = EditorDockCommandKind::hide_panel, .panel_id = std::string{panel_id}};
    }
    if (action == "activate") {
        return ParsedDockCommand{
            .recognized = true, .kind = EditorDockCommandKind::activate_tab, .panel_id = std::string{panel_id}};
    }
    if (action == "move") {
        return ParsedDockCommand{
            .recognized = true, .kind = EditorDockCommandKind::move_panel_to_stack, .panel_id = std::string{panel_id}};
    }
    if (action == "split") {
        return ParsedDockCommand{
            .recognized = true, .kind = EditorDockCommandKind::split_panel_to_stack, .panel_id = std::string{panel_id}};
    }

    return {};
}

[[nodiscard]] ParsedDockParameters parse_dock_parameters(const ParsedDockCommand& parsed,
                                                         const EditorAiCommandRequest& request) {
    ParsedDockParameters result;

    if (parsed.kind == EditorDockCommandKind::move_panel_to_stack) {
        if (has_unsupported_parameter(request, target_stack_parameter)) {
            result.diagnostics.push_back(
                diagnostic("unsupported_parameters", "AI editor dock move command accepts target_stack_id only"));
            return result;
        }
        const auto target_stack = find_parameter(request, target_stack_parameter);
        if (!target_stack.has_value() || target_stack->empty()) {
            result.diagnostics.push_back(
                diagnostic("missing_parameter", "AI editor dock move command requires target_stack_id"));
            return result;
        }
        result.target_stack_id = std::string{*target_stack};
    } else if (parsed.kind == EditorDockCommandKind::show_panel) {
        if (has_unsupported_parameter(request, target_stack_parameter)) {
            result.diagnostics.push_back(
                diagnostic("unsupported_parameters", "AI editor dock show command accepts target_stack_id only"));
            return result;
        }
        if (const auto target_stack = find_parameter(request, target_stack_parameter); target_stack.has_value()) {
            result.target_stack_id = std::string{*target_stack};
        }
    } else if (parsed.kind == EditorDockCommandKind::split_panel_to_stack) {
        if (has_unsupported_split_parameter(request)) {
            result.diagnostics.push_back(diagnostic(
                "unsupported_parameters",
                "AI editor dock split command accepts source_stack_id, new_stack_id, split_axis, and split_ratio"));
            return result;
        }
        const auto source_stack = find_parameter(request, source_stack_parameter);
        const auto new_stack = find_parameter(request, new_stack_parameter);
        if (!source_stack.has_value() || source_stack->empty() || !new_stack.has_value() || new_stack->empty()) {
            result.diagnostics.push_back(diagnostic(
                "missing_parameter", "AI editor dock split command requires source_stack_id and new_stack_id"));
            return result;
        }
        result.source_stack_id = std::string{*source_stack};
        result.new_stack_id = std::string{*new_stack};
        try {
            if (const auto split_axis = find_parameter(request, split_axis_parameter); split_axis.has_value()) {
                result.split_axis = parse_dock_split_axis(*split_axis);
            }
            if (const auto split_ratio = find_parameter(request, split_ratio_parameter); split_ratio.has_value()) {
                result.split_ratio = parse_dock_split_ratio(*split_ratio);
            }
        } catch (const std::invalid_argument& error) {
            result.diagnostics.push_back(diagnostic("invalid_parameter", error.what()));
            return result;
        }
    } else if (!request.parameters.empty()) {
        result.diagnostics.push_back(
            diagnostic("unsupported_parameters", "AI editor dock command does not accept parameters"));
        return result;
    }

    result.accepted = true;
    return result;
}

[[nodiscard]] EditorDockCommandRequest make_dock_request(const ParsedDockCommand& parsed,
                                                         const ParsedDockParameters& parameters, bool user_confirmed) {
    return EditorDockCommandRequest{.kind = parsed.kind,
                                    .panel_id = parsed.panel_id,
                                    .target_stack_id = parameters.target_stack_id,
                                    .source_stack_id = parameters.source_stack_id,
                                    .new_stack_id = parameters.new_stack_id,
                                    .split_axis = parameters.split_axis,
                                    .split_ratio = parameters.split_ratio,
                                    .user_confirmed = user_confirmed};
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

void append_dock_layout_diagnostics(EditorAiOperationSnapshot& snapshot, const EditorDockLayoutValidation& validation) {
    for (const auto& item : validation.diagnostics) {
        snapshot.diagnostics.push_back(diagnostic(item.code, item.message));
    }
}

void append_dock_command_diagnostics(EditorAiCommandDryRunResult& result, const EditorDockCommandPlan& plan) {
    for (const auto& item : plan.diagnostics) {
        result.diagnostics.push_back(diagnostic(item.code, item.message));
    }
}

void append_dock_command_diagnostics(EditorAiCommandApplyResult& result, const EditorDockCommandPlan& plan) {
    for (const auto& item : plan.diagnostics) {
        result.diagnostics.push_back(diagnostic(item.code, item.message));
    }
}

[[nodiscard]] std::string dock_command_label(std::string_view action, std::string_view panel_label) {
    return std::string{action} + " " + std::string{panel_label};
}

[[nodiscard]] bool dock_move_command_can_mutate(const EditorDockLayout& dock_layout,
                                                const EditorDockPanelCatalogRow& panel) noexcept {
    if (panel.shell_chrome || !panel.native_shell_panel) {
        return false;
    }
    const auto* source_stack = find_dock_stack_containing_panel(dock_layout, panel.id);
    return source_stack != nullptr && source_stack->tabs.size() > 1U;
}

[[nodiscard]] bool dock_split_command_can_mutate(const EditorDockLayout& dock_layout,
                                                 const EditorDockPanelCatalogRow& panel) noexcept {
    if (panel.shell_chrome || !panel.native_shell_panel) {
        return false;
    }
    const auto* source_stack = find_dock_stack_containing_panel(dock_layout, panel.id);
    return source_stack != nullptr && source_stack->tabs.size() > 1U;
}

[[nodiscard]] bool dock_planned_command_can_mutate(const EditorDockLayout& dock_layout,
                                                   const EditorDockCommandRequest& request) {
    const auto plan = plan_editor_dock_command(dock_layout, request);
    return plan.accepted && plan.would_mutate;
}

[[nodiscard]] bool dock_show_command_can_mutate(const EditorDockLayout& dock_layout,
                                                const EditorDockPanelCatalogRow& panel) {
    if (!panel.native_shell_panel) {
        return false;
    }
    if (!dock_panel_is_visible(dock_layout, panel.id)) {
        return !panel.shell_chrome;
    }
    return dock_planned_command_can_mutate(dock_layout,
                                           EditorDockCommandRequest{.kind = EditorDockCommandKind::show_panel,
                                                                    .panel_id = panel.id,
                                                                    .target_stack_id = {},
                                                                    .user_confirmed = false});
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

EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace,
                                                            const EditorDockLayout& dock_layout) {
    auto snapshot = make_editor_ai_operation_snapshot(workspace);
    snapshot.revision = combined_operation_revision(workspace, dock_layout);

    const auto validation = validate_editor_dock_layout(dock_layout);
    append_dock_layout_diagnostics(snapshot, validation);

    snapshot.elements.push_back(EditorAiOperationElementRow{
        .id = dock_layout_element_id(),
        .role = "dock_layout",
        .label = "Dock Layout",
        .visible = true,
        .enabled = validation.valid,
    });

    for (const auto& node : dock_layout.nodes) {
        if (node.kind == EditorDockNodeKind::tab_stack) {
            snapshot.elements.push_back(EditorAiOperationElementRow{
                .id = dock_stack_element_id(node.id),
                .role = "dock_stack",
                .label = node.id,
                .visible = true,
                .enabled = validation.valid,
            });
        }
    }

    for (const auto& panel : editor_dock_panel_catalog()) {
        if (!panel.native_shell_panel) {
            continue;
        }
        snapshot.elements.push_back(EditorAiOperationElementRow{
            .id = dock_panel_element_id(panel.id),
            .role = "dock_panel",
            .label = panel.label,
            .visible = dock_panel_is_visible(dock_layout, panel.id),
            .enabled = validation.valid,
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

EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace, const EditorDockLayout& dock_layout) {
    auto catalog = make_editor_ai_command_catalog(workspace);
    catalog.revision = combined_operation_revision(workspace, dock_layout);

    for (const auto& panel : editor_dock_panel_catalog()) {
        if (!panel.native_shell_panel) {
            continue;
        }

        catalog.commands.push_back(EditorAiCommandRow{
            .id = dock_panel_element_id(panel.id) + ".show",
            .label = dock_command_label("Show", panel.label),
            .target_element_id = dock_panel_element_id(panel.id),
            .enabled = dock_show_command_can_mutate(dock_layout, panel),
            .mutates_state = true,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = dock_panel_element_id(panel.id) + ".hide",
            .label = dock_command_label("Hide", panel.label),
            .target_element_id = dock_panel_element_id(panel.id),
            .enabled = dock_planned_command_can_mutate(
                dock_layout, EditorDockCommandRequest{.kind = EditorDockCommandKind::hide_panel,
                                                      .panel_id = panel.id,
                                                      .target_stack_id = {},
                                                      .user_confirmed = false}),
            .mutates_state = true,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = dock_panel_element_id(panel.id) + ".activate",
            .label = dock_command_label("Activate", panel.label),
            .target_element_id = dock_panel_element_id(panel.id),
            .enabled = dock_planned_command_can_mutate(
                dock_layout, EditorDockCommandRequest{.kind = EditorDockCommandKind::activate_tab,
                                                      .panel_id = panel.id,
                                                      .target_stack_id = {},
                                                      .user_confirmed = false}),
            .mutates_state = true,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = dock_panel_element_id(panel.id) + ".move",
            .label = dock_command_label("Move", panel.label),
            .target_element_id = dock_panel_element_id(panel.id),
            .enabled = dock_move_command_can_mutate(dock_layout, panel),
            .mutates_state = true,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = dock_panel_element_id(panel.id) + ".split",
            .label = dock_command_label("Split", panel.label),
            .target_element_id = dock_panel_element_id(panel.id),
            .enabled = dock_split_command_can_mutate(dock_layout, panel),
            .mutates_state = true,
            .requires_confirmation = false,
        });
    }

    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_layout_reset_command},
        .label = "Reset Dock Layout",
        .target_element_id = dock_layout_element_id(),
        .enabled = true,
        .mutates_state = true,
        .requires_confirmation = true,
    });

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

EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                                      const EditorAiCommandCatalog& catalog,
                                                      const EditorAiCommandRequest& request) {
    if (!is_dock_ai_command(request.command_id)) {
        return dry_run_editor_ai_command(workspace, catalog, request);
    }

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

    const auto parsed = parse_dock_command(request.command_id);
    if (!parsed.recognized) {
        result.diagnostics.push_back(diagnostic("unknown_command", "AI editor dock command id is not implemented"));
        return result;
    }

    auto parsed_parameters = parse_dock_parameters(parsed, request);
    if (!parsed_parameters.accepted) {
        result.diagnostics = std::move(parsed_parameters.diagnostics);
        return result;
    }

    if (!command->enabled) {
        result.diagnostics.push_back(diagnostic("command_disabled", "AI editor command is disabled for this revision"));
        return result;
    }

    auto dock_request = make_dock_request(parsed, parsed_parameters, request.user_confirmed);
    if (parsed.kind == EditorDockCommandKind::reset_layout) {
        dock_request.user_confirmed = true;
    }

    const auto plan = plan_editor_dock_command(dock_layout, dock_request);
    if (!plan.accepted) {
        append_dock_command_diagnostics(result, plan);
        return result;
    }

    result.accepted = true;
    result.would_mutate = plan.would_mutate;
    result.requires_confirmation = command->requires_confirmation || plan.requires_confirmation;
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

EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace, EditorDockLayout& dock_layout,
                                                   const EditorAiCommandCatalog& catalog,
                                                   const EditorAiCommandRequest& request) {
    EditorAiCommandApplyResult result;
    result.before_revision = combined_operation_revision(workspace, dock_layout);
    result.after_revision = result.before_revision;

    const auto dry_run = dry_run_editor_ai_command(workspace, dock_layout, catalog, request);
    if (!dry_run.accepted) {
        result.diagnostics = dry_run.diagnostics;
        return result;
    }
    if (dry_run.requires_confirmation && !request.user_confirmed) {
        result.diagnostics.push_back(
            diagnostic("confirmation_required", "AI editor command requires explicit user confirmation"));
        return result;
    }

    if (!is_dock_ai_command(request.command_id)) {
        const PanelCommandInfo* command = find_panel_command(request.command_id);
        if (command == nullptr) {
            result.diagnostics.push_back(diagnostic("unknown_command", "AI editor command id is not implemented"));
            return result;
        }
        workspace.set_panel_visible(command->panel, command->target_visible);
        result.after_revision = combined_operation_revision(workspace, dock_layout);
        result.applied = result.before_revision != result.after_revision;
        return result;
    }

    const auto parsed = parse_dock_command(request.command_id);
    if (!parsed.recognized) {
        result.diagnostics.push_back(diagnostic("unknown_command", "AI editor dock command id is not implemented"));
        return result;
    }

    auto parsed_parameters = parse_dock_parameters(parsed, request);
    if (!parsed_parameters.accepted) {
        result.diagnostics = std::move(parsed_parameters.diagnostics);
        return result;
    }

    const auto plan = apply_editor_dock_command(dock_layout, make_dock_request(parsed, parsed_parameters, true));
    if (!plan.accepted) {
        append_dock_command_diagnostics(result, plan);
        return result;
    }

    result.after_revision = combined_operation_revision(workspace, dock_layout);
    result.applied = result.before_revision != result.after_revision;
    return result;
}

} // namespace mirakana::editor
