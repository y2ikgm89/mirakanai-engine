// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/material_graph.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

/**
 * Editor-side wrapper for `GameEngine.MaterialGraph.v1` documents. Mirrors `MaterialAuthoringDocument` patterns:
 * dirty/staged tracking, `ITextStore` save/load, and registry-aware validation for texture nodes.
 */
class MaterialGraphAuthoringDocument {
  public:
    [[nodiscard]] static MaterialGraphAuthoringDocument from_graph(MaterialGraphDesc graph,
                                                                   std::string artifact_path = {});

    [[nodiscard]] const MaterialGraphDesc& graph() const noexcept;
    [[nodiscard]] std::string_view artifact_path() const noexcept;
    [[nodiscard]] bool dirty() const;
    [[nodiscard]] bool staged() const noexcept;

    [[nodiscard]] bool replace_graph(MaterialGraphDesc graph);
    [[nodiscard]] bool stage_changes();
    [[nodiscard]] bool stage_changes(const AssetRegistry& registry);
    void mark_saved();

  private:
    MaterialGraphDesc graph_;
    MaterialGraphDesc saved_graph_;
    std::string artifact_path_;
    DocumentDirtyState dirty_state_;
    bool staged_{false};
};

[[nodiscard]] std::vector<MaterialGraphDiagnostic>
validate_material_graph_authoring_document(const MaterialGraphAuthoringDocument& document);
[[nodiscard]] std::vector<MaterialGraphDiagnostic>
validate_material_graph_authoring_document(const MaterialGraphAuthoringDocument& document,
                                           const AssetRegistry& registry);

[[nodiscard]] std::vector<AssetId> material_graph_texture_dependencies(const MaterialGraphAuthoringDocument& document);

// Returned actions capture document by reference; keep their UndoStack scoped to the document lifetime.
[[nodiscard]] UndoableAction make_material_graph_authoring_replace_action(MaterialGraphAuthoringDocument& document,
                                                                          MaterialGraphDesc graph);

void save_material_graph_authoring_document(ITextStore& store, std::string_view path,
                                            MaterialGraphAuthoringDocument& document);
void save_material_graph_authoring_document(ITextStore& store, std::string_view path,
                                            MaterialGraphAuthoringDocument& document, const AssetRegistry& registry);

[[nodiscard]] MaterialGraphAuthoringDocument load_material_graph_authoring_document(ITextStore& store,
                                                                                    std::string_view path);

} // namespace mirakana::editor
