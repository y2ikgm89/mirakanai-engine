// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/editor/material_authoring.hpp"

#include "mirakana/assets/material.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana::editor {
namespace {

[[nodiscard]] bool valid_texture_slot(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
    case MaterialTextureSlot::normal:
    case MaterialTextureSlot::metallic_roughness:
    case MaterialTextureSlot::emissive:
    case MaterialTextureSlot::occlusion:
        return true;
    case MaterialTextureSlot::unknown:
        break;
    }
    return false;
}

[[nodiscard]] std::string_view texture_slot_id(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return "base_color";
    case MaterialTextureSlot::normal:
        return "normal";
    case MaterialTextureSlot::metallic_roughness:
        return "metallic_roughness";
    case MaterialTextureSlot::emissive:
        return "emissive";
    case MaterialTextureSlot::occlusion:
        return "occlusion";
    case MaterialTextureSlot::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] std::string_view texture_slot_label(MaterialTextureSlot slot) noexcept {
    switch (slot) {
    case MaterialTextureSlot::base_color:
        return "Base Color";
    case MaterialTextureSlot::normal:
        return "Normal";
    case MaterialTextureSlot::metallic_roughness:
        return "Metallic Roughness";
    case MaterialTextureSlot::emissive:
        return "Emissive";
    case MaterialTextureSlot::occlusion:
        return "Occlusion";
    case MaterialTextureSlot::unknown:
        break;
    }
    return "Unknown";
}

[[nodiscard]] std::array<MaterialTextureSlot, 5> authored_texture_slots() noexcept {
    return {
        MaterialTextureSlot::base_color, MaterialTextureSlot::normal,    MaterialTextureSlot::metallic_roughness,
        MaterialTextureSlot::emissive,   MaterialTextureSlot::occlusion,
    };
}

[[nodiscard]] std::string_view factor_id(MaterialAuthoringFactor factor) noexcept {
    switch (factor) {
    case MaterialAuthoringFactor::base_color:
        return "base_color";
    case MaterialAuthoringFactor::emissive:
        return "emissive";
    case MaterialAuthoringFactor::metallic:
        return "metallic";
    case MaterialAuthoringFactor::roughness:
        return "roughness";
    case MaterialAuthoringFactor::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] bool in_range(float value, float minimum, float maximum) noexcept {
    return std::isfinite(value) && value >= minimum && value <= maximum;
}

void add_diagnostic(std::vector<MaterialAuthoringDiagnostic>& diagnostics, MaterialAuthoringDiagnosticCode code,
                    std::string field, std::string message) {
    diagnostics.push_back(
        MaterialAuthoringDiagnostic{.code = code, .field = std::move(field), .message = std::move(message)});
}

void validate_factor(std::vector<MaterialAuthoringDiagnostic>& diagnostics, std::string field,
                     std::span<const float> values, float minimum, float maximum) {
    for (const auto value : values) {
        if (!in_range(value, minimum, maximum)) {
            add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::invalid_factor, std::move(field),
                           "material factor is outside the supported authoring range");
            return;
        }
    }
}

[[nodiscard]] std::vector<MaterialTextureBinding>
sorted_texture_bindings(std::vector<MaterialTextureBinding> bindings) {
    std::ranges::sort(bindings, [](const MaterialTextureBinding& lhs, const MaterialTextureBinding& rhs) {
        if (lhs.slot != rhs.slot) {
            return static_cast<int>(lhs.slot) < static_cast<int>(rhs.slot);
        }
        return lhs.texture.value < rhs.texture.value;
    });
    return bindings;
}

[[nodiscard]] bool same_texture_bindings(std::vector<MaterialTextureBinding> lhs,
                                         std::vector<MaterialTextureBinding> rhs) {
    lhs = sorted_texture_bindings(std::move(lhs));
    rhs = sorted_texture_bindings(std::move(rhs));
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (lhs[index].slot != rhs[index].slot || !(lhs[index].texture == rhs[index].texture)) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] bool same_material(const MaterialDefinition& lhs, const MaterialDefinition& rhs) {
    return lhs.id == rhs.id && lhs.name == rhs.name && lhs.shading_model == rhs.shading_model &&
           lhs.surface_mode == rhs.surface_mode && lhs.factors.base_color == rhs.factors.base_color &&
           lhs.factors.emissive == rhs.factors.emissive && lhs.factors.metallic == rhs.factors.metallic &&
           lhs.factors.roughness == rhs.factors.roughness && lhs.double_sided == rhs.double_sided &&
           same_texture_bindings(lhs.texture_bindings, rhs.texture_bindings);
}

void sort_document_textures(MaterialDefinition& material) {
    material.texture_bindings = sorted_texture_bindings(std::move(material.texture_bindings));
}

[[nodiscard]] MaterialTextureBinding* find_texture_binding(MaterialDefinition& material, MaterialTextureSlot slot) {
    const auto it = std::ranges::find_if(
        material.texture_bindings, [slot](const MaterialTextureBinding& binding) { return binding.slot == slot; });
    return it == material.texture_bindings.end() ? nullptr : &*it;
}

[[nodiscard]] std::vector<MaterialPipelineBinding>
bindings_for_slot(const std::vector<MaterialPipelineBinding>& bindings, MaterialTextureSlot slot) {
    std::vector<MaterialPipelineBinding> result;
    for (const auto& binding : bindings) {
        if (binding.texture_slot == slot && (binding.resource_kind == MaterialBindingResourceKind::sampled_texture ||
                                             binding.resource_kind == MaterialBindingResourceKind::sampler)) {
            result.push_back(binding);
        }
    }
    std::ranges::sort(result, [](const MaterialPipelineBinding& lhs, const MaterialPipelineBinding& rhs) {
        return lhs.binding < rhs.binding;
    });
    return result;
}

[[nodiscard]] bool has_binding_kind(const std::vector<MaterialPipelineBinding>& bindings,
                                    MaterialBindingResourceKind kind) noexcept {
    return std::ranges::any_of(
        bindings, [kind](const MaterialPipelineBinding& binding) { return binding.resource_kind == kind; });
}

[[nodiscard]] MaterialAuthoringFactorRow make_factor_row(MaterialAuthoringFactor factor, std::array<float, 4> values,
                                                         std::size_t component_count, float maximum) {
    return MaterialAuthoringFactorRow{
        .factor = factor,
        .id = std::string(factor_id(factor)),
        .label = std::string(material_authoring_factor_label(factor)),
        .values = values,
        .component_count = component_count,
        .minimum = 0.0F,
        .maximum = maximum,
    };
}

[[nodiscard]] std::vector<MaterialAuthoringFactorRow> make_factor_rows(const MaterialFactors& factors) {
    return {
        make_factor_row(MaterialAuthoringFactor::base_color, factors.base_color, 4, 1.0F),
        make_factor_row(MaterialAuthoringFactor::emissive,
                        {factors.emissive[0], factors.emissive[1], factors.emissive[2], 0.0F}, 3, 1024.0F),
        make_factor_row(MaterialAuthoringFactor::metallic, {factors.metallic, 0.0F, 0.0F, 0.0F}, 1, 1.0F),
        make_factor_row(MaterialAuthoringFactor::roughness, {factors.roughness, 0.0F, 0.0F, 0.0F}, 1, 1.0F),
    };
}

[[nodiscard]] bool apply_factor(MaterialDefinition& material, MaterialAuthoringFactor factor,
                                std::array<float, 4> values) {
    switch (factor) {
    case MaterialAuthoringFactor::base_color:
        material.factors.base_color = values;
        return true;
    case MaterialAuthoringFactor::emissive:
        material.factors.emissive = {values[0], values[1], values[2]};
        return true;
    case MaterialAuthoringFactor::metallic:
        material.factors.metallic = values[0];
        return true;
    case MaterialAuthoringFactor::roughness:
        material.factors.roughness = values[0];
        return true;
    case MaterialAuthoringFactor::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool valid_factor_values(MaterialAuthoringFactor factor, std::array<float, 4> values) noexcept {
    switch (factor) {
    case MaterialAuthoringFactor::base_color:
        return in_range(values[0], 0.0F, 1.0F) && in_range(values[1], 0.0F, 1.0F) && in_range(values[2], 0.0F, 1.0F) &&
               in_range(values[3], 0.0F, 1.0F);
    case MaterialAuthoringFactor::emissive:
        return in_range(values[0], 0.0F, 1024.0F) && in_range(values[1], 0.0F, 1024.0F) &&
               in_range(values[2], 0.0F, 1024.0F);
    case MaterialAuthoringFactor::metallic:
    case MaterialAuthoringFactor::roughness:
        return in_range(values[0], 0.0F, 1.0F);
    case MaterialAuthoringFactor::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool apply_texture(MaterialDefinition& material, MaterialTextureSlot slot, AssetId texture) {
    if (!valid_texture_slot(slot)) {
        return false;
    }
    if (texture.value == 0) {
        const auto removed_bindings = std::ranges::remove_if(
            material.texture_bindings, [slot](const MaterialTextureBinding& binding) { return binding.slot == slot; });
        material.texture_bindings.erase(removed_bindings.begin(), removed_bindings.end());
        return true;
    }
    if (auto* binding = find_texture_binding(material, slot); binding != nullptr) {
        binding->texture = texture;
    } else {
        material.texture_bindings.push_back(MaterialTextureBinding{.slot = slot, .texture = texture});
    }
    sort_document_textures(material);
    return true;
}

[[nodiscard]] MaterialAuthoringTextureStatus texture_status(const AssetRegistry* registry, AssetId texture,
                                                            std::string& diagnostic) {
    if (texture.value == 0) {
        return MaterialAuthoringTextureStatus::empty;
    }
    if (registry == nullptr) {
        return MaterialAuthoringTextureStatus::unknown;
    }
    const auto* record = registry->find(texture);
    if (record == nullptr) {
        diagnostic = "texture asset is not registered";
        return MaterialAuthoringTextureStatus::missing;
    }
    if (record->kind != AssetKind::texture) {
        diagnostic = "registered asset is not a texture";
        return MaterialAuthoringTextureStatus::wrong_kind;
    }
    return MaterialAuthoringTextureStatus::resolved;
}

[[nodiscard]] std::vector<MaterialAuthoringTextureRow>
make_texture_rows(const MaterialDefinition& material, const std::vector<MaterialPipelineBinding>& bindings,
                  const AssetRegistry* registry) {
    std::vector<MaterialAuthoringTextureRow> rows;
    rows.reserve(authored_texture_slots().size());
    for (const auto slot : authored_texture_slots()) {
        const auto binding =
            std::ranges::find_if(material.texture_bindings,
                                 [slot](const MaterialTextureBinding& candidate) { return candidate.slot == slot; });
        AssetId texture;
        if (binding != material.texture_bindings.end()) {
            texture = binding->texture;
        }

        std::string diagnostic;
        const auto status = texture_status(registry, texture, diagnostic);
        rows.push_back(MaterialAuthoringTextureRow{
            .slot = slot,
            .id = "texture." + std::string(texture_slot_id(slot)),
            .label = std::string(texture_slot_label(slot)),
            .texture = texture,
            .status = status,
            .diagnostic = std::move(diagnostic),
            .bindings = bindings_for_slot(bindings, slot),
        });
    }
    return rows;
}

[[nodiscard]] MaterialAuthoringModel make_material_authoring_model(const MaterialAuthoringDocument& document,
                                                                   const AssetRegistry* registry) {
    const auto& material = document.material();
    MaterialAuthoringModel model;
    model.material = material.id;
    model.name = material.name;
    model.artifact_path = std::string(document.artifact_path());
    model.factor_rows = make_factor_rows(material.factors);
    model.diagnostics = registry == nullptr ? validate_material_authoring_document(document)
                                            : validate_material_authoring_document(document, *registry);
    model.dirty = document.dirty();
    model.staged = document.staged();

    if (model.diagnostics.empty()) {
        const auto metadata = build_material_pipeline_binding_metadata(material);
        model.bindings = metadata.bindings;
    }
    model.texture_rows = make_texture_rows(material, model.bindings, registry);
    return model;
}

} // namespace

MaterialAuthoringDocument MaterialAuthoringDocument::from_material(MaterialDefinition material,
                                                                   std::string artifact_path) {
    if (!is_valid_material_definition(material)) {
        throw std::invalid_argument("material authoring document requires a valid material definition");
    }
    sort_document_textures(material);
    MaterialAuthoringDocument document;
    document.material_ = std::move(material);
    document.saved_material_ = document.material_;
    document.artifact_path_ = std::move(artifact_path);
    document.dirty_state_.reset_clean();
    return document;
}

const MaterialDefinition& MaterialAuthoringDocument::material() const noexcept {
    return material_;
}

std::string_view MaterialAuthoringDocument::artifact_path() const noexcept {
    return artifact_path_;
}

bool MaterialAuthoringDocument::dirty() const {
    return !same_material(material_, saved_material_);
}

bool MaterialAuthoringDocument::staged() const noexcept {
    return staged_;
}

MaterialAuthoringModel MaterialAuthoringDocument::model() const {
    return make_material_authoring_model(*this, nullptr);
}

MaterialAuthoringModel MaterialAuthoringDocument::model(const AssetRegistry& registry) const {
    return make_material_authoring_model(*this, &registry);
}

bool MaterialAuthoringDocument::set_base_color(std::array<float, 4> value) {
    return set_factor(MaterialAuthoringFactor::base_color, value);
}

bool MaterialAuthoringDocument::set_emissive(std::array<float, 3> value) {
    return set_factor(MaterialAuthoringFactor::emissive, {value[0], value[1], value[2], 0.0F});
}

bool MaterialAuthoringDocument::set_metallic(float value) {
    return set_factor(MaterialAuthoringFactor::metallic, {value, 0.0F, 0.0F, 0.0F});
}

bool MaterialAuthoringDocument::set_roughness(float value) {
    return set_factor(MaterialAuthoringFactor::roughness, {value, 0.0F, 0.0F, 0.0F});
}

bool MaterialAuthoringDocument::set_factor(MaterialAuthoringFactor factor, std::array<float, 4> values) {
    if (!valid_factor_values(factor, values) || !apply_factor(material_, factor, values)) {
        return false;
    }
    dirty_state_.mark_dirty();
    staged_ = false;
    return true;
}

bool MaterialAuthoringDocument::set_texture(MaterialTextureSlot slot, AssetId texture) {
    if (!apply_texture(material_, slot, texture)) {
        return false;
    }
    dirty_state_.mark_dirty();
    staged_ = false;
    return true;
}

bool MaterialAuthoringDocument::clear_texture(MaterialTextureSlot slot) {
    const auto old_size = material_.texture_bindings.size();
    const auto cleared_bindings = std::ranges::remove_if(
        material_.texture_bindings, [slot](const MaterialTextureBinding& binding) { return binding.slot == slot; });
    material_.texture_bindings.erase(cleared_bindings.begin(), cleared_bindings.end());
    if (material_.texture_bindings.size() == old_size) {
        return false;
    }
    dirty_state_.mark_dirty();
    staged_ = false;
    return true;
}

bool MaterialAuthoringDocument::replace_material(MaterialDefinition material) {
    if (!is_valid_material_definition(material)) {
        return false;
    }
    sort_document_textures(material);
    material_ = std::move(material);
    dirty_state_.mark_dirty();
    staged_ = false;
    return true;
}

bool MaterialAuthoringDocument::stage_changes() {
    if (!dirty() || !validate_material_authoring_document(*this).empty()) {
        staged_ = false;
        return false;
    }
    staged_ = true;
    return true;
}

bool MaterialAuthoringDocument::stage_changes(const AssetRegistry& registry) {
    if (!dirty() || !validate_material_authoring_document(*this, registry).empty()) {
        staged_ = false;
        return false;
    }
    staged_ = true;
    return true;
}

void MaterialAuthoringDocument::mark_saved() {
    saved_material_ = material_;
    dirty_state_.mark_saved();
    staged_ = false;
}

std::string_view material_authoring_factor_label(MaterialAuthoringFactor factor) noexcept {
    switch (factor) {
    case MaterialAuthoringFactor::base_color:
        return "Base Color";
    case MaterialAuthoringFactor::emissive:
        return "Emissive";
    case MaterialAuthoringFactor::metallic:
        return "Metallic";
    case MaterialAuthoringFactor::roughness:
        return "Roughness";
    case MaterialAuthoringFactor::unknown:
        break;
    }
    return "Unknown";
}

std::vector<MaterialAuthoringDiagnostic>
validate_material_authoring_document(const MaterialAuthoringDocument& document) {
    std::vector<MaterialAuthoringDiagnostic> diagnostics;
    const auto& material = document.material();

    validate_factor(diagnostics, "factor.base_color", material.factors.base_color, 0.0F, 1.0F);
    validate_factor(diagnostics, "factor.emissive", material.factors.emissive, 0.0F, 1024.0F);
    validate_factor(diagnostics, "factor.metallic", std::span{&material.factors.metallic, 1U}, 0.0F, 1.0F);
    validate_factor(diagnostics, "factor.roughness", std::span{&material.factors.roughness, 1U}, 0.0F, 1.0F);

    for (std::size_t index = 0; index < material.texture_bindings.size(); ++index) {
        const auto& binding = material.texture_bindings[index];
        const auto field = "texture." + std::string(texture_slot_id(binding.slot));
        if (!valid_texture_slot(binding.slot) || binding.texture.value == 0) {
            add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::invalid_texture_binding, field,
                           "material texture binding requires a valid slot and texture asset id");
            continue;
        }
        for (std::size_t other = index + 1U; other < material.texture_bindings.size(); ++other) {
            if (binding.slot == material.texture_bindings[other].slot) {
                add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::duplicate_texture_slot, field,
                               "material texture slot is authored more than once");
                break;
            }
        }
    }

    if (!diagnostics.empty()) {
        return diagnostics;
    }

    const auto metadata = build_material_pipeline_binding_metadata(material);
    for (const auto& texture : material.texture_bindings) {
        const auto slot_bindings = bindings_for_slot(metadata.bindings, texture.slot);
        if (!has_binding_kind(slot_bindings, MaterialBindingResourceKind::sampled_texture) ||
            !has_binding_kind(slot_bindings, MaterialBindingResourceKind::sampler)) {
            add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::missing_binding_metadata,
                           "texture." + std::string(texture_slot_id(texture.slot)),
                           "material texture slot is missing sampled texture or sampler binding metadata");
        }
    }

    return diagnostics;
}

std::vector<MaterialAuthoringDiagnostic> validate_material_authoring_document(const MaterialAuthoringDocument& document,
                                                                              const AssetRegistry& registry) {
    auto diagnostics = validate_material_authoring_document(document);
    for (const auto& binding : document.material().texture_bindings) {
        if (binding.texture.value == 0 || !valid_texture_slot(binding.slot)) {
            continue;
        }

        const auto field = "texture." + std::string(texture_slot_id(binding.slot));
        const auto* record = registry.find(binding.texture);
        if (record == nullptr) {
            add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::invalid_texture_binding, field,
                           "material texture asset is not registered");
        } else if (record->kind != AssetKind::texture) {
            add_diagnostic(diagnostics, MaterialAuthoringDiagnosticCode::invalid_texture_binding, field,
                           "material texture asset is not registered as a texture");
        }
    }
    return diagnostics;
}

std::vector<AssetId> material_authoring_texture_dependencies(const MaterialAuthoringDocument& document) {
    std::vector<AssetId> dependencies;
    for (const auto slot : authored_texture_slots()) {
        const auto binding =
            std::ranges::find_if(document.material().texture_bindings, [slot](const MaterialTextureBinding& candidate) {
                return candidate.slot == slot && candidate.texture.value != 0;
            });
        if (binding == document.material().texture_bindings.end()) {
            continue;
        }
        if (!std::ranges::contains(dependencies, binding->texture)) {
            dependencies.push_back(binding->texture);
        }
    }
    return dependencies;
}

UndoableAction make_material_authoring_factor_edit_action(MaterialAuthoringDocument& document,
                                                          MaterialAuthoringFactor factor, std::array<float, 4> values) {
    if (factor == MaterialAuthoringFactor::unknown || !valid_factor_values(factor, values)) {
        return {};
    }

    auto before = document.material();
    auto after = before;
    (void)apply_factor(after, factor, values);
    return UndoableAction{
        .label = "Edit Material " + std::string(material_authoring_factor_label(factor)),
        .redo = [&document, after]() { (void)document.replace_material(after); },
        .undo = [&document, before]() { (void)document.replace_material(before); },
    };
}

UndoableAction make_material_authoring_texture_edit_action(MaterialAuthoringDocument& document,
                                                           MaterialTextureSlot slot, AssetId texture) {
    if (!valid_texture_slot(slot)) {
        return {};
    }

    auto before = document.material();
    auto after = before;
    (void)apply_texture(after, slot, texture);
    return UndoableAction{
        .label = "Edit Material " + std::string(texture_slot_label(slot)) + " Texture",
        .redo = [&document, after]() { (void)document.replace_material(after); },
        .undo = [&document, before]() { (void)document.replace_material(before); },
    };
}

void save_material_authoring_document(ITextStore& store, std::string_view path, MaterialAuthoringDocument& document) {
    const auto diagnostics = validate_material_authoring_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material authoring document has validation diagnostics");
    }
    store.write_text(path, serialize_material_definition(document.material()));
    document = MaterialAuthoringDocument::from_material(document.material(), std::string(path));
}

void save_material_authoring_document(ITextStore& store, std::string_view path, MaterialAuthoringDocument& document,
                                      const AssetRegistry& registry) {
    const auto diagnostics = validate_material_authoring_document(document, registry);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("material authoring document has validation diagnostics");
    }
    store.write_text(path, serialize_material_definition(document.material()));
    document = MaterialAuthoringDocument::from_material(document.material(), std::string(path));
}

MaterialAuthoringDocument load_material_authoring_document(ITextStore& store, std::string_view path) {
    return MaterialAuthoringDocument::from_material(deserialize_material_definition(store.read_text(path)),
                                                    std::string(path));
}

} // namespace mirakana::editor
