// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/project.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

constexpr int current_project_format_version = 4;
constexpr std::string_view default_source_registry_path = "source/assets/package.geassets";

void validate_field(std::string_view field, const char* name) {
    if (field.empty()) {
        throw std::invalid_argument(std::string(name) + " must not be empty");
    }
    if (field.find_first_of("\r\n=") != std::string_view::npos) {
        throw std::invalid_argument(std::string(name) + " must not contain newlines or '='");
    }
}

bool is_windows_drive_path(std::string_view path) {
    return path.size() >= 2 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':';
}

[[nodiscard]] bool has_parent_path_segment(std::string_view path) noexcept {
    std::size_t segment_begin = 0;
    while (segment_begin <= path.size()) {
        const auto segment_end = path.find_first_of("/\\", segment_begin);
        const auto segment =
            path.substr(segment_begin,
                        segment_end == std::string_view::npos ? std::string_view::npos : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

void validate_safe_relative_path(std::string_view path, const char* name) {
    if (path.empty()) {
        throw std::invalid_argument(std::string(name) + " must not be empty");
    }
    if (path.front() == '/' || path.front() == '\\' || is_windows_drive_path(path)) {
        throw std::invalid_argument(std::string(name) + " must be project-relative");
    }
    if (has_parent_path_segment(path)) {
        throw std::invalid_argument(std::string(name) + " must not contain parent directory traversal");
    }
}

void validate_project_source_registry_path(std::string_view path) {
    validate_field(path, "project source registry path");
    validate_safe_relative_path(path, "project source registry path");
    if (!path.ends_with(".geassets")) {
        throw std::invalid_argument("project source registry path must end with .geassets");
    }
}

char lower_ascii(char value) noexcept {
    return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
}

std::string executable_leaf(std::string_view executable) {
    const auto separator = executable.find_last_of("/\\");
    auto leaf = executable.substr(separator == std::string_view::npos ? 0U : separator + 1U);
    std::string result(leaf);
    std::ranges::transform(result, result.begin(), lower_ascii);
    return result;
}

bool is_shell_executable(std::string_view executable) {
    const auto leaf = executable_leaf(executable);
    return leaf == "cmd" || leaf == "cmd.exe" || leaf == "powershell" || leaf == "powershell.exe" || leaf == "pwsh" ||
           leaf == "pwsh.exe" || leaf == "bash" || leaf == "bash.exe" || leaf == "sh" || leaf == "sh.exe" ||
           leaf == "wscript" || leaf == "wscript.exe" || leaf == "cscript" || leaf == "cscript.exe";
}

[[nodiscard]] std::string sanitize_text(std::string_view value) {
    if (value.empty()) {
        return "-";
    }

    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text;
}

[[nodiscard]] std::string sanitize_element_id(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (std::isalnum(byte) != 0 || character == '_' || character == '-' || character == '.') {
            text.push_back(character);
        } else {
            text.push_back('_');
        }
    }
    return text.empty() ? "row" : text;
}

[[nodiscard]] mirakana::ui::TextContent make_ui_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void add_ui_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("project file dialog ui element could not be added");
    }
}

[[nodiscard]] mirakana::ui::ElementDesc make_ui_root(std::string id, mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

[[nodiscard]] mirakana::ui::ElementDesc make_ui_child(std::string id, mirakana::ui::ElementId parent,
                                                      mirakana::ui::SemanticRole role) {
    mirakana::ui::ElementDesc desc;
    desc.id = mirakana::ui::ElementId{std::move(id)};
    desc.parent = std::move(parent);
    desc.role = role;
    desc.bounds = mirakana::ui::Rect{.x = 0.0F, .y = 0.0F, .width = 1.0F, .height = 1.0F};
    return desc;
}

void append_ui_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                     std::string label) {
    mirakana::ui::ElementDesc desc = make_ui_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_ui_text(std::move(label));
    add_ui_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string_view project_file_dialog_mode_id(EditorProjectFileDialogMode mode) noexcept {
    switch (mode) {
    case EditorProjectFileDialogMode::open:
        return "open";
    case EditorProjectFileDialogMode::save:
        return "save";
    }
    return "open";
}

[[nodiscard]] std::string project_file_dialog_action(EditorProjectFileDialogMode mode) {
    return mode == EditorProjectFileDialogMode::open ? "open" : "save";
}

[[nodiscard]] std::string project_file_dialog_status(EditorProjectFileDialogMode mode, std::string_view state) {
    return "Project " + project_file_dialog_action(mode) + " dialog " + std::string(state);
}

[[nodiscard]] std::string selected_filter_value(int selected_filter) {
    return selected_filter < 0 ? "-" : std::to_string(selected_filter);
}

void append_project_file_dialog_rows(EditorProjectFileDialogModel& model, std::size_t selected_count,
                                     int selected_filter) {
    model.rows = {
        EditorProjectFileDialogRow{.id = "status", .label = "Status", .value = model.status_label},
        EditorProjectFileDialogRow{
            .id = "selected_path", .label = "Selected path", .value = sanitize_text(model.selected_path)},
        EditorProjectFileDialogRow{
            .id = "selected_count", .label = "Selected count", .value = std::to_string(selected_count)},
        EditorProjectFileDialogRow{
            .id = "selected_filter", .label = "Selected filter", .value = selected_filter_value(selected_filter)},
    };
}

[[nodiscard]] mirakana::FileDialogRequest make_project_dialog_request(mirakana::FileDialogKind kind, std::string title,
                                                                      std::string accept_label,
                                                                      std::string_view default_location) {
    mirakana::FileDialogRequest request;
    request.kind = kind;
    request.title = std::move(title);
    request.filters = {mirakana::FileDialogFilter{.name = "Project", .pattern = "geproject"}};
    request.default_location = std::string(default_location);
    request.allow_many = false;
    request.accept_label = std::move(accept_label);
    request.cancel_label = "Cancel";
    return request;
}

[[nodiscard]] EditorProjectFileDialogModel make_project_file_dialog_model(const mirakana::FileDialogResult& result,
                                                                          EditorProjectFileDialogMode mode) {
    EditorProjectFileDialogModel model;
    model.mode = mode;
    model.status_label = project_file_dialog_status(mode, "idle");

    if (auto diagnostic = mirakana::validate_file_dialog_result(result); !diagnostic.empty()) {
        model.status_label = project_file_dialog_status(mode, "blocked");
        model.diagnostics.push_back(std::move(diagnostic));
        append_project_file_dialog_rows(model, result.paths.size(), result.selected_filter);
        return model;
    }

    switch (result.status) {
    case mirakana::FileDialogStatus::canceled:
        model.status_label = project_file_dialog_status(mode, "canceled");
        break;
    case mirakana::FileDialogStatus::failed:
        model.status_label = project_file_dialog_status(mode, "failed");
        model.diagnostics.push_back(result.error);
        break;
    case mirakana::FileDialogStatus::accepted:
        if (result.paths.size() != 1U) {
            model.status_label = project_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("project " + project_file_dialog_action(mode) +
                                        " dialog requires exactly one selected path");
            break;
        }

        model.selected_path = result.paths.front();
        if (!model.selected_path.ends_with(".geproject")) {
            model.status_label = project_file_dialog_status(mode, "blocked");
            model.diagnostics.push_back("project " + project_file_dialog_action(mode) +
                                        " dialog selection must end with .geproject");
            break;
        }

        model.accepted = true;
        model.status_label = project_file_dialog_status(mode, "accepted");
        break;
    }

    append_project_file_dialog_rows(model, result.paths.size(), result.selected_filter);
    return model;
}

[[nodiscard]] bool has_invalid_field_characters(std::string_view field) noexcept {
    return field.empty() || field.find_first_of("\r\n=") != std::string_view::npos;
}

[[nodiscard]] bool is_parent_relative_or_absolute_path(std::string_view path) noexcept {
    if (path.empty()) {
        return true;
    }
    if (path.front() == '/' || path.front() == '\\' || is_windows_drive_path(path)) {
        return true;
    }
    return has_parent_path_segment(path);
}

[[nodiscard]] bool is_valid_shader_tool_path_field(std::string_view value) noexcept {
    return !has_invalid_field_characters(value) && !is_parent_relative_or_absolute_path(value);
}

[[nodiscard]] bool is_valid_shader_tool_executable(std::string_view executable) {
    if (has_invalid_field_characters(executable) || has_parent_path_segment(executable)) {
        return false;
    }

    const auto leaf = executable_leaf(executable);
    return !leaf.empty() && leaf != "." && !is_shell_executable(executable);
}

void validate_shader_tool_path_field(std::string_view value, const char* name) {
    validate_field(value, name);
    validate_safe_relative_path(value, name);
}

void validate_shader_tool_executable_field(std::string_view value, const char* name) {
    validate_field(value, name);
    if (has_parent_path_segment(value)) {
        throw std::invalid_argument(std::string(name) + " must not contain parent directory traversal");
    }

    const auto leaf = executable_leaf(value);
    if (leaf.empty() || leaf == "." || is_shell_executable(value)) {
        throw std::invalid_argument(std::string(name) + " must name a reviewed non-shell tool");
    }
}

void add_shader_tool_error_if_invalid(std::vector<ProjectSettingsError>& errors, std::string field, std::string message,
                                      bool valid) {
    if (!valid) {
        errors.push_back(ProjectSettingsError{.field = std::move(field), .message = std::move(message)});
    }
}

std::vector<ProjectSettingsError> validate_shader_tool_settings_collect(const ProjectShaderToolSettings& settings) {
    std::vector<ProjectSettingsError> errors;
    add_shader_tool_error_if_invalid(errors, "shader_tool.executable",
                                     "Executable must be a reviewed non-shell tool name or path",
                                     is_valid_shader_tool_executable(settings.executable));
    add_shader_tool_error_if_invalid(errors, "shader_tool.working_directory",
                                     "Working directory must be project-relative",
                                     is_valid_shader_tool_path_field(settings.working_directory));
    add_shader_tool_error_if_invalid(errors, "shader_tool.artifact_output_root",
                                     "Artifact output root must be project-relative",
                                     is_valid_shader_tool_path_field(settings.artifact_output_root));
    add_shader_tool_error_if_invalid(errors, "shader_tool.cache_index", "Cache index must be project-relative",
                                     is_valid_shader_tool_path_field(settings.cache_index_path));
    return errors;
}

void validate_shader_tool_settings(const ProjectShaderToolSettings& settings) {
    validate_shader_tool_executable_field(settings.executable, "shader tool executable");
    validate_shader_tool_path_field(settings.working_directory, "shader tool working directory");
    validate_shader_tool_path_field(settings.artifact_output_root, "shader artifact output root");
    validate_shader_tool_path_field(settings.cache_index_path, "shader cache index path");
}

int parse_project_format_version(std::string_view value) {
    if (value == "GameEngine.Project.v0") {
        return 0;
    }
    if (value == "GameEngine.Project.v1") {
        return 1;
    }
    if (value == "GameEngine.Project.v2") {
        return 2;
    }
    if (value == "GameEngine.Project.v3") {
        return 3;
    }
    if (value == "GameEngine.Project.v4") {
        return 4;
    }
    throw std::invalid_argument("unsupported project format");
}

} // namespace

ProjectSettingsDraft ProjectSettingsDraft::from_project(const ProjectDocument& document) {
    return ProjectSettingsDraft(document);
}

const ProjectDocument& ProjectSettingsDraft::document() const noexcept {
    return document_;
}

const ProjectShaderToolSettings& ProjectSettingsDraft::shader_tool() const noexcept {
    return document_.shader_tool;
}

EditorRenderBackend ProjectSettingsDraft::render_backend() const noexcept {
    return document_.render_backend;
}

std::vector<ProjectSettingsError> ProjectSettingsDraft::validation_errors() const {
    return validate_shader_tool_settings_collect(document_.shader_tool);
}

bool ProjectSettingsDraft::can_apply() const {
    return validation_errors().empty();
}

ProjectDocument ProjectSettingsDraft::apply() const {
    validate_project_document(document_);
    return document_;
}

void ProjectSettingsDraft::set_shader_tool_executable(std::string value) {
    document_.shader_tool.executable = std::move(value);
}

void ProjectSettingsDraft::set_shader_tool_working_directory(std::string value) {
    document_.shader_tool.working_directory = std::move(value);
}

void ProjectSettingsDraft::set_shader_artifact_output_root(std::string value) {
    document_.shader_tool.artifact_output_root = std::move(value);
}

void ProjectSettingsDraft::set_shader_cache_index_path(std::string value) {
    document_.shader_tool.cache_index_path = std::move(value);
}

void ProjectSettingsDraft::set_shader_tool_descriptor(const ShaderToolDescriptor& descriptor) {
    if (descriptor.kind == ShaderToolKind::unknown || descriptor.executable_path.empty()) {
        throw std::invalid_argument("shader tool descriptor must name a known executable");
    }
    document_.shader_tool.executable = descriptor.executable_path;
}

void ProjectSettingsDraft::set_render_backend(EditorRenderBackend backend) noexcept {
    document_.render_backend = backend;
}

ProjectSettingsDraft::ProjectSettingsDraft(ProjectDocument document) : document_(std::move(document)) {}

void validate_project_shader_tool_settings(const ProjectShaderToolSettings& settings) {
    validate_shader_tool_settings(settings);
}

void validate_project_document(const ProjectDocument& document) {
    validate_field(document.name, "project name");
    validate_field(document.root_path, "project root path");
    validate_field(document.asset_root, "project asset root");
    validate_field(document.source_registry_path, "project source registry path");
    validate_field(document.game_manifest_path, "project game manifest path");
    validate_field(document.startup_scene_path, "project startup scene path");
    validate_safe_relative_path(document.root_path, "project root path");
    validate_safe_relative_path(document.asset_root, "project asset root");
    validate_project_source_registry_path(document.source_registry_path);
    validate_safe_relative_path(document.game_manifest_path, "project game manifest path");
    validate_safe_relative_path(document.startup_scene_path, "project startup scene path");
    validate_shader_tool_settings(document.shader_tool);
}

std::string serialize_project_document(const ProjectDocument& document) {
    validate_project_document(document);

    std::ostringstream output;
    output << "format=GameEngine.Project.v4\n";
    output << "project.name=" << document.name << '\n';
    output << "project.root=" << document.root_path << '\n';
    output << "project.asset_root=" << document.asset_root << '\n';
    output << "project.source_registry=" << document.source_registry_path << '\n';
    output << "project.game_manifest=" << document.game_manifest_path << '\n';
    output << "project.startup_scene=" << document.startup_scene_path << '\n';
    output << "project.render_backend=" << editor_render_backend_id(document.render_backend) << '\n';
    output << "project.shader_tool.executable=" << document.shader_tool.executable << '\n';
    output << "project.shader_tool.working_directory=" << document.shader_tool.working_directory << '\n';
    output << "project.shader_tool.artifact_output_root=" << document.shader_tool.artifact_output_root << '\n';
    output << "project.shader_tool.cache_index=" << document.shader_tool.cache_index_path << '\n';
    return output.str();
}

ProjectMigrationResult migrate_project_document(std::string_view text) {
    bool has_format = false;
    int source_version = 1;
    ProjectDocument document;
    std::istringstream input(std::string{text});
    std::string line;

    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }

        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            throw std::invalid_argument("project line must contain '='");
        }

        const auto key = std::string_view(line).substr(0, separator);
        const auto value = std::string_view(line).substr(separator + 1);

        if (key == "format") {
            source_version = parse_project_format_version(value);
            has_format = true;
        } else if (key == "project.name") {
            document.name = std::string(value);
        } else if (key == "project.root") {
            document.root_path = std::string(value);
        } else if (key == "project.asset_root") {
            document.asset_root = std::string(value);
        } else if (key == "project.source_registry") {
            document.source_registry_path = std::string(value);
        } else if (key == "project.game_manifest") {
            document.game_manifest_path = std::string(value);
        } else if (key == "project.startup_scene") {
            document.startup_scene_path = std::string(value);
        } else if (key == "project.render_backend") {
            const auto parsed_backend = parse_editor_render_backend(value);
            if (!parsed_backend.has_value()) {
                throw std::invalid_argument("unknown project render backend");
            }
            document.render_backend = *parsed_backend;
        } else if (key == "project.shader_tool.executable") {
            document.shader_tool.executable = std::string(value);
        } else if (key == "project.shader_tool.working_directory") {
            document.shader_tool.working_directory = std::string(value);
        } else if (key == "project.shader_tool.artifact_output_root") {
            document.shader_tool.artifact_output_root = std::string(value);
        } else if (key == "project.shader_tool.cache_index") {
            document.shader_tool.cache_index_path = std::string(value);
        } else {
            throw std::invalid_argument("unknown project key");
        }
    }

    if (!has_format) {
        throw std::invalid_argument("project format is missing");
    }

    if (source_version == 0) {
        if (document.asset_root.empty()) {
            document.asset_root = "assets";
        }
        if (document.game_manifest_path.empty()) {
            document.game_manifest_path = "game.agent.json";
        }
        if (document.startup_scene_path.empty()) {
            document.startup_scene_path = "scenes/start.scene";
        }
    }
    if (source_version <= 3 && document.source_registry_path.empty()) {
        document.source_registry_path = std::string(default_source_registry_path);
    }

    validate_project_document(document);
    return ProjectMigrationResult{
        .document = document,
        .source_version = source_version,
        .target_version = current_project_format_version,
        .migrated = source_version != current_project_format_version,
    };
}

ProjectDocument deserialize_project_document(std::string_view text) {
    return migrate_project_document(text).document;
}

mirakana::FileDialogRequest make_project_open_dialog_request(std::string_view default_location) {
    return make_project_dialog_request(mirakana::FileDialogKind::open_file, "Open Project", "Open", default_location);
}

mirakana::FileDialogRequest make_project_save_dialog_request(std::string_view default_location) {
    return make_project_dialog_request(mirakana::FileDialogKind::save_file, "Save Project", "Save", default_location);
}

EditorProjectFileDialogModel make_project_open_dialog_model(const mirakana::FileDialogResult& result) {
    return make_project_file_dialog_model(result, EditorProjectFileDialogMode::open);
}

EditorProjectFileDialogModel make_project_save_dialog_model(const mirakana::FileDialogResult& result) {
    return make_project_file_dialog_model(result, EditorProjectFileDialogMode::save);
}

mirakana::ui::UiDocument make_project_file_dialog_ui_model(const EditorProjectFileDialogModel& model) {
    mirakana::ui::UiDocument document;
    const auto mode_id = std::string(project_file_dialog_mode_id(model.mode));
    const std::string root_prefix = "project_file_dialog." + mode_id;
    add_ui_or_throw(document, make_ui_root(root_prefix, mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{root_prefix};

    for (const auto& row : model.rows) {
        const std::string row_prefix = root_prefix + "." + sanitize_element_id(row.id);
        add_ui_or_throw(document, make_ui_child(row_prefix, root, mirakana::ui::SemanticRole::list_item));
        const mirakana::ui::ElementId row_root{row_prefix};
        append_ui_label(document, row_root, row_prefix + ".label", row.label);
        append_ui_label(document, row_root, row_prefix + ".value", row.value);
    }

    const std::string diagnostics_prefix = root_prefix + ".diagnostics";
    add_ui_or_throw(document, make_ui_child(diagnostics_prefix, root, mirakana::ui::SemanticRole::list));
    const mirakana::ui::ElementId diagnostics_root{diagnostics_prefix};
    for (std::size_t index = 0; index < model.diagnostics.size(); ++index) {
        append_ui_label(document, diagnostics_root, diagnostics_prefix + "." + std::to_string(index),
                        model.diagnostics[index]);
    }

    return document;
}

} // namespace mirakana::editor
