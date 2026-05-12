// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/transform.hpp"

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

struct SceneDocumentV2 {
    std::string name;
    std::vector<SceneNodeDocumentV2> nodes;
    std::vector<SceneComponentDocumentV2> components;
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

enum class SceneSchemaV2DiagnosticCode {
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

[[nodiscard]] std::vector<SceneSchemaV2Diagnostic> validate_scene_document_v2(const SceneDocumentV2& scene);
[[nodiscard]] std::string serialize_scene_document_v2(const SceneDocumentV2& scene);
[[nodiscard]] SceneDocumentV2 deserialize_scene_document_v2(std::string_view text);
[[nodiscard]] std::vector<SceneSchemaV2Diagnostic> validate_prefab_document_v2(const PrefabDocumentV2& prefab);
[[nodiscard]] std::string serialize_prefab_document_v2(const PrefabDocumentV2& prefab);
[[nodiscard]] PrefabDocumentV2 deserialize_prefab_document_v2(std::string_view text);
[[nodiscard]] PrefabVariantComposeResultV2 compose_prefab_variant_v2(const PrefabVariantDocumentV2& variant);

} // namespace mirakana
