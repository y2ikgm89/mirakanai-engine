// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/platform/filesystem.hpp"
#include "mirakana/scene/schema_v2.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class ScenePrefabAuthoringCommandKind : std::uint8_t {
    create_scene,
    add_node,
    add_or_update_component,
    create_prefab,
    instantiate_prefab,
    free_form_edit,
};

struct ScenePrefabAuthoringChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct ScenePrefabAuthoringModelMutation {
    std::string kind;
    std::string target_path;
    AuthoringId node;
    AuthoringId component;
    std::string prefab_path;
};

struct ScenePrefabAuthoringDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    AuthoringId node;
    AuthoringId component;
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct ScenePrefabAuthoringRequest {
    ScenePrefabAuthoringCommandKind kind{ScenePrefabAuthoringCommandKind::create_scene};

    std::string scene_path;
    std::string scene_content;
    SceneDocumentV2 scene;

    std::string prefab_path;
    std::string prefab_content;
    PrefabDocumentV2 prefab;

    AuthoringId node_id;
    std::string node_name;
    AuthoringId parent_id;
    Transform3D node_transform;

    AuthoringId component_id;
    AuthoringId component_node_id;
    SceneComponentTypeId component_type;
    std::vector<SceneComponentPropertyV2> component_properties;
    std::string component_payload_format{"properties"};

    std::string instance_id_prefix;
    std::string instance_name_prefix;

    std::string runtime_package_migration{"unsupported"};
    std::string source_asset_import{"unsupported"};
    std::string package_cooking{"unsupported"};
    std::string editor_productization{"unsupported"};
    std::string nested_prefab_conflict_ux{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string arbitrary_shell{"unsupported"};
    std::string free_form_edit{"unsupported"};
};

struct ScenePrefabAuthoringResult {
    std::string scene_content;
    std::string prefab_content;
    std::vector<ScenePrefabAuthoringChangedFile> changed_files;
    std::vector<ScenePrefabAuthoringModelMutation> model_mutations;
    std::vector<ScenePrefabAuthoringDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] ScenePrefabAuthoringResult plan_scene_prefab_authoring(const ScenePrefabAuthoringRequest& request);
[[nodiscard]] ScenePrefabAuthoringResult apply_scene_prefab_authoring(IFileSystem& filesystem,
                                                                      const ScenePrefabAuthoringRequest& request);

} // namespace mirakana
