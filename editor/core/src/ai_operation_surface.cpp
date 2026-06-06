// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/ai_operation_surface.hpp"

#include <algorithm>
#include <array>
#include <initializer_list>
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

struct ParsedDockWindowCommand {
    bool recognized{false};
    EditorDockWindowCommandKind kind{EditorDockWindowCommandKind::tear_off_panel};
};

struct ParsedDockWindowParameters {
    bool accepted{false};
    std::string window_id;
    std::string target_window_id;
    std::string new_window_id;
    std::string panel_id;
    std::string source_stack_id;
    std::string target_stack_id;
    std::string monitor_id;
    EditorDockWindowBounds bounds;
    float dpi_scale{1.0F};
    std::vector<EditorAiOperationDiagnostic> diagnostics;
};

constexpr std::string_view dock_command_prefix = "editor.dock.";
constexpr std::string_view dock_panel_command_prefix = "editor.dock.panel.";
constexpr std::string_view dock_layout_reset_command = "editor.dock.layout.reset";
constexpr std::string_view dock_window_create_command = "editor.dock.window.create";
constexpr std::string_view dock_window_close_command = "editor.dock.window.close";
constexpr std::string_view dock_panel_tear_off_command = "editor.dock.panel.tear_off";
constexpr std::string_view dock_panel_move_to_window_command = "editor.dock.panel.move_to_window";
constexpr std::string_view dock_window_merge_command = "editor.dock.window.merge";
constexpr std::string_view dock_window_reset_all_command = "editor.dock.window.reset_all";
constexpr std::string_view target_stack_parameter = "target_stack_id";
constexpr std::string_view source_stack_parameter = "source_stack_id";
constexpr std::string_view new_stack_parameter = "new_stack_id";
constexpr std::string_view split_axis_parameter = "split_axis";
constexpr std::string_view split_ratio_parameter = "split_ratio";
constexpr std::string_view window_parameter = "window_id";
constexpr std::string_view target_window_parameter = "target_window_id";
constexpr std::string_view new_window_parameter = "new_window_id";
constexpr std::string_view panel_parameter = "panel_id";
constexpr std::string_view monitor_parameter = "monitor_id";
constexpr std::string_view window_bounds_parameter = "bounds";
constexpr std::string_view dpi_scale_parameter = "dpi_scale";
constexpr std::string_view rich_text_insert_suffix = ".insert_text";
constexpr std::string_view rich_text_delete_selection_suffix = ".delete_selection";
constexpr std::string_view rich_text_replace_selection_suffix = ".replace_selection";
constexpr std::string_view rich_text_toggle_bold_suffix = ".toggle_bold";
constexpr std::string_view rich_text_toggle_italic_suffix = ".toggle_italic";
constexpr std::string_view rich_text_copy_plain_suffix = ".copy_plain_text";
constexpr std::string_view rich_text_copy_selection_suffix = ".copy_selection_plain_text";
constexpr std::string_view rich_text_copy_rich_suffix = ".copy_rich_text";
constexpr std::string_view rich_text_cut_selection_suffix = ".cut_selection";
constexpr std::string_view rich_text_paste_plain_suffix = ".paste_plain_text";
constexpr std::string_view rich_text_paste_rich_suffix = ".paste_rich_text";
constexpr std::string_view plain_text_mime_type = "text/plain;charset=utf-8";
constexpr std::string_view rich_text_mime_type = "application/vnd.mirakanai.rich-text+json";
constexpr std::string_view rich_text_text_parameter = "text";

[[nodiscard]] std::string panel_element_id(PanelId panel) {
    return "editor.panel." + std::string{panel_id_to_string(panel)};
}

[[nodiscard]] std::string dock_layout_element_id() {
    return "editor.dock.layout";
}

[[nodiscard]] std::string dock_windows_element_id() {
    return "editor.dock.windows";
}

[[nodiscard]] std::string dock_window_element_id(std::string_view window_id) {
    return "editor.dock.window." + std::string{window_id};
}

[[nodiscard]] std::string dock_panel_window_command_element_id() {
    return "editor.dock.panel";
}

[[nodiscard]] std::string dock_stack_element_id(std::string_view stack_id) {
    return "editor.dock.stack." + std::string{stack_id};
}

[[nodiscard]] std::string dock_panel_element_id(std::string_view panel_id) {
    return "editor.dock.panel." + std::string{panel_id};
}

[[nodiscard]] std::string rich_text_copy_plain_command_id(std::string_view document_id) {
    return std::string{document_id} + std::string{rich_text_copy_plain_suffix};
}

[[nodiscard]] std::string rich_text_copy_selection_command_id(std::string_view document_id) {
    return std::string{document_id} + std::string{rich_text_copy_selection_suffix};
}

[[nodiscard]] std::string rich_text_command_id(std::string_view document_id, std::string_view suffix) {
    return std::string{document_id} + std::string{suffix};
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

[[nodiscard]] std::uint64_t combined_operation_revision(const Workspace& workspace,
                                                        const EditorDockMultiWindowLayout& dock_layout) noexcept {
    return (workspace_operation_revision(workspace) * 131U) + dock_layout.layout_revision;
}

[[nodiscard]] std::uint64_t combine_revision_value(std::uint64_t revision, std::uint64_t value) noexcept {
    return (revision * 131U) + value;
}

[[nodiscard]] std::uint64_t combine_revision_text(std::uint64_t revision, std::string_view text) noexcept {
    for (const char ch : text) {
        revision = combine_revision_value(revision, static_cast<unsigned char>(ch) + 1U);
    }
    return revision;
}

[[nodiscard]] bool is_dock_ai_command(std::string_view command_id) noexcept {
    return command_id.starts_with(dock_command_prefix);
}

[[nodiscard]] bool is_rich_text_ai_command(std::string_view command_id) noexcept {
    return command_id.ends_with(rich_text_insert_suffix) || command_id.ends_with(rich_text_delete_selection_suffix) ||
           command_id.ends_with(rich_text_replace_selection_suffix) ||
           command_id.ends_with(rich_text_toggle_bold_suffix) || command_id.ends_with(rich_text_toggle_italic_suffix) ||
           command_id.ends_with(rich_text_copy_plain_suffix) || command_id.ends_with(rich_text_copy_selection_suffix) ||
           command_id.ends_with(rich_text_copy_rich_suffix) || command_id.ends_with(rich_text_cut_selection_suffix) ||
           command_id.ends_with(rich_text_paste_plain_suffix) || command_id.ends_with(rich_text_paste_rich_suffix);
}

[[nodiscard]] std::string_view rich_text_document_id_from_command(std::string_view command_id) noexcept {
    for (const auto suffix :
         std::array{rich_text_insert_suffix, rich_text_delete_selection_suffix, rich_text_replace_selection_suffix,
                    rich_text_toggle_bold_suffix, rich_text_toggle_italic_suffix, rich_text_copy_plain_suffix,
                    rich_text_copy_selection_suffix, rich_text_copy_rich_suffix, rich_text_cut_selection_suffix,
                    rich_text_paste_plain_suffix, rich_text_paste_rich_suffix}) {
        if (command_id.ends_with(suffix)) {
            return command_id.substr(0U, command_id.size() - suffix.size());
        }
    }
    return {};
}

[[nodiscard]] EditorRichTextEditCommandKind rich_text_edit_kind_from_command(std::string_view command_id) noexcept {
    if (command_id.ends_with(rich_text_insert_suffix)) {
        return EditorRichTextEditCommandKind::insert_text;
    }
    if (command_id.ends_with(rich_text_delete_selection_suffix)) {
        return EditorRichTextEditCommandKind::delete_selection;
    }
    if (command_id.ends_with(rich_text_replace_selection_suffix)) {
        return EditorRichTextEditCommandKind::replace_selection;
    }
    if (command_id.ends_with(rich_text_toggle_bold_suffix)) {
        return EditorRichTextEditCommandKind::toggle_bold;
    }
    if (command_id.ends_with(rich_text_toggle_italic_suffix)) {
        return EditorRichTextEditCommandKind::toggle_italic;
    }
    if (command_id.ends_with(rich_text_copy_selection_suffix)) {
        return EditorRichTextEditCommandKind::copy_selection_plain_text;
    }
    if (command_id.ends_with(rich_text_copy_plain_suffix)) {
        return EditorRichTextEditCommandKind::copy_plain_text;
    }
    if (command_id.ends_with(rich_text_copy_rich_suffix)) {
        return EditorRichTextEditCommandKind::copy_rich_text;
    }
    if (command_id.ends_with(rich_text_cut_selection_suffix)) {
        return EditorRichTextEditCommandKind::cut_selection;
    }
    if (command_id.ends_with(rich_text_paste_plain_suffix)) {
        return EditorRichTextEditCommandKind::paste_plain_text;
    }
    if (command_id.ends_with(rich_text_paste_rich_suffix)) {
        return EditorRichTextEditCommandKind::paste_rich_text;
    }
    return EditorRichTextEditCommandKind::copy_plain_text;
}

[[nodiscard]] bool rich_text_command_mutates(std::string_view command_id) noexcept {
    return command_id.ends_with(rich_text_insert_suffix) || command_id.ends_with(rich_text_delete_selection_suffix) ||
           command_id.ends_with(rich_text_replace_selection_suffix) ||
           command_id.ends_with(rich_text_toggle_bold_suffix) || command_id.ends_with(rich_text_toggle_italic_suffix) ||
           command_id.ends_with(rich_text_cut_selection_suffix) || command_id.ends_with(rich_text_paste_plain_suffix) ||
           command_id.ends_with(rich_text_paste_rich_suffix);
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

[[nodiscard]] const EditorDockNode* find_dock_stack_containing_panel(const EditorDockMultiWindowLayout& layout,
                                                                     std::string_view panel_id) noexcept {
    const auto it = std::ranges::find_if(layout.nodes, [panel_id](const EditorDockNode& node) {
        return node.kind == EditorDockNodeKind::tab_stack && contains_tab(node, panel_id);
    });
    return it == layout.nodes.end() ? nullptr : &(*it);
}

[[nodiscard]] bool dock_panel_is_visible(const EditorDockLayout& layout, std::string_view panel_id) noexcept {
    return find_dock_stack_containing_panel(layout, panel_id) != nullptr;
}

[[nodiscard]] bool dock_panel_is_visible(const EditorDockMultiWindowLayout& layout,
                                         std::string_view panel_id) noexcept {
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

[[nodiscard]] float parse_dock_float(std::string_view value, std::string_view label) {
    std::size_t consumed = 0;
    const auto parsed = std::stof(std::string{value}, &consumed);
    if (consumed != value.size()) {
        throw std::invalid_argument("AI editor dock " + std::string{label} + " must be a float");
    }
    return parsed;
}

[[nodiscard]] std::vector<std::string_view> split_comma_values(std::string_view value) {
    std::vector<std::string_view> tokens;
    std::size_t begin = 0U;
    while (begin <= value.size()) {
        const auto separator = value.find(',', begin);
        const auto end = separator == std::string_view::npos ? value.size() : separator;
        const auto token = value.substr(begin, end - begin);
        if (token.empty()) {
            throw std::invalid_argument("AI editor dock comma-separated value must not contain empty entries");
        }
        tokens.push_back(token);
        if (separator == std::string_view::npos) {
            break;
        }
        begin = separator + 1U;
    }
    return tokens;
}

[[nodiscard]] EditorDockWindowBounds parse_dock_window_bounds(std::string_view value) {
    const auto tokens = split_comma_values(value);
    if (tokens.size() != 4U) {
        throw std::invalid_argument("AI editor dock window bounds must be x,y,width,height");
    }
    return EditorDockWindowBounds{
        .x = parse_dock_float(tokens[0], "window bounds x"),
        .y = parse_dock_float(tokens[1], "window bounds y"),
        .width = parse_dock_float(tokens[2], "window bounds width"),
        .height = parse_dock_float(tokens[3], "window bounds height"),
    };
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

[[nodiscard]] ParsedDockWindowCommand parse_dock_window_command(std::string_view command_id) {
    if (command_id == dock_window_create_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::create_window};
    }
    if (command_id == dock_window_close_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::close_window};
    }
    if (command_id == dock_panel_tear_off_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::tear_off_panel};
    }
    if (command_id == dock_panel_move_to_window_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::move_panel_to_window};
    }
    if (command_id == dock_window_merge_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::merge_window};
    }
    if (command_id == dock_window_reset_all_command) {
        return ParsedDockWindowCommand{.recognized = true, .kind = EditorDockWindowCommandKind::reset_all_windows};
    }
    return {};
}

[[nodiscard]] bool has_only_parameters(const EditorAiCommandRequest& request,
                                       std::initializer_list<std::string_view> allowed_keys) noexcept {
    return std::ranges::all_of(request.parameters, [allowed_keys](const EditorAiCommandParameter& parameter) {
        return std::ranges::any_of(allowed_keys,
                                   [&parameter](std::string_view allowed_key) { return parameter.key == allowed_key; });
    });
}

[[nodiscard]] std::string required_parameter(ParsedDockWindowParameters& result, const EditorAiCommandRequest& request,
                                             std::string_view key) {
    const auto value = find_parameter(request, key);
    if (!value.has_value() || value->empty()) {
        result.diagnostics.push_back(
            diagnostic("missing_parameter", "AI editor dock window command requires parameter: " + std::string{key}));
        return {};
    }
    return std::string{*value};
}

[[nodiscard]] ParsedDockWindowParameters parse_dock_window_parameters(const ParsedDockWindowCommand& parsed,
                                                                      const EditorAiCommandRequest& request) {
    ParsedDockWindowParameters result;
    const auto parse_common_window_creation = [&]() {
        if (!has_only_parameters(request,
                                 {window_parameter, new_window_parameter, panel_parameter, source_stack_parameter,
                                  monitor_parameter, window_bounds_parameter, dpi_scale_parameter})) {
            result.diagnostics.push_back(
                diagnostic("unsupported_parameters",
                           "AI editor dock window create/tear-off command accepts window_id, new_window_id, panel_id, "
                           "source_stack_id, monitor_id, bounds, and dpi_scale"));
            return false;
        }
        result.window_id = required_parameter(result, request, window_parameter);
        result.new_window_id = required_parameter(result, request, new_window_parameter);
        result.panel_id = required_parameter(result, request, panel_parameter);
        result.source_stack_id = required_parameter(result, request, source_stack_parameter);
        result.monitor_id = required_parameter(result, request, monitor_parameter);
        const auto bounds = find_parameter(request, window_bounds_parameter);
        if (!bounds.has_value() || bounds->empty()) {
            result.diagnostics.push_back(
                diagnostic("missing_parameter", "AI editor dock window command requires bounds"));
            return false;
        }
        try {
            result.bounds = parse_dock_window_bounds(*bounds);
            if (const auto dpi = find_parameter(request, dpi_scale_parameter); dpi.has_value()) {
                result.dpi_scale = parse_dock_float(*dpi, "window dpi_scale");
            }
        } catch (const std::invalid_argument& error) {
            result.diagnostics.push_back(diagnostic("invalid_parameter", error.what()));
            return false;
        }
        return result.diagnostics.empty();
    };

    switch (parsed.kind) {
    case EditorDockWindowCommandKind::create_window:
    case EditorDockWindowCommandKind::tear_off_panel:
        if (!parse_common_window_creation()) {
            return result;
        }
        break;
    case EditorDockWindowCommandKind::move_panel_to_window:
        if (!has_only_parameters(
                request, {window_parameter, target_window_parameter, panel_parameter, target_stack_parameter})) {
            result.diagnostics.push_back(diagnostic(
                "unsupported_parameters",
                "AI editor dock panel move-to-window command accepts window_id, target_window_id, panel_id, and "
                "target_stack_id"));
            return result;
        }
        result.window_id = required_parameter(result, request, window_parameter);
        result.target_window_id = required_parameter(result, request, target_window_parameter);
        result.panel_id = required_parameter(result, request, panel_parameter);
        result.target_stack_id = required_parameter(result, request, target_stack_parameter);
        break;
    case EditorDockWindowCommandKind::merge_window:
        if (!has_only_parameters(request, {window_parameter, target_window_parameter, target_stack_parameter})) {
            result.diagnostics.push_back(diagnostic(
                "unsupported_parameters",
                "AI editor dock window merge command accepts window_id, target_window_id, and target_stack_id"));
            return result;
        }
        result.window_id = required_parameter(result, request, window_parameter);
        result.target_window_id = required_parameter(result, request, target_window_parameter);
        result.target_stack_id = required_parameter(result, request, target_stack_parameter);
        break;
    case EditorDockWindowCommandKind::close_window:
        if (!has_only_parameters(request, {window_parameter})) {
            result.diagnostics.push_back(
                diagnostic("unsupported_parameters", "AI editor dock window close command accepts window_id only"));
            return result;
        }
        result.window_id = required_parameter(result, request, window_parameter);
        break;
    case EditorDockWindowCommandKind::reset_all_windows:
        if (!request.parameters.empty()) {
            result.diagnostics.push_back(diagnostic(
                "unsupported_parameters", "AI editor dock window reset-all command does not accept parameters"));
            return result;
        }
        break;
    }

    if (!result.diagnostics.empty()) {
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

[[nodiscard]] EditorDockWindowCommandRequest make_dock_window_request(const ParsedDockWindowCommand& parsed,
                                                                      const ParsedDockWindowParameters& parameters,
                                                                      bool user_confirmed) {
    return EditorDockWindowCommandRequest{
        .kind = parsed.kind,
        .window_id = parameters.window_id,
        .target_window_id = parameters.target_window_id,
        .new_window_id = parameters.new_window_id,
        .panel_id = parameters.panel_id,
        .source_stack_id = parameters.source_stack_id,
        .target_stack_id = parameters.target_stack_id,
        .monitor_id = parameters.monitor_id,
        .bounds = parameters.bounds,
        .dpi_scale = parameters.dpi_scale,
        .user_confirmed = user_confirmed,
    };
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

[[nodiscard]] const EditorRichTextDocument* find_rich_text_document(std::span<const EditorRichTextDocument> documents,
                                                                    std::string_view document_id) noexcept {
    for (const auto& document : documents) {
        if (document.id == document_id) {
            return &document;
        }
    }
    return nullptr;
}

[[nodiscard]] EditorRichTextDocument* find_rich_text_document(std::span<EditorRichTextDocument> documents,
                                                              std::string_view document_id) noexcept {
    for (auto& document : documents) {
        if (document.id == document_id) {
            return &document;
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

[[nodiscard]] bool contains_forbidden_token(std::string_view text, std::string_view token) noexcept {
    return text.find(token) != std::string_view::npos;
}

[[nodiscard]] std::optional<EditorAiOperationDiagnostic>
reject_forbidden_command_surface(const EditorAiCommandRequest& request) {
    const auto has_token = [&request](std::string_view token) {
        if (contains_forbidden_token(request.command_id, token) ||
            contains_forbidden_token(request.target_element_id, token)) {
            return true;
        }
        return std::ranges::any_of(request.parameters, [token](const EditorAiCommandParameter& parameter) {
            return contains_forbidden_token(parameter.key, token) || contains_forbidden_token(parameter.value, token);
        });
    };

    if (has_token("native_handle") || has_token("hwnd") || has_token("dxgi") || has_token("d3d12_handle")) {
        return diagnostic("native_handle_unsupported", "AI editor commands must not request or expose native handles");
    }
    if (request.command_id.starts_with("editor.shell.") || request.command_id.starts_with("editor.process.") ||
        has_token("shell.execute")) {
        return diagnostic("shell_execution_unsupported",
                          "AI editor operation surface does not execute shell or process commands");
    }
    if (contains_forbidden_token(request.command_id, "validation.recipe") ||
        contains_forbidden_token(request.target_element_id, "validation.recipe")) {
        return diagnostic("validation_recipe_execution_unsupported",
                          "AI editor operation surface cannot execute validation recipes from editor core");
    }
    if (has_token("screen_x") || has_token("screen_y") || has_token("screen_coordinate")) {
        return diagnostic("screen_coordinates_unsupported",
                          "AI editor commands must use stable retained ids, not screen coordinates");
    }

    return std::nullopt;
}

[[nodiscard]] std::optional<EditorAiOperationDiagnostic> reject_stale_revision(const EditorAiCommandCatalog& catalog,
                                                                               const EditorAiCommandRequest& request) {
    if (request.expected_revision != 0U && request.expected_revision != catalog.revision) {
        return diagnostic("stale_revision", "AI editor command expected_revision does not match the catalog revision");
    }
    return std::nullopt;
}

[[nodiscard]] std::optional<EditorAiOperationDiagnostic>
reject_invalid_command_request(const EditorAiCommandCatalog& catalog, const EditorAiCommandRequest& request) {
    if (auto forbidden = reject_forbidden_command_surface(request); forbidden.has_value()) {
        return forbidden;
    }
    return reject_stale_revision(catalog, request);
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

void append_dock_command_diagnostics(EditorAiCommandDryRunResult& result, const EditorDockWindowCommandPlan& plan) {
    for (const auto& item : plan.diagnostics) {
        result.diagnostics.push_back(diagnostic(item.code, item.message));
    }
}

void append_dock_command_diagnostics(EditorAiCommandApplyResult& result, const EditorDockCommandPlan& plan) {
    for (const auto& item : plan.diagnostics) {
        result.diagnostics.push_back(diagnostic(item.code, item.message));
    }
}

void append_dock_command_diagnostics(EditorAiCommandApplyResult& result, const EditorDockWindowCommandPlan& plan) {
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

[[nodiscard]] bool status_is_ready(std::string_view status) noexcept {
    return status == "ready" || status == "d3d12_texture_ready" || status == "vulkan_texture_ready" ||
           status == "metal_texture_ready" || status == "uia_provider_ready" || status == "win32_tsf_selected" ||
           status == "win32_tsf_session_active" || status == "value_text_input_controller_ready" ||
           status == "value_text_input_session_active" || status == "value_text_input_commit_applied";
}

void append_status_row(std::vector<EditorAiOperationStatusRow>& rows, EditorAiOperationStatusRow row) {
    rows.push_back(std::move(row));
}

} // namespace

std::vector<EditorAiOperationStatusRow>
make_editor_ai_operation_ux_status_rows(const EditorAiOperationUxStatusDesc& desc) {
    std::vector<EditorAiOperationStatusRow> rows;
    rows.reserve(20U);

    if (!desc.selected_dock_panel_id.empty()) {
        append_status_row(rows, EditorAiOperationStatusRow{
                                    .id = "editor.ai.dock.selected_panel",
                                    .role = "selected_dock_panel",
                                    .label = "Selected Dock Panel",
                                    .target_element_id = dock_panel_element_id(desc.selected_dock_panel_id),
                                    .status = desc.selected_dock_panel_id,
                                    .count = 1U,
                                    .ready = true,
                                });
    }
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.rich_text.documents",
                                .role = "rich_text_summary",
                                .label = "Rich Text Documents",
                                .target_element_id = "editor.rich_text",
                                .status = desc.rich_text_document_count > 0U ? "ready" : "not_available",
                                .count = desc.rich_text_document_count,
                                .ready = desc.rich_text_document_count > 0U,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text_input.focused_target",
                                .role = "text_input_status",
                                .label = "Focused Text Input Target",
                                .target_element_id = desc.focused_text_target_id,
                                .status = desc.text_input_status,
                                .count = desc.ime_text_input_session_rows,
                                .ready = status_is_ready(desc.text_input_status),
                                .native_handles_public = desc.ime_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.adapter.text_font",
                                .role = "text_font_adapter_status",
                                .label = "Text Font Adapter",
                                .target_element_id = "editor.adapter.text_font",
                                .status = desc.text_atlas_handoff_status,
                                .count = desc.text_font_adapter_invoked ? 1U : 0U,
                                .ready = desc.text_font_adapter_invoked && desc.text_font_glyphs_ready,
                                .host_gated = desc.text_atlas_handoff_host_gated_rows > 0U,
                                .native_handles_public = desc.text_font_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text.shaping",
                                .role = "text_shaping_status",
                                .label = "Text Shaping",
                                .target_element_id = "editor.text.shaping",
                                .status = desc.text_shaping_status,
                                .count = desc.text_shaping_segment_rows,
                                .ready = desc.text_shaping_status == "ready" && desc.text_glyph_cluster_rows > 0U &&
                                         desc.text_glyph_advance_offset_rows > 0U,
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text.font_fallback",
                                .role = "text_font_fallback_status",
                                .label = "Text Font Fallback",
                                .target_element_id = "editor.text.font_fallback",
                                .status = desc.text_font_fallback_status,
                                .count = desc.text_font_face_rows,
                                .ready = desc.text_font_fallback_status == "ready",
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text.glyph_atlas",
                                .role = "text_glyph_atlas_status",
                                .label = "Text Glyph Atlas",
                                .target_element_id = "editor.text.glyph_atlas",
                                .status = desc.text_glyph_atlas_status,
                                .count = desc.text_glyph_atlas_allocation_rows,
                                .ready = desc.text_glyph_atlas_status == "ready" &&
                                         desc.text_glyph_bitmap_format_rows > 0U && desc.text_glyph_metric_rows > 0U,
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text.bidi",
                                .role = "text_bidi_status",
                                .label = "Text Bidi",
                                .target_element_id = "editor.text.bidi",
                                .status = desc.text_bidi_status,
                                .count = desc.text_bidi_boundary_rows,
                                .ready = desc.text_bidi_status == "ready",
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text.line_break",
                                .role = "text_line_break_status",
                                .label = "Text Line Break",
                                .target_element_id = "editor.text.line_break",
                                .status = desc.text_line_break_status,
                                .count = desc.text_line_break_boundary_rows,
                                .ready = desc.text_line_break_status == "ready",
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.text_dependency.licenses",
                                .role = "text_dependency_license_status",
                                .label = "Text Dependency Licenses",
                                .target_element_id = "editor.text_dependency.licenses",
                                .status = desc.text_dependency_license_records,
                                .count = desc.text_font_license_provenance_rows,
                                .ready = desc.text_dependency_license_records == "ready",
                                .host_gated = desc.text_dependency_gated_rows > 0U,
                                .native_handles_public = desc.text_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.ime.session",
                                .role = "ime_status",
                                .label = "IME Session",
                                .target_element_id = desc.focused_text_target_id,
                                .status = desc.text_input_status,
                                .count = desc.ime_text_input_session_rows,
                                .ready = status_is_ready(desc.text_input_status),
                                .host_gated = !desc.ime_candidate_ui_host_owned,
                                .native_handles_public = desc.ime_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.ime.parity",
                                .role = "ime_parity_status",
                                .label = "IME Parity",
                                .target_element_id = desc.focused_text_target_id,
                                .status = desc.ime_parity_status.empty() ? "not_ready" : desc.ime_parity_status,
                                .count = desc.ime_grapheme_boundary_rows,
                                .ready = desc.ime_parity_status == "ready" && desc.ime_grapheme_cursor_rows > 0U &&
                                         desc.ime_grapheme_selection_rows > 0U && desc.ime_composition_range_rows > 0U,
                                .native_handles_public = desc.ime_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.ime.candidate_selection",
                                .role = "ime_candidate_selection_status",
                                .label = "IME Candidate Selection",
                                .target_element_id = desc.focused_text_target_id,
                                .status = desc.ime_candidate_selection_rows > 0U ? "ready" : "not_ready",
                                .count = desc.ime_candidate_selection_rows,
                                .ready = desc.ime_candidate_selection_rows > 0U && desc.ime_candidate_ui_host_owned,
                                .host_gated = !desc.ime_candidate_ui_host_owned,
                                .native_handles_public = desc.ime_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.ime.reconversion",
                                .role = "ime_reconversion_status",
                                .label = "IME Reconversion",
                                .target_element_id = desc.focused_text_target_id,
                                .status = desc.ime_reconversion_request_rows > 0U ? "ready" : "not_ready",
                                .count = desc.ime_reconversion_request_rows,
                                .ready = desc.ime_reconversion_request_rows > 0U,
                                .native_handles_public = desc.ime_native_handles_exposed,
                            });
    append_status_row(
        rows,
        EditorAiOperationStatusRow{
            .id = "editor.ai.ime.platform_host_gates",
            .role = "ime_platform_host_gate_status",
            .label = "IME Platform Host Gates",
            .target_element_id = "editor.ime.platform_host_gates",
            .status =
                "macos:" + (desc.ime_macos_status.empty() ? std::string{"host_gated"} : desc.ime_macos_status) +
                ";linux_ibus:" +
                (desc.ime_linux_ibus_status.empty() ? std::string{"host_gated"} : desc.ime_linux_ibus_status) +
                ";linux_fcitx:" +
                (desc.ime_linux_fcitx_status.empty() ? std::string{"host_gated"} : desc.ime_linux_fcitx_status) +
                ";android:" + (desc.ime_android_status.empty() ? std::string{"host_gated"} : desc.ime_android_status) +
                ";ios:" + (desc.ime_ios_status.empty() ? std::string{"host_gated"} : desc.ime_ios_status),
            .count = 5U,
            .ready = false,
            .host_gated = true,
            .native_handles_public = desc.ime_native_handles_exposed,
        });
    append_status_row(
        rows, EditorAiOperationStatusRow{
                  .id = "editor.ai.accessibility.uia_provider",
                  .role = "accessibility_uia_status",
                  .label = "Windows UIA Provider",
                  .target_element_id = "editor.accessibility.uia_provider",
                  .status = desc.accessibility_status,
                  .count = desc.accessibility_nodes,
                  .ready = desc.accessibility_status == "uia_provider_ready" && desc.accessibility_diagnostics == 0U,
                  .native_handles_public = desc.accessibility_native_handles_exposed,
              });
    append_status_row(
        rows,
        EditorAiOperationStatusRow{
            .id = "editor.ai.accessibility.parity",
            .role = "accessibility_parity_status",
            .label = "Accessibility Parity",
            .target_element_id = "editor.accessibility.parity",
            .status = desc.accessibility_parity_status.empty() ? "not_ready" : desc.accessibility_parity_status,
            .count = desc.accessibility_nodes,
            .ready = desc.accessibility_parity_status == "ready" && desc.accessibility_windows_uia_patterns_ready &&
                     desc.accessibility_windows_uia_events_ready && !desc.accessibility_native_handles_exposed,
            .host_gated = desc.accessibility_macos_status == "host_gated" ||
                          desc.accessibility_linux_at_spi_status == "host_gated" ||
                          desc.accessibility_android_status == "host_gated" ||
                          desc.accessibility_ios_status == "host_gated",
            .native_handles_public = desc.accessibility_native_handles_exposed,
        });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.viewport.display",
                                .role = "viewport_display_status",
                                .label = "Viewport Display",
                                .target_element_id = "editor.dock.panel.viewport",
                                .status = desc.viewport_status,
                                .count = desc.viewport_visible_texture_composites,
                                .ready = status_is_ready(desc.viewport_status),
                                .native_handles_public = desc.viewport_native_handles_exposed,
                            });
    append_status_row(rows, EditorAiOperationStatusRow{
                                .id = "editor.ai.material_preview.display",
                                .role = "material_preview_display_status",
                                .label = "Material Preview Display",
                                .target_element_id = "editor.ai.material_preview.display",
                                .status = desc.material_preview_status,
                                .count = desc.material_preview_visible_texture_composites,
                                .ready = status_is_ready(desc.material_preview_status),
                                .native_handles_public = desc.material_preview_native_handles_exposed,
                            });
    append_status_row(
        rows,
        EditorAiOperationStatusRow{
            .id = "editor.ai.shell.cross_platform",
            .role = "cross_platform_editor_shell_status",
            .label = "Cross-Platform Editor Shell",
            .target_element_id = "editor.shell.cross_platform",
            .status =
                "macos:" + (desc.macos_shell_status.empty() ? std::string{"host_gated"} : desc.macos_shell_status) +
                ";linux:" + (desc.linux_shell_status.empty() ? std::string{"host_gated"} : desc.linux_shell_status) +
                ";android:" +
                (desc.android_shell_status.empty() ? std::string{"unsupported"} : desc.android_shell_status) +
                ";ios:" + (desc.ios_shell_status.empty() ? std::string{"unsupported"} : desc.ios_shell_status),
            .count = 4U,
            .ready = false,
            .host_gated =
                (desc.cross_platform_shell_status.empty() ? std::string{"host_gated"}
                                                          : desc.cross_platform_shell_status) == "host_gated" ||
                desc.macos_shell_status == "host_gated" || desc.linux_shell_status == "host_gated",
            .native_handles_public = desc.cross_platform_shell_native_handles_exposed,
        });

    return rows;
}

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

EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace,
                                                            const EditorDockMultiWindowLayout& dock_layout) {
    auto snapshot = make_editor_ai_operation_snapshot(workspace);
    snapshot.revision = combined_operation_revision(workspace, dock_layout);

    const auto validation = validate_editor_dock_multi_window_layout(dock_layout);
    append_dock_layout_diagnostics(snapshot, validation);

    snapshot.elements.push_back(EditorAiOperationElementRow{
        .id = dock_windows_element_id(),
        .role = "dock_windows",
        .label = "Dock Windows",
        .visible = true,
        .enabled = validation.valid,
    });
    for (const auto& window : dock_layout.windows) {
        snapshot.elements.push_back(EditorAiOperationElementRow{
            .id = dock_window_element_id(window.id),
            .role = "dock_window",
            .label = window.title.empty() ? window.id : window.title,
            .visible = true,
            .enabled = validation.valid,
        });
    }
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
    snapshot.status_rows.push_back(EditorAiOperationStatusRow{
        .id = "editor.ai.dock.workspace_v3",
        .role = "workspace_v3",
        .label = "Workspace v3",
        .target_element_id = dock_windows_element_id(),
        .status = validation.valid ? "ready" : "invalid",
        .count = dock_layout.windows.size(),
        .ready = validation.valid,
        .native_handles_public = dock_layout.native_handles_public,
    });
    return snapshot;
}

EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace,
                                                            const EditorDockLayout& dock_layout,
                                                            std::span<const EditorRichTextDocument> rich_text_documents,
                                                            EditorRichTextViewport viewport) {
    auto snapshot = make_editor_ai_operation_snapshot(workspace, dock_layout);
    for (const auto& document : rich_text_documents) {
        const auto rich_text = make_editor_rich_text_ai_snapshot(document, viewport);
        snapshot.revision = (snapshot.revision * 131U) + rich_text.revision;
        snapshot.rich_text_rows.insert(snapshot.rich_text_rows.end(), rich_text.rows.begin(), rich_text.rows.end());
        for (const auto& item : rich_text.diagnostics) {
            snapshot.diagnostics.push_back(diagnostic(item.code, item.message));
        }
    }
    return snapshot;
}

EditorAiOperationSnapshot make_editor_ai_operation_snapshot(const Workspace& workspace,
                                                            const EditorDockLayout& dock_layout,
                                                            std::span<const EditorRichTextDocument> rich_text_documents,
                                                            std::span<const EditorAiOperationStatusRow> status_rows,
                                                            EditorRichTextViewport viewport) {
    auto snapshot = make_editor_ai_operation_snapshot(workspace, dock_layout, rich_text_documents, viewport);
    snapshot.status_rows.insert(snapshot.status_rows.end(), status_rows.begin(), status_rows.end());
    for (const auto& row : status_rows) {
        snapshot.revision = combine_revision_text(snapshot.revision, row.id);
        snapshot.revision = combine_revision_text(snapshot.revision, row.status);
        snapshot.revision = combine_revision_value(snapshot.revision, row.count);
        snapshot.revision = combine_revision_value(snapshot.revision, row.ready ? 1U : 0U);
        if (row.native_handles_public) {
            snapshot.diagnostics.push_back(
                diagnostic("native_handle_unsupported", row.id + " must not expose native handles"));
        }
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

EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace,
                                                      const EditorDockMultiWindowLayout& dock_layout) {
    auto catalog = make_editor_ai_command_catalog(workspace);
    const auto validation = validate_editor_dock_multi_window_layout(dock_layout);
    catalog.revision = combined_operation_revision(workspace, dock_layout);
    const bool enabled = validation.valid;

    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_window_create_command},
        .label = "Create Dock Window",
        .target_element_id = dock_windows_element_id(),
        .enabled = enabled,
        .mutates_state = true,
        .requires_confirmation = false,
    });
    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_window_close_command},
        .label = "Close Dock Window",
        .target_element_id = dock_windows_element_id(),
        .enabled = enabled,
        .mutates_state = true,
        .requires_confirmation = false,
    });
    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_panel_tear_off_command},
        .label = "Tear Off Dock Panel",
        .target_element_id = dock_panel_window_command_element_id(),
        .enabled = enabled,
        .mutates_state = true,
        .requires_confirmation = false,
    });
    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_panel_move_to_window_command},
        .label = "Move Dock Panel To Window",
        .target_element_id = dock_panel_window_command_element_id(),
        .enabled = enabled,
        .mutates_state = true,
        .requires_confirmation = false,
    });
    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_window_merge_command},
        .label = "Merge Dock Window",
        .target_element_id = dock_windows_element_id(),
        .enabled = enabled,
        .mutates_state = true,
        .requires_confirmation = false,
    });
    catalog.commands.push_back(EditorAiCommandRow{
        .id = std::string{dock_window_reset_all_command},
        .label = "Reset All Dock Windows",
        .target_element_id = dock_windows_element_id(),
        .enabled = true,
        .mutates_state = true,
        .requires_confirmation = true,
    });

    return catalog;
}

EditorAiCommandCatalog make_editor_ai_command_catalog(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                                      std::span<const EditorRichTextDocument> rich_text_documents) {
    auto catalog = make_editor_ai_command_catalog(workspace, dock_layout);

    for (const auto& document : rich_text_documents) {
        const auto validation = validate_editor_rich_text_document(document);
        const auto plain_text = copy_editor_rich_text_plain_text(document);
        const auto selected_text = copy_editor_rich_text_selection_plain_text(document);
        const auto rich_text = make_editor_rich_text_ai_snapshot(document);
        catalog.revision = combine_revision_value(catalog.revision, rich_text.revision);

        catalog.commands.push_back(EditorAiCommandRow{
            .id = rich_text_copy_plain_command_id(document.id),
            .label = "Copy Rich Text Plain Text",
            .target_element_id = document.id,
            .enabled = validation.valid && plain_text.valid && !plain_text.text.empty(),
            .mutates_state = false,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = rich_text_copy_selection_command_id(document.id),
            .label = "Copy Rich Text Selection Plain Text",
            .target_element_id = document.id,
            .enabled = validation.valid && selected_text.valid && !selected_text.text.empty(),
            .mutates_state = false,
            .requires_confirmation = false,
        });
        catalog.commands.push_back(EditorAiCommandRow{
            .id = rich_text_command_id(document.id, rich_text_copy_rich_suffix),
            .label = "Copy Rich Text",
            .target_element_id = document.id,
            .enabled = validation.valid && selected_text.valid && !selected_text.text.empty(),
            .mutates_state = false,
            .requires_confirmation = false,
        });

        if (document.editable && validation.valid) {
            const auto append_edit_command = [&catalog, &document](std::string_view suffix, std::string label,
                                                                   bool requires_selection) {
                catalog.commands.push_back(EditorAiCommandRow{
                    .id = rich_text_command_id(document.id, suffix),
                    .label = std::move(label),
                    .target_element_id = document.id,
                    .enabled = !requires_selection || document.selection.active,
                    .mutates_state = true,
                    .requires_confirmation = false,
                });
            };
            append_edit_command(rich_text_insert_suffix, "Insert Rich Text", false);
            append_edit_command(rich_text_delete_selection_suffix, "Delete Rich Text Selection", true);
            append_edit_command(rich_text_replace_selection_suffix, "Replace Rich Text Selection", true);
            append_edit_command(rich_text_toggle_bold_suffix, "Toggle Rich Text Bold", true);
            append_edit_command(rich_text_toggle_italic_suffix, "Toggle Rich Text Italic", true);
            append_edit_command(rich_text_cut_selection_suffix, "Cut Rich Text Selection", true);
            append_edit_command(rich_text_paste_plain_suffix, "Paste Rich Text Plain Text", false);
            append_edit_command(rich_text_paste_rich_suffix, "Paste Rich Text", false);
        }
    }

    return catalog;
}

EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace, const EditorAiCommandCatalog& catalog,
                                                      const EditorAiCommandRequest& request) {
    (void)workspace;
    EditorAiCommandDryRunResult result;
    if (const auto rejected = reject_invalid_command_request(catalog, request); rejected.has_value()) {
        result.diagnostics.push_back(*rejected);
        return result;
    }
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
    if (const auto rejected = reject_invalid_command_request(catalog, request); rejected.has_value()) {
        EditorAiCommandDryRunResult result;
        result.diagnostics.push_back(*rejected);
        return result;
    }
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

EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace,
                                                      const EditorDockMultiWindowLayout& dock_layout,
                                                      const EditorAiCommandCatalog& catalog,
                                                      const EditorAiCommandRequest& request) {
    if (const auto rejected = reject_invalid_command_request(catalog, request); rejected.has_value()) {
        EditorAiCommandDryRunResult result;
        result.diagnostics.push_back(*rejected);
        return result;
    }
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

    const auto parsed = parse_dock_window_command(request.command_id);
    if (!parsed.recognized) {
        result.diagnostics.push_back(
            diagnostic("unknown_command", "AI editor dock window command id is not implemented"));
        return result;
    }
    auto parsed_parameters = parse_dock_window_parameters(parsed, request);
    if (!parsed_parameters.accepted) {
        result.diagnostics = std::move(parsed_parameters.diagnostics);
        return result;
    }

    if (!command->enabled) {
        result.diagnostics.push_back(diagnostic("command_disabled", "AI editor command is disabled for this revision"));
        return result;
    }

    auto dock_request = make_dock_window_request(parsed, parsed_parameters, request.user_confirmed);
    if (parsed.kind == EditorDockWindowCommandKind::reset_all_windows) {
        dock_request.user_confirmed = true;
    }
    const auto plan = plan_editor_dock_window_command(dock_layout, dock_request);
    if (!plan.accepted) {
        append_dock_command_diagnostics(result, plan);
        return result;
    }

    result.accepted = true;
    result.would_mutate = plan.would_mutate;
    result.requires_confirmation = command->requires_confirmation || plan.requires_confirmation;
    return result;
}

EditorAiCommandDryRunResult dry_run_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                                      std::span<const EditorRichTextDocument> rich_text_documents,
                                                      const EditorAiCommandCatalog& catalog,
                                                      const EditorAiCommandRequest& request) {
    if (const auto rejected = reject_invalid_command_request(catalog, request); rejected.has_value()) {
        EditorAiCommandDryRunResult result;
        result.diagnostics.push_back(*rejected);
        return result;
    }
    if (!is_rich_text_ai_command(request.command_id)) {
        return dry_run_editor_ai_command(workspace, dock_layout, catalog, request);
    }

    EditorAiCommandDryRunResult result;
    const EditorAiCommandRow* command = find_command(catalog, request.command_id);
    if (command == nullptr) {
        result.diagnostics.push_back(
            diagnostic("unknown_command", "AI editor rich text command id is not in the catalog"));
        return result;
    }
    if (command->target_element_id != request.target_element_id) {
        result.diagnostics.push_back(
            diagnostic("target_mismatch", "AI editor command target does not match the catalog row"));
        return result;
    }
    if (!command->enabled) {
        result.diagnostics.push_back(diagnostic("command_disabled", "AI editor command is disabled for this revision"));
        return result;
    }

    const auto document_id = rich_text_document_id_from_command(request.command_id);
    const auto* document = find_rich_text_document(rich_text_documents, document_id);
    if (document == nullptr) {
        result.diagnostics.push_back(
            diagnostic("missing_rich_text_document", "AI editor rich text command target document is unavailable"));
        return result;
    }

    const auto kind = rich_text_edit_kind_from_command(request.command_id);
    EditorRichTextEditRequest edit_request{
        .kind = kind,
        .document_id = document->id,
        .expected_revision = editor_rich_text_revision(*document),
    };
    const auto text_parameter = find_parameter(request, rich_text_text_parameter);
    const bool command_needs_text = kind == EditorRichTextEditCommandKind::insert_text ||
                                    kind == EditorRichTextEditCommandKind::replace_selection ||
                                    kind == EditorRichTextEditCommandKind::paste_plain_text ||
                                    kind == EditorRichTextEditCommandKind::paste_rich_text;
    if (command_needs_text) {
        if (request.parameters.size() != 1U || !text_parameter.has_value()) {
            result.diagnostics.push_back(
                diagnostic("missing_parameter", "AI editor rich text command requires text parameter only"));
            return result;
        }
        if (kind == EditorRichTextEditCommandKind::paste_plain_text) {
            edit_request.clipboard.has_plain_text = true;
            edit_request.clipboard.plain_text = std::string{*text_parameter};
        } else if (kind == EditorRichTextEditCommandKind::paste_rich_text) {
            edit_request.clipboard.has_plain_text = true;
            edit_request.clipboard.has_rich_text = true;
            edit_request.clipboard.plain_text = std::string{*text_parameter};
            edit_request.clipboard.rich_paragraphs.push_back(EditorRichTextParagraph{
                .id = "ai_paste",
                .spans =
                    {
                        EditorRichTextSpan{
                            .id = "body",
                            .style_token = "editor.text",
                            .text = std::string{*text_parameter},
                        },
                    },
            });
        } else {
            edit_request.text = std::string{*text_parameter};
        }
    } else if (!request.parameters.empty()) {
        result.diagnostics.push_back(
            diagnostic("unsupported_parameters", "AI editor rich text command does not accept parameters"));
        return result;
    }

    const auto edit = apply_editor_rich_text_edit_command(*document, edit_request);
    if (!edit.accepted) {
        for (const auto& item : edit.diagnostics) {
            result.diagnostics.push_back(diagnostic(item.code, item.message));
        }
        return result;
    }

    result.accepted = true;
    result.would_mutate = rich_text_command_mutates(request.command_id);
    result.requires_confirmation = false;
    result.output_text = edit.output_text;
    result.output_mime_type = edit.output_mime_type.empty() ? (kind == EditorRichTextEditCommandKind::copy_rich_text
                                                                   ? std::string{rich_text_mime_type}
                                                                   : std::string{plain_text_mime_type})
                                                            : edit.output_mime_type;
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
    result.accepted = true;
    result.completed = true;
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
        result.accepted = true;
        result.completed = true;
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
    result.accepted = true;
    result.completed = true;
    result.applied = result.before_revision != result.after_revision;
    return result;
}

EditorAiCommandApplyResult apply_editor_ai_command(Workspace& workspace, EditorDockMultiWindowLayout& dock_layout,
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
        result.accepted = true;
        result.completed = true;
        result.applied = result.before_revision != result.after_revision;
        return result;
    }

    const auto parsed = parse_dock_window_command(request.command_id);
    if (!parsed.recognized) {
        result.diagnostics.push_back(
            diagnostic("unknown_command", "AI editor dock window command id is not implemented"));
        return result;
    }

    auto parsed_parameters = parse_dock_window_parameters(parsed, request);
    if (!parsed_parameters.accepted) {
        result.diagnostics = std::move(parsed_parameters.diagnostics);
        return result;
    }

    const auto plan =
        apply_editor_dock_window_command(dock_layout, make_dock_window_request(parsed, parsed_parameters, true));
    if (!plan.accepted) {
        append_dock_command_diagnostics(result, plan);
        return result;
    }

    result.after_revision = combined_operation_revision(workspace, dock_layout);
    result.accepted = true;
    result.completed = true;
    result.applied = result.before_revision != result.after_revision;
    return result;
}

EditorAiCommandApplyResult apply_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                                   std::span<const EditorRichTextDocument> rich_text_documents,
                                                   const EditorAiCommandCatalog& catalog,
                                                   const EditorAiCommandRequest& request) {
    EditorAiCommandApplyResult result;
    result.before_revision = catalog.revision;
    result.after_revision = result.before_revision;

    const auto dry_run = dry_run_editor_ai_command(workspace, dock_layout, rich_text_documents, catalog, request);
    if (!dry_run.accepted) {
        result.diagnostics = dry_run.diagnostics;
        return result;
    }
    if (dry_run.requires_confirmation && !request.user_confirmed) {
        result.diagnostics.push_back(
            diagnostic("confirmation_required", "AI editor command requires explicit user confirmation"));
        return result;
    }

    result.accepted = true;
    result.completed = true;
    result.applied = false;
    result.output_text = dry_run.output_text;
    result.output_mime_type = dry_run.output_mime_type;
    return result;
}

EditorAiCommandApplyResult apply_editor_ai_command(const Workspace& workspace, const EditorDockLayout& dock_layout,
                                                   std::span<EditorRichTextDocument> rich_text_documents,
                                                   const EditorAiCommandCatalog& catalog,
                                                   const EditorAiCommandRequest& request) {
    EditorAiCommandApplyResult result;
    result.before_revision = catalog.revision;
    result.after_revision = result.before_revision;

    const auto dry_run = dry_run_editor_ai_command(
        workspace, dock_layout,
        std::span<const EditorRichTextDocument>{rich_text_documents.data(), rich_text_documents.size()}, catalog,
        request);
    if (!dry_run.accepted) {
        result.diagnostics = dry_run.diagnostics;
        return result;
    }
    if (dry_run.requires_confirmation && !request.user_confirmed) {
        result.diagnostics.push_back(
            diagnostic("confirmation_required", "AI editor command requires explicit user confirmation"));
        return result;
    }

    if (!rich_text_command_mutates(request.command_id)) {
        result.accepted = true;
        result.completed = true;
        result.applied = false;
        result.output_text = dry_run.output_text;
        result.output_mime_type = dry_run.output_mime_type;
        return result;
    }

    const auto document_id = rich_text_document_id_from_command(request.command_id);
    auto* document = find_rich_text_document(rich_text_documents, document_id);
    if (document == nullptr) {
        result.diagnostics.push_back(
            diagnostic("missing_rich_text_document", "AI editor rich text command target document is unavailable"));
        return result;
    }

    const auto kind = rich_text_edit_kind_from_command(request.command_id);
    EditorRichTextEditRequest edit_request{
        .kind = kind,
        .document_id = document->id,
        .expected_revision = editor_rich_text_revision(*document),
    };
    if (const auto text_parameter = find_parameter(request, rich_text_text_parameter); text_parameter.has_value()) {
        if (kind == EditorRichTextEditCommandKind::paste_plain_text) {
            edit_request.clipboard.has_plain_text = true;
            edit_request.clipboard.plain_text = std::string{*text_parameter};
        } else if (kind == EditorRichTextEditCommandKind::paste_rich_text) {
            edit_request.clipboard.has_plain_text = true;
            edit_request.clipboard.has_rich_text = true;
            edit_request.clipboard.plain_text = std::string{*text_parameter};
            edit_request.clipboard.rich_paragraphs.push_back(EditorRichTextParagraph{
                .id = "ai_paste",
                .spans =
                    {
                        EditorRichTextSpan{
                            .id = "body",
                            .style_token = "editor.text",
                            .text = std::string{*text_parameter},
                        },
                    },
            });
        } else {
            edit_request.text = std::string{*text_parameter};
        }
    }

    const auto edit = apply_editor_rich_text_edit_command(*document, edit_request);
    if (!edit.accepted) {
        for (const auto& item : edit.diagnostics) {
            result.diagnostics.push_back(diagnostic(item.code, item.message));
        }
        return result;
    }

    *document = edit.document;
    result.accepted = true;
    result.completed = true;
    result.applied = edit.applied;
    result.output_text = edit.output_text;
    result.output_mime_type = edit.output_mime_type;
    result.after_revision = combine_revision_value(catalog.revision, editor_rich_text_revision(*document));
    return result;
}

} // namespace mirakana::editor
