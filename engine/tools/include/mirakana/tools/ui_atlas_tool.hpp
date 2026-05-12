// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_package.hpp"
#include "mirakana/assets/asset_source_format.hpp"
#include "mirakana/assets/sprite_atlas_packing.hpp"
#include "mirakana/assets/ui_atlas_metadata.hpp"
#include "mirakana/platform/filesystem.hpp"
#include "mirakana/ui/ui.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

using CookedUiAtlasPageDesc = UiAtlasMetadataPage;

struct CookedUiAtlasImageDesc {
    std::string resource_id;
    std::string asset_uri;
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct CookedUiAtlasGlyphDesc {
    std::string font_family;
    std::uint32_t glyph{0};
    AssetId page;
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct CookedUiAtlasAuthoringDesc {
    AssetId atlas_asset;
    std::string output_path;
    std::uint64_t source_revision{1};
    std::string source_decoding{"unsupported"};
    std::string atlas_packing{"unsupported"};
    std::vector<CookedUiAtlasPageDesc> pages;
    std::vector<CookedUiAtlasImageDesc> images;
    std::vector<CookedUiAtlasGlyphDesc> glyphs;
};

struct CookedUiAtlasAuthoringFailure {
    AssetId asset;
    std::string path;
    std::string diagnostic;
};

struct CookedUiAtlasAuthoringResult {
    std::string content;
    AssetCookedArtifact artifact;
    std::vector<AssetDependencyEdge> dependency_edges;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct CookedUiAtlasPackageVerificationResult {
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct CookedUiAtlasPackageChangedFile {
    std::string path;
    std::string content;
    std::uint64_t content_hash{0};
};

struct CookedUiAtlasPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    CookedUiAtlasAuthoringDesc atlas;
};

struct CookedUiAtlasPackageApplyDesc {
    std::string package_index_path;
    CookedUiAtlasAuthoringDesc atlas;
};

struct CookedUiAtlasPackageUpdateResult {
    std::string atlas_content;
    std::string package_index_content;
    std::vector<CookedUiAtlasPackageChangedFile> changed_files;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct PackedUiAtlasImageDesc {
    std::string resource_id;
    std::string asset_uri;
    ui::ImageDecodeResult decoded_image;
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct PackedUiAtlasAuthoringDesc {
    AssetId atlas_asset;
    AssetId atlas_page_asset;
    std::string atlas_metadata_output_path;
    std::string atlas_page_output_path;
    std::uint64_t source_revision{1};
    std::uint32_t max_side{sprite_atlas_packing_max_side};
    std::string source_decoding{"decoded-image-adapter"};
    std::string atlas_packing{"deterministic-sprite-atlas-rgba8-max-side"};
    std::vector<PackedUiAtlasImageDesc> images;
};

struct PackedUiAtlasAuthoringResult {
    TextureSourceDocument atlas_texture;
    std::string atlas_texture_content;
    CookedUiAtlasAuthoringDesc atlas_metadata_desc;
    std::string atlas_metadata_content;
    AssetCookedArtifact atlas_page_artifact;
    CookedUiAtlasAuthoringResult atlas_metadata;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct PackedUiAtlasPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    PackedUiAtlasAuthoringDesc atlas;
};

struct PackedUiAtlasPackageApplyDesc {
    std::string package_index_path;
    PackedUiAtlasAuthoringDesc atlas;
};

struct PackedUiAtlasPackageUpdateResult {
    TextureSourceDocument atlas_texture;
    std::string atlas_texture_content;
    std::string atlas_metadata_content;
    std::string package_index_content;
    std::vector<CookedUiAtlasPackageChangedFile> changed_files;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct PackedUiGlyphAtlasGlyphDesc {
    std::string font_family;
    std::uint32_t glyph{0};
    ui::ImageDecodeResult rasterized_glyph;
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct PackedUiGlyphAtlasAuthoringDesc {
    AssetId atlas_asset;
    AssetId atlas_page_asset;
    std::string atlas_metadata_output_path;
    std::string atlas_page_output_path;
    std::uint64_t source_revision{1};
    std::uint32_t max_side{sprite_atlas_packing_max_side};
    std::string source_decoding{"rasterized-glyph-adapter"};
    std::string atlas_packing{"deterministic-glyph-atlas-rgba8-max-side"};
    std::vector<PackedUiGlyphAtlasGlyphDesc> glyphs;
};

struct PackedUiGlyphAtlasAuthoringResult {
    TextureSourceDocument atlas_texture;
    std::string atlas_texture_content;
    CookedUiAtlasAuthoringDesc atlas_metadata_desc;
    std::string atlas_metadata_content;
    AssetCookedArtifact atlas_page_artifact;
    CookedUiAtlasAuthoringResult atlas_metadata;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

struct PackedUiGlyphAtlasPackageUpdateDesc {
    std::string package_index_path;
    std::string package_index_content;
    PackedUiGlyphAtlasAuthoringDesc atlas;
};

struct PackedUiGlyphAtlasPackageApplyDesc {
    std::string package_index_path;
    PackedUiGlyphAtlasAuthoringDesc atlas;
};

struct PackedUiGlyphAtlasPackageUpdateResult {
    TextureSourceDocument atlas_texture;
    std::string atlas_texture_content;
    std::string atlas_metadata_content;
    std::string package_index_content;
    std::vector<CookedUiAtlasPackageChangedFile> changed_files;
    std::vector<CookedUiAtlasAuthoringFailure> failures;

    [[nodiscard]] bool succeeded() const noexcept {
        return failures.empty();
    }
};

[[nodiscard]] CookedUiAtlasAuthoringResult author_cooked_ui_atlas_metadata(const CookedUiAtlasAuthoringDesc& desc);
[[nodiscard]] CookedUiAtlasAuthoringResult write_cooked_ui_atlas_metadata(IFileSystem& filesystem,
                                                                          const CookedUiAtlasAuthoringDesc& desc);
[[nodiscard]] CookedUiAtlasPackageVerificationResult
verify_cooked_ui_atlas_package_metadata(const AssetCookedPackageIndex& index, AssetId atlas_asset,
                                        std::string_view atlas_content);
[[nodiscard]] CookedUiAtlasPackageUpdateResult
plan_cooked_ui_atlas_package_update(const CookedUiAtlasPackageUpdateDesc& desc);
[[nodiscard]] CookedUiAtlasPackageUpdateResult
apply_cooked_ui_atlas_package_update(IFileSystem& filesystem, const CookedUiAtlasPackageApplyDesc& desc);
[[nodiscard]] PackedUiAtlasAuthoringResult
author_packed_ui_atlas_from_decoded_images(const PackedUiAtlasAuthoringDesc& desc);
[[nodiscard]] PackedUiAtlasPackageUpdateResult
plan_packed_ui_atlas_package_update(const PackedUiAtlasPackageUpdateDesc& desc);
[[nodiscard]] PackedUiAtlasPackageUpdateResult
apply_packed_ui_atlas_package_update(IFileSystem& filesystem, const PackedUiAtlasPackageApplyDesc& desc);
[[nodiscard]] PackedUiGlyphAtlasAuthoringResult
author_packed_ui_glyph_atlas_from_rasterized_glyphs(const PackedUiGlyphAtlasAuthoringDesc& desc);
[[nodiscard]] PackedUiGlyphAtlasPackageUpdateResult
plan_packed_ui_glyph_atlas_package_update(const PackedUiGlyphAtlasPackageUpdateDesc& desc);
[[nodiscard]] PackedUiGlyphAtlasPackageUpdateResult
apply_packed_ui_glyph_atlas_package_update(IFileSystem& filesystem, const PackedUiGlyphAtlasPackageApplyDesc& desc);

} // namespace mirakana
