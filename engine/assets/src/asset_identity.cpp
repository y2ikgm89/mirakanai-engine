// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_identity.hpp"

#include "mirakana/assets/asset_registry.hpp"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mirakana {
namespace {

struct AssetIdentityTextRow {
    bool has_key{false};
    bool has_id{false};
    bool has_kind{false};
    bool has_source{false};
    std::string key;
    std::uint64_t id{0};
    AssetKind kind{AssetKind::unknown};
    std::string source;
};

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool contains_ascii_control(std::string_view value) noexcept {
    return std::ranges::any_of(value, is_ascii_control);
}

[[nodiscard]] bool contains_ascii_whitespace(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool contains_parent_segment(std::string_view value) noexcept {
    std::size_t segment_begin = 0;
    while (segment_begin <= value.size()) {
        const auto segment_end = value.find('/', segment_begin);
        const auto segment =
            value.substr(segment_begin, segment_end == std::string_view::npos ? value.size() - segment_begin
                                                                              : segment_end - segment_begin);
        if (segment == "..") {
            return true;
        }
        if (segment_end == std::string_view::npos) {
            break;
        }
        segment_begin = segment_end + 1U;
    }
    return false;
}

[[nodiscard]] bool valid_key(std::string_view key) noexcept {
    return !key.empty() && !contains_ascii_control(key) && !contains_ascii_whitespace(key) && key.front() != '/' &&
           !contains_parent_segment(key);
}

[[nodiscard]] bool valid_source_path(std::string_view source_path) noexcept {
    return !source_path.empty() && !contains_ascii_control(source_path) && source_path.front() != '/' &&
           !contains_parent_segment(source_path) && source_path.find(';') == std::string_view::npos;
}

[[nodiscard]] bool valid_kind(AssetKind kind) noexcept {
    return kind != AssetKind::unknown;
}

[[nodiscard]] bool is_ascii_alnum(char character) noexcept {
    return std::isalnum(static_cast<unsigned char>(character)) != 0;
}

[[nodiscard]] bool is_placement_segment_character(char character) noexcept {
    return is_ascii_alnum(character) || character == '_' || character == '-';
}

[[nodiscard]] bool valid_placement(std::string_view placement) noexcept {
    if (placement.empty() || contains_ascii_control(placement) || contains_ascii_whitespace(placement) ||
        placement.front() == '.' || placement.back() == '.') {
        return false;
    }

    bool previous_was_separator = false;
    for (const char character : placement) {
        if (character == '.') {
            if (previous_was_separator) {
                return false;
            }
            previous_was_separator = true;
            continue;
        }
        if (!is_placement_segment_character(character)) {
            return false;
        }
        previous_was_separator = false;
    }
    return true;
}

[[nodiscard]] std::string_view asset_kind_name(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return "texture";
    case AssetKind::mesh:
        return "mesh";
    case AssetKind::morph_mesh_cpu:
        return "morph_mesh_cpu";
    case AssetKind::animation_float_clip:
        return "animation_float_clip";
    case AssetKind::animation_quaternion_clip:
        return "animation_quaternion_clip";
    case AssetKind::sprite_animation:
        return "sprite_animation";
    case AssetKind::skinned_mesh:
        return "skinned_mesh";
    case AssetKind::material:
        return "material";
    case AssetKind::scene:
        return "scene";
    case AssetKind::audio:
        return "audio";
    case AssetKind::script:
        return "script";
    case AssetKind::shader:
        return "shader";
    case AssetKind::ui_atlas:
        return "ui_atlas";
    case AssetKind::tilemap:
        return "tilemap";
    case AssetKind::physics_collision_scene:
        return "physics_collision_scene";
    case AssetKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] AssetKind parse_asset_kind(std::string_view value) noexcept {
    if (value == "texture") {
        return AssetKind::texture;
    }
    if (value == "mesh") {
        return AssetKind::mesh;
    }
    if (value == "morph_mesh_cpu") {
        return AssetKind::morph_mesh_cpu;
    }
    if (value == "animation_float_clip") {
        return AssetKind::animation_float_clip;
    }
    if (value == "animation_quaternion_clip") {
        return AssetKind::animation_quaternion_clip;
    }
    if (value == "sprite_animation") {
        return AssetKind::sprite_animation;
    }
    if (value == "skinned_mesh") {
        return AssetKind::skinned_mesh;
    }
    if (value == "material") {
        return AssetKind::material;
    }
    if (value == "scene") {
        return AssetKind::scene;
    }
    if (value == "audio") {
        return AssetKind::audio;
    }
    if (value == "script") {
        return AssetKind::script;
    }
    if (value == "shader") {
        return AssetKind::shader;
    }
    if (value == "ui_atlas") {
        return AssetKind::ui_atlas;
    }
    if (value == "tilemap") {
        return AssetKind::tilemap;
    }
    if (value == "physics_collision_scene") {
        return AssetKind::physics_collision_scene;
    }
    return AssetKind::unknown;
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] std::uint64_t parse_decimal_u64(std::string_view value, std::string_view error_message) {
    if (value.empty()) {
        throw std::invalid_argument(std::string(error_message));
    }

    std::uint64_t parsed = 0;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string(error_message));
        }
        const auto digit = static_cast<std::uint64_t>(character - '0');
        if (parsed > (std::numeric_limits<std::uint64_t>::max() - digit) / 10U) {
            throw std::invalid_argument(std::string(error_message));
        }
        parsed = (parsed * 10U) + digit;
    }
    return parsed;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value) {
    return parse_decimal_u64(value, "asset identity integer value is invalid");
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value) {
    const auto parsed = parse_decimal_u64(value, "asset identity row ordinal is invalid");
    if (parsed >= static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        throw std::invalid_argument("asset identity row ordinal is invalid");
    }
    return static_cast<std::size_t>(parsed);
}

void parse_asset_row_value(std::unordered_map<std::size_t, AssetIdentityTextRow>& rows, std::string_view key,
                           std::string_view value) {
    constexpr std::string_view prefix = "asset.";
    if (!starts_with(key, prefix)) {
        throw std::invalid_argument("asset identity text contains unsupported key");
    }

    const auto after_prefix = key.substr(prefix.size());
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("asset identity row key is malformed");
    }

    const auto ordinal = parse_ordinal(after_prefix.substr(0, separator));
    const auto field = after_prefix.substr(separator + 1U);
    auto& row = rows[ordinal];

    if (field == "key") {
        row.has_key = true;
        row.key = std::string(value);
    } else if (field == "id") {
        row.has_id = true;
        row.id = parse_u64(value);
    } else if (field == "kind") {
        row.has_kind = true;
        row.kind = parse_asset_kind(value);
    } else if (field == "source") {
        row.has_source = true;
        row.source = std::string(value);
    } else {
        throw std::invalid_argument("asset identity row field is unsupported");
    }
}

} // namespace

AssetId asset_id_from_key_v2(const AssetKeyV2& key) noexcept {
    return AssetId::from_name(key.value);
}

std::vector<AssetIdentityDiagnosticV2> validate_asset_identity_document_v2(const AssetIdentityDocumentV2& document) {
    std::vector<AssetIdentityDiagnosticV2> diagnostics;
    std::unordered_set<std::string> keys;
    std::unordered_set<std::string> source_paths;

    for (const auto& row : document.assets) {
        if (!valid_key(row.key.value)) {
            diagnostics.push_back(AssetIdentityDiagnosticV2{
                .code = AssetIdentityDiagnosticCodeV2::invalid_key,
                .key = row.key,
                .source_path = row.source_path,
            });
        } else if (!keys.insert(row.key.value).second) {
            diagnostics.push_back(AssetIdentityDiagnosticV2{
                .code = AssetIdentityDiagnosticCodeV2::duplicate_key,
                .key = row.key,
                .source_path = row.source_path,
            });
        }

        if (!valid_kind(row.kind)) {
            diagnostics.push_back(AssetIdentityDiagnosticV2{
                .code = AssetIdentityDiagnosticCodeV2::invalid_kind,
                .key = row.key,
                .source_path = row.source_path,
            });
        }

        if (!valid_source_path(row.source_path)) {
            diagnostics.push_back(AssetIdentityDiagnosticV2{
                .code = AssetIdentityDiagnosticCodeV2::invalid_source_path,
                .key = row.key,
                .source_path = row.source_path,
            });
        } else if (!source_paths.insert(row.source_path).second) {
            diagnostics.push_back(AssetIdentityDiagnosticV2{
                .code = AssetIdentityDiagnosticCodeV2::duplicate_source_path,
                .key = row.key,
                .source_path = row.source_path,
            });
        }
    }

    return diagnostics;
}

std::string serialize_asset_identity_document_v2(const AssetIdentityDocumentV2& document) {
    const auto diagnostics = validate_asset_identity_document_v2(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset identity document is invalid");
    }

    std::ostringstream output;
    output << "format=GameEngine.AssetIdentity.v2\n";
    for (std::size_t ordinal = 0; ordinal < document.assets.size(); ++ordinal) {
        const auto& row = document.assets[ordinal];
        output << "asset." << ordinal << ".key=" << row.key.value << '\n';
        output << "asset." << ordinal << ".id=" << asset_id_from_key_v2(row.key).value << '\n';
        output << "asset." << ordinal << ".kind=" << asset_kind_name(row.kind) << '\n';
        output << "asset." << ordinal << ".source=" << row.source_path << '\n';
    }
    return output.str();
}

AssetIdentityDocumentV2 deserialize_asset_identity_document_v2(std::string_view text) {
    bool saw_format = false;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, AssetIdentityTextRow> rows;

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
            throw std::invalid_argument("asset identity text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("asset identity line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string(key)).second) {
            throw std::invalid_argument("asset identity text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != "GameEngine.AssetIdentity.v2") {
                throw std::invalid_argument("asset identity format is unsupported");
            }
            continue;
        }

        parse_asset_row_value(rows, key, value);
    }

    if (!saw_format) {
        throw std::invalid_argument("asset identity format is missing");
    }

    std::vector<std::size_t> ordinals;
    ordinals.reserve(rows.size());
    for (const auto& [ordinal, _] : rows) {
        ordinals.push_back(ordinal);
    }
    std::ranges::sort(ordinals);

    AssetIdentityDocumentV2 document;
    document.assets.reserve(ordinals.size());
    std::size_t expected_ordinal = 0;
    for (const auto ordinal : ordinals) {
        if (ordinal != expected_ordinal) {
            throw std::invalid_argument("asset identity row ordinals are not contiguous");
        }
        ++expected_ordinal;

        const auto& row = rows.at(ordinal);
        if (!row.has_key || !row.has_id || !row.has_kind || !row.has_source) {
            throw std::invalid_argument("asset identity row is incomplete");
        }
        if (AssetId{row.id} != AssetId::from_name(row.key)) {
            throw std::invalid_argument("asset identity row id does not match key");
        }
        document.assets.push_back(AssetIdentityRowV2{
            .key = AssetKeyV2{row.key},
            .kind = row.kind,
            .source_path = row.source,
        });
    }

    const auto diagnostics = validate_asset_identity_document_v2(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("asset identity document is invalid");
    }
    return document;
}

AssetIdentityPlacementPlanV2
plan_asset_identity_placements_v2(const AssetIdentityDocumentV2& document,
                                  std::span<const AssetIdentityPlacementRequestV2> requests) {
    AssetIdentityPlacementPlanV2 plan;

    plan.identity_diagnostics = validate_asset_identity_document_v2(document);
    if (!plan.identity_diagnostics.empty()) {
        plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
            .code = AssetIdentityPlacementDiagnosticCodeV2::invalid_identity_document,
        });
        return plan;
    }

    std::unordered_map<std::string, const AssetIdentityRowV2*> rows_by_key;
    rows_by_key.reserve(document.assets.size());
    for (const auto& row : document.assets) {
        rows_by_key.emplace(row.key.value, &row);
    }

    std::unordered_set<std::string> placements;
    placements.reserve(requests.size());
    for (const auto& request : requests) {
        bool request_valid = true;
        if (!valid_placement(request.placement)) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::invalid_placement,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
            });
            request_valid = false;
        } else if (!placements.insert(request.placement).second) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::duplicate_placement,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
            });
            request_valid = false;
        }

        if (!valid_key(request.key.value)) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::invalid_key,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
            });
            request_valid = false;
        }
        if (!valid_kind(request.expected_kind)) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::invalid_expected_kind,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
            });
            request_valid = false;
        }
        if (!request_valid) {
            continue;
        }

        const auto row = rows_by_key.find(request.key.value);
        if (row == rows_by_key.end()) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::missing_key,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
            });
            continue;
        }

        if (row->second->kind != request.expected_kind) {
            plan.diagnostics.push_back(AssetIdentityPlacementDiagnosticV2{
                .code = AssetIdentityPlacementDiagnosticCodeV2::kind_mismatch,
                .placement = request.placement,
                .key = request.key,
                .expected_kind = request.expected_kind,
                .actual_kind = row->second->kind,
            });
            continue;
        }

        plan.rows.push_back(AssetIdentityPlacementRowV2{
            .placement = request.placement,
            .key = row->second->key,
            .id = asset_id_from_key_v2(row->second->key),
            .kind = row->second->kind,
            .source_path = row->second->source_path,
        });
    }

    plan.can_place = plan.diagnostics.empty();
    if (!plan.can_place) {
        plan.rows.clear();
    }
    return plan;
}

} // namespace mirakana
