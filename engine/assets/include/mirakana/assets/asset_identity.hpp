// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AssetKeyV2 {
    std::string value;
};

struct AssetIdentityRowV2 {
    AssetKeyV2 key;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
};

struct AssetIdentityDocumentV2 {
    std::vector<AssetIdentityRowV2> assets;
};

enum class AssetIdentityDiagnosticCodeV2 {
    invalid_key,
    duplicate_key,
    invalid_kind,
    invalid_source_path,
    duplicate_source_path,
};

struct AssetIdentityDiagnosticV2 {
    AssetIdentityDiagnosticCodeV2 code{AssetIdentityDiagnosticCodeV2::invalid_key};
    AssetKeyV2 key;
    std::string source_path;
};

struct AssetIdentityPlacementRequestV2 {
    std::string placement;
    AssetKeyV2 key;
    AssetKind expected_kind{AssetKind::unknown};
};

struct AssetIdentityPlacementRowV2 {
    std::string placement;
    AssetKeyV2 key;
    AssetId id;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
};

enum class AssetIdentityPlacementDiagnosticCodeV2 {
    invalid_identity_document,
    invalid_placement,
    duplicate_placement,
    invalid_key,
    invalid_expected_kind,
    missing_key,
    kind_mismatch,
};

struct AssetIdentityPlacementDiagnosticV2 {
    AssetIdentityPlacementDiagnosticCodeV2 code{AssetIdentityPlacementDiagnosticCodeV2::invalid_key};
    std::string placement;
    AssetKeyV2 key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
};

struct AssetIdentityPlacementPlanV2 {
    bool can_place{false};
    std::vector<AssetIdentityPlacementRowV2> rows;
    std::vector<AssetIdentityPlacementDiagnosticV2> diagnostics;
    std::vector<AssetIdentityDiagnosticV2> identity_diagnostics;
};

[[nodiscard]] AssetId asset_id_from_key_v2(const AssetKeyV2& key) noexcept;
[[nodiscard]] std::vector<AssetIdentityDiagnosticV2>
validate_asset_identity_document_v2(const AssetIdentityDocumentV2& document);
[[nodiscard]] std::string serialize_asset_identity_document_v2(const AssetIdentityDocumentV2& document);
[[nodiscard]] AssetIdentityDocumentV2 deserialize_asset_identity_document_v2(std::string_view text);
[[nodiscard]] AssetIdentityPlacementPlanV2
plan_asset_identity_placements_v2(const AssetIdentityDocumentV2& document,
                                  std::span<const AssetIdentityPlacementRequestV2> requests);

} // namespace mirakana
