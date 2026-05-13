// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/scene/scene.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RuntimeScenePackageValidationCommandKind : std::uint8_t {
    validate_runtime_scene_package,
    free_form_edit,
};

struct RuntimeScenePackageValidationReference {
    SceneNodeId node;
    AssetId asset;
    std::string reference_kind;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
};

struct RuntimeScenePackageValidationSummary {
    std::string package_index_path;
    std::string content_root;
    AssetKeyV2 scene_asset_key;
    AssetId scene_asset;
    std::uint64_t package_record_count{0};
    std::string scene_name;
    std::uint32_t scene_node_count{0};
    std::vector<RuntimeScenePackageValidationReference> references;
};

struct RuntimeScenePackageValidationDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    AssetKeyV2 scene_asset_key;
    AssetId asset;
    SceneNodeId node;
    std::string reference_kind;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct RuntimeScenePackageValidationRequest {
    RuntimeScenePackageValidationCommandKind kind{
        RuntimeScenePackageValidationCommandKind::validate_runtime_scene_package};

    std::string package_index_path;
    std::string content_root;
    AssetKeyV2 scene_asset_key;
    bool validate_asset_references{true};
    bool require_unique_node_names{false};

    std::string package_cooking{"unsupported"};
    std::string runtime_source_parsing{"unsupported"};
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

struct RuntimeScenePackageValidationResult {
    RuntimeScenePackageValidationSummary summary;
    std::vector<RuntimeScenePackageValidationDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RuntimeScenePackageValidationResult
plan_runtime_scene_package_validation(const RuntimeScenePackageValidationRequest& request);
[[nodiscard]] RuntimeScenePackageValidationResult
execute_runtime_scene_package_validation(IFileSystem& filesystem, const RuntimeScenePackageValidationRequest& request);

} // namespace mirakana
