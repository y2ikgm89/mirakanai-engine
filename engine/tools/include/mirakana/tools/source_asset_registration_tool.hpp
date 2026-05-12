// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_import_pipeline.hpp"
#include "mirakana/assets/source_asset_registry.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class SourceAssetRegistrationCommandKind {
    register_source_asset,
    free_form_edit,
};

struct SourceAssetRegistrationChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct SourceAssetRegistrationModelMutation {
    std::string kind;
    std::string target_path;
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetKind asset_kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;
};

struct SourceAssetRegistrationImportMetadata {
    AssetId asset;
    AssetImportActionKind kind{AssetImportActionKind::unknown};
    std::string source_path;
    std::string imported_path;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;
};

struct SourceAssetRegistrationDiagnostic {
    std::string severity{"error"};
    std::string code;
    std::string message;
    std::string path;
    AssetKeyV2 asset_key;
    std::string unsupported_gap_id;
    std::string validation_recipe;
};

struct SourceAssetRegistrationRequest {
    SourceAssetRegistrationCommandKind kind{SourceAssetRegistrationCommandKind::register_source_asset};

    std::string source_registry_path;
    std::string source_registry_content;
    AssetKeyV2 asset_key;
    AssetKind asset_kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<SourceAssetDependencyRowV1> dependency_rows;

    std::string import_settings{"default-only"};
    std::string external_importer{"unsupported"};
    std::string package_cooking{"unsupported"};
    std::string renderer_rhi_residency{"unsupported"};
    std::string package_streaming{"unsupported"};
    std::string material_graph{"unsupported"};
    std::string shader_graph{"unsupported"};
    std::string live_shader_generation{"unsupported"};
    std::string shader_compilation_execution{"unsupported"};
    std::string editor_productization{"unsupported"};
    std::string metal_readiness{"unsupported"};
    std::string public_native_rhi_handles{"unsupported"};
    std::string arbitrary_shell{"unsupported"};
    std::string free_form_edit{"unsupported"};
};

struct SourceAssetRegistrationResult {
    std::string source_registry_content;
    std::vector<SourceAssetRegistrationChangedFile> changed_files;
    std::vector<SourceAssetRegistrationModelMutation> model_mutations;
    std::vector<SourceAssetRegistrationImportMetadata> import_metadata;
    AssetIdentityDocumentV2 asset_identity_projection;
    std::vector<SourceAssetRegistrationDiagnostic> diagnostics;
    std::vector<std::string> validation_recipes;
    std::vector<std::string> unsupported_gap_ids;
    std::string undo_token{"placeholder-only"};

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

[[nodiscard]] SourceAssetRegistrationResult
plan_source_asset_registration(const SourceAssetRegistrationRequest& request);
[[nodiscard]] SourceAssetRegistrationResult
apply_source_asset_registration(IFileSystem& filesystem, const SourceAssetRegistrationRequest& request);

} // namespace mirakana
