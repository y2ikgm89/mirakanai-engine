// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/animation/keyframe_animation.hpp"
#include "mirakana/animation/skeleton.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/runtime/asset_runtime.hpp"
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
    AssetKeyV2 key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string source_path;
};

struct RuntimeSceneAssetIdentityDiagnostic {
    RuntimeSceneAssetIdentityDiagnosticCode code{RuntimeSceneAssetIdentityDiagnosticCode::missing_identity};
    std::string placement;
    SceneNodeId node;
    AssetId asset;
    AssetKeyV2 key;
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

struct RuntimeSceneAnimationTransformApplyResult {
    bool succeeded{false};
    std::string diagnostic;
    std::size_t applied_sample_count{0};
    std::vector<RuntimeSceneAnimationTransformBindingDiagnostic> binding_diagnostics;
};

[[nodiscard]] RuntimeSceneLoadResult instantiate_runtime_scene(const runtime::RuntimeAssetPackage& package,
                                                               AssetId scene, RuntimeSceneLoadOptions options = {});

[[nodiscard]] RuntimeSceneAssetIdentityAudit
audit_runtime_scene_asset_identity(const Scene& scene, const AssetIdentityDocumentV2& identities);

[[nodiscard]] std::vector<SceneNodeId> find_runtime_scene_nodes_by_name(const RuntimeSceneInstance& instance,
                                                                        std::string_view name);

[[nodiscard]] RuntimeSceneAnimationTransformBindingResolution
resolve_runtime_scene_animation_transform_bindings(const RuntimeSceneInstance& instance,
                                                   const AnimationTransformBindingSourceDocument& binding_source);

[[nodiscard]] RuntimeSceneAnimationTransformApplyResult
apply_runtime_scene_animation_transform_samples(RuntimeSceneInstance& instance,
                                                const AnimationTransformBindingSourceDocument& binding_source,
                                                std::span<const FloatAnimationCurveSample> samples);
[[nodiscard]] RuntimeSceneAnimationTransformApplyResult
apply_runtime_scene_animation_pose_3d(RuntimeSceneInstance& instance, const AnimationSkeleton3dDesc& skeleton,
                                      const AnimationPose3d& pose);

} // namespace mirakana::runtime_scene
