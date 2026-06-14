// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana::runtime_scene {
namespace {

[[nodiscard]] std::string asset_kind_name(AssetKind kind) {
    switch (kind) {
    case AssetKind::unknown:
        return "unknown";
    case AssetKind::texture:
        return "texture";
    case AssetKind::mesh:
        return "mesh";
    case AssetKind::morph_mesh_cpu:
        return "morph_mesh_cpu";
    case AssetKind::animation_float_clip:
        return "animation_float_clip";
    case AssetKind::skinned_mesh:
        return "skinned_mesh";
    case AssetKind::material:
        return "material";
    case AssetKind::scene:
        return "scene";
    case AssetKind::audio:
        return "audio";
    case AssetKind::script:
        return "script";
    case AssetKind::shader:
        return "shader";
    case AssetKind::ui_atlas:
        return "ui_atlas";
    case AssetKind::tilemap:
        return "tilemap";
    case AssetKind::physics_collision_scene:
        return "physics_collision_scene";
    case AssetKind::environment_profile:
        return "environment_profile";
    case AssetKind::environment_preset_pack:
        return "environment_preset_pack";
    case AssetKind::mavg_cluster_graph:
        return "mavg_cluster_graph";
    }
    return "unknown";
}

[[nodiscard]] std::string reference_kind_name(RuntimeSceneReferenceKind kind) {
    switch (kind) {
    case RuntimeSceneReferenceKind::mesh:
        return "mesh";
    case RuntimeSceneReferenceKind::material:
        return "material";
    case RuntimeSceneReferenceKind::sprite:
        return "sprite";
    case RuntimeSceneReferenceKind::environment_profile:
        return "environment_profile";
    }
    return "reference";
}

[[nodiscard]] bool has_equivalent_diagnostic(const std::vector<RuntimeSceneDiagnostic>& diagnostics,
                                             const RuntimeSceneDiagnostic& candidate) noexcept {
    return std::ranges::any_of(diagnostics, [&candidate](const RuntimeSceneDiagnostic& existing) {
        return existing.code == candidate.code && existing.asset == candidate.asset &&
               existing.node == candidate.node && existing.reference_kind == candidate.reference_kind &&
               existing.expected_kind == candidate.expected_kind && existing.actual_kind == candidate.actual_kind;
    });
}

void append_diagnostic(std::vector<RuntimeSceneDiagnostic>& diagnostics, RuntimeSceneDiagnostic diagnostic) {
    if (!has_equivalent_diagnostic(diagnostics, diagnostic)) {
        diagnostics.push_back(std::move(diagnostic));
    }
}

void append_scene_asset_diagnostic(std::vector<RuntimeSceneDiagnostic>& diagnostics, RuntimeSceneDiagnosticCode code,
                                   AssetId asset, AssetKind expected, AssetKind actual, std::string message) {
    append_diagnostic(diagnostics, RuntimeSceneDiagnostic{
                                       .code = code,
                                       .asset = asset,
                                       .node = null_scene_node,
                                       .reference_kind = RuntimeSceneReferenceKind::mesh,
                                       .expected_kind = expected,
                                       .actual_kind = actual,
                                       .message = std::move(message),
                                   });
}

void append_reference(std::vector<RuntimeSceneReference>& references, SceneNodeId node, AssetId asset,
                      RuntimeSceneReferenceKind reference_kind, AssetKind expected_kind) {
    references.push_back(RuntimeSceneReference{
        .node = node,
        .asset = asset,
        .kind = reference_kind,
        .expected_kind = expected_kind,
    });
}

[[nodiscard]] bool reference_accepts_asset_kind(RuntimeSceneReferenceKind reference_kind, AssetKind expected_kind,
                                                AssetKind actual_kind) noexcept {
    if (reference_kind == RuntimeSceneReferenceKind::mesh && expected_kind == AssetKind::mesh) {
        return actual_kind == AssetKind::mesh || actual_kind == AssetKind::skinned_mesh;
    }
    return actual_kind == expected_kind;
}

[[nodiscard]] std::unordered_map<AssetId, AssetIdentityRowV2, AssetIdHash>
make_identity_lookup(const AssetIdentityDocumentV2& identities) {
    std::unordered_map<AssetId, AssetIdentityRowV2, AssetIdHash> lookup;
    lookup.reserve(identities.assets.size());
    for (const auto& row : identities.assets) {
        const auto asset = asset_id_from_key_v2(row.key);
        if (!lookup.contains(asset)) {
            lookup.emplace(asset, row);
        }
    }
    return lookup;
}

void append_invalid_identity_document_diagnostic(RuntimeSceneAssetIdentityAudit& audit,
                                                 const AssetIdentityDiagnosticV2& diagnostic) {
    audit.diagnostics.push_back(RuntimeSceneAssetIdentityDiagnostic{
        .code = RuntimeSceneAssetIdentityDiagnosticCode::invalid_identity_document,
        .placement = "asset_identity.document",
        .node = null_scene_node,
        .asset = asset_id_from_key_v2(diagnostic.key),
        .key = diagnostic.key,
        .expected_kind = AssetKind::unknown,
        .actual_kind = AssetKind::unknown,
        .source_path = diagnostic.source_path,
        .message = "Asset Identity v2 document is invalid",
    });
}

void append_asset_identity_audit(RuntimeSceneAssetIdentityAudit& audit,
                                 const std::unordered_map<AssetId, AssetIdentityRowV2, AssetIdHash>& identities,
                                 std::string placement, SceneNodeId node, AssetId asset,
                                 RuntimeSceneReferenceKind reference_kind, AssetKind expected_kind) {
    const auto identity = identities.find(asset);
    if (identity == identities.end()) {
        audit.diagnostics.push_back(RuntimeSceneAssetIdentityDiagnostic{
            .code = RuntimeSceneAssetIdentityDiagnosticCode::missing_identity,
            .placement = std::move(placement),
            .node = node,
            .asset = asset,
            .key = {},
            .expected_kind = expected_kind,
            .actual_kind = AssetKind::unknown,
            .source_path = {},
            .message = "runtime scene reference has no Asset Identity v2 row",
        });
        return;
    }

    const auto& row = identity->second;
    if (!reference_accepts_asset_kind(reference_kind, expected_kind, row.kind)) {
        audit.diagnostics.push_back(RuntimeSceneAssetIdentityDiagnostic{
            .code = RuntimeSceneAssetIdentityDiagnosticCode::kind_mismatch,
            .placement = std::move(placement),
            .node = node,
            .asset = asset,
            .key = row.key,
            .expected_kind = expected_kind,
            .actual_kind = row.kind,
            .source_path = row.source_path,
            .message = "runtime scene reference Asset Identity v2 kind mismatch",
        });
        return;
    }

    audit.references.push_back(RuntimeSceneAssetIdentityReferenceRow{
        .placement = std::move(placement),
        .node = node,
        .asset = asset,
        .key = row.key,
        .expected_kind = expected_kind,
        .actual_kind = row.kind,
        .source_path = row.source_path,
    });
}

void collect_references(const Scene& scene, std::vector<RuntimeSceneReference>& references) {
    if (scene.environment().has_value()) {
        append_reference(references, null_scene_node, scene.environment()->profile,
                         RuntimeSceneReferenceKind::environment_profile, AssetKind::environment_profile);
    }
    for (const auto& node : scene.nodes()) {
        if (node.components.mesh_renderer.has_value()) {
            append_reference(references, node.id, node.components.mesh_renderer->mesh, RuntimeSceneReferenceKind::mesh,
                             AssetKind::mesh);
            append_reference(references, node.id, node.components.mesh_renderer->material,
                             RuntimeSceneReferenceKind::material, AssetKind::material);
        }
        if (node.components.sprite_renderer.has_value()) {
            append_reference(references, node.id, node.components.sprite_renderer->sprite,
                             RuntimeSceneReferenceKind::sprite, AssetKind::texture);
            append_reference(references, node.id, node.components.sprite_renderer->material,
                             RuntimeSceneReferenceKind::material, AssetKind::material);
        }
    }
}

void validate_references(const runtime::RuntimeAssetPackage& package,
                         const std::vector<RuntimeSceneReference>& references,
                         std::vector<RuntimeSceneDiagnostic>& diagnostics) {
    for (const auto& reference : references) {
        const auto* record = package.find(reference.asset);
        if (record == nullptr) {
            append_diagnostic(diagnostics, RuntimeSceneDiagnostic{
                                               .code = RuntimeSceneDiagnosticCode::missing_referenced_asset,
                                               .asset = reference.asset,
                                               .node = reference.node,
                                               .reference_kind = reference.kind,
                                               .expected_kind = reference.expected_kind,
                                               .actual_kind = AssetKind::unknown,
                                               .message = "runtime scene referenced " +
                                                          reference_kind_name(reference.kind) + " asset is missing",
                                           });
            continue;
        }
        if (!reference_accepts_asset_kind(reference.kind, reference.expected_kind, record->kind)) {
            append_diagnostic(diagnostics,
                              RuntimeSceneDiagnostic{
                                  .code = RuntimeSceneDiagnosticCode::referenced_asset_kind_mismatch,
                                  .asset = reference.asset,
                                  .node = reference.node,
                                  .reference_kind = reference.kind,
                                  .expected_kind = reference.expected_kind,
                                  .actual_kind = record->kind,
                                  .message = "runtime scene referenced " + reference_kind_name(reference.kind) +
                                             " asset has kind " + asset_kind_name(record->kind) + " but expected " +
                                             asset_kind_name(reference.expected_kind),
                              });
        }
    }
}

void validate_unique_node_names(AssetId scene_asset, const Scene& scene,
                                std::vector<RuntimeSceneDiagnostic>& diagnostics) {
    const auto& nodes = scene.nodes();
    for (auto current = nodes.begin(); current != nodes.end(); ++current) {
        const auto duplicate =
            std::ranges::find_if(std::ranges::subrange(nodes.begin(), current),
                                 [name = current->name](const SceneNode& candidate) { return candidate.name == name; });
        if (duplicate != current) {
            diagnostics.push_back(RuntimeSceneDiagnostic{
                .code = RuntimeSceneDiagnosticCode::duplicate_node_name,
                .asset = scene_asset,
                .node = current->id,
                .reference_kind = RuntimeSceneReferenceKind::mesh,
                .expected_kind = AssetKind::unknown,
                .actual_kind = AssetKind::unknown,
                .message = "runtime scene node name is not unique: " + current->name,
            });
        }
    }
}

[[nodiscard]] AnimationTransformComponent
to_animation_transform_component(AnimationTransformBindingComponent component) noexcept {
    switch (component) {
    case AnimationTransformBindingComponent::translation_x:
        return AnimationTransformComponent::translation_x;
    case AnimationTransformBindingComponent::translation_y:
        return AnimationTransformComponent::translation_y;
    case AnimationTransformBindingComponent::translation_z:
        return AnimationTransformComponent::translation_z;
    case AnimationTransformBindingComponent::rotation_z:
        return AnimationTransformComponent::rotation_z;
    case AnimationTransformBindingComponent::scale_x:
        return AnimationTransformComponent::scale_x;
    case AnimationTransformBindingComponent::scale_y:
        return AnimationTransformComponent::scale_y;
    case AnimationTransformBindingComponent::scale_z:
        return AnimationTransformComponent::scale_z;
    case AnimationTransformBindingComponent::unknown:
        break;
    }
    return AnimationTransformComponent::translation_x;
}

void append_animation_binding_diagnostic(std::vector<RuntimeSceneAnimationTransformBindingDiagnostic>& diagnostics,
                                         RuntimeSceneAnimationTransformBindingDiagnosticCode code, std::string target,
                                         std::string node_name, AnimationTransformBindingComponent component,
                                         std::string message) {
    diagnostics.push_back(RuntimeSceneAnimationTransformBindingDiagnostic{
        .code = code,
        .target = std::move(target),
        .node_name = std::move(node_name),
        .component = component,
        .message = std::move(message),
    });
}

[[nodiscard]] std::string gameplay_binding_component_kind_name(RuntimeSceneGameplayBindingComponentKind kind) {
    switch (kind) {
    case RuntimeSceneGameplayBindingComponentKind::none:
        return "none";
    case RuntimeSceneGameplayBindingComponentKind::any_renderable:
        return "any_renderable";
    case RuntimeSceneGameplayBindingComponentKind::camera:
        return "camera";
    case RuntimeSceneGameplayBindingComponentKind::light:
        return "light";
    case RuntimeSceneGameplayBindingComponentKind::mesh_renderer:
        return "mesh_renderer";
    case RuntimeSceneGameplayBindingComponentKind::sprite_renderer:
        return "sprite_renderer";
    }
    return "unknown";
}

[[nodiscard]] bool is_line_text_value(const std::string_view value) noexcept {
    return !value.empty() && value.find_first_of("\r\n=") == std::string_view::npos;
}

[[nodiscard]] bool contains_string(const std::span<const std::string> values, const std::string_view value) noexcept {
    return std::ranges::find_if(values, [value](const std::string& candidate) {
               return std::string_view{candidate} == value;
           }) != values.end();
}

[[nodiscard]] bool same_cell(const runtime::RuntimeConstructionPlacementCellDesc& lhs,
                             const runtime::RuntimeConstructionPlacementCellDesc& rhs) noexcept {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

[[nodiscard]] bool is_finite_vec3(const Vec3 value) noexcept {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

[[nodiscard]] bool is_valid_scene_transform(const Transform3D transform) noexcept {
    return is_finite_vec3(transform.position) && is_finite_vec3(transform.rotation_radians) &&
           is_finite_vec3(transform.scale) && transform.scale.x != 0.0F && transform.scale.y != 0.0F &&
           transform.scale.z != 0.0F;
}

[[nodiscard]] bool nearly_equal(const float lhs, const float rhs) noexcept {
    return std::abs(lhs - rhs) <= 0.0001F;
}

[[nodiscard]] bool
matches_candidate_world_position(const Transform3D transform,
                                 const runtime::RuntimeConstructionPlacementValidationRow& candidate) noexcept {
    return nearly_equal(transform.position.x, candidate.world_x) &&
           nearly_equal(transform.position.y, candidate.world_y) &&
           nearly_equal(transform.position.z, candidate.world_z);
}

struct RuntimeScenePlacementCandidateRows {
    bool found{false};
    runtime::RuntimeConstructionPlacementValidationRow candidate;
    std::vector<runtime::RuntimeConstructionPlacementCellDesc> occupied_cells;
};

[[nodiscard]] RuntimeScenePlacementCandidateRows
find_construction_placement_candidate_rows(const runtime::RuntimeConstructionPlacementValidationResult& placement,
                                           const std::uint32_t candidate_index) {
    RuntimeScenePlacementCandidateRows result;
    for (const auto& row : placement.rows) {
        if (row.candidate_index != candidate_index) {
            continue;
        }
        if (row.kind == runtime::RuntimeConstructionPlacementValidationRowKind::candidate) {
            if (!result.found) {
                result.found = true;
                result.candidate = row;
            }
            continue;
        }
        if (row.kind == runtime::RuntimeConstructionPlacementValidationRowKind::occupied_cell) {
            result.occupied_cells.push_back(runtime::RuntimeConstructionPlacementCellDesc{
                .x = row.cell_x,
                .y = row.cell_y,
                .z = row.cell_z,
            });
        }
    }
    return result;
}

[[nodiscard]] const RuntimeSceneConstructionPlacementOccupiedCell*
find_occupied_cell(const std::span<const RuntimeSceneConstructionPlacementOccupiedCell> occupied_cells,
                   const std::vector<runtime::RuntimeConstructionPlacementCellDesc>& candidate_cells) noexcept {
    for (const auto& cell : candidate_cells) {
        const auto occupied = std::ranges::find_if(
            occupied_cells, [&cell](const auto& existing) { return same_cell(existing.cell, cell); });
        if (occupied != occupied_cells.end()) {
            return &(*occupied);
        }
    }
    return nullptr;
}

void append_construction_placement_intent_diagnostic(
    RuntimeSceneConstructionPlacementIntentPlan& plan, const RuntimeSceneConstructionPlacementIntentDiagnosticCode code,
    const RuntimeSceneConstructionPlacementIntentRow& row, std::string message,
    const RuntimeSceneConstructionPlacementOccupiedCell* occupied_cell = nullptr) {
    RuntimeSceneConstructionPlacementIntentDiagnostic diagnostic{
        .code = code,
        .candidate_index = row.candidate_index,
        .item_id = row.item_id,
        .placement_id = row.placement_id,
        .surface_id = row.surface_id,
        .node_name = row.node_name,
        .message = std::move(message),
    };
    if (occupied_cell != nullptr) {
        diagnostic.existing_node = occupied_cell->node;
        diagnostic.existing_node_name = occupied_cell->node_name;
        diagnostic.cell_x = occupied_cell->cell.x;
        diagnostic.cell_y = occupied_cell->cell.y;
        diagnostic.cell_z = occupied_cell->cell.z;
    }
    plan.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] bool
supports_procedural_scene_placement_kind(const runtime::RuntimeProceduralGenerationContentKind kind) noexcept {
    switch (kind) {
    case runtime::RuntimeProceduralGenerationContentKind::encounter:
    case runtime::RuntimeProceduralGenerationContentKind::loot:
    case runtime::RuntimeProceduralGenerationContentKind::object:
        return true;
    case runtime::RuntimeProceduralGenerationContentKind::map_tile:
        return false;
    }
    return false;
}

[[nodiscard]] const runtime::RuntimeProceduralGenerationOutputRow*
find_procedural_output_row(const runtime::RuntimeProceduralGenerationPlan& generation,
                           const std::string_view output_id) noexcept {
    const auto found = std::ranges::find_if(
        generation.rows, [output_id](const auto& row) { return std::string_view{row.id} == output_id; });
    return found != generation.rows.end() ? &(*found) : nullptr;
}

[[nodiscard]] bool has_duplicate_procedural_output_id(const runtime::RuntimeProceduralGenerationPlan& generation,
                                                      const std::string_view output_id) noexcept {
    std::uint32_t matching_rows = 0U;
    for (const auto& row : generation.rows) {
        if (std::string_view{row.id} != output_id) {
            continue;
        }
        ++matching_rows;
        if (matching_rows > 1U) {
            return true;
        }
    }
    return false;
}

[[nodiscard]] RuntimeSceneConstructionPlacementIntentRow
make_procedural_construction_placement_row(const RuntimeSceneProceduralConstructionPlacementIntentDesc& source_row,
                                           const runtime::RuntimeProceduralGenerationOutputRow* output_row) {
    return RuntimeSceneConstructionPlacementIntentRow{
        .candidate_index = source_row.candidate_index,
        .status = RuntimeSceneConstructionPlacementIntentStatus::invalid,
        .node_name = source_row.node_name,
        .transform = source_row.transform,
        .components = source_row.components,
        .procedural_output_id = source_row.procedural_output_id,
        .anchor_id = source_row.anchor_id,
        .procedural_kind =
            output_row != nullptr ? output_row->kind : runtime::RuntimeProceduralGenerationContentKind::object,
        .package_visible = source_row.package_visible,
    };
}

void append_procedural_construction_placement_diagnostic(
    RuntimeSceneConstructionPlacementIntentPlan& plan, const RuntimeSceneConstructionPlacementIntentDiagnosticCode code,
    const RuntimeSceneConstructionPlacementIntentRow& row, std::string message) {
    plan.diagnostics.push_back(RuntimeSceneConstructionPlacementIntentDiagnostic{
        .code = code,
        .candidate_index = row.candidate_index,
        .item_id = row.item_id,
        .placement_id = row.placement_id,
        .surface_id = row.surface_id,
        .node_name = row.node_name,
        .message = std::move(message),
        .procedural_output_id = row.procedural_output_id,
        .anchor_id = row.anchor_id,
        .procedural_kind = row.procedural_kind,
    });
}

[[nodiscard]] RuntimeSceneConstructionPlacementIntentDesc
make_construction_placement_intent_desc(const RuntimeSceneProceduralConstructionPlacementIntentDesc& source_row) {
    return RuntimeSceneConstructionPlacementIntentDesc{
        .candidate_index = source_row.candidate_index,
        .node_name = source_row.node_name,
        .transform = source_row.transform,
        .components = source_row.components,
        .reviewed = source_row.reviewed,
    };
}

void apply_procedural_construction_placement_metadata(
    RuntimeSceneConstructionPlacementIntentPlan& plan,
    const std::span<const RuntimeSceneProceduralConstructionPlacementIntentDesc> source_rows,
    const std::span<const std::size_t> forwarded_source_indices,
    const runtime::RuntimeProceduralGenerationPlan& generation) {
    for (std::size_t index = 0; index < plan.rows.size() && index < forwarded_source_indices.size(); ++index) {
        auto& row = plan.rows[index];
        const auto& source_row = source_rows[forwarded_source_indices[index]];
        const auto* output_row = find_procedural_output_row(generation, source_row.procedural_output_id);
        row.procedural_output_id = source_row.procedural_output_id;
        row.anchor_id = source_row.anchor_id;
        row.procedural_kind =
            output_row != nullptr ? output_row->kind : runtime::RuntimeProceduralGenerationContentKind::object;
        row.package_visible = source_row.package_visible;
    }

    std::size_t diagnostic_row_index = 0U;
    for (auto& diagnostic : plan.diagnostics) {
        while (diagnostic_row_index < plan.rows.size() &&
               plan.rows[diagnostic_row_index].status == RuntimeSceneConstructionPlacementIntentStatus::accepted) {
            ++diagnostic_row_index;
        }
        if (diagnostic_row_index >= plan.rows.size() || diagnostic_row_index >= forwarded_source_indices.size()) {
            continue;
        }
        const auto& source_row = source_rows[forwarded_source_indices[diagnostic_row_index]];
        const auto* output_row = find_procedural_output_row(generation, source_row.procedural_output_id);
        diagnostic.procedural_output_id = source_row.procedural_output_id;
        diagnostic.anchor_id = source_row.anchor_id;
        diagnostic.procedural_kind =
            output_row != nullptr ? output_row->kind : runtime::RuntimeProceduralGenerationContentKind::object;
        ++diagnostic_row_index;
    }
}

void append_gameplay_binding_diagnostic(std::vector<RuntimeSceneGameplayBindingDiagnostic>& diagnostics,
                                        RuntimeSceneGameplayBindingDiagnosticCode code,
                                        const RuntimeSceneGameplayBindingSourceRow& source_row, SceneNodeId node,
                                        std::string message) {
    diagnostics.push_back(RuntimeSceneGameplayBindingDiagnostic{
        .code = code,
        .binding_id = source_row.binding_id,
        .gameplay_system_id = source_row.gameplay_system_id,
        .slot_id = source_row.slot_id,
        .node_name = source_row.node_name,
        .node = node,
        .required_component = source_row.required_component,
        .message = std::move(message),
    });
}

[[nodiscard]] bool node_has_gameplay_binding_component(const SceneNode& node,
                                                       RuntimeSceneGameplayBindingComponentKind required_component) {
    switch (required_component) {
    case RuntimeSceneGameplayBindingComponentKind::none:
        return true;
    case RuntimeSceneGameplayBindingComponentKind::any_renderable:
        return node.components.mesh_renderer.has_value() || node.components.sprite_renderer.has_value();
    case RuntimeSceneGameplayBindingComponentKind::camera:
        return node.components.camera.has_value();
    case RuntimeSceneGameplayBindingComponentKind::light:
        return node.components.light.has_value();
    case RuntimeSceneGameplayBindingComponentKind::mesh_renderer:
        return node.components.mesh_renderer.has_value();
    case RuntimeSceneGameplayBindingComponentKind::sprite_renderer:
        return node.components.sprite_renderer.has_value();
    }
    return false;
}

void append_gameplay_interaction_diagnostic(std::vector<RuntimeSceneGameplayInteractionDiagnostic>& diagnostics,
                                            RuntimeSceneGameplayInteractionDiagnosticCode code,
                                            const RuntimeSceneGameplayInteractionSourceRow& source_row,
                                            RuntimeSceneGameplaySessionState session_state, std::string message) {
    diagnostics.push_back(RuntimeSceneGameplayInteractionDiagnostic{
        .code = code,
        .action_id = source_row.action_id,
        .kind = source_row.kind,
        .source_binding_id = source_row.source_binding_id,
        .target_binding_id = source_row.target_binding_id,
        .objective_id = source_row.objective_id,
        .amount = source_row.amount,
        .session_state = session_state,
        .message = std::move(message),
    });
}

[[nodiscard]] bool gameplay_interaction_requires_target(RuntimeSceneGameplayInteractionKind kind) noexcept {
    switch (kind) {
    case RuntimeSceneGameplayInteractionKind::trigger:
    case RuntimeSceneGameplayInteractionKind::pickup:
    case RuntimeSceneGameplayInteractionKind::damage:
    case RuntimeSceneGameplayInteractionKind::heal:
        return true;
    case RuntimeSceneGameplayInteractionKind::objective_progress:
    case RuntimeSceneGameplayInteractionKind::objective_complete:
    case RuntimeSceneGameplayInteractionKind::win:
    case RuntimeSceneGameplayInteractionKind::loss:
    case RuntimeSceneGameplayInteractionKind::restart:
        return false;
    }
    return false;
}

[[nodiscard]] bool gameplay_interaction_requires_objective(RuntimeSceneGameplayInteractionKind kind) noexcept {
    switch (kind) {
    case RuntimeSceneGameplayInteractionKind::objective_progress:
    case RuntimeSceneGameplayInteractionKind::objective_complete:
        return true;
    case RuntimeSceneGameplayInteractionKind::trigger:
    case RuntimeSceneGameplayInteractionKind::pickup:
    case RuntimeSceneGameplayInteractionKind::damage:
    case RuntimeSceneGameplayInteractionKind::heal:
    case RuntimeSceneGameplayInteractionKind::win:
    case RuntimeSceneGameplayInteractionKind::loss:
    case RuntimeSceneGameplayInteractionKind::restart:
        return false;
    }
    return false;
}

[[nodiscard]] bool gameplay_interaction_requires_positive_amount(RuntimeSceneGameplayInteractionKind kind) noexcept {
    switch (kind) {
    case RuntimeSceneGameplayInteractionKind::pickup:
    case RuntimeSceneGameplayInteractionKind::damage:
    case RuntimeSceneGameplayInteractionKind::heal:
    case RuntimeSceneGameplayInteractionKind::objective_progress:
        return true;
    case RuntimeSceneGameplayInteractionKind::trigger:
    case RuntimeSceneGameplayInteractionKind::objective_complete:
    case RuntimeSceneGameplayInteractionKind::win:
    case RuntimeSceneGameplayInteractionKind::loss:
    case RuntimeSceneGameplayInteractionKind::restart:
        return false;
    }
    return false;
}

[[nodiscard]] RuntimeSceneGameplaySessionState
gameplay_interaction_resulting_session_state(RuntimeSceneGameplayInteractionKind kind,
                                             RuntimeSceneGameplaySessionState current) noexcept {
    switch (kind) {
    case RuntimeSceneGameplayInteractionKind::win:
        return RuntimeSceneGameplaySessionState::won;
    case RuntimeSceneGameplayInteractionKind::loss:
        return RuntimeSceneGameplaySessionState::lost;
    case RuntimeSceneGameplayInteractionKind::restart:
        return RuntimeSceneGameplaySessionState::running;
    case RuntimeSceneGameplayInteractionKind::trigger:
    case RuntimeSceneGameplayInteractionKind::pickup:
    case RuntimeSceneGameplayInteractionKind::damage:
    case RuntimeSceneGameplayInteractionKind::heal:
    case RuntimeSceneGameplayInteractionKind::objective_progress:
    case RuntimeSceneGameplayInteractionKind::objective_complete:
        return current;
    }
    return current;
}

[[nodiscard]] std::size_t transform_index_for_node(const RuntimeSceneInstance& instance, SceneNodeId node) {
    if (node == null_scene_node || node.value == 0U ||
        node.value > static_cast<std::uint32_t>(instance.scene.nodes().size())) {
        throw std::out_of_range("runtime scene animation binding node index is invalid");
    }
    return static_cast<std::size_t>(node.value - 1U);
}

[[nodiscard]] Vec3 euler_radians_from_quat(Quat rotation) noexcept {
    const auto matrix = Mat4::rotation_quat(rotation);
    const auto sine_y = std::clamp(-matrix.at(2, 0), -1.0F, 1.0F);
    const auto y = std::asin(sine_y);
    const auto cosine_y = std::cos(y);

    float x = 0.0F;
    float z = 0.0F;
    if (std::abs(cosine_y) > 0.000001F) {
        x = std::atan2(matrix.at(2, 1), matrix.at(2, 2));
        z = std::atan2(matrix.at(1, 0), matrix.at(0, 0));
    } else {
        z = std::atan2(-matrix.at(0, 1), matrix.at(1, 1));
    }
    return Vec3{.x = x, .y = y, .z = z};
}

} // namespace

bool RuntimeSceneLoadResult::succeeded() const noexcept {
    return instance.has_value() && diagnostics.empty();
}

bool RuntimeSceneAnimationTransformBindingResolution::succeeded() const noexcept {
    return diagnostics.empty();
}

bool RuntimeSceneGameplayBindingResolution::succeeded() const noexcept {
    return diagnostics.empty();
}

bool RuntimeSceneGameplayInteractionPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

bool RuntimeSceneConstructionPlacementIntentPlan::succeeded() const noexcept {
    return diagnostics.empty();
}

RuntimeSceneAssetIdentityAudit audit_runtime_scene_asset_identity(const Scene& scene,
                                                                  const AssetIdentityDocumentV2& identities) {
    RuntimeSceneAssetIdentityAudit audit;
    const auto identity_diagnostics = validate_asset_identity_document_v2(identities);
    if (!identity_diagnostics.empty()) {
        for (const auto& diagnostic : identity_diagnostics) {
            append_invalid_identity_document_diagnostic(audit, diagnostic);
        }
        return audit;
    }

    const auto lookup = make_identity_lookup(identities);

    if (scene.environment().has_value()) {
        append_asset_identity_audit(audit, lookup, "scene.environment.profile", null_scene_node,
                                    scene.environment()->profile, RuntimeSceneReferenceKind::environment_profile,
                                    AssetKind::environment_profile);
    }

    for (const auto& node : scene.nodes()) {
        if (node.components.mesh_renderer.has_value()) {
            append_asset_identity_audit(audit, lookup, "scene.component.mesh_renderer.mesh", node.id,
                                        node.components.mesh_renderer->mesh, RuntimeSceneReferenceKind::mesh,
                                        AssetKind::mesh);
            append_asset_identity_audit(audit, lookup, "scene.component.mesh_renderer.material", node.id,
                                        node.components.mesh_renderer->material, RuntimeSceneReferenceKind::material,
                                        AssetKind::material);
        }
        if (node.components.sprite_renderer.has_value()) {
            append_asset_identity_audit(audit, lookup, "scene.component.sprite_renderer.sprite", node.id,
                                        node.components.sprite_renderer->sprite, RuntimeSceneReferenceKind::sprite,
                                        AssetKind::texture);
            append_asset_identity_audit(audit, lookup, "scene.component.sprite_renderer.material", node.id,
                                        node.components.sprite_renderer->material, RuntimeSceneReferenceKind::material,
                                        AssetKind::material);
        }
    }

    return audit;
}

RuntimeSceneLoadResult instantiate_runtime_scene(const runtime::RuntimeAssetPackage& package, AssetId scene_asset,
                                                 RuntimeSceneLoadOptions options) {
    RuntimeSceneLoadResult result;

    const auto* record = package.find(scene_asset);
    if (record == nullptr) {
        append_scene_asset_diagnostic(result.diagnostics, RuntimeSceneDiagnosticCode::missing_scene_asset, scene_asset,
                                      AssetKind::scene, AssetKind::unknown, "runtime scene asset is missing");
        return result;
    }

    if (record->kind != AssetKind::scene) {
        append_scene_asset_diagnostic(
            result.diagnostics, RuntimeSceneDiagnosticCode::wrong_asset_kind, scene_asset, AssetKind::scene,
            record->kind, "runtime scene asset has kind " + asset_kind_name(record->kind) + " but expected scene");
        return result;
    }

    const auto payload = runtime::runtime_scene_payload(*record);
    if (!payload.succeeded()) {
        append_scene_asset_diagnostic(result.diagnostics, RuntimeSceneDiagnosticCode::malformed_scene_payload,
                                      scene_asset, AssetKind::scene, record->kind, payload.diagnostic);
        return result;
    }

    try {
        RuntimeSceneInstance instance{
            .scene_asset = scene_asset,
            .handle = payload.payload.handle,
            .scene = deserialize_scene(payload.payload.serialized_scene),
            .references = {},
        };
        collect_references(instance.scene, instance.references);
        if (options.require_environment_profile && !instance.scene.environment().has_value()) {
            append_diagnostic(result.diagnostics,
                              RuntimeSceneDiagnostic{
                                  .code = RuntimeSceneDiagnosticCode::missing_environment_profile,
                                  .asset = scene_asset,
                                  .node = null_scene_node,
                                  .reference_kind = RuntimeSceneReferenceKind::environment_profile,
                                  .expected_kind = AssetKind::environment_profile,
                                  .actual_kind = AssetKind::unknown,
                                  .message = "runtime scene requires an environment profile reference",
                              });
        }
        if (options.validate_asset_references) {
            validate_references(package, instance.references, result.diagnostics);
        }
        if (options.require_unique_node_names) {
            validate_unique_node_names(scene_asset, instance.scene, result.diagnostics);
        }
        result.instance = std::move(instance);
    } catch (const std::exception& error) {
        append_scene_asset_diagnostic(result.diagnostics, RuntimeSceneDiagnosticCode::malformed_scene_payload,
                                      scene_asset, AssetKind::scene, record->kind,
                                      std::string("runtime scene deserialize failed: ") + error.what());
    }

    return result;
}

std::vector<SceneNodeId> find_runtime_scene_nodes_by_name(const RuntimeSceneInstance& instance, std::string_view name) {
    std::vector<SceneNodeId> matches;
    for (const auto& node : instance.scene.nodes()) {
        if (node.name == name) {
            matches.push_back(node.id);
        }
    }
    return matches;
}

RuntimeSceneAnimationTransformBindingResolution
resolve_runtime_scene_animation_transform_bindings(const RuntimeSceneInstance& instance,
                                                   const AnimationTransformBindingSourceDocument& binding_source) {
    RuntimeSceneAnimationTransformBindingResolution result;
    if (!is_valid_animation_transform_binding_source_document(binding_source)) {
        append_animation_binding_diagnostic(
            result.diagnostics, RuntimeSceneAnimationTransformBindingDiagnosticCode::invalid_binding_document, {}, {},
            AnimationTransformBindingComponent::unknown, "animation transform binding source document is invalid");
        return result;
    }

    result.bindings.reserve(binding_source.bindings.size());
    for (const auto& source_binding : binding_source.bindings) {
        const auto matches = find_runtime_scene_nodes_by_name(instance, source_binding.node_name);
        if (matches.empty()) {
            append_animation_binding_diagnostic(
                result.diagnostics, RuntimeSceneAnimationTransformBindingDiagnosticCode::missing_node,
                source_binding.target, source_binding.node_name, source_binding.component,
                "animation transform binding node is missing: " + source_binding.node_name);
            continue;
        }
        if (matches.size() > 1U) {
            append_animation_binding_diagnostic(
                result.diagnostics, RuntimeSceneAnimationTransformBindingDiagnosticCode::duplicate_node_name,
                source_binding.target, source_binding.node_name, source_binding.component,
                "animation transform binding node name is ambiguous: " + source_binding.node_name);
            continue;
        }

        result.bindings.push_back(AnimationTransformCurveBinding{
            .target = source_binding.target,
            .transform_index = transform_index_for_node(instance, matches.front()),
            .component = to_animation_transform_component(source_binding.component),
        });
    }

    if (!result.diagnostics.empty()) {
        result.bindings.clear();
    }
    return result;
}

RuntimeSceneGameplayBindingResolution
resolve_runtime_scene_gameplay_bindings(const RuntimeSceneInstance& instance,
                                        std::span<const RuntimeSceneGameplayBindingSourceRow> source_rows) {
    RuntimeSceneGameplayBindingResolution result;
    result.bindings.reserve(source_rows.size());

    std::unordered_set<std::string> binding_ids;
    binding_ids.reserve(source_rows.size());

    for (const auto& source_row : source_rows) {
        bool source_row_valid = true;
        if (source_row.binding_id.empty()) {
            append_gameplay_binding_diagnostic(
                result.diagnostics, RuntimeSceneGameplayBindingDiagnosticCode::invalid_binding_id, source_row,
                null_scene_node, "runtime scene gameplay binding id is empty");
            source_row_valid = false;
        } else if (!binding_ids.insert(source_row.binding_id).second) {
            append_gameplay_binding_diagnostic(
                result.diagnostics, RuntimeSceneGameplayBindingDiagnosticCode::duplicate_binding_id, source_row,
                null_scene_node, "runtime scene gameplay binding id is duplicated: " + source_row.binding_id);
            source_row_valid = false;
        }
        if (source_row.gameplay_system_id.empty()) {
            append_gameplay_binding_diagnostic(
                result.diagnostics, RuntimeSceneGameplayBindingDiagnosticCode::invalid_gameplay_system_id, source_row,
                null_scene_node, "runtime scene gameplay system id is empty");
            source_row_valid = false;
        }
        if (source_row.slot_id.empty()) {
            append_gameplay_binding_diagnostic(result.diagnostics,
                                               RuntimeSceneGameplayBindingDiagnosticCode::invalid_slot_id, source_row,
                                               null_scene_node, "runtime scene gameplay binding slot id is empty");
            source_row_valid = false;
        }
        if (source_row.node_name.empty()) {
            append_gameplay_binding_diagnostic(result.diagnostics,
                                               RuntimeSceneGameplayBindingDiagnosticCode::invalid_node_name, source_row,
                                               null_scene_node, "runtime scene gameplay binding node name is empty");
            source_row_valid = false;
        }
        if (!source_row_valid) {
            continue;
        }

        const auto matches = find_runtime_scene_nodes_by_name(instance, source_row.node_name);
        if (matches.empty()) {
            append_gameplay_binding_diagnostic(
                result.diagnostics, RuntimeSceneGameplayBindingDiagnosticCode::missing_node, source_row,
                null_scene_node, "runtime scene gameplay binding node is missing: " + source_row.node_name);
            continue;
        }
        if (matches.size() > 1U) {
            append_gameplay_binding_diagnostic(
                result.diagnostics, RuntimeSceneGameplayBindingDiagnosticCode::duplicate_node_name, source_row,
                matches.front(), "runtime scene gameplay binding node name is ambiguous: " + source_row.node_name);
            continue;
        }

        const auto resolved_node = matches.front();
        const auto* node = instance.scene.find_node(resolved_node);
        if (node == nullptr || !node_has_gameplay_binding_component(*node, source_row.required_component)) {
            append_gameplay_binding_diagnostic(result.diagnostics,
                                               RuntimeSceneGameplayBindingDiagnosticCode::missing_required_component,
                                               source_row, resolved_node,
                                               "runtime scene gameplay binding node lacks required component: " +
                                                   gameplay_binding_component_kind_name(source_row.required_component));
            continue;
        }

        result.bindings.push_back(RuntimeSceneGameplayBindingRow{
            .binding_id = source_row.binding_id,
            .gameplay_system_id = source_row.gameplay_system_id,
            .slot_id = source_row.slot_id,
            .node_name = source_row.node_name,
            .node = resolved_node,
            .required_component = source_row.required_component,
        });
    }

    if (!result.diagnostics.empty()) {
        result.bindings.clear();
    }
    return result;
}

RuntimeSceneGameplayInteractionPlan
plan_runtime_scene_gameplay_interactions(std::span<const RuntimeSceneGameplayBindingRow> bindings,
                                         std::span<const RuntimeSceneGameplayInteractionSourceRow> source_rows,
                                         RuntimeSceneGameplayInteractionPlanRequest request) {
    RuntimeSceneGameplayInteractionPlan plan;
    plan.final_session_state = request.session_state;
    plan.rows.reserve(source_rows.size());

    std::unordered_map<std::string, const RuntimeSceneGameplayBindingRow*> binding_lookup;
    binding_lookup.reserve(bindings.size());
    for (const auto& binding : bindings) {
        if (!binding.binding_id.empty() && !binding_lookup.contains(binding.binding_id)) {
            binding_lookup.emplace(binding.binding_id, &binding);
        }
    }

    std::unordered_set<std::string> action_ids;
    action_ids.reserve(source_rows.size());
    RuntimeSceneGameplaySessionState current_state = request.session_state;

    for (const auto& source_row : source_rows) {
        bool source_row_valid = true;
        if (source_row.action_id.empty()) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::invalid_action_id, source_row,
                current_state, "runtime scene gameplay interaction action id is empty");
            source_row_valid = false;
        } else if (!action_ids.insert(source_row.action_id).second) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::duplicate_action_id, source_row,
                current_state, "runtime scene gameplay interaction action id is duplicated: " + source_row.action_id);
            source_row_valid = false;
        }
        if (source_row.source_binding_id.empty()) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::invalid_source_binding_id, source_row,
                current_state, "runtime scene gameplay interaction source binding id is empty");
            source_row_valid = false;
        }
        const bool requires_target = gameplay_interaction_requires_target(source_row.kind);
        if (requires_target && source_row.target_binding_id.empty()) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::invalid_target_binding_id, source_row,
                current_state, "runtime scene gameplay interaction target binding id is empty");
            source_row_valid = false;
        }
        if (gameplay_interaction_requires_objective(source_row.kind) && source_row.objective_id.empty()) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::missing_objective_id, source_row,
                current_state, "runtime scene gameplay interaction objective id is empty");
            source_row_valid = false;
        }
        if (gameplay_interaction_requires_positive_amount(source_row.kind) && source_row.amount <= 0) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::invalid_amount, source_row,
                current_state, "runtime scene gameplay interaction amount must be positive");
            source_row_valid = false;
        }
        if (current_state != RuntimeSceneGameplaySessionState::running &&
            source_row.kind != RuntimeSceneGameplayInteractionKind::restart) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::rejected_transition, source_row,
                current_state, "runtime scene gameplay interaction is rejected after terminal session state");
            source_row_valid = false;
        }

        const auto source_binding = binding_lookup.find(source_row.source_binding_id);
        if (!source_row.source_binding_id.empty() && source_binding == binding_lookup.end()) {
            append_gameplay_interaction_diagnostic(
                plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::missing_source_binding, source_row,
                current_state,
                "runtime scene gameplay interaction source binding is missing: " + source_row.source_binding_id);
            source_row_valid = false;
        }

        const RuntimeSceneGameplayBindingRow* target_binding = nullptr;
        if (!source_row.target_binding_id.empty()) {
            const auto target_binding_iter = binding_lookup.find(source_row.target_binding_id);
            if (target_binding_iter == binding_lookup.end()) {
                append_gameplay_interaction_diagnostic(
                    plan.diagnostics, RuntimeSceneGameplayInteractionDiagnosticCode::missing_target_binding, source_row,
                    current_state,
                    "runtime scene gameplay interaction target binding is missing: " + source_row.target_binding_id);
                source_row_valid = false;
            } else {
                target_binding = target_binding_iter->second;
            }
        }

        if (!source_row_valid) {
            continue;
        }

        const auto resulting_state = gameplay_interaction_resulting_session_state(source_row.kind, current_state);
        plan.rows.push_back(RuntimeSceneGameplayInteractionRow{
            .action_id = source_row.action_id,
            .kind = source_row.kind,
            .source_binding_id = source_row.source_binding_id,
            .target_binding_id = source_row.target_binding_id,
            .source_node = source_binding->second->node,
            .target_node = target_binding != nullptr ? target_binding->node : null_scene_node,
            .objective_id = source_row.objective_id,
            .amount = source_row.amount,
            .resulting_session_state = resulting_state,
        });
        current_state = resulting_state;
    }

    plan.final_session_state = current_state;
    if (!plan.diagnostics.empty()) {
        plan.rows.clear();
        plan.final_session_state = request.session_state;
    }
    return plan;
}

RuntimeSceneConstructionPlacementIntentPlan plan_runtime_scene_construction_placement_intents(
    const runtime::RuntimeConstructionPlacementValidationResult& placement,
    const std::span<const RuntimeSceneConstructionPlacementIntentDesc> source_rows,
    const RuntimeSceneConstructionPlacementIntentContext context) {
    RuntimeSceneConstructionPlacementIntentPlan plan;
    plan.rows.reserve(source_rows.size());

    std::vector<std::string> seen_node_names;
    seen_node_names.reserve(source_rows.size());
    std::vector<RuntimeSceneConstructionPlacementOccupiedCell> planned_occupied_cells;

    for (const auto& source_row : source_rows) {
        RuntimeSceneConstructionPlacementIntentRow row{
            .candidate_index = source_row.candidate_index,
            .status = RuntimeSceneConstructionPlacementIntentStatus::invalid,
            .node_name = source_row.node_name,
            .transform = source_row.transform,
            .components = source_row.components,
        };

        if (!placement.succeeded || !placement.diagnostics.empty()) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_placement_validation,
                plan.rows.back(), "construction placement validation did not succeed");
            continue;
        }

        auto candidate_rows = find_construction_placement_candidate_rows(placement, source_row.candidate_index);
        if (!candidate_rows.found) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::missing_candidate, plan.rows.back(),
                "construction placement candidate row is missing");
            continue;
        }
        row.item_id = candidate_rows.candidate.item_id;
        row.placement_id = candidate_rows.candidate.placement_id;
        row.surface_id = candidate_rows.candidate.surface_id;
        row.occupied_cells = std::move(candidate_rows.occupied_cells);

        if (!source_row.reviewed) {
            row.status = RuntimeSceneConstructionPlacementIntentStatus::blocked;
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::placement_not_reviewed, plan.rows.back(),
                "construction placement scene intent was not reviewed");
            continue;
        }
        if (!is_line_text_value(source_row.node_name)) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_node_name, plan.rows.back(),
                "construction placement scene node name is invalid");
            continue;
        }
        if (std::ranges::find(seen_node_names, source_row.node_name) != seen_node_names.end()) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::duplicate_intent_node_name,
                plan.rows.back(), "construction placement scene node name is duplicated in the intent rows");
            continue;
        }
        if (contains_string(context.existing_node_names, source_row.node_name)) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::duplicate_scene_node_name,
                plan.rows.back(), "construction placement scene node name already exists");
            continue;
        }
        seen_node_names.push_back(source_row.node_name);

        if (!is_valid_scene_node_components(source_row.components)) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_components, plan.rows.back(),
                "construction placement scene components are invalid");
            continue;
        }
        if (!is_valid_scene_transform(source_row.transform)) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_transform, plan.rows.back(),
                "construction placement scene transform is invalid");
            continue;
        }
        if (!matches_candidate_world_position(source_row.transform, candidate_rows.candidate)) {
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::mismatched_transform_position,
                plan.rows.back(),
                "construction placement scene transform position does not match validation world position");
            continue;
        }
        if (const auto* occupied_cell = find_occupied_cell(context.occupied_cells, row.occupied_cells);
            occupied_cell != nullptr) {
            row.status = RuntimeSceneConstructionPlacementIntentStatus::already_occupied;
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::already_occupied, plan.rows.back(),
                "construction placement occupied cell already has a scene node", occupied_cell);
            continue;
        }
        if (const auto* occupied_cell = find_occupied_cell(planned_occupied_cells, row.occupied_cells);
            occupied_cell != nullptr) {
            row.status = RuntimeSceneConstructionPlacementIntentStatus::already_occupied;
            plan.rows.push_back(std::move(row));
            append_construction_placement_intent_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::already_occupied, plan.rows.back(),
                "construction placement occupied cell already has an accepted intent row", occupied_cell);
            continue;
        }

        row.status = RuntimeSceneConstructionPlacementIntentStatus::accepted;
        plan.rows.push_back(std::move(row));
        const auto& accepted_row = plan.rows.back();
        for (const auto& cell : accepted_row.occupied_cells) {
            planned_occupied_cells.push_back(RuntimeSceneConstructionPlacementOccupiedCell{
                .cell = cell,
                .node = null_scene_node,
                .node_name = accepted_row.node_name,
            });
        }
    }

    return plan;
}

RuntimeSceneConstructionPlacementIntentPlan plan_runtime_scene_procedural_construction_placement_intents(
    const runtime::RuntimeProceduralGenerationPlan& generation,
    const runtime::RuntimeConstructionPlacementValidationResult& placement,
    const std::span<const RuntimeSceneProceduralConstructionPlacementIntentDesc> source_rows,
    const RuntimeSceneConstructionPlacementIntentContext context) {
    RuntimeSceneConstructionPlacementIntentPlan plan;
    std::vector<RuntimeSceneConstructionPlacementIntentRow> source_indexed_rows(source_rows.size());
    std::vector<bool> source_indexed_row_written(source_rows.size(), false);
    plan.rows.reserve(source_rows.size());

    if (!generation.succeeded || !generation.diagnostics.empty()) {
        for (const auto& source_row : source_rows) {
            const auto* output_row = find_procedural_output_row(generation, source_row.procedural_output_id);
            plan.rows.push_back(make_procedural_construction_placement_row(source_row, output_row));
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::invalid_procedural_generation,
                plan.rows.back(), "procedural generation plan did not succeed");
        }
        return plan;
    }

    std::vector<std::string> seen_source_output_ids;
    seen_source_output_ids.reserve(source_rows.size());
    std::vector<RuntimeSceneConstructionPlacementIntentDesc> forwarded_rows;
    forwarded_rows.reserve(source_rows.size());
    std::vector<std::size_t> forwarded_source_indices;
    forwarded_source_indices.reserve(source_rows.size());

    for (std::size_t source_index = 0; source_index < source_rows.size(); ++source_index) {
        const auto& source_row = source_rows[source_index];
        const auto* output_row = find_procedural_output_row(generation, source_row.procedural_output_id);
        auto row = make_procedural_construction_placement_row(source_row, output_row);
        const auto duplicate_source_output =
            std::ranges::find(seen_source_output_ids, source_row.procedural_output_id) != seen_source_output_ids.end();
        seen_source_output_ids.push_back(source_row.procedural_output_id);

        if (output_row == nullptr) {
            source_indexed_rows[source_index] = std::move(row);
            source_indexed_row_written[source_index] = true;
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::missing_procedural_output,
                source_indexed_rows[source_index], "procedural output row is missing");
            continue;
        }
        if (has_duplicate_procedural_output_id(generation, source_row.procedural_output_id) ||
            duplicate_source_output) {
            source_indexed_rows[source_index] = std::move(row);
            source_indexed_row_written[source_index] = true;
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::duplicate_procedural_output,
                source_indexed_rows[source_index], "procedural output id is duplicated");
            continue;
        }
        if (!supports_procedural_scene_placement_kind(output_row->kind)) {
            source_indexed_rows[source_index] = std::move(row);
            source_indexed_row_written[source_index] = true;
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::unsupported_procedural_output_kind,
                source_indexed_rows[source_index], "procedural output kind cannot become a scene placement intent");
            continue;
        }
        if (source_row.anchor_id.empty()) {
            source_indexed_rows[source_index] = std::move(row);
            source_indexed_row_written[source_index] = true;
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::missing_procedural_anchor,
                source_indexed_rows[source_index], "procedural placement anchor id is missing");
            continue;
        }
        if (!source_row.package_visible) {
            source_indexed_rows[source_index] = std::move(row);
            source_indexed_row_written[source_index] = true;
            append_procedural_construction_placement_diagnostic(
                plan, RuntimeSceneConstructionPlacementIntentDiagnosticCode::package_invisible_procedural_output,
                source_indexed_rows[source_index],
                "procedural placement output is not visible in runtime package evidence");
            continue;
        }

        forwarded_rows.push_back(make_construction_placement_intent_desc(source_row));
        forwarded_source_indices.push_back(source_index);
    }

    if (!forwarded_rows.empty()) {
        auto placement_plan = plan_runtime_scene_construction_placement_intents(placement, forwarded_rows, context);
        apply_procedural_construction_placement_metadata(placement_plan, source_rows, forwarded_source_indices,
                                                         generation);
        for (std::size_t row_index = 0U;
             row_index < placement_plan.rows.size() && row_index < forwarded_source_indices.size(); ++row_index) {
            const auto source_index = forwarded_source_indices[row_index];
            source_indexed_rows[source_index] = std::move(placement_plan.rows[row_index]);
            source_indexed_row_written[source_index] = true;
        }
        for (auto& diagnostic : placement_plan.diagnostics) {
            plan.diagnostics.push_back(std::move(diagnostic));
        }
    }

    for (std::size_t source_index = 0; source_index < source_rows.size(); ++source_index) {
        if (source_indexed_row_written[source_index]) {
            plan.rows.push_back(std::move(source_indexed_rows[source_index]));
        }
    }
    return plan;
}

RuntimeSceneAnimationTransformApplyResult
apply_runtime_scene_animation_transform_samples(RuntimeSceneInstance& instance,
                                                const AnimationTransformBindingSourceDocument& binding_source,
                                                std::span<const FloatAnimationCurveSample> samples) {
    auto resolution = resolve_runtime_scene_animation_transform_bindings(instance, binding_source);
    if (!resolution.succeeded()) {
        return RuntimeSceneAnimationTransformApplyResult{
            .succeeded = false,
            .diagnostic = "animation transform binding resolution failed",
            .applied_sample_count = 0,
            .binding_diagnostics = std::move(resolution.diagnostics),
        };
    }

    const auto& nodes = instance.scene.nodes();
    std::vector<Transform3D> transforms;
    transforms.reserve(nodes.size());
    for (const auto& node : nodes) {
        transforms.push_back(node.transform);
    }

    const auto apply_result = apply_float_animation_samples_to_transform3d(samples, resolution.bindings, transforms);
    if (!apply_result.succeeded) {
        return RuntimeSceneAnimationTransformApplyResult{
            .succeeded = false,
            .diagnostic = apply_result.diagnostic,
            .applied_sample_count = 0,
            .binding_diagnostics = {},
        };
    }

    for (std::size_t index = 0; index < transforms.size(); ++index) {
        auto* node = instance.scene.find_node(SceneNodeId{static_cast<std::uint32_t>(index + 1U)});
        if (node != nullptr) {
            node->transform = transforms[index];
        }
    }

    return RuntimeSceneAnimationTransformApplyResult{
        .succeeded = true,
        .diagnostic = {},
        .applied_sample_count = apply_result.applied_sample_count,
        .binding_diagnostics = {},
    };
}

RuntimeSceneAnimationTransformApplyResult apply_runtime_scene_animation_pose_3d(RuntimeSceneInstance& instance,
                                                                                const AnimationSkeleton3dDesc& skeleton,
                                                                                const AnimationPose3d& pose) {
    if (!validate_animation_skeleton_3d_desc(skeleton).empty()) {
        return RuntimeSceneAnimationTransformApplyResult{
            .succeeded = false,
            .diagnostic = "animation 3D skeleton is invalid",
            .applied_sample_count = 0,
            .binding_diagnostics = {},
        };
    }
    if (!validate_animation_pose_3d(skeleton, pose).empty()) {
        return RuntimeSceneAnimationTransformApplyResult{
            .succeeded = false,
            .diagnostic = "animation 3D pose is invalid",
            .applied_sample_count = 0,
            .binding_diagnostics = {},
        };
    }

    std::vector<SceneNodeId> resolved_nodes;
    resolved_nodes.reserve(skeleton.joints.size());
    std::vector<RuntimeSceneAnimationTransformBindingDiagnostic> diagnostics;
    for (const auto& joint : skeleton.joints) {
        const auto matches = find_runtime_scene_nodes_by_name(instance, joint.name);
        if (matches.empty()) {
            append_animation_binding_diagnostic(diagnostics,
                                                RuntimeSceneAnimationTransformBindingDiagnosticCode::missing_node,
                                                joint.name, joint.name, AnimationTransformBindingComponent::unknown,
                                                "animation 3D pose binding node is missing: " + joint.name);
            continue;
        }
        if (matches.size() > 1U) {
            append_animation_binding_diagnostic(
                diagnostics, RuntimeSceneAnimationTransformBindingDiagnosticCode::duplicate_node_name, joint.name,
                joint.name, AnimationTransformBindingComponent::unknown,
                "animation 3D pose binding node name is ambiguous: " + joint.name);
            continue;
        }
        resolved_nodes.push_back(matches.front());
    }

    if (!diagnostics.empty()) {
        return RuntimeSceneAnimationTransformApplyResult{
            .succeeded = false,
            .diagnostic = "animation 3D pose binding resolution failed",
            .applied_sample_count = 0,
            .binding_diagnostics = std::move(diagnostics),
        };
    }

    for (std::size_t index = 0; index < resolved_nodes.size(); ++index) {
        auto* node = instance.scene.find_node(resolved_nodes[index]);
        if (node == nullptr) {
            return RuntimeSceneAnimationTransformApplyResult{
                .succeeded = false,
                .diagnostic = "animation 3D pose resolved node is missing",
                .applied_sample_count = index,
                .binding_diagnostics = {},
            };
        }
        const auto& joint_pose = pose.joints[index];
        node->transform.position = joint_pose.translation;
        node->transform.rotation_radians = euler_radians_from_quat(joint_pose.rotation);
        node->transform.scale = joint_pose.scale;
    }

    return RuntimeSceneAnimationTransformApplyResult{
        .succeeded = true,
        .diagnostic = {},
        .applied_sample_count = resolved_nodes.size(),
        .binding_diagnostics = {},
    };
}

} // namespace mirakana::runtime_scene
