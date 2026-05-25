// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AssetKey {
    std::string value;
};

struct AssetIdentityRow {
    AssetKey key;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
};

struct AssetIdentityDocument {
    std::vector<AssetIdentityRow> assets;
};

enum class AssetIdentityDiagnosticCode : std::uint8_t {
    invalid_key,
    duplicate_key,
    invalid_kind,
    invalid_source_path,
    duplicate_source_path,
};

struct AssetIdentityDiagnostic {
    AssetIdentityDiagnosticCode code{AssetIdentityDiagnosticCode::invalid_key};
    AssetKey key;
    std::string source_path;
};

struct AssetIdentityPlacementRequest {
    std::string placement;
    AssetKey key;
    AssetKind expected_kind{AssetKind::unknown};
};

struct AssetIdentityPlacementRow {
    std::string placement;
    AssetKey key;
    AssetId id;
    AssetKind kind{AssetKind::unknown};
    std::string source_path;
};

enum class AssetIdentityPlacementDiagnosticCode : std::uint8_t {
    invalid_identity_document,
    invalid_placement,
    duplicate_placement,
    invalid_key,
    invalid_expected_kind,
    missing_key,
    kind_mismatch,
};

struct AssetIdentityPlacementDiagnostic {
    AssetIdentityPlacementDiagnosticCode code{AssetIdentityPlacementDiagnosticCode::invalid_key};
    std::string placement;
    AssetKey key;
    AssetKind expected_kind{AssetKind::unknown};
    AssetKind actual_kind{AssetKind::unknown};
};

struct AssetIdentityPlacementPlan {
    bool can_place{false};
    std::vector<AssetIdentityPlacementRow> rows;
    std::vector<AssetIdentityPlacementDiagnostic> diagnostics;
    std::vector<AssetIdentityDiagnostic> identity_diagnostics;
};

[[nodiscard]] AssetId asset_id_from_key(const AssetKey& key) noexcept;
[[nodiscard]] std::vector<AssetIdentityDiagnostic>
validate_asset_identity_document(const AssetIdentityDocument& document);
[[nodiscard]] std::string serialize_asset_identity_document(const AssetIdentityDocument& document);
[[nodiscard]] AssetIdentityDocument deserialize_asset_identity_document(std::string_view text);
[[nodiscard]] AssetIdentityPlacementPlan
plan_asset_identity_placements(const AssetIdentityDocument& document,
                               std::span<const AssetIdentityPlacementRequest> requests);

} // namespace mirakana
