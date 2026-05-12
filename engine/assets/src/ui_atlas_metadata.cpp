// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/ui_atlas_metadata.hpp"

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

constexpr std::string_view unsupported_ui_atlas_capability = "unsupported";
constexpr std::string_view decoded_image_adapter_source_decoding = "decoded-image-adapter";
constexpr std::string_view deterministic_rgba8_atlas_packing = "deterministic-sprite-atlas-rgba8-max-side";
constexpr std::string_view rasterized_glyph_adapter_source_decoding = "rasterized-glyph-adapter";
constexpr std::string_view deterministic_glyph_atlas_packing = "deterministic-glyph-atlas-rgba8-max-side";

struct UiAtlasTextImageRow {
    bool has_resource_id{false};
    bool has_asset_uri{false};
    bool has_page{false};
    bool has_u0{false};
    bool has_v0{false};
    bool has_u1{false};
    bool has_v1{false};
    bool has_color{false};
    UiAtlasMetadataImage image;
};

struct UiAtlasTextGlyphRow {
    bool has_font_family{false};
    bool has_glyph{false};
    bool has_page{false};
    bool has_u0{false};
    bool has_v0{false};
    bool has_u1{false};
    bool has_v1{false};
    bool has_color{false};
    UiAtlasMetadataGlyph glyph;
};

struct UiAtlasTextPageRow {
    bool has_asset{false};
    bool has_asset_uri{false};
    UiAtlasMetadataPage page;
};

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] bool valid_text_field(std::string_view value) noexcept {
    return value.find('\0') == std::string_view::npos && value.find('\n') == std::string_view::npos &&
           value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_non_empty_text_field(std::string_view value) noexcept {
    return !value.empty() && valid_text_field(value);
}

[[nodiscard]] bool supported_source_decoding(std::string_view value) noexcept {
    return value == unsupported_ui_atlas_capability || value == decoded_image_adapter_source_decoding ||
           value == rasterized_glyph_adapter_source_decoding;
}

[[nodiscard]] bool supported_atlas_packing(std::string_view value) noexcept {
    return value == unsupported_ui_atlas_capability || value == deterministic_rgba8_atlas_packing ||
           value == deterministic_glyph_atlas_packing;
}

[[nodiscard]] bool valid_uv_rect(UiAtlasUvRect uv) noexcept {
    return std::isfinite(uv.u0) && std::isfinite(uv.v0) && std::isfinite(uv.u1) && std::isfinite(uv.v1) &&
           uv.u0 >= 0.0F && uv.v0 >= 0.0F && uv.u1 <= 1.0F && uv.v1 <= 1.0F && uv.u0 < uv.u1 && uv.v0 < uv.v1;
}

[[nodiscard]] bool valid_color(std::array<float, 4> color) noexcept {
    return std::ranges::all_of(color,
                               [](float value) { return std::isfinite(value) && value >= 0.0F && value <= 1.0F; });
}

[[nodiscard]] std::string diagnostic_message(UiAtlasMetadataDiagnosticCode code) {
    switch (code) {
    case UiAtlasMetadataDiagnosticCode::invalid_asset:
        return "ui atlas metadata asset is invalid";
    case UiAtlasMetadataDiagnosticCode::unsupported_source_decoding:
        return "ui atlas source image decoding is not supported";
    case UiAtlasMetadataDiagnosticCode::unsupported_atlas_packing:
        return "ui atlas production atlas packing is not supported";
    case UiAtlasMetadataDiagnosticCode::missing_page:
        return "ui atlas metadata must declare at least one page";
    case UiAtlasMetadataDiagnosticCode::invalid_page_asset:
        return "ui atlas page asset is invalid";
    case UiAtlasMetadataDiagnosticCode::invalid_page_asset_uri:
        return "ui atlas page asset_uri is invalid";
    case UiAtlasMetadataDiagnosticCode::duplicate_page_asset:
        return "ui atlas page asset is duplicated";
    case UiAtlasMetadataDiagnosticCode::missing_image:
        return "ui atlas metadata must declare at least one image or glyph";
    case UiAtlasMetadataDiagnosticCode::missing_image_identity:
        return "ui atlas image binding must declare resource_id or asset_uri";
    case UiAtlasMetadataDiagnosticCode::duplicate_resource_id:
        return "ui atlas image resource_id is duplicated";
    case UiAtlasMetadataDiagnosticCode::duplicate_asset_uri:
        return "ui atlas image asset_uri is duplicated";
    case UiAtlasMetadataDiagnosticCode::undeclared_image_page:
        return "ui atlas image page is not declared";
    case UiAtlasMetadataDiagnosticCode::invalid_uv_rect:
        return "ui atlas image uv rect is invalid";
    case UiAtlasMetadataDiagnosticCode::invalid_color:
        return "ui atlas image color is invalid";
    case UiAtlasMetadataDiagnosticCode::missing_glyph_identity:
        return "ui atlas glyph binding must declare font_family and glyph";
    case UiAtlasMetadataDiagnosticCode::duplicate_glyph:
        return "ui atlas glyph binding is duplicated";
    case UiAtlasMetadataDiagnosticCode::undeclared_glyph_page:
        return "ui atlas glyph page is not declared";
    case UiAtlasMetadataDiagnosticCode::invalid_glyph_uv_rect:
        return "ui atlas glyph uv rect is invalid";
    case UiAtlasMetadataDiagnosticCode::invalid_glyph_color:
        return "ui atlas glyph color is invalid";
    }
    return "ui atlas metadata diagnostic is unknown";
}

void add_diagnostic(std::vector<UiAtlasMetadataDiagnostic>& diagnostics, UiAtlasMetadataDiagnosticCode code,
                    AssetId asset, std::string field = {}) {
    diagnostics.push_back(UiAtlasMetadataDiagnostic{
        .code = code,
        .asset = asset,
        .field = std::move(field),
        .message = diagnostic_message(code),
    });
}

void sort_pages(std::vector<UiAtlasMetadataPage>& pages) {
    std::ranges::sort(pages, [](const UiAtlasMetadataPage& lhs, const UiAtlasMetadataPage& rhs) {
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        return lhs.asset_uri < rhs.asset_uri;
    });
}

void sort_images(std::vector<UiAtlasMetadataImage>& images) {
    std::ranges::sort(images, [](const UiAtlasMetadataImage& lhs, const UiAtlasMetadataImage& rhs) {
        if (lhs.resource_id != rhs.resource_id) {
            return lhs.resource_id < rhs.resource_id;
        }
        if (lhs.asset_uri != rhs.asset_uri) {
            return lhs.asset_uri < rhs.asset_uri;
        }
        return lhs.page.value < rhs.page.value;
    });
}

void sort_glyphs(std::vector<UiAtlasMetadataGlyph>& glyphs) {
    std::ranges::sort(glyphs, [](const UiAtlasMetadataGlyph& lhs, const UiAtlasMetadataGlyph& rhs) {
        if (lhs.font_family != rhs.font_family) {
            return lhs.font_family < rhs.font_family;
        }
        if (lhs.glyph != rhs.glyph) {
            return lhs.glyph < rhs.glyph;
        }
        return lhs.page.value < rhs.page.value;
    });
}

[[nodiscard]] UiAtlasMetadataDocument canonical_document(UiAtlasMetadataDocument document) {
    sort_pages(document.pages);
    sort_images(document.images);
    sort_glyphs(document.glyphs);
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
            throw std::invalid_argument("ui atlas metadata line contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos || separator == 0U) {
            throw std::invalid_argument("ui atlas metadata line is invalid");
        }
        auto [_, inserted] =
            values.emplace(std::string(line.substr(0, separator)), std::string(line.substr(separator + 1U)));
        if (!inserted) {
            throw std::invalid_argument("ui atlas metadata contains duplicate keys");
        }
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const KeyValues& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("ui atlas metadata is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] const std::string* optional_value(const KeyValues& values, const std::string& key) noexcept {
    const auto it = values.find(key);
    return it == values.end() ? nullptr : &it->second;
}

void throw_if_invalid(const UiAtlasMetadataDocument& document) {
    const auto diagnostics = validate_ui_atlas_metadata_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument(diagnostics.front().message);
    }
}

} // namespace

std::vector<UiAtlasMetadataDiagnostic> validate_ui_atlas_metadata_document(const UiAtlasMetadataDocument& document) {
    std::vector<UiAtlasMetadataDiagnostic> diagnostics;
    if (document.asset.value == 0) {
        add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_asset, document.asset, "asset.id");
    }
    if (!supported_source_decoding(document.source_decoding)) {
        add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::unsupported_source_decoding, document.asset,
                       "source.decoding");
    }
    if (!supported_atlas_packing(document.atlas_packing)) {
        add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::unsupported_atlas_packing, document.asset,
                       "atlas.packing");
    }
    if (document.pages.empty()) {
        add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::missing_page, document.asset, "page.count");
    }

    std::unordered_set<AssetId, AssetIdHash> page_assets;
    page_assets.reserve(document.pages.size());
    for (const auto& page : document.pages) {
        if (page.asset.value == 0) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_page_asset, page.asset, "page.asset");
            continue;
        }
        if (!valid_non_empty_text_field(page.asset_uri)) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_page_asset_uri, page.asset,
                           "page.asset_uri");
        }
        if (!page_assets.insert(page.asset).second) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::duplicate_page_asset, page.asset, "page.asset");
        }
    }

    if (document.images.empty() && document.glyphs.empty()) {
        add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::missing_image, document.asset, "image.count");
    }

    std::unordered_set<std::string> resource_ids;
    std::unordered_set<std::string> asset_uris;
    for (const auto& image : document.images) {
        if (!valid_text_field(image.resource_id) || !valid_text_field(image.asset_uri) ||
            (image.resource_id.empty() && image.asset_uri.empty())) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::missing_image_identity, image.page,
                           "image.identity");
        }
        if (!image.resource_id.empty() && !resource_ids.insert(image.resource_id).second) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::duplicate_resource_id, image.page,
                           "image.resource_id");
        }
        if (!image.asset_uri.empty() && !asset_uris.insert(image.asset_uri).second) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::duplicate_asset_uri, image.page,
                           "image.asset_uri");
        }
        if (page_assets.find(image.page) == page_assets.end()) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::undeclared_image_page, image.page, "image.page");
        }
        if (!valid_uv_rect(image.uv)) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_uv_rect, image.page, "image.uv");
        }
        if (!valid_color(image.color)) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_color, image.page, "image.color");
        }
    }

    std::unordered_set<std::string> glyph_keys;
    for (const auto& glyph : document.glyphs) {
        if (!valid_non_empty_text_field(glyph.font_family) || glyph.glyph == 0) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::missing_glyph_identity, glyph.page,
                           "glyph.identity");
        }

        const auto key = glyph.font_family + '\0' + std::to_string(glyph.glyph);
        if (!glyph.font_family.empty() && glyph.glyph != 0 && !glyph_keys.insert(key).second) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::duplicate_glyph, glyph.page, "glyph.identity");
        }

        if (page_assets.find(glyph.page) == page_assets.end()) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::undeclared_glyph_page, glyph.page, "glyph.page");
        }
        if (!valid_uv_rect(glyph.uv)) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_glyph_uv_rect, glyph.page, "glyph.uv");
        }
        if (!valid_color(glyph.color)) {
            add_diagnostic(diagnostics, UiAtlasMetadataDiagnosticCode::invalid_glyph_color, glyph.page, "glyph.color");
        }
    }

    return diagnostics;
}

std::string serialize_ui_atlas_metadata_document(const UiAtlasMetadataDocument& document) {
    throw_if_invalid(document);
    const auto canonical = canonical_document(document);

    std::ostringstream output;
    output << "format=GameEngine.UiAtlas.v1\n";
    output << "asset.id=" << canonical.asset.value << '\n';
    output << "asset.kind=ui_atlas\n";
    output << "source.decoding=" << canonical.source_decoding << '\n';
    output << "atlas.packing=" << canonical.atlas_packing << '\n';
    output << "page.count=" << canonical.pages.size() << '\n';
    for (std::size_t index = 0; index < canonical.pages.size(); ++index) {
        const auto& page = canonical.pages[index];
        output << "page." << index << ".asset=" << page.asset.value << '\n';
        output << "page." << index << ".asset_uri=" << page.asset_uri << '\n';
    }
    output << "image.count=" << canonical.images.size() << '\n';
    for (std::size_t index = 0; index < canonical.images.size(); ++index) {
        const auto& image = canonical.images[index];
        output << "image." << index << ".resource_id=" << image.resource_id << '\n';
        output << "image." << index << ".asset_uri=" << image.asset_uri << '\n';
        output << "image." << index << ".page=" << image.page.value << '\n';
        output << "image." << index << ".u0=" << image.uv.u0 << '\n';
        output << "image." << index << ".v0=" << image.uv.v0 << '\n';
        output << "image." << index << ".u1=" << image.uv.u1 << '\n';
        output << "image." << index << ".v1=" << image.uv.v1 << '\n';
        output << "image." << index << ".color=" << image.color[0] << ',' << image.color[1] << ',' << image.color[2]
               << ',' << image.color[3] << '\n';
    }
    output << "glyph.count=" << canonical.glyphs.size() << '\n';
    for (std::size_t index = 0; index < canonical.glyphs.size(); ++index) {
        const auto& glyph = canonical.glyphs[index];
        output << "glyph." << index << ".font_family=" << glyph.font_family << '\n';
        output << "glyph." << index << ".glyph=" << glyph.glyph << '\n';
        output << "glyph." << index << ".page=" << glyph.page.value << '\n';
        output << "glyph." << index << ".u0=" << glyph.uv.u0 << '\n';
        output << "glyph." << index << ".v0=" << glyph.uv.v0 << '\n';
        output << "glyph." << index << ".u1=" << glyph.uv.u1 << '\n';
        output << "glyph." << index << ".v1=" << glyph.uv.v1 << '\n';
        output << "glyph." << index << ".color=" << glyph.color[0] << ',' << glyph.color[1] << ',' << glyph.color[2]
               << ',' << glyph.color[3] << '\n';
    }
    return output.str();
}

UiAtlasMetadataDocument deserialize_ui_atlas_metadata_document(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != "GameEngine.UiAtlas.v1") {
        throw std::invalid_argument("ui atlas metadata format is unsupported");
    }
    if (required_value(values, "asset.kind") != "ui_atlas") {
        throw std::invalid_argument("ui atlas metadata asset kind is unsupported");
    }

    UiAtlasMetadataDocument document;
    document.asset = AssetId{parse_u64(required_value(values, "asset.id"), "ui atlas metadata asset id")};
    document.source_decoding = required_value(values, "source.decoding");
    document.atlas_packing = required_value(values, "atlas.packing");

    const auto page_count = parse_u32(required_value(values, "page.count"), "ui atlas metadata page count");
    document.pages.reserve(page_count);
    for (std::uint32_t ordinal = 0; ordinal < page_count; ++ordinal) {
        const auto prefix = "page." + std::to_string(ordinal) + ".";
        document.pages.push_back(UiAtlasMetadataPage{
            .asset = AssetId{parse_u64(required_value(values, prefix + "asset"), "ui atlas metadata page asset")},
            .asset_uri = required_value(values, prefix + "asset_uri"),
        });
    }

    const auto image_count = parse_u32(required_value(values, "image.count"), "ui atlas metadata image count");
    document.images.reserve(image_count);
    for (std::uint32_t ordinal = 0; ordinal < image_count; ++ordinal) {
        const auto prefix = "image." + std::to_string(ordinal) + ".";
        document.images.push_back(UiAtlasMetadataImage{
            .resource_id = required_value(values, prefix + "resource_id"),
            .asset_uri = required_value(values, prefix + "asset_uri"),
            .page = AssetId{parse_u64(required_value(values, prefix + "page"), "ui atlas metadata image page")},
            .uv =
                UiAtlasUvRect{
                    .u0 = parse_float(required_value(values, prefix + "u0"), "ui atlas metadata image u0"),
                    .v0 = parse_float(required_value(values, prefix + "v0"), "ui atlas metadata image v0"),
                    .u1 = parse_float(required_value(values, prefix + "u1"), "ui atlas metadata image u1"),
                    .v1 = parse_float(required_value(values, prefix + "v1"), "ui atlas metadata image v1"),
                },
            .color = parse_float4(required_value(values, prefix + "color"), "ui atlas metadata image color"),
        });
    }

    const auto* glyph_count_value = optional_value(values, "glyph.count");
    const auto glyph_count =
        glyph_count_value == nullptr ? 0U : parse_u32(*glyph_count_value, "ui atlas metadata glyph count");
    document.glyphs.reserve(glyph_count);
    for (std::uint32_t ordinal = 0; ordinal < glyph_count; ++ordinal) {
        const auto prefix = "glyph." + std::to_string(ordinal) + ".";
        document.glyphs.push_back(UiAtlasMetadataGlyph{
            .font_family = required_value(values, prefix + "font_family"),
            .glyph = parse_u32(required_value(values, prefix + "glyph"), "ui atlas metadata glyph id"),
            .page = AssetId{parse_u64(required_value(values, prefix + "page"), "ui atlas metadata glyph page")},
            .uv =
                UiAtlasUvRect{
                    .u0 = parse_float(required_value(values, prefix + "u0"), "ui atlas metadata glyph u0"),
                    .v0 = parse_float(required_value(values, prefix + "v0"), "ui atlas metadata glyph v0"),
                    .u1 = parse_float(required_value(values, prefix + "u1"), "ui atlas metadata glyph u1"),
                    .v1 = parse_float(required_value(values, prefix + "v1"), "ui atlas metadata glyph v1"),
                },
            .color = parse_float4(required_value(values, prefix + "color"), "ui atlas metadata glyph color"),
        });
    }

    throw_if_invalid(document);
    return canonical_document(std::move(document));
}

} // namespace mirakana
