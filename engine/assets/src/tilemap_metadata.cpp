// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/tilemap_metadata.hpp"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <limits>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] bool valid_text_field(std::string_view value) noexcept {
    return value.find('\0') == std::string_view::npos && value.find('\n') == std::string_view::npos &&
           value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_non_empty_text_field(std::string_view value) noexcept {
    return !value.empty() && valid_text_field(value);
}

[[nodiscard]] bool valid_tile_token(std::string_view value) noexcept {
    return valid_non_empty_text_field(value) && value.find(',') == std::string_view::npos &&
           value.find('=') == std::string_view::npos;
}

[[nodiscard]] bool valid_cell_token(std::string_view value) noexcept {
    return value.empty() || valid_tile_token(value);
}

[[nodiscard]] bool valid_uv_rect(TilemapUvRect uv) noexcept {
    return std::isfinite(uv.u0) && std::isfinite(uv.v0) && std::isfinite(uv.u1) && std::isfinite(uv.v1) &&
           uv.u0 >= 0.0F && uv.v0 >= 0.0F && uv.u1 <= 1.0F && uv.v1 <= 1.0F && uv.u0 < uv.u1 && uv.v0 < uv.v1;
}

[[nodiscard]] bool valid_color(std::array<float, 4> color) noexcept {
    return std::ranges::all_of(color,
                               [](float value) { return std::isfinite(value) && value >= 0.0F && value <= 1.0F; });
}

[[nodiscard]] std::string diagnostic_message(TilemapMetadataDiagnosticCode code) {
    switch (code) {
    case TilemapMetadataDiagnosticCode::invalid_asset:
        return "tilemap metadata asset is invalid";
    case TilemapMetadataDiagnosticCode::unsupported_source_decoding:
        return "tilemap source image decoding is not supported";
    case TilemapMetadataDiagnosticCode::unsupported_atlas_packing:
        return "tilemap production atlas packing is not supported";
    case TilemapMetadataDiagnosticCode::unsupported_native_gpu_sprite_batching:
        return "tilemap native GPU sprite batching is not supported";
    case TilemapMetadataDiagnosticCode::invalid_atlas_page_asset:
        return "tilemap atlas page asset is invalid";
    case TilemapMetadataDiagnosticCode::invalid_atlas_page_uri:
        return "tilemap atlas page asset_uri is invalid";
    case TilemapMetadataDiagnosticCode::invalid_tile_size:
        return "tilemap tile size is invalid";
    case TilemapMetadataDiagnosticCode::missing_tile:
        return "tilemap metadata must declare at least one tile";
    case TilemapMetadataDiagnosticCode::invalid_tile_id:
        return "tilemap tile id is invalid";
    case TilemapMetadataDiagnosticCode::duplicate_tile_id:
        return "tilemap tile id is duplicated";
    case TilemapMetadataDiagnosticCode::undeclared_tile_page:
        return "tilemap tile page is not the declared atlas page";
    case TilemapMetadataDiagnosticCode::invalid_tile_uv_rect:
        return "tilemap tile uv rect is invalid";
    case TilemapMetadataDiagnosticCode::invalid_tile_color:
        return "tilemap tile color is invalid";
    case TilemapMetadataDiagnosticCode::missing_layer:
        return "tilemap metadata must declare at least one layer";
    case TilemapMetadataDiagnosticCode::invalid_layer_name:
        return "tilemap layer name is invalid";
    case TilemapMetadataDiagnosticCode::duplicate_layer_name:
        return "tilemap layer name is duplicated";
    case TilemapMetadataDiagnosticCode::invalid_layer_size:
        return "tilemap layer size is invalid";
    case TilemapMetadataDiagnosticCode::invalid_layer_cell_count:
        return "tilemap layer cell count does not match width * height";
    case TilemapMetadataDiagnosticCode::invalid_layer_cell:
        return "tilemap layer cell token is invalid";
    case TilemapMetadataDiagnosticCode::unknown_cell_tile:
        return "tilemap layer cell references an unknown tile id";
    }
    return "tilemap metadata diagnostic is unknown";
}

void add_diagnostic(std::vector<TilemapMetadataDiagnostic>& diagnostics, TilemapMetadataDiagnosticCode code,
                    AssetId asset, std::string field = {}) {
    diagnostics.push_back(TilemapMetadataDiagnostic{
        .code = code,
        .asset = asset,
        .field = std::move(field),
        .message = diagnostic_message(code),
    });
}

void sort_tiles(std::vector<TilemapAtlasTile>& tiles) {
    std::ranges::sort(tiles, [](const TilemapAtlasTile& lhs, const TilemapAtlasTile& rhs) {
        if (lhs.id != rhs.id) {
            return lhs.id < rhs.id;
        }
        return lhs.page.value < rhs.page.value;
    });
}

[[nodiscard]] TilemapMetadataDocument canonical_document(TilemapMetadataDocument document) {
    sort_tiles(document.tiles);
    return document;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view field) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument(std::string(field) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] std::uint32_t parse_u32(std::string_view value, std::string_view field) {
    const auto parsed = parse_u64(value, field);
    if (parsed > std::numeric_limits<std::uint32_t>::max()) {
        throw std::invalid_argument(std::string(field) + " is invalid");
    }
    return static_cast<std::uint32_t>(parsed);
}

[[nodiscard]] float parse_float(std::string_view value, std::string_view field) {
    const auto valid_character = [](char character) noexcept {
        return (character >= '0' && character <= '9') || character == '+' || character == '-' || character == '.' ||
               character == 'e' || character == 'E';
    };
    if (value.empty() || !std::ranges::all_of(value, valid_character)) {
        throw std::invalid_argument(std::string(field) + " is invalid");
    }

    float parsed = 0.0F;
    std::istringstream stream{std::string{value}};
    stream.imbue(std::locale::classic());
    stream >> std::noskipws >> parsed;

    char trailing = '\0';
    if (!stream || (stream >> trailing) || !std::isfinite(parsed)) {
        throw std::invalid_argument(std::string(field) + " is invalid");
    }
    return parsed;
}

[[nodiscard]] bool parse_bool(std::string_view value, std::string_view field) {
    if (value == "true") {
        return true;
    }
    if (value == "false") {
        return false;
    }
    throw std::invalid_argument(std::string(field) + " is invalid");
}

[[nodiscard]] std::array<float, 4> parse_float4(std::string_view value, std::string_view field) {
    std::array<float, 4> result{};
    std::size_t begin = 0;
    for (std::size_t index = 0; index < result.size(); ++index) {
        const auto end = value.find(',', begin);
        const auto token = value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        result[index] = parse_float(token, field);
        if (end == std::string_view::npos) {
            if (index + 1U != result.size()) {
                throw std::invalid_argument(std::string(field) + " is incomplete");
            }
            return result;
        }
        begin = end + 1U;
    }
    if (begin < value.size()) {
        throw std::invalid_argument(std::string(field) + " has too many components");
    }
    return result;
}

[[nodiscard]] std::pair<std::uint32_t, std::uint32_t> parse_u32_pair(std::string_view value, std::string_view field) {
    const auto separator = value.find(',');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument(std::string(field) + " is invalid");
    }
    const auto first = parse_u32(value.substr(0, separator), field);
    const auto second = parse_u32(value.substr(separator + 1U), field);
    return {first, second};
}

[[nodiscard]] std::vector<std::string> parse_cell_list(std::string_view value) {
    std::vector<std::string> cells;
    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find(',', begin);
        cells.emplace_back(value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin));
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return cells;
}

[[nodiscard]] KeyValues parse_key_values(std::string_view text) {
    KeyValues values;
    std::size_t line_start = 0;
    while (line_start < text.size()) {
        const auto line_end = text.find('\n', line_start);
        const auto line = text.substr(line_start, line_end == std::string_view::npos ? text.size() - line_start
                                                                                     : line_end - line_start);
        line_start = line_end == std::string_view::npos ? text.size() : line_end + 1U;
        if (line.empty()) {
            continue;
        }
        if (line.find('\r') != std::string_view::npos) {
            throw std::invalid_argument("tilemap metadata line contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0U) {
            throw std::invalid_argument("tilemap metadata line is invalid");
        }
        auto [_, inserted] =
            values.emplace(std::string(line.substr(0, separator)), std::string(line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument("tilemap metadata contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const KeyValues& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("tilemap metadata is missing key: " + key);
    }
    return it->second;
}

void throw_if_invalid(const TilemapMetadataDocument& document) {
    const auto diagnostics = validate_tilemap_metadata_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument(diagnostics.front().message);
    }
}

} // namespace

std::vector<TilemapMetadataDiagnostic> validate_tilemap_metadata_document(const TilemapMetadataDocument& document) {
    std::vector<TilemapMetadataDiagnostic> diagnostics;
    if (document.asset.value == 0) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_asset, document.asset, "asset.id");
    }
    if (document.source_decoding != "unsupported") {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::unsupported_source_decoding, document.asset,
                       "source.decoding");
    }
    if (document.atlas_packing != "unsupported") {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::unsupported_atlas_packing, document.asset,
                       "atlas.packing");
    }
    if (document.native_gpu_sprite_batching != "unsupported") {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::unsupported_native_gpu_sprite_batching,
                       document.asset, "native_gpu_sprite_batching");
    }
    if (document.atlas_page.value == 0) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_atlas_page_asset, document.atlas_page,
                       "atlas.page.asset");
    }
    if (!valid_non_empty_text_field(document.atlas_page_uri)) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_atlas_page_uri, document.atlas_page,
                       "atlas.page.asset_uri");
    }
    if (document.tile_width == 0 || document.tile_height == 0) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_tile_size, document.asset, "tile.size");
    }
    if (document.tiles.empty()) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::missing_tile, document.asset, "tile.count");
    }

    std::unordered_set<std::string> tile_ids;
    for (const auto& tile : document.tiles) {
        if (!valid_tile_token(tile.id)) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_tile_id, tile.page, "tile.id");
        } else if (!tile_ids.insert(tile.id).second) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::duplicate_tile_id, tile.page, "tile.id");
        }
        if (tile.page != document.atlas_page) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::undeclared_tile_page, tile.page, "tile.page");
        }
        if (!valid_uv_rect(tile.uv)) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_tile_uv_rect, tile.page, "tile.uv");
        }
        if (!valid_color(tile.color)) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_tile_color, tile.page, "tile.color");
        }
    }

    if (document.layers.empty()) {
        add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::missing_layer, document.asset, "layer.count");
    }

    std::unordered_set<std::string> layer_names;
    for (const auto& layer : document.layers) {
        if (!valid_tile_token(layer.name)) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_layer_name, document.asset,
                           "layer.name");
        } else if (!layer_names.insert(layer.name).second) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::duplicate_layer_name, document.asset,
                           "layer.name");
        }
        if (layer.width == 0 || layer.height == 0) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_layer_size, document.asset,
                           "layer.size");
        }
        const auto expected_cells = static_cast<std::uint64_t>(layer.width) * static_cast<std::uint64_t>(layer.height);
        if (expected_cells != layer.cells.size()) {
            add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_layer_cell_count, document.asset,
                           "layer.cells");
        }
        for (const auto& cell : layer.cells) {
            if (!valid_cell_token(cell)) {
                add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::invalid_layer_cell, document.asset,
                               "layer.cells");
            } else if (!cell.empty() && tile_ids.find(cell) == tile_ids.end()) {
                add_diagnostic(diagnostics, TilemapMetadataDiagnosticCode::unknown_cell_tile, document.asset,
                               "layer.cells");
            }
        }
    }

    return diagnostics;
}

std::string serialize_tilemap_metadata_document(const TilemapMetadataDocument& document) {
    throw_if_invalid(document);
    const auto canonical = canonical_document(document);

    std::ostringstream output;
    output << "format=GameEngine.Tilemap.v1\n";
    output << "asset.id=" << canonical.asset.value << '\n';
    output << "asset.kind=tilemap\n";
    output << "source.decoding=" << canonical.source_decoding << '\n';
    output << "atlas.packing=" << canonical.atlas_packing << '\n';
    output << "native_gpu_sprite_batching=" << canonical.native_gpu_sprite_batching << '\n';
    output << "atlas.page.asset=" << canonical.atlas_page.value << '\n';
    output << "atlas.page.asset_uri=" << canonical.atlas_page_uri << '\n';
    output << "tile.size=" << canonical.tile_width << ',' << canonical.tile_height << '\n';
    output << "tile.count=" << canonical.tiles.size() << '\n';
    for (std::size_t index = 0; index < canonical.tiles.size(); ++index) {
        const auto& tile = canonical.tiles[index];
        output << "tile." << index << ".id=" << tile.id << '\n';
        output << "tile." << index << ".page=" << tile.page.value << '\n';
        output << "tile." << index << ".u0=" << tile.uv.u0 << '\n';
        output << "tile." << index << ".v0=" << tile.uv.v0 << '\n';
        output << "tile." << index << ".u1=" << tile.uv.u1 << '\n';
        output << "tile." << index << ".v1=" << tile.uv.v1 << '\n';
        output << "tile." << index << ".color=" << tile.color[0] << ',' << tile.color[1] << ',' << tile.color[2] << ','
               << tile.color[3] << '\n';
    }
    output << "layer.count=" << canonical.layers.size() << '\n';
    for (std::size_t index = 0; index < canonical.layers.size(); ++index) {
        const auto& layer = canonical.layers[index];
        output << "layer." << index << ".name=" << layer.name << '\n';
        output << "layer." << index << ".width=" << layer.width << '\n';
        output << "layer." << index << ".height=" << layer.height << '\n';
        output << "layer." << index << ".visible=" << (layer.visible ? "true" : "false") << '\n';
        output << "layer." << index << ".cells=";
        for (std::size_t cell = 0; cell < layer.cells.size(); ++cell) {
            if (cell != 0U) {
                output << ',';
            }
            output << layer.cells[cell];
        }
        output << '\n';
    }
    return output.str();
}

TilemapMetadataDocument deserialize_tilemap_metadata_document(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != "GameEngine.Tilemap.v1") {
        throw std::invalid_argument("tilemap metadata format is unsupported");
    }
    if (required_value(values, "asset.kind") != "tilemap") {
        throw std::invalid_argument("tilemap metadata asset kind is unsupported");
    }

    TilemapMetadataDocument document;
    document.asset = AssetId{parse_u64(required_value(values, "asset.id"), "tilemap metadata asset id")};
    document.source_decoding = required_value(values, "source.decoding");
    document.atlas_packing = required_value(values, "atlas.packing");
    document.native_gpu_sprite_batching = required_value(values, "native_gpu_sprite_batching");
    document.atlas_page =
        AssetId{parse_u64(required_value(values, "atlas.page.asset"), "tilemap metadata atlas page asset")};
    document.atlas_page_uri = required_value(values, "atlas.page.asset_uri");
    const auto [tile_width, tile_height] = parse_u32_pair(required_value(values, "tile.size"), "tilemap tile size");
    document.tile_width = tile_width;
    document.tile_height = tile_height;

    const auto tile_count = parse_u32(required_value(values, "tile.count"), "tilemap tile count");
    document.tiles.reserve(tile_count);
    for (std::uint32_t ordinal = 0; ordinal < tile_count; ++ordinal) {
        const auto prefix = "tile." + std::to_string(ordinal) + ".";
        document.tiles.push_back(TilemapAtlasTile{
            .id = required_value(values, prefix + "id"),
            .page = AssetId{parse_u64(required_value(values, prefix + "page"), "tilemap tile page")},
            .uv =
                TilemapUvRect{
                    .u0 = parse_float(required_value(values, prefix + "u0"), "tilemap tile u0"),
                    .v0 = parse_float(required_value(values, prefix + "v0"), "tilemap tile v0"),
                    .u1 = parse_float(required_value(values, prefix + "u1"), "tilemap tile u1"),
                    .v1 = parse_float(required_value(values, prefix + "v1"), "tilemap tile v1"),
                },
            .color = parse_float4(required_value(values, prefix + "color"), "tilemap tile color"),
        });
    }

    const auto layer_count = parse_u32(required_value(values, "layer.count"), "tilemap layer count");
    document.layers.reserve(layer_count);
    for (std::uint32_t ordinal = 0; ordinal < layer_count; ++ordinal) {
        const auto prefix = "layer." + std::to_string(ordinal) + ".";
        document.layers.push_back(TilemapLayer{
            .name = required_value(values, prefix + "name"),
            .width = parse_u32(required_value(values, prefix + "width"), "tilemap layer width"),
            .height = parse_u32(required_value(values, prefix + "height"), "tilemap layer height"),
            .visible = parse_bool(required_value(values, prefix + "visible"), "tilemap layer visible"),
            .cells = parse_cell_list(required_value(values, prefix + "cells")),
        });
    }

    throw_if_invalid(document);
    return canonical_document(std::move(document));
}

} // namespace mirakana
