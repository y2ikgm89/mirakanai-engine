// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/transform.hpp"

#include <cstddef>
#include <cstdint>
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

struct SceneComponentProperty {
    std::string name;
    std::string value;
};

struct SceneNodeDocument {
    AuthoringId id;
    std::string name;
    Transform3D transform;
    AuthoringId parent;
};

struct SceneComponentDocument {
    AuthoringId id;
    AuthoringId node;
    SceneComponentTypeId type;
    std::vector<SceneComponentProperty> properties;
};

struct SceneNodePrefabSource {
    AuthoringId node;
    std::string prefab_path;
    AuthoringId source_node_id;
};

struct SceneComponentPrefabSource {
    AuthoringId component;
    std::string prefab_path;
    AuthoringId source_component_id;
};

struct SceneDocument {
    std::string name;
    std::vector<SceneNodeDocument> nodes;
    std::vector<SceneComponentDocument> components;
    std::vector<SceneNodePrefabSource> node_prefab_sources;
    std::vector<SceneComponentPrefabSource> component_prefab_sources;
};

struct PrefabDocument {
    std::string name;
    SceneDocument scene;
};

struct PrefabOverridePath {
    std::string value;
};

struct PrefabOverride {
    PrefabOverridePath path;
    std::string value;
};

struct PrefabVariantDocument {
    std::string name;
    PrefabDocument base_prefab;
    std::vector<PrefabOverride> overrides;
};

enum class SceneSchemaDiagnosticCode : std::uint8_t {
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
};

struct SceneSchemaDiagnostic {
    SceneSchemaDiagnosticCode code{SceneSchemaDiagnosticCode::invalid_scene_name};
    AuthoringId node;
    AuthoringId component;
    SceneComponentTypeId component_type;
    std::string property;
};

struct PrefabVariantComposeResult {
    bool success{false};
    PrefabDocument prefab;
    std::vector<SceneSchemaDiagnostic> diagnostics;
};

enum class ScenePrefabInstanceRefreshRowKind : std::uint8_t {
    preserve_node,
    add_source_node,
    remove_stale_node,
    preserve_component,
    add_source_component,
    remove_stale_component,
};

struct ScenePrefabInstanceRefreshRow {
    ScenePrefabInstanceRefreshRowKind kind{ScenePrefabInstanceRefreshRowKind::preserve_node};
    AuthoringId current_node;
    AuthoringId current_component;
    AuthoringId source_node_id;
    AuthoringId source_component_id;
    std::string prefab_path;
};

struct ScenePrefabInstanceRefreshPlan {
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
    std::vector<ScenePrefabInstanceRefreshRow> rows;
    std::vector<SceneSchemaDiagnostic> diagnostics;
};

struct ScenePrefabInstanceRefreshNodeMapping {
    AuthoringId source_node_id;
    AuthoringId result_node;
};

struct ScenePrefabInstanceRefreshComponentMapping {
    AuthoringId source_component_id;
    AuthoringId result_component;
};

struct ScenePrefabInstanceRefreshResult {
    bool applied{false};
    bool mutates{false};
    bool executes{false};
    std::size_t preserved_node_count{0};
    std::size_t added_node_count{0};
    std::size_t removed_node_count{0};
    std::size_t preserved_component_count{0};
    std::size_t added_component_count{0};
    std::size_t removed_component_count{0};
    SceneDocument scene;
    std::vector<ScenePrefabInstanceRefreshNodeMapping> source_to_result_node_ids;
    std::vector<ScenePrefabInstanceRefreshComponentMapping> source_to_result_component_ids;
    std::vector<SceneSchemaDiagnostic> diagnostics;
};

[[nodiscard]] std::vector<SceneSchemaDiagnostic> validate_scene_document(const SceneDocument& scene);
[[nodiscard]] std::string serialize_scene_document(const SceneDocument& scene);
[[nodiscard]] SceneDocument deserialize_scene_document(std::string_view text);
[[nodiscard]] std::vector<SceneSchemaDiagnostic> validate_prefab_document(const PrefabDocument& prefab);
[[nodiscard]] std::string serialize_prefab_document(const PrefabDocument& prefab);
[[nodiscard]] PrefabDocument deserialize_prefab_document(std::string_view text);
[[nodiscard]] PrefabVariantComposeResult compose_prefab_variant(const PrefabVariantDocument& variant);
[[nodiscard]] ScenePrefabInstanceRefreshPlan plan_scene_prefab_instance_refresh(const SceneDocument& scene,
                                                                                const AuthoringId& instance_root_node,
                                                                                const PrefabDocument& refreshed_prefab);
[[nodiscard]] ScenePrefabInstanceRefreshResult
apply_scene_prefab_instance_refresh(const SceneDocument& scene, const AuthoringId& instance_root_node,
                                    const PrefabDocument& refreshed_prefab);

} // namespace mirakana
