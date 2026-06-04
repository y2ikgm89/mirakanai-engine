// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/math/transform.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AuthoringId {
    std::string value;
};

struct SceneComponentTypeId {
    std::string value;
};

struct SceneComponentPropertyV2 {
    std::string name;
    std::string value;
};

struct SceneNodeDocumentV2 {
    AuthoringId id;
    std::string name;
    Transform3D transform;
    AuthoringId parent;
};

struct SceneComponentDocumentV2 {
    AuthoringId id;
    AuthoringId node;
    SceneComponentTypeId type;
    std::vector<SceneComponentPropertyV2> properties;
};

struct SceneNodePrefabSourceV2 {
    AuthoringId node;
    std::string prefab_path;
    AuthoringId source_node_id;
};

struct SceneComponentPrefabSourceV2 {
    AuthoringId component;
    std::string prefab_path;
    AuthoringId source_component_id;
};

struct SceneEnvironmentDocumentV2 {
    AssetKeyV2 profile;
    bool required{false};
};

struct SceneDocumentV2 {
    std::string name;
    std::optional<SceneEnvironmentDocumentV2> environment;
    std::vector<SceneNodeDocumentV2> nodes;
    std::vector<SceneComponentDocumentV2> components;
    std::vector<SceneNodePrefabSourceV2> node_prefab_sources;
    std::vector<SceneComponentPrefabSourceV2> component_prefab_sources;
};

struct PrefabDocumentV2 {
    std::string name;
    SceneDocumentV2 scene;
};

struct PrefabOverridePathV2 {
    std::string value;
};

struct PrefabOverrideV2 {
    PrefabOverridePathV2 path;
    std::string value;
};

struct PrefabVariantDocumentV2 {
    std::string name;
    PrefabDocumentV2 base_prefab;
    std::vector<PrefabOverrideV2> overrides;
};

enum class SceneSchemaV2DiagnosticCode : std::uint8_t {
    invalid_scene_name,
    invalid_authoring_id,
    duplicate_node_id,
    duplicate_component_id,
    missing_parent_node,
    missing_component_node,
    invalid_component_type,
    duplicate_component_property,
    invalid_component_property,
    invalid_text_value,
    invalid_transform,
    missing_override_target,
    duplicate_override_path,
    duplicate_prefab_source_identity,
    unsupported_nested_prefab_instance,
    unsupported_local_prefab_child,
    unsupported_local_prefab_component,
    invalid_environment_profile_reference,
};

struct SceneSchemaV2Diagnostic {
    SceneSchemaV2DiagnosticCode code{SceneSchemaV2DiagnosticCode::invalid_scene_name};
    AuthoringId node;
    AuthoringId component;
    SceneComponentTypeId component_type;
    std::string property;
};

struct PrefabVariantComposeResultV2 {
    bool success{false};
    PrefabDocumentV2 prefab;
    std::vector<SceneSchemaV2Diagnostic> diagnostics;
};

enum class ScenePrefabInstanceRefreshRowKindV2 : std::uint8_t {
    preserve_node,
    add_source_node,
    remove_stale_node,
    preserve_component,
    add_source_component,
    remove_stale_component,
};

struct ScenePrefabInstanceRefreshRowV2 {
    ScenePrefabInstanceRefreshRowKindV2 kind{ScenePrefabInstanceRefreshRowKindV2::preserve_node};
    AuthoringId current_node;
    AuthoringId current_component;
    AuthoringId source_node_id;
    AuthoringId source_component_id;
    std::string prefab_path;
};

struct ScenePrefabInstanceRefreshPlanV2 {
    bool valid{false};
    bool mutates{false};
    bool executes{false};
    AuthoringId instance_root_node;
    std::string prefab_path;
    std::size_t preserve_node_count{0};
    std::size_t add_node_count{0};
    std::size_t remove_node_count{0};
    std::size_t preserve_component_count{0};
    std::size_t add_component_count{0};
    std::size_t remove_component_count{0};
    std::vector<ScenePrefabInstanceRefreshRowV2> rows;
    std::vector<SceneSchemaV2Diagnostic> diagnostics;
};

struct ScenePrefabInstanceRefreshNodeMappingV2 {
    AuthoringId source_node_id;
    AuthoringId result_node;
};

struct ScenePrefabInstanceRefreshComponentMappingV2 {
    AuthoringId source_component_id;
    AuthoringId result_component;
};

struct ScenePrefabInstanceRefreshResultV2 {
    bool applied{false};
    bool mutates{false};
    bool executes{false};
    std::size_t preserved_node_count{0};
    std::size_t added_node_count{0};
    std::size_t removed_node_count{0};
    std::size_t preserved_component_count{0};
    std::size_t added_component_count{0};
    std::size_t removed_component_count{0};
    SceneDocumentV2 scene;
    std::vector<ScenePrefabInstanceRefreshNodeMappingV2> source_to_result_node_ids;
    std::vector<ScenePrefabInstanceRefreshComponentMappingV2> source_to_result_component_ids;
    std::vector<SceneSchemaV2Diagnostic> diagnostics;
};

[[nodiscard]] std::vector<SceneSchemaV2Diagnostic> validate_scene_document_v2(const SceneDocumentV2& scene);
[[nodiscard]] std::string serialize_scene_document_v2(const SceneDocumentV2& scene);
[[nodiscard]] SceneDocumentV2 deserialize_scene_document_v2(std::string_view text);
[[nodiscard]] std::vector<SceneSchemaV2Diagnostic> validate_prefab_document_v2(const PrefabDocumentV2& prefab);
[[nodiscard]] std::string serialize_prefab_document_v2(const PrefabDocumentV2& prefab);
[[nodiscard]] PrefabDocumentV2 deserialize_prefab_document_v2(std::string_view text);
[[nodiscard]] PrefabVariantComposeResultV2 compose_prefab_variant_v2(const PrefabVariantDocumentV2& variant);
[[nodiscard]] ScenePrefabInstanceRefreshPlanV2
plan_scene_prefab_instance_refresh_v2(const SceneDocumentV2& scene, const AuthoringId& instance_root_node,
                                      const PrefabDocumentV2& refreshed_prefab);
[[nodiscard]] ScenePrefabInstanceRefreshResultV2
apply_scene_prefab_instance_refresh_v2(const SceneDocumentV2& scene, const AuthoringId& instance_root_node,
                                       const PrefabDocumentV2& refreshed_prefab);

} // namespace mirakana
