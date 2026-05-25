// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/scene/schema.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class SceneRuntimePackageMigrationCommandKind : std::uint8_t {
    migrate_scene_runtime_package,
    free_form_edit,
};

struct SceneRuntimePackageMigrationChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct SceneRuntimePackageMigrationModelMutation {
    std::string kind;
    std::string target_path;
    std::string scene_path;
    std::string package_index_path;
    AssetKey scene_asset_key;
    AssetId scene_asset;
    std::vector<AssetIdentityPlacementRow> placement_rows;
    std::vector<SourceAssetDependencyRow> dependency_rows;
};

struct SceneRuntimePackageMigrationDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    AssetKey asset_key;
    AuthoringId node;
    AuthoringId component;
    SceneComponentTypeId component_type;
    std::string property;
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct SceneRuntimePackageMigrationRequest {
    SceneRuntimePackageMigrationCommandKind kind{
        SceneRuntimePackageMigrationCommandKind::migrate_scene_runtime_package};

    std::string scene_path;
    std::string scene_content;
    std::string source_registry_path;
    std::string source_registry_content;
    std::string package_index_path;
    std::string package_index_content;
    std::string output_scene_path;
    AssetKey scene_asset_key;
    std::uint64_t source_revision{0};

    std::string package_cooking{"unsupported"};
    std::string dependent_asset_cooking{"unsupported"};
    std::string external_importer_execution{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string editor_productization{"unsupported"};
    std::string metal_readiness{"unsupported"};
    std::string public_native_rhi_handles{"unsupported"};
    std::string general_production_renderer_quality{"unsupported"};
    std::string arbitrary_shell{"unsupported"};
    std::string free_form_edit{"unsupported"};
};

struct SceneRuntimePackageMigrationResult {
    std::string runtime_scene_content;
    std::string package_index_content;
    std::vector<SceneRuntimePackageMigrationChangedFile> changed_files;
    std::vector<SceneRuntimePackageMigrationModelMutation> model_mutations;
    std::vector<SceneRuntimePackageMigrationDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] SceneRuntimePackageMigrationResult
plan_scene_runtime_package_migration(const SceneRuntimePackageMigrationRequest& request);
[[nodiscard]] SceneRuntimePackageMigrationResult
apply_scene_runtime_package_migration(IFileSystem& filesystem, const SceneRuntimePackageMigrationRequest& request);

} // namespace mirakana
