// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class RegisteredSourceAssetCookPackageCommandKind : std::uint8_t {
    cook_registered_source_assets,
    free_form_edit,
};

/// Selects how `selected_asset_keys` are interpreted for `cook_registered_source_assets`.
/// - `explicit_dependency_selection`: every registered dependency key must appear in `selected_asset_keys`.
/// - `registered_source_registry_closure`: expand the selection to the transitive closure of registry
///   `dependencies[].key` rows within the same `GameEngine.SourceAssetRegistry.v1` document.
enum class RegisteredSourceAssetCookDependencyExpansion : std::uint8_t {
    explicit_dependency_selection,
    registered_source_registry_closure,
};

struct RegisteredSourceAssetCookPackageSourceFile {
    std::string path;
    std::string content;
};

struct RegisteredSourceAssetCookPackageChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct RegisteredSourceAssetCookPackageModelMutation {
    std::string kind;
    std::string target_path;
    std::string source_registry_path;
    std::string package_index_path;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetKind asset_kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;
};

struct RegisteredSourceAssetCookPackageDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    AssetKeyV2 asset_key;
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct RegisteredSourceAssetCookPackageRequest {
    RegisteredSourceAssetCookPackageCommandKind kind{
        RegisteredSourceAssetCookPackageCommandKind::cook_registered_source_assets};

    std::string source_registry_path;
    std::string source_registry_content;
    std::string package_index_path;
    std::string package_index_content;
    std::vector<AssetKeyV2> selected_asset_keys;
    std::vector<RegisteredSourceAssetCookPackageSourceFile> source_files;
    std::uint64_t source_revision{0};

    RegisteredSourceAssetCookDependencyExpansion dependency_expansion{
        RegisteredSourceAssetCookDependencyExpansion::explicit_dependency_selection};

    /// When `dependency_expansion` is `explicit_dependency_selection`, this must remain `unsupported`.
    /// When `dependency_expansion` is `registered_source_registry_closure`, this must be `registry_closure`
    /// so callers acknowledge the narrower in-registry closure contract (not importer-driven broad cooking).
    std::string dependency_cooking{"unsupported"};
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

struct RegisteredSourceAssetCookPackageResult {
    std::string package_index_content;
    std::vector<RegisteredSourceAssetCookPackageChangedFile> changed_files;
    std::vector<RegisteredSourceAssetCookPackageModelMutation> model_mutations;
    std::vector<RegisteredSourceAssetCookPackageDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] RegisteredSourceAssetCookPackageResult
plan_registered_source_asset_cook_package(const RegisteredSourceAssetCookPackageRequest& request);
[[nodiscard]] RegisteredSourceAssetCookPackageResult
apply_registered_source_asset_cook_package(IFileSystem& filesystem,
                                           const RegisteredSourceAssetCookPackageRequest& request);

} // namespace mirakana
