// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/tilemap_metadata.hpp"
#include "mirakana/platform/filesystem.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct CookedTilemapTileDesc {
    std::string id;
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct CookedTilemapLayerDesc {
    std::string name;
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool visible{true};
    std::vector<std::string> cells;
};

struct CookedTilemapAuthoringDesc {
    AssetId tilemap_asset;
    std::string output_path;
    std::uint64_t source_revision{1};
    std::string source_decoding{"unsupported"};
    std::string atlas_packing{"unsupported"};
    std::string native_gpu_sprite_batching{"unsupported"};
    AssetId atlas_page;
    std::string atlas_page_uri;
    std::uint32_t tile_width{0};
    std::uint32_t tile_height{0};
    std::vector<CookedTilemapTileDesc> tiles;
    std::vector<CookedTilemapLayerDesc> layers;
};

struct CookedTilemapAuthoringFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

struct CookedTilemapAuthoringResult {
    std::string content;
    AssetCookedArtifact artifact;
    std::vector<AssetDependencyEdge> dependency_edges;
    std::vector<CookedTilemapAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct CookedTilemapPackageVerificationResult {
    std::vector<CookedTilemapAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct CookedTilemapPackageChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct CookedTilemapPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    CookedTilemapAuthoringDesc tilemap;
};

struct CookedTilemapPackageApplyDesc {
    std::string package_index_path;
    CookedTilemapAuthoringDesc tilemap;
};

struct CookedTilemapPackageUpdateResult {
    std::string tilemap_content;
    std::string package_index_content;
    std::vector<CookedTilemapPackageChangedFile> changed_files;
    std::vector<CookedTilemapAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] CookedTilemapAuthoringResult author_cooked_tilemap_metadata(const CookedTilemapAuthoringDesc& desc);
[[nodiscard]] CookedTilemapAuthoringResult write_cooked_tilemap_metadata(IFileSystem& filesystem,
                                                                         const CookedTilemapAuthoringDesc& desc);
[[nodiscard]] CookedTilemapPackageVerificationResult
verify_cooked_tilemap_package_metadata(const AssetCookedPackageIndex& index, AssetId tilemap_asset,
                                       std::string_view tilemap_content);
[[nodiscard]] CookedTilemapPackageUpdateResult
plan_cooked_tilemap_package_update(const CookedTilemapPackageUpdateDesc& desc);
[[nodiscard]] CookedTilemapPackageUpdateResult
apply_cooked_tilemap_package_update(IFileSystem& filesystem, const CookedTilemapPackageApplyDesc& desc);

} // namespace mirakana
