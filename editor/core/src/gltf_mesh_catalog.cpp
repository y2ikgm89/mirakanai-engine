// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/gltf_mesh_catalog.hpp"

#include <cctype>
#include <sstream>
#include <string>
#include <string_view>

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

} // namespace

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
