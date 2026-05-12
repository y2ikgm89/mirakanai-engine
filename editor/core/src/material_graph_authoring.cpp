// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/material_graph_authoring.hpp"

#include <ranges>
#include <stdexcept>
#include <utility>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool texture_asset_ok(const AssetRegistry& registry, AssetId texture, std::string& diagnostic) {
    if (texture.value == 0) {
        return true;
    }
    const auto* record = registry.find(texture);
    if (record == nullptr) {
        diagnostic = "texture asset is not registered";
        return false;
    }
    if (record->kind != AssetKind::texture) {
        diagnostic = "registered asset is not a texture";
        return false;
    }
    return true;
}

} // namespace

MaterialGraphAuthoringDocument MaterialGraphAuthoringDocument::from_graph(MaterialGraphDesc graph,
                                                                          std::string artifact_path) {
    const auto diagnostics = validate_material_graph(graph);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material graph authoring document requires a valid material graph description");
    }
    MaterialGraphAuthoringDocument document;
    document.graph_ = std::move(graph);
    document.saved_graph_ = document.graph_;
    document.artifact_path_ = std::move(artifact_path);
    document.dirty_state_.reset_clean();
    return document;
}

const MaterialGraphDesc& MaterialGraphAuthoringDocument::graph() const noexcept {
    return graph_;
}

std::string_view MaterialGraphAuthoringDocument::artifact_path() const noexcept {
    return artifact_path_;
}

bool MaterialGraphAuthoringDocument::dirty() const {
    return !(graph_ == saved_graph_);
}

bool MaterialGraphAuthoringDocument::staged() const noexcept {
    return staged_;
}

bool MaterialGraphAuthoringDocument::replace_graph(MaterialGraphDesc graph) {
    if (!validate_material_graph(graph).empty()) {
        return false;
    }
    graph_ = std::move(graph);
    dirty_state_.mark_dirty();
    staged_ = false;
    return true;
}

bool MaterialGraphAuthoringDocument::stage_changes() {
    if (!dirty() || !validate_material_graph_authoring_document(*this).empty()) {
        staged_ = false;
        return false;
    }
    staged_ = true;
    return true;
}

bool MaterialGraphAuthoringDocument::stage_changes(const AssetRegistry& registry) {
    if (!dirty() || !validate_material_graph_authoring_document(*this, registry).empty()) {
        staged_ = false;
        return false;
    }
    staged_ = true;
    return true;
}

void MaterialGraphAuthoringDocument::mark_saved() {
    saved_graph_ = graph_;
    dirty_state_.mark_saved();
    staged_ = false;
}

std::vector<MaterialGraphDiagnostic>
validate_material_graph_authoring_document(const MaterialGraphAuthoringDocument& document) {
    return validate_material_graph(document.graph());
}

std::vector<MaterialGraphDiagnostic>
validate_material_graph_authoring_document(const MaterialGraphAuthoringDocument& document,
                                           const AssetRegistry& registry) {
    auto diagnostics = validate_material_graph(document.graph());
    for (const auto& node : document.graph().nodes) {
        if (node.kind != MaterialGraphNodeKind::texture || node.texture_id.value == 0) {
            continue;
        }
        std::string diagnostic;
        if (!texture_asset_ok(registry, node.texture_id, diagnostic)) {
            diagnostics.push_back({.code = MaterialGraphDiagnosticCode::invalid_texture_binding,
                                   .field = node.id,
                                   .message = diagnostic});
        }
    }
    return diagnostics;
}

std::vector<AssetId> material_graph_texture_dependencies(const MaterialGraphAuthoringDocument& document) {
    std::vector<AssetId> dependencies;
    for (const auto& node : document.graph().nodes) {
        if (node.kind != MaterialGraphNodeKind::texture || node.texture_id.value == 0) {
            continue;
        }
        if (!std::ranges::contains(dependencies, node.texture_id)) {
            dependencies.push_back(node.texture_id);
        }
    }
    return dependencies;
}

UndoableAction make_material_graph_authoring_replace_action(MaterialGraphAuthoringDocument& document,
                                                            MaterialGraphDesc graph) {
    if (!validate_material_graph(graph).empty()) {
        return {};
    }
    auto before = document.graph();
    auto after = std::move(graph);
    return UndoableAction{
        .label = "Replace Material Graph",
        .redo = [&document, after]() { (void)document.replace_graph(after); },
        .undo = [&document, before]() { (void)document.replace_graph(before); },
    };
}

void save_material_graph_authoring_document(ITextStore& store, std::string_view path,
                                            MaterialGraphAuthoringDocument& document) {
    if (!validate_material_graph_authoring_document(document).empty()) {
        throw std::invalid_argument("material graph authoring document has validation diagnostics");
    }
    const auto text = serialize_material_graph(document.graph());
    store.write_text(path, text);
    document = MaterialGraphAuthoringDocument::from_graph(deserialize_material_graph(text), std::string(path));
}

void save_material_graph_authoring_document(ITextStore& store, std::string_view path,
                                            MaterialGraphAuthoringDocument& document, const AssetRegistry& registry) {
    if (!validate_material_graph_authoring_document(document, registry).empty()) {
        throw std::invalid_argument("material graph authoring document has validation diagnostics");
    }
    const auto text = serialize_material_graph(document.graph());
    store.write_text(path, text);
    document = MaterialGraphAuthoringDocument::from_graph(deserialize_material_graph(text), std::string(path));
}

MaterialGraphAuthoringDocument load_material_graph_authoring_document(ITextStore& store, std::string_view path) {
    return MaterialGraphAuthoringDocument::from_graph(deserialize_material_graph(store.read_text(path)),
                                                      std::string(path));
}

} // namespace mirakana::editor
