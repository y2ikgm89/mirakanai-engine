// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"

#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct SourceAssetDependencyRowV1 {
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    AssetKeyV2 key;

    friend bool operator==(const SourceAssetDependencyRowV1& lhs, const SourceAssetDependencyRowV1& rhs) noexcept {
        return lhs.kind == rhs.kind && lhs.key.value == rhs.key.value;
    }
};

struct SourceAssetRegistryRowV1 {
    AssetKeyV2 key;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<SourceAssetDependencyRowV1> dependencies;
};

struct SourceAssetRegistryDocumentV1 {
    std::vector<SourceAssetRegistryRowV1> assets;
};

enum class SourceAssetRegistryDiagnosticCodeV1 {
    invalid_key,
    duplicate_key,
    duplicate_asset_id,
    invalid_kind,
    invalid_source_path,
    duplicate_source_path,
    invalid_source_format,
    invalid_imported_path,
    duplicate_imported_path,
    invalid_dependency_kind,
    invalid_dependency_target,
    invalid_dependency_key,
    missing_dependency_key,
    duplicate_dependency,
    invalid_identity_projection,
    invalid_import_metadata,
};

struct SourceAssetRegistryDiagnosticV1 {
    SourceAssetRegistryDiagnosticCodeV1 code{SourceAssetRegistryDiagnosticCodeV1::invalid_key};
    AssetKeyV2 key;
    std::string path;
    AssetDependencyKind dependency_kind{AssetDependencyKind::unknown};
    AssetKeyV2 dependency_key;
};

[[nodiscard]] std::string_view source_asset_registry_format_v1() noexcept;
[[nodiscard]] bool is_supported_source_asset_kind_v1(AssetKind kind) noexcept;
[[nodiscard]] std::string_view expected_source_asset_format_v1(AssetKind kind) noexcept;
[[nodiscard]] std::string_view source_asset_dependency_kind_name_v1(AssetDependencyKind kind) noexcept;
[[nodiscard]] AssetDependencyKind parse_source_asset_dependency_kind_v1(std::string_view value) noexcept;

[[nodiscard]] std::vector<SourceAssetRegistryDiagnosticV1>
validate_source_asset_registry_document(const SourceAssetRegistryDocumentV1& document);
[[nodiscard]] std::string serialize_source_asset_registry_document(const SourceAssetRegistryDocumentV1& document);
[[nodiscard]] SourceAssetRegistryDocumentV1 parse_source_asset_registry_document_unvalidated_v1(std::string_view text);
[[nodiscard]] SourceAssetRegistryDocumentV1 deserialize_source_asset_registry_document(std::string_view text);
[[nodiscard]] AssetIdentityDocumentV2
project_source_asset_registry_identity_v2(const SourceAssetRegistryDocumentV1& document);
[[nodiscard]] AssetImportMetadataRegistry
build_source_asset_import_metadata_registry(const SourceAssetRegistryDocumentV1& document);

} // namespace mirakana
