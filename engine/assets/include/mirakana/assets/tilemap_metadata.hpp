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

struct TilemapUvRect {
    float u0{0.0F};
    float v0{0.0F};
    float u1{1.0F};
    float v1{1.0F};
};

struct TilemapAtlasTile {
    std::string id;
    AssetId page;
    TilemapUvRect uv;
    std::array<float, 4> color{1.0F, 1.0F, 1.0F, 1.0F};
};

struct TilemapLayer {
    std::string name;
    std::uint32_t width{0};
    std::uint32_t height{0};
    bool visible{true};
    std::vector<std::string> cells;
};

struct TilemapMetadataDocument {
    AssetId asset;
    std::string source_decoding{"unsupported"};
    std::string atlas_packing{"unsupported"};
    std::string native_gpu_sprite_batching{"unsupported"};
    AssetId atlas_page;
    std::string atlas_page_uri;
    std::uint32_t tile_width{0};
    std::uint32_t tile_height{0};
    std::vector<TilemapAtlasTile> tiles;
    std::vector<TilemapLayer> layers;
};

enum class TilemapMetadataDiagnosticCode : std::uint8_t {
    invalid_asset,
    unsupported_source_decoding,
    unsupported_atlas_packing,
    unsupported_native_gpu_sprite_batching,
    invalid_atlas_page_asset,
    invalid_atlas_page_uri,
    invalid_tile_size,
    missing_tile,
    invalid_tile_id,
    duplicate_tile_id,
    undeclared_tile_page,
    invalid_tile_uv_rect,
    invalid_tile_color,
    missing_layer,
    invalid_layer_name,
    duplicate_layer_name,
    invalid_layer_size,
    invalid_layer_cell_count,
    invalid_layer_cell,
    unknown_cell_tile,
};

struct TilemapMetadataDiagnostic {
    TilemapMetadataDiagnosticCode code{TilemapMetadataDiagnosticCode::invalid_asset};
    AssetId asset;
    std::string field;
    std::string message;
};

[[nodiscard]] std::vector<TilemapMetadataDiagnostic>
validate_tilemap_metadata_document(const TilemapMetadataDocument& document);

// Serializes a cooked data-only GameEngine.Tilemap.v1 descriptor. Source image
// decoding, production atlas packing, and native GPU sprite batching must remain
// unsupported in this format.
[[nodiscard]] std::string serialize_tilemap_metadata_document(const TilemapMetadataDocument& document);
[[nodiscard]] TilemapMetadataDocument deserialize_tilemap_metadata_document(std::string_view text);

} // namespace mirakana
