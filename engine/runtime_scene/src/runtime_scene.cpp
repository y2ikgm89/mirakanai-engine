// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime_scene/runtime_scene.hpp"

#include <algorithm>
#include <cmath>
#include <exception>
#include <ranges>
#include <stdexcept>
#include <unordered_map>
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
        const bool mesh_reference_ok = reference.kind == RuntimeSceneReferenceKind::mesh &&
                                       reference.expected_kind == AssetKind::mesh &&
                                       (record->kind == AssetKind::mesh || record->kind == AssetKind::skinned_mesh);
        if (!mesh_reference_ok && record->kind != reference.expected_kind) {
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
