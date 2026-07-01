// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/gltf_mesh_catalog.hpp"

#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool ascii_path_ends_with_insensitive(std::string_view path, std::string_view suffix) noexcept {
    if (suffix.size() > path.size()) {
        return false;
    }
    const auto start = path.size() - suffix.size();
    for (std::size_t i = 0; i < suffix.size(); ++i) {
        const auto a = static_cast<unsigned char>(path[start + i]);
        const auto b = static_cast<unsigned char>(suffix[i]);
        if (std::tolower(a) != std::tolower(b)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::string bool_label(bool value) noexcept {
    return value ? "true" : "false";
}

[[nodiscard]] bool contains_line_separator(std::string_view value) noexcept {
    return value.find('\n') != std::string_view::npos || value.find('\r') != std::string_view::npos;
}

void require_safe_field(std::string_view field, std::string_view value) {
    if (value.empty() || contains_line_separator(value) || value.find('=') != std::string_view::npos) {
        throw std::invalid_argument(std::string("editor gltf mesh inspect ui field is invalid: ") + std::string(field));
    }
}

[[nodiscard]] std::string sanitize_visible_text(std::string_view value) {
    std::string text;
    text.reserve(value.size());
    for (const auto character : value) {
        if (character == '\n' || character == '\r' || character == '=') {
            text.push_back(' ');
        } else {
            text.push_back(character);
        }
    }
    return text.empty() ? "-" : text;
}

[[nodiscard]] mirakana::ui::TextContent make_text(std::string label) {
    mirakana::ui::TextContent text;
    text.label = std::move(label);
    text.font_family = "ui/body";
    text.wrap = mirakana::ui::TextWrapMode::ellipsis;
    return text;
}

void add_or_throw(mirakana::ui::UiDocument& document, mirakana::ui::ElementDesc desc) {
    if (!document.try_add_element(std::move(desc))) {
        throw std::invalid_argument("editor gltf mesh inspect ui element could not be added");
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

void append_label(mirakana::ui::UiDocument& document, const mirakana::ui::ElementId& parent, std::string id,
                  const std::string& label) {
    mirakana::ui::ElementDesc desc = make_child(std::move(id), parent, mirakana::ui::SemanticRole::label);
    desc.text = make_text(label);
    add_or_throw(document, std::move(desc));
}

[[nodiscard]] std::string fallback_asset_label(AssetId asset) {
    return "asset:" + std::to_string(asset.value);
}

[[nodiscard]] std::string gltf_selection_row_id(AssetId asset) {
    return "asset_browser.import_workflow.gltf_mesh_inspect." + std::to_string(asset.value);
}

[[nodiscard]] bool has_unsafe_core_claims(const EditorGltfMeshInspectSelectionDesc& desc) noexcept {
    return desc.mutates_project_files || desc.executes_import_tools || desc.executes_package_scripts ||
           desc.executes_validation_recipes || desc.exposes_native_handles;
}

void append_non_empty(std::vector<std::string>& diagnostics, std::string diagnostic) {
    if (!diagnostic.empty()) {
        diagnostics.push_back(std::move(diagnostic));
    }
}

[[nodiscard]] std::string first_or_default(const std::vector<std::string>& values, std::string_view fallback) {
    if (values.empty() || values.front().empty()) {
        return std::string{fallback};
    }
    return values.front();
}

[[nodiscard]] std::string gltf_selection_detail_label(const EditorGltfMeshInspectSelectionModel& model) {
    if (model.ready) {
        return "primitives=" + std::to_string(model.primitive_count) +
               " warnings=" + std::to_string(model.warning_count);
    }
    if (model.status == EditorGltfMeshInspectSelectionStatus::unsupported) {
        return "supported_suffixes=.gltf,.glb";
    }
    return "primitives=0 warnings=" + std::to_string(model.warning_count);
}

void append_gltf_selection_command(EditorGltfMeshInspectSelectionModel& model) {
    model.command_rows.push_back(EditorAssetBrowserRetainedCommandRow{
        .command_id = "asset_browser.selection.inspect",
        .label = "Inspect glTF mesh",
        .status_label = model.ready ? "ready" : "blocked",
        .enabled = model.ready,
        .requires_user_confirmation = false,
        .mutates_project_files = model.mutates_project_files,
        .executes_import_tools = model.executes_import_tools,
        .executes_package_scripts = model.executes_package_scripts,
        .executes_validation_recipes = model.executes_validation_recipes,
        .exposes_native_handles = model.exposes_native_handles,
    });
}

void append_gltf_selection_workflow_row(EditorGltfMeshInspectSelectionModel& model) {
    if (!model.selected) {
        return;
    }
    model.import_workflow_rows.push_back(EditorAssetBrowserImportWorkflowRow{
        .id = model.row_id,
        .category_label = "glTF mesh inspect",
        .asset_id = std::to_string(model.asset.value),
        .asset_key_label = model.asset_key_label,
        .source_path = model.source_path,
        .status_label =
            model.ready ? "ready" : std::string{editor_gltf_mesh_inspect_selection_status_label(model.status)},
        .detail_label = gltf_selection_detail_label(model),
        .diagnostic = first_or_default(model.diagnostics, model.ready ? "value-only glTF mesh inspect ready"
                                                                      : "glTF mesh inspect review is blocked"),
        .ready = model.ready,
        .blocked = !model.ready,
        .selected = model.selected,
        .host_owned = true,
        .mutates_project_files = model.mutates_project_files,
        .executes_import_tools = model.executes_import_tools,
        .executes_package_scripts = model.executes_package_scripts,
        .executes_validation_recipes = model.executes_validation_recipes,
        .exposes_native_handles = model.exposes_native_handles,
    });
}

} // namespace

std::string_view editor_gltf_mesh_inspect_selection_status_label(EditorGltfMeshInspectSelectionStatus status) noexcept {
    switch (status) {
    case EditorGltfMeshInspectSelectionStatus::empty:
        return "glTF mesh inspect empty";
    case EditorGltfMeshInspectSelectionStatus::unsupported:
        return "glTF mesh inspect unsupported";
    case EditorGltfMeshInspectSelectionStatus::ready:
        return "glTF mesh inspect ready";
    case EditorGltfMeshInspectSelectionStatus::blocked:
        return "glTF mesh inspect blocked";
    }
    return "glTF mesh inspect blocked";
}

EditorGltfMeshInspectSelectionModel
make_editor_gltf_mesh_inspect_selection_model(const EditorGltfMeshInspectSelectionDesc& desc) {
    EditorGltfMeshInspectSelectionModel model;
    model.asset = desc.asset;
    model.asset_key_label =
        sanitize_visible_text(desc.asset_key_label.empty() ? fallback_asset_label(desc.asset) : desc.asset_key_label);
    model.source_path = sanitize_visible_text(desc.source_path);
    model.row_id = gltf_selection_row_id(desc.asset);
    model.selected = desc.selected;
    model.source_visible = desc.source_visible;
    model.inspect_supported = desc.selected && editor_asset_path_supports_gltf_mesh_inspect(desc.source_path);
    model.mutates_project_files = desc.mutates_project_files;
    model.executes_import_tools = desc.executes_import_tools;
    model.executes_package_scripts = desc.executes_package_scripts;
    model.executes_validation_recipes = desc.executes_validation_recipes;
    model.exposes_native_handles = desc.exposes_native_handles;

    if (desc.report != nullptr) {
        model.warning_count = static_cast<std::uint64_t>(desc.report->warnings.size());
        model.primitive_count = static_cast<std::uint64_t>(desc.report->rows.size());
        model.inspector_rows = gltf_mesh_inspect_report_to_inspector_rows(*desc.report);
    }

    if (!desc.selected) {
        model.status = EditorGltfMeshInspectSelectionStatus::empty;
        append_non_empty(model.diagnostics, "no Assets selection is active");
    } else if (!model.inspect_supported) {
        model.status = EditorGltfMeshInspectSelectionStatus::unsupported;
        model.blocked = true;
        append_non_empty(model.diagnostics, "selected asset is not a glTF 2.0 source path ending in .gltf or .glb");
    } else if (!desc.source_visible) {
        model.status = EditorGltfMeshInspectSelectionStatus::blocked;
        model.blocked = true;
        append_non_empty(model.diagnostics, "selected glTF asset is not visible in the retained Assets source rows");
    } else if (has_unsafe_core_claims(desc)) {
        model.status = EditorGltfMeshInspectSelectionStatus::blocked;
        model.blocked = true;
        append_non_empty(
            model.diagnostics,
            "glTF mesh inspect selection is blocked because editor-core input claimed mutation, execution, "
            "package/validation script use, or native handle exposure");
    } else if (desc.report == nullptr) {
        model.status = EditorGltfMeshInspectSelectionStatus::blocked;
        model.blocked = true;
        append_non_empty(model.diagnostics, "glTF mesh inspect report is required before retained review can be ready");
    } else if (!desc.report->parse_succeeded) {
        model.status = EditorGltfMeshInspectSelectionStatus::blocked;
        model.blocked = true;
        append_non_empty(model.diagnostics, desc.report->diagnostic.empty()
                                                ? "glTF mesh inspect report failed without diagnostic"
                                                : sanitize_visible_text(desc.report->diagnostic));
    } else {
        model.status = EditorGltfMeshInspectSelectionStatus::ready;
        model.ready = true;
        if (!desc.report->warnings.empty()) {
            append_non_empty(model.diagnostics, sanitize_visible_text(desc.report->warnings.front()));
        }
    }

    model.status_label = std::string{editor_gltf_mesh_inspect_selection_status_label(model.status)};
    append_gltf_selection_command(model);
    append_gltf_selection_workflow_row(model);
    return model;
}

EditorAssetBrowserRetainedUiDesc
make_editor_gltf_mesh_inspect_selection_retained_ui_desc(const EditorGltfMeshInspectSelectionModel& model) {
    return EditorAssetBrowserRetainedUiDesc{
        .query_status_label = model.status_label,
        .command_rows = model.command_rows,
        .import_workflow_rows = model.import_workflow_rows,
    };
}

std::vector<EditorPropertyRow>
gltf_mesh_inspect_report_to_inspector_rows(const mirakana::GltfMeshInspectReport& report) {
    std::vector<EditorPropertyRow> rows;
    EditorPropertyRow status_row;
    status_row.id = "gltf.inspect.status";
    status_row.label = "glTF inspect";
    status_row.editable = false;
    if (!report.parse_succeeded) {
        status_row.value = std::string("failed: ") + report.diagnostic;
        rows.push_back(std::move(status_row));
        return rows;
    }
    status_row.value = "ok";
    rows.push_back(std::move(status_row));

    for (std::size_t i = 0; i < report.warnings.size(); ++i) {
        EditorPropertyRow warn_row;
        warn_row.id = "gltf.inspect.warning." + std::to_string(i);
        warn_row.label = "warning";
        warn_row.value = report.warnings[i];
        warn_row.editable = false;
        rows.push_back(std::move(warn_row));
    }

    for (const auto& primitive : report.rows) {
        const std::string base_id =
            "gltf.primitive." + std::to_string(primitive.mesh_index) + "." + std::to_string(primitive.primitive_index);

        EditorPropertyRow mesh_row;
        mesh_row.id = base_id + ".mesh";
        mesh_row.label = "mesh";
        mesh_row.value = primitive.mesh_name;
        mesh_row.editable = false;
        rows.push_back(std::move(mesh_row));

        std::ostringstream summary;
        summary << "positions=" << primitive.position_vertex_count << " indexed=" << bool_label(primitive.indexed)
                << " normal=" << bool_label(primitive.has_normal) << " uv0=" << bool_label(primitive.has_texcoord0);

        EditorPropertyRow layout_row;
        layout_row.id = base_id + ".layout";
        layout_row.label = "primitive";
        layout_row.value = summary.str();
        layout_row.editable = false;
        rows.push_back(std::move(layout_row));
    }

    if (report.rows.empty() && report.warnings.empty()) {
        EditorPropertyRow empty_row;
        empty_row.id = "gltf.inspect.empty";
        empty_row.label = "meshes";
        empty_row.value = "(no triangle primitives)";
        empty_row.editable = false;
        rows.push_back(std::move(empty_row));
    }

    return rows;
}

mirakana::ui::UiDocument make_gltf_mesh_inspect_ui_model(const mirakana::GltfMeshInspectReport& report) {
    constexpr std::string_view k_contract = "ge.editor.gltf_mesh_inspect.v1";
    require_safe_field("gltf_mesh_inspect.contract", k_contract);

    mirakana::ui::UiDocument document;
    add_or_throw(document, make_root("gltf_mesh_inspect", mirakana::ui::SemanticRole::panel));
    const mirakana::ui::ElementId root{"gltf_mesh_inspect"};
    append_label(document, root, "gltf_mesh_inspect.contract", std::string{k_contract});

    const auto rows = gltf_mesh_inspect_report_to_inspector_rows(report);
    for (const auto& row : rows) {
        require_safe_field("gltf_mesh_inspect.row.id", row.id);
        require_safe_field("gltf_mesh_inspect.row.label", row.label);
        const std::string base = std::string{"gltf_mesh_inspect.rows."} + row.id;
        append_label(document, root, base + ".caption", sanitize_visible_text(row.label));
        append_label(document, root, base + ".text", sanitize_visible_text(row.value));
    }

    return document;
}

bool editor_asset_path_supports_gltf_mesh_inspect(std::string_view path) noexcept {
    return ascii_path_ends_with_insensitive(path, ".gltf") || ascii_path_ends_with_insensitive(path, ".glb");
}

} // namespace mirakana::editor
