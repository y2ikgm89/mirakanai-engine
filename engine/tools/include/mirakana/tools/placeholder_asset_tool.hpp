// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_identity.hpp"
#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/tools/registered_source_asset_cook_package_tool.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

struct PlaceholderAssetRequest {
    AssetKeyV2 asset_key;
    AssetKind asset_kind{AssetKind::unknown};
    std::string source_path;
    std::string imported_path;
    std::uint32_t seed{1};
    std::uint32_t texture_width{16};
    std::uint32_t texture_height{16};
    std::array<float, 4> material_base_color{1.0F, 0.0F, 1.0F, 1.0F};
    std::uint32_t audio_sample_rate{48000};
    std::uint64_t audio_frame_count{4800};
};

struct PlaceholderAssetBundleRequest {
    std::string source_registry_path;
    std::string source_registry_content;
    std::vector<PlaceholderAssetRequest> assets;
};

struct PlaceholderAssetChangedFile {
    std::string path;
    std::string document_kind;
    std::string content;
    std::uint64_t content_hash{0};
};

struct PlaceholderAssetProvenanceRow {
    AssetKeyV2 asset_key;
    AssetId asset;
    AssetKind asset_kind{AssetKind::unknown};
    std::string source_path;
    std::string source_format;
    std::string imported_path;
    std::string generator;
    std::string license;
    std::uint32_t seed{0};
    std::uint64_t content_hash{0};
};

struct PlaceholderAssetDiagnostic {
    std::string code;
    std::string message;
    std::string path;
    AssetKeyV2 asset_key;
};

struct PlaceholderAssetBundlePlan {
    std::string source_registry_content;
    std::vector<PlaceholderAssetChangedFile> changed_files;
    std::vector<PlaceholderAssetProvenanceRow> provenance_rows;
    std::vector<PlaceholderAssetDiagnostic> diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return diagnostics.empty();
    }
};

struct PlaceholderAssetCookPackageRequest {
    PlaceholderAssetBundleRequest placeholder_assets;
    std::string package_index_path;
    std::string package_index_content;
    std::uint64_t source_revision{1};
};

struct PlaceholderAssetCookPackagePlan {
    PlaceholderAssetBundlePlan placeholder_plan;
    RegisteredSourceAssetCookPackageResult package_plan;

    [[nodiscard]] bool succeeded() const noexcept {
        return placeholder_plan.succeeded() && package_plan.succeeded();
    }
};

[[nodiscard]] PlaceholderAssetBundlePlan plan_placeholder_asset_bundle(const PlaceholderAssetBundleRequest& request);
[[nodiscard]] PlaceholderAssetCookPackagePlan
plan_placeholder_asset_cook_package(const PlaceholderAssetCookPackageRequest& request);

} // namespace mirakana
