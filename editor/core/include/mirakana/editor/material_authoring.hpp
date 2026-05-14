// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/material.hpp"
#include "mirakana/editor/history.hpp"
#include "mirakana/editor/io.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::editor {

enum class MaterialAuthoringFactor : std::uint8_t { unknown, base_color, emissive, metallic, roughness };

enum class MaterialAuthoringDiagnosticCode : std::uint8_t {
    unknown,
    invalid_factor,
    invalid_texture_binding,
    duplicate_texture_slot,
    missing_binding_metadata,
};

enum class MaterialAuthoringTextureStatus : std::uint8_t { unknown, empty, resolved, missing, wrong_kind };

struct MaterialAuthoringDiagnostic {
    MaterialAuthoringDiagnosticCode code{MaterialAuthoringDiagnosticCode::unknown};
    std::string field;
    std::string message;
};

struct MaterialAuthoringFactorRow {
    MaterialAuthoringFactor factor{MaterialAuthoringFactor::unknown};
    std::string id;
    std::string label;
    std::array<float, 4> values{0.0F, 0.0F, 0.0F, 0.0F};
    std::size_t component_count{0};
    float minimum{0.0F};
    float maximum{1.0F};
};

struct MaterialAuthoringTextureRow {
    MaterialTextureSlot slot{MaterialTextureSlot::unknown};
    std::string id;
    std::string label;
    AssetId texture;
    MaterialAuthoringTextureStatus status{MaterialAuthoringTextureStatus::unknown};
    std::string diagnostic;
    std::vector<MaterialPipelineBinding> bindings;
};

struct MaterialAuthoringModel {
    AssetId material;
    std::string name;
    std::string artifact_path;
    std::vector<MaterialAuthoringFactorRow> factor_rows;
    std::vector<MaterialAuthoringTextureRow> texture_rows;
    std::vector<MaterialPipelineBinding> bindings;
    std::vector<MaterialAuthoringDiagnostic> diagnostics;
    bool dirty{false};
    bool staged{false};

    [[nodiscard]] bool valid() const noexcept {
        return diagnostics.empty();
    }
};

class MaterialAuthoringDocument {
  public:
    [[nodiscard]] static MaterialAuthoringDocument from_material(MaterialDefinition material,
                                                                 std::string artifact_path = {});

    [[nodiscard]] const MaterialDefinition& material() const noexcept;
    [[nodiscard]] std::string_view artifact_path() const noexcept;
    [[nodiscard]] bool dirty() const;
    [[nodiscard]] bool staged() const noexcept;
    [[nodiscard]] MaterialAuthoringModel model() const;
    [[nodiscard]] MaterialAuthoringModel model(const AssetRegistry& registry) const;

    [[nodiscard]] bool set_base_color(std::array<float, 4> value);
    [[nodiscard]] bool set_emissive(std::array<float, 3> value);
    [[nodiscard]] bool set_metallic(float value);
    [[nodiscard]] bool set_roughness(float value);
    [[nodiscard]] bool set_factor(MaterialAuthoringFactor factor, std::array<float, 4> values);
    [[nodiscard]] bool set_texture(MaterialTextureSlot slot, AssetId texture);
    [[nodiscard]] bool clear_texture(MaterialTextureSlot slot);
    [[nodiscard]] bool replace_material(MaterialDefinition material);
    [[nodiscard]] bool stage_changes();
    [[nodiscard]] bool stage_changes(const AssetRegistry& registry);
    void mark_saved();

  private:
    MaterialDefinition material_;
    MaterialDefinition saved_material_;
    std::string artifact_path_;
    DocumentDirtyState dirty_state_;
    bool staged_{false};
};

[[nodiscard]] std::string_view material_authoring_factor_label(MaterialAuthoringFactor factor) noexcept;
[[nodiscard]] std::vector<MaterialAuthoringDiagnostic>
validate_material_authoring_document(const MaterialAuthoringDocument& document);
[[nodiscard]] std::vector<MaterialAuthoringDiagnostic>
validate_material_authoring_document(const MaterialAuthoringDocument& document, const AssetRegistry& registry);
[[nodiscard]] std::vector<AssetId> material_authoring_texture_dependencies(const MaterialAuthoringDocument& document);

// Returned actions capture document by reference; keep their UndoStack scoped to the document lifetime.
[[nodiscard]] UndoableAction make_material_authoring_factor_edit_action(MaterialAuthoringDocument& document,
                                                                        MaterialAuthoringFactor factor,
                                                                        std::array<float, 4> values);
[[nodiscard]] UndoableAction make_material_authoring_texture_edit_action(MaterialAuthoringDocument& document,
                                                                         MaterialTextureSlot slot, AssetId texture);
void save_material_authoring_document(ITextStore& store, std::string_view path, MaterialAuthoringDocument& document);
void save_material_authoring_document(ITextStore& store, std::string_view path, MaterialAuthoringDocument& document,
                                      const AssetRegistry& registry);
[[nodiscard]] MaterialAuthoringDocument load_material_authoring_document(ITextStore& store, std::string_view path);

} // namespace mirakana::editor
