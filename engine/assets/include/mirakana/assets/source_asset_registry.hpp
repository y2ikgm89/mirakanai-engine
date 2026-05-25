// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_import_metadata.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct SourceAssetDependencyRow {
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    AssetKey key;

    friend bool operator==(const SourceAssetDependencyRow& lhs, const SourceAssetDependencyRow& rhs) noexcept {
        return lhs.kind == rhs.kind && lhs.key.value == rhs.key.value;
    }
};

struct SourceAssetRegistryRow {
    AssetKey key;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::vector<SourceAssetDependencyRow> dependencies;
};

struct SourceAssetRegistryDocument {
    std::vector<SourceAssetRegistryRow> assets;
};

enum class SourceAssetRegistryDiagnosticCode : std::uint8_t {
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

struct SourceAssetRegistryDiagnostic {
    SourceAssetRegistryDiagnosticCode code{SourceAssetRegistryDiagnosticCode::invalid_key};
    AssetKey key;
    std::string path;
    AssetDependencyKind dependency_kind{AssetDependencyKind::unknown};
    AssetKey dependency_key;
};

[[nodiscard]] std::string_view source_asset_registry_format() noexcept;
[[nodiscard]] bool is_supported_source_asset_kind(AssetKind kind) noexcept;
[[nodiscard]] std::string_view expected_source_asset_format(AssetKind kind) noexcept;
[[nodiscard]] std::string_view source_asset_dependency_kind_name(AssetDependencyKind kind) noexcept;
[[nodiscard]] AssetDependencyKind parse_source_asset_dependency_kind(std::string_view value) noexcept;

[[nodiscard]] std::vector<SourceAssetRegistryDiagnostic>
validate_source_asset_registry_document(const SourceAssetRegistryDocument& document);
[[nodiscard]] std::string serialize_source_asset_registry_document(const SourceAssetRegistryDocument& document);
[[nodiscard]] SourceAssetRegistryDocument parse_source_asset_registry_document_unvalidated(std::string_view text);
[[nodiscard]] SourceAssetRegistryDocument deserialize_source_asset_registry_document(std::string_view text);
[[nodiscard]] AssetIdentityDocument project_source_asset_registry_identity(const SourceAssetRegistryDocument& document);
[[nodiscard]] AssetImportMetadataRegistry
build_source_asset_import_metadata_registry(const SourceAssetRegistryDocument& document);

} // namespace mirakana
