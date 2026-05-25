// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
#include "mirakana/runtime/inventory_items.hpp"
#include "mirakana/runtime/procedural_generation.hpp"
#include "mirakana/scene/scene.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana::runtime_scene {

enum class RuntimeSceneDiagnosticCode : std::uint8_t {
    none,
    missing_scene_asset,
    wrong_asset_kind,
    malformed_scene_payload,
    missing_referenced_asset,
    referenced_asset_kind_mismatch,
    duplicate_node_name,
};

enum class RuntimeSceneReferenceKind : std::uint8_t { mesh, material, sprite };

struct RuntimeSceneReference {
    SceneNodeId node;
    AssetId asset;
    RuntimeSceneReferenceKind kind{RuntimeSceneReferenceKind::mesh};
    AssetKind expected_kind{AssetKind::unknown};
};

enum class RuntimeSceneAssetIdentityDiagnosticCode : std::uint8_t {
    invalid_identity_document,
    missing_identity,
    kind_mismatch,
};

struct RuntimeSceneAssetIdentityReferenceRow {
    std::string placement;
    SceneNodeId node;
    AssetId asset;
    AssetKey key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string source_path;
};

struct RuntimeSceneAssetIdentityDiagnostic {
    RuntimeSceneAssetIdentityDiagnosticCode code{RuntimeSceneAssetIdentityDiagnosticCode::missing_identity};
    std::string placement;
    SceneNodeId node;
    AssetId asset;
    AssetKey key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string source_path;
    std::string message;
};

struct RuntimeSceneAssetIdentityAudit {
    std::vector<RuntimeSceneAssetIdentityReferenceRow> references;
    std::vector<RuntimeSceneAssetIdentityDiagnostic> diagnostics;
};

struct RuntimeSceneDiagnostic {
    RuntimeSceneDiagnosticCode code{RuntimeSceneDiagnosticCode::none};
    AssetId asset;
    SceneNodeId node;
    RuntimeSceneReferenceKind reference_kind{RuntimeSceneReferenceKind::mesh};
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string message;
};

struct RuntimeSceneInstance {
    AssetId scene_asset;
    runtime::RuntimeAssetHandle handle;
    Scene scene{""};
    std::vector<RuntimeSceneReference> references;
};

struct RuntimeSceneLoadOptions {
    bool validate_asset_references{true};
    bool require_unique_node_names{false};
};

struct RuntimeSceneLoadResult {
    std::optional<RuntimeSceneInstance> instance;
    std::vector<RuntimeSceneDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeSceneAnimationTransformBindingDiagnosticCode : std::uint8_t {
    none,
    invalid_binding_document,
    missing_node,
    duplicate_node_name,
};

struct RuntimeSceneAnimationTransformBindingDiagnostic {
    RuntimeSceneAnimationTransformBindingDiagnosticCode code{RuntimeSceneAnimationTransformBindingDiagnosticCode::none};
    std::string target;
    std::string node_name;
    AnimationTransformBindingComponent component{AnimationTransformBindingComponent::unknown};
    std::string message;
};

struct RuntimeSceneAnimationTransformBindingResolution {
    std::vector<AnimationTransformCurveBinding> bindings;
    std::vector<RuntimeSceneAnimationTransformBindingDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeSceneGameplayBindingComponentKind : std::uint8_t {
    none,
    any_renderable,
    camera,
    light,
    mesh_renderer,
    sprite_renderer,
};

enum class RuntimeSceneGameplayBindingDiagnosticCode : std::uint8_t {
    none,
    invalid_binding_id,
    invalid_gameplay_system_id,
    invalid_slot_id,
    invalid_node_name,
    duplicate_binding_id,
    missing_node,
    duplicate_node_name,
    missing_required_component,
};

struct RuntimeSceneGameplayBindingSourceRow {
    std::string binding_id;
    std::string gameplay_system_id;
    std::string slot_id;
    std::string node_name;
    RuntimeSceneGameplayBindingComponentKind required_component{RuntimeSceneGameplayBindingComponentKind::none};
};

struct RuntimeSceneGameplayBindingRow {
    std::string binding_id;
    std::string gameplay_system_id;
    std::string slot_id;
    std::string node_name;
    SceneNodeId node{null_scene_node};
    RuntimeSceneGameplayBindingComponentKind required_component{RuntimeSceneGameplayBindingComponentKind::none};
};

struct RuntimeSceneGameplayBindingDiagnostic {
    RuntimeSceneGameplayBindingDiagnosticCode code{RuntimeSceneGameplayBindingDiagnosticCode::none};
    std::string binding_id;
    std::string gameplay_system_id;
    std::string slot_id;
    std::string node_name;
    SceneNodeId node{null_scene_node};
    RuntimeSceneGameplayBindingComponentKind required_component{RuntimeSceneGameplayBindingComponentKind::none};
    std::string message;
};

struct RuntimeSceneGameplayBindingResolution {
    std::vector<RuntimeSceneGameplayBindingRow> bindings;
    std::vector<RuntimeSceneGameplayBindingDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeSceneGameplayInteractionKind : std::uint8_t {
    trigger,
    pickup,
    damage,
    heal,
    objective_progress,
    objective_complete,
    win,
    loss,
    restart,
};

enum class RuntimeSceneGameplaySessionState : std::uint8_t {
    running,
    won,
    lost,
};

enum class RuntimeSceneGameplayInteractionDiagnosticCode : std::uint8_t {
    none,
    invalid_action_id,
    duplicate_action_id,
    invalid_source_binding_id,
    invalid_target_binding_id,
    missing_source_binding,
    missing_target_binding,
    missing_objective_id,
    invalid_amount,
    rejected_transition,
};

struct RuntimeSceneGameplayInteractionSourceRow {
    std::string action_id;
    RuntimeSceneGameplayInteractionKind kind{RuntimeSceneGameplayInteractionKind::trigger};
    std::string source_binding_id;
    std::string target_binding_id;
    std::string objective_id;
    int amount{0};
};

struct RuntimeSceneGameplayInteractionPlanRequest {
    RuntimeSceneGameplaySessionState session_state{RuntimeSceneGameplaySessionState::running};
};

struct RuntimeSceneGameplayInteractionRow {
    std::string action_id;
    RuntimeSceneGameplayInteractionKind kind{RuntimeSceneGameplayInteractionKind::trigger};
    std::string source_binding_id;
    std::string target_binding_id;
    SceneNodeId source_node{null_scene_node};
    SceneNodeId target_node{null_scene_node};
    std::string objective_id;
    int amount{0};
    RuntimeSceneGameplaySessionState resulting_session_state{RuntimeSceneGameplaySessionState::running};
};

struct RuntimeSceneGameplayInteractionDiagnostic {
    RuntimeSceneGameplayInteractionDiagnosticCode code{RuntimeSceneGameplayInteractionDiagnosticCode::none};
    std::string action_id;
    RuntimeSceneGameplayInteractionKind kind{RuntimeSceneGameplayInteractionKind::trigger};
    std::string source_binding_id;
    std::string target_binding_id;
    std::string objective_id;
    int amount{0};
    RuntimeSceneGameplaySessionState session_state{RuntimeSceneGameplaySessionState::running};
    std::string message;
};

struct RuntimeSceneGameplayInteractionPlan {
    std::vector<RuntimeSceneGameplayInteractionRow> rows;
    std::vector<RuntimeSceneGameplayInteractionDiagnostic> diagnostics;
    RuntimeSceneGameplaySessionState final_session_state{RuntimeSceneGameplaySessionState::running};

    [[nodiscard]] bool succeeded() const noexcept;
};

enum class RuntimeSceneConstructionPlacementIntentStatus : std::uint8_t {
    accepted,
    blocked,
    invalid,
    already_occupied,
};

enum class RuntimeSceneConstructionPlacementIntentDiagnosticCode : std::uint8_t {
    none,
    invalid_placement_validation,
    missing_candidate,
    placement_not_reviewed,
    invalid_node_name,
    duplicate_intent_node_name,
    duplicate_scene_node_name,
    invalid_components,
    invalid_transform,
    mismatched_transform_position,
    already_occupied,
    invalid_procedural_generation,
    missing_procedural_output,
    duplicate_procedural_output,
    missing_procedural_anchor,
    unsupported_procedural_output_kind,
    package_invisible_procedural_output,
};

struct RuntimeSceneConstructionPlacementOccupiedCell {
    runtime::RuntimeConstructionPlacementCellDesc cell;
    SceneNodeId node{null_scene_node};
    std::string node_name;
};

struct RuntimeSceneConstructionPlacementIntentContext {
    std::span<const RuntimeSceneConstructionPlacementOccupiedCell> occupied_cells;
    std::span<const std::string> existing_node_names;
};

struct RuntimeSceneConstructionPlacementIntentDesc {
    std::uint32_t candidate_index{0U};
    std::string node_name;
    Transform3D transform;
    SceneNodeComponents components;
    bool reviewed{false};
};

struct RuntimeSceneProceduralConstructionPlacementIntentDesc {
    std::string procedural_output_id;
    std::string anchor_id;
    std::uint32_t candidate_index{0U};
    std::string node_name;
    Transform3D transform;
    SceneNodeComponents components;
    bool reviewed{false};
    bool package_visible{false};
};

struct RuntimeSceneConstructionPlacementIntentRow {
    std::uint32_t candidate_index{0U};
    RuntimeSceneConstructionPlacementIntentStatus status{RuntimeSceneConstructionPlacementIntentStatus::invalid};
    std::string item_id;
    std::string placement_id;
    std::string surface_id;
    std::string node_name;
    Transform3D transform;
    SceneNodeComponents components;
    std::vector<runtime::RuntimeConstructionPlacementCellDesc> occupied_cells;
    std::string procedural_output_id;
    std::string anchor_id;
    runtime::RuntimeProceduralGenerationContentKind procedural_kind{
        runtime::RuntimeProceduralGenerationContentKind::object};
    bool package_visible{false};
};

struct RuntimeSceneConstructionPlacementIntentDiagnostic {
    RuntimeSceneConstructionPlacementIntentDiagnosticCode code{
        RuntimeSceneConstructionPlacementIntentDiagnosticCode::none};
    std::uint32_t candidate_index{0U};
    std::string item_id;
    std::string placement_id;
    std::string surface_id;
    std::string node_name;
    SceneNodeId existing_node{null_scene_node};
    std::string existing_node_name;
    std::int32_t cell_x{0};
    std::int32_t cell_y{0};
    std::int32_t cell_z{0};
    std::string message;
    std::string procedural_output_id;
    std::string anchor_id;
    runtime::RuntimeProceduralGenerationContentKind procedural_kind{
        runtime::RuntimeProceduralGenerationContentKind::object};
};

struct RuntimeSceneConstructionPlacementIntentPlan {
    std::vector<RuntimeSceneConstructionPlacementIntentRow> rows;
    std::vector<RuntimeSceneConstructionPlacementIntentDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept;
};

struct RuntimeSceneAnimationTransformApplyResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t applied_sample_count{0};
    std::vector<RuntimeSceneAnimationTransformBindingDiagnostic> binding_diagnostics;
};

[[nodiscard]] RuntimeSceneLoadResult instantiate_runtime_scene(const runtime::RuntimeAssetPackage& package,
                                                               AssetId scene, RuntimeSceneLoadOptions options = {});

[[nodiscard]] RuntimeSceneAssetIdentityAudit
audit_runtime_scene_asset_identity(const Scene& scene, const AssetIdentityDocument& identities);

[[nodiscard]] std::vector<SceneNodeId> find_runtime_scene_nodes_by_name(const RuntimeSceneInstance& instance,
                                                                        std::string_view name);

[[nodiscard]] RuntimeSceneAnimationTransformBindingResolution
resolve_runtime_scene_animation_transform_bindings(const RuntimeSceneInstance& instance,
                                                   const AnimationTransformBindingSourceDocument& binding_source);

[[nodiscard]] RuntimeSceneGameplayBindingResolution
resolve_runtime_scene_gameplay_bindings(const RuntimeSceneInstance& instance,
                                        std::span<const RuntimeSceneGameplayBindingSourceRow> source_rows);

[[nodiscard]] RuntimeSceneGameplayInteractionPlan
plan_runtime_scene_gameplay_interactions(std::span<const RuntimeSceneGameplayBindingRow> bindings,
                                         std::span<const RuntimeSceneGameplayInteractionSourceRow> source_rows,
                                         RuntimeSceneGameplayInteractionPlanRequest request = {});

[[nodiscard]] RuntimeSceneConstructionPlacementIntentPlan plan_runtime_scene_construction_placement_intents(
    const runtime::RuntimeConstructionPlacementValidationResult& placement,
    std::span<const RuntimeSceneConstructionPlacementIntentDesc> source_rows,
    RuntimeSceneConstructionPlacementIntentContext context = {});

[[nodiscard]] RuntimeSceneConstructionPlacementIntentPlan plan_runtime_scene_procedural_construction_placement_intents(
    const runtime::RuntimeProceduralGenerationPlan& generation,
    const runtime::RuntimeConstructionPlacementValidationResult& placement,
    std::span<const RuntimeSceneProceduralConstructionPlacementIntentDesc> source_rows,
    RuntimeSceneConstructionPlacementIntentContext context = {});

[[nodiscard]] RuntimeSceneAnimationTransformApplyResult
apply_runtime_scene_animation_transform_samples(RuntimeSceneInstance& instance,
                                                const AnimationTransformBindingSourceDocument& binding_source,
                                                std::span<const FloatAnimationCurveSample> samples);
[[nodiscard]] RuntimeSceneAnimationTransformApplyResult
apply_runtime_scene_animation_pose_3d(RuntimeSceneInstance& instance, const AnimationSkeleton3dDesc& skeleton,
                                      const AnimationPose3d& pose);

} // namespace mirakana::runtime_scene
