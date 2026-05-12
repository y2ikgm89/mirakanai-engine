// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct UiAtlasUvRect {
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
};

struct UiAtlasMetadataPage {
    AssetId asset;
    std::string asset_uri;
};

struct UiAtlasMetadataImage {
    std::string resource_id;
    std::string asset_uri;
    AssetId page;
    UiAtlasUvRect uv;
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct UiAtlasMetadataGlyph {
    std::string font_family;
    std::uint32_t glyph{0};
    AssetId page;
    UiAtlasUvRect uv;
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct UiAtlasMetadataDocument {
    AssetId asset;
    std::string source_decoding{"unsupported"};
    std::string atlas_packing{"unsupported"};
    std::vector<UiAtlasMetadataPage> pages;
    std::vector<UiAtlasMetadataImage> images;
    std::vector<UiAtlasMetadataGlyph> glyphs;
};

enum class UiAtlasMetadataDiagnosticCode {
    invalid_asset,
    unsupported_source_decoding,
    unsupported_atlas_packing,
    missing_page,
    invalid_page_asset,
    invalid_page_asset_uri,
    duplicate_page_asset,
    missing_image,
    missing_image_identity,
    duplicate_resource_id,
    duplicate_asset_uri,
    undeclared_image_page,
    invalid_uv_rect,
    invalid_color,
    missing_glyph_identity,
    duplicate_glyph,
    undeclared_glyph_page,
    invalid_glyph_uv_rect,
    invalid_glyph_color,
};

struct UiAtlasMetadataDiagnostic {
    UiAtlasMetadataDiagnosticCode code{UiAtlasMetadataDiagnosticCode::invalid_asset};
    AssetId asset;
    std::string field;
    std::string message;
};

[[nodiscard]] std::vector<UiAtlasMetadataDiagnostic>
validate_ui_atlas_metadata_document(const UiAtlasMetadataDocument& document);

// Serializes the cooked metadata-only GameEngine.UiAtlas.v1 format. This document does not
// embed decoded pixels or automatic pack results. `mirakana_tools` may stamp the reviewed
// decoded-image adapter / deterministic RGBA8 packing labels after composing a cooked page.
[[nodiscard]] std::string serialize_ui_atlas_metadata_document(const UiAtlasMetadataDocument& document);
[[nodiscard]] UiAtlasMetadataDocument deserialize_ui_atlas_metadata_document(std::string_view text);

} // namespace mirakana
