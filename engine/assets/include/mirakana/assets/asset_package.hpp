// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_dependency_graph.hpp"
#include "mirakana/assets/asset_registry.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct AssetCookedArtifact {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::string content;
    std::uint64_t source_revision{0};
    std::vector<AssetId> dependencies;
};

struct AssetCookedPackageEntry {
    AssetId asset;
    AssetKind kind{AssetKind::unknown};
    std::string path;
    std::uint64_t content_hash{0};
    std::uint64_t source_revision{0};
    std::vector<AssetId> dependencies;
};

struct AssetCookedPackageIndex {
    std::vector<AssetCookedPackageEntry> entries;
    std::vector<AssetDependencyEdge> dependencies;
};

enum class AssetCookedPackageRecookDecisionKind : std::uint8_t {
    unknown,
    up_to_date,
    missing_from_previous_index,
    content_hash_changed,
    source_revision_changed,
    dependencies_changed,
};

struct AssetCookedPackageRecookDecision {
    AssetId asset;
    std::string path;
    AssetCookedPackageRecookDecisionKind kind{AssetCookedPackageRecookDecisionKind::unknown};
    std::string diagnostic;
};

[[nodiscard]] std::uint64_t hash_asset_cooked_content(std::string_view content) noexcept;
[[nodiscard]] bool is_valid_asset_cooked_artifact(const AssetCookedArtifact& artifact) noexcept;
[[nodiscard]] bool is_valid_asset_cooked_package_entry(const AssetCookedPackageEntry& entry) noexcept;

[[nodiscard]] AssetCookedPackageIndex build_asset_cooked_package_index(std::vector<AssetCookedArtifact> artifacts,
                                                                       std::vector<AssetDependencyEdge> dependencies);
[[nodiscard]] std::vector<AssetCookedPackageRecookDecision>
build_asset_cooked_package_recook_decisions(const AssetCookedPackageIndex& previous,
                                            const AssetCookedPackageIndex& current);

[[nodiscard]] std::string serialize_asset_cooked_package_index(const AssetCookedPackageIndex& index);
[[nodiscard]] AssetCookedPackageIndex deserialize_asset_cooked_package_index(std::string_view text);

} // namespace mirakana
