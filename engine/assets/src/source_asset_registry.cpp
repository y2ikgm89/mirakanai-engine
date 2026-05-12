// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/source_asset_registry.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

constexpr std::string_view source_asset_registry_format = "GameEngine.SourceAssetRegistry.v1";
constexpr std::string_view texture_source_format = "GameEngine.TextureSource.v1";
constexpr std::string_view mesh_source_format = "GameEngine.MeshSource.v2";
constexpr std::string_view audio_source_format = "GameEngine.AudioSource.v1";
constexpr std::string_view material_source_format = "GameEngine.Material.v1";
constexpr std::string_view scene_source_format = "GameEngine.Scene.v1";
constexpr std::string_view morph_mesh_cpu_source_format = "GameEngine.MorphMeshCpuSource.v1";
constexpr std::string_view animation_float_clip_source_format = "GameEngine.AnimationFloatClipSource.v1";
constexpr std::string_view animation_quaternion_clip_source_format = "GameEngine.AnimationQuaternionClipSource.v1";

struct SourceAssetTextDependencyRow {
    bool has_kind{false};
    bool has_key{false};
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    std::string key;
};

struct SourceAssetTextRow {
    bool has_key{false};
    bool has_id{false};
    bool has_kind{false};
    bool has_source{false};
    bool has_source_format{false};
    bool has_imported{false};
    std::string key;
    std::uint64_t id{0};
    AssetKind kind{AssetKind::unknown};
    std::string source;
    std::string source_format;
    std::string imported;
    std::unordered_map<std::size_t, SourceAssetTextDependencyRow> dependencies;
};

[[nodiscard]] bool is_ascii_control(char character) noexcept {
    const auto value = static_cast<unsigned char>(character);
    return value < 0x20U || value == 0x7FU;
}

[[nodiscard]] bool has_invalid_path_character(std::string_view value) noexcept {
    return std::ranges::any_of(value, [](char character) {
        return is_ascii_control(character) || character == '\\' || character == ':' || character == ';';
    });
}

[[nodiscard]] bool has_ascii_space(std::string_view value) noexcept {
    return std::ranges::any_of(value,
                               [](char character) { return std::isspace(static_cast<unsigned char>(character)) != 0; });
}

[[nodiscard]] bool is_valid_key_segment(std::string_view segment) noexcept {
    if (segment.empty() || segment == "." || segment == "..") {
        return false;
    }
    return std::ranges::all_of(segment, [](char character) {
        return (character >= 'a' && character <= 'z') || (character >= '0' && character <= '9') || character == '.' ||
               character == '_' || character == '-';
    });
}

[[nodiscard]] bool is_valid_asset_key(std::string_view key) noexcept {
    if (key.empty() || key.front() == '/' || key.find('\\') != std::string_view::npos ||
        key.find(':') != std::string_view::npos || has_ascii_space(key)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= key.size()) {
        const auto end = key.find('/', begin);
        const auto segment = key.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (!is_valid_key_segment(segment)) {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return true;
}

[[nodiscard]] bool is_safe_repository_path(std::string_view path) noexcept {
    if (path.empty() || path.front() == '/' || path.front() == '\\' || has_invalid_path_character(path)) {
        return false;
    }

    std::size_t begin = 0;
    while (begin <= path.size()) {
        const auto end = path.find('/', begin);
        const auto segment = path.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        if (segment.empty() || segment == "." || segment == "..") {
            return false;
        }
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
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

[[nodiscard]] bool dependency_kind_allowed_for_asset(AssetKind asset_kind,
                                                     AssetDependencyKind dependency_kind) noexcept {
    if (asset_kind == AssetKind::material) {
        return dependency_kind == AssetDependencyKind::material_texture;
    }
    if (asset_kind == AssetKind::scene) {
        return dependency_kind == AssetDependencyKind::scene_mesh ||
               dependency_kind == AssetDependencyKind::scene_material ||
               dependency_kind == AssetDependencyKind::scene_sprite;
    }
    return false;
}

[[nodiscard]] bool dependency_target_kind_matches(AssetDependencyKind dependency_kind, AssetKind target_kind) noexcept {
    switch (dependency_kind) {
    case AssetDependencyKind::material_texture:
    case AssetDependencyKind::scene_sprite:
        return target_kind == AssetKind::texture;
    case AssetDependencyKind::scene_mesh:
        return target_kind == AssetKind::mesh || target_kind == AssetKind::skinned_mesh;
    case AssetDependencyKind::scene_material:
        return target_kind == AssetKind::material;
    case AssetDependencyKind::unknown:
    case AssetDependencyKind::shader_include:
    case AssetDependencyKind::ui_atlas_texture:
    case AssetDependencyKind::tilemap_texture:
    case AssetDependencyKind::sprite_animation_texture:
    case AssetDependencyKind::sprite_animation_material:
    case AssetDependencyKind::generated_artifact:
    case AssetDependencyKind::source_file:
        break;
    }
    return false;
}

[[nodiscard]] bool starts_with(std::string_view value, std::string_view prefix) noexcept {
    return value.size() >= prefix.size() && value.substr(0, prefix.size()) == prefix;
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value) {
    std::uint64_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size()) {
        throw std::invalid_argument("source asset registry integer value is invalid");
    }
    return parsed;
}

[[nodiscard]] std::size_t parse_ordinal(std::string_view value) {
    std::size_t parsed = 0;
    const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (error != std::errc{} || end != value.data() + value.size() ||
        parsed == std::numeric_limits<std::size_t>::max()) {
        throw std::invalid_argument("source asset registry row ordinal is invalid");
    }
    return parsed;
}

void parse_dependency_value(SourceAssetTextRow& row, std::string_view dependency_suffix, std::string_view value) {
    const auto separator = dependency_suffix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("source asset registry dependency row key is malformed");
    }

    const auto ordinal = parse_ordinal(dependency_suffix.substr(0, separator));
    const auto field = dependency_suffix.substr(separator + 1U);
    auto& dependency = row.dependencies[ordinal];

    if (field == "kind") {
        dependency.has_kind = true;
        dependency.kind = parse_source_asset_dependency_kind_v1(value);
    } else if (field == "key") {
        dependency.has_key = true;
        dependency.key = std::string(value);
    } else {
        throw std::invalid_argument("source asset registry dependency field is unsupported");
    }
}

void parse_asset_row_value(std::unordered_map<std::size_t, SourceAssetTextRow>& rows, std::string_view key,
                           std::string_view value) {
    constexpr std::string_view prefix = "asset.";
    if (!starts_with(key, prefix)) {
        throw std::invalid_argument("source asset registry text contains unsupported key");
    }

    const auto after_prefix = key.substr(prefix.size());
    const auto separator = after_prefix.find('.');
    if (separator == std::string_view::npos) {
        throw std::invalid_argument("source asset registry row key is malformed");
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
    } else if (field == "source_format") {
        row.has_source_format = true;
        row.source_format = std::string(value);
    } else if (field == "imported") {
        row.has_imported = true;
        row.imported = std::string(value);
    } else if (starts_with(field, "dependency.")) {
        parse_dependency_value(row, field.substr(std::string_view{"dependency."}.size()), value);
    } else {
        throw std::invalid_argument("source asset registry row field is unsupported");
    }
}

void add_diagnostic(std::vector<SourceAssetRegistryDiagnosticV1>& diagnostics, SourceAssetRegistryDiagnosticCodeV1 code,
                    const SourceAssetRegistryRowV1& row, std::string path = {},
                    const SourceAssetDependencyRowV1* dependency = nullptr) {
    SourceAssetRegistryDiagnosticV1 diagnostic;
    diagnostic.code = code;
    diagnostic.key = row.key;
    diagnostic.path = std::move(path);
    if (dependency != nullptr) {
        diagnostic.dependency_kind = dependency->kind;
        diagnostic.dependency_key = dependency->key;
    }
    diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] std::string dependency_sort_key(const SourceAssetDependencyRowV1& dependency) {
    return std::string{source_asset_dependency_kind_name_v1(dependency.kind)} + ":" + dependency.key.value;
}

void sort_diagnostics(std::vector<SourceAssetRegistryDiagnosticV1>& diagnostics) {
    std::ranges::sort(diagnostics,
                      [](const SourceAssetRegistryDiagnosticV1& lhs, const SourceAssetRegistryDiagnosticV1& rhs) {
                          if (lhs.key.value != rhs.key.value) {
                              return lhs.key.value < rhs.key.value;
                          }
                          if (lhs.path != rhs.path) {
                              return lhs.path < rhs.path;
                          }
                          if (lhs.code != rhs.code) {
                              return static_cast<int>(lhs.code) < static_cast<int>(rhs.code);
                          }
                          if (lhs.dependency_key.value != rhs.dependency_key.value) {
                              return lhs.dependency_key.value < rhs.dependency_key.value;
                          }
                          return source_asset_dependency_kind_name_v1(lhs.dependency_kind) <
                                 source_asset_dependency_kind_name_v1(rhs.dependency_kind);
                      });
}

} // namespace

std::string_view source_asset_registry_format_v1() noexcept {
    return source_asset_registry_format;
}

bool is_supported_source_asset_kind_v1(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
    case AssetKind::mesh:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::audio:
    case AssetKind::material:
    case AssetKind::scene:
        return true;
    case AssetKind::unknown:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
        break;
    }
    return false;
}

std::string_view expected_source_asset_format_v1(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
        return texture_source_format;
    case AssetKind::mesh:
        return mesh_source_format;
    case AssetKind::morph_mesh_cpu:
        return morph_mesh_cpu_source_format;
    case AssetKind::animation_float_clip:
        return animation_float_clip_source_format;
    case AssetKind::animation_quaternion_clip:
        return animation_quaternion_clip_source_format;
    case AssetKind::audio:
        return audio_source_format;
    case AssetKind::material:
        return material_source_format;
    case AssetKind::scene:
        return scene_source_format;
    case AssetKind::unknown:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
        break;
    }
    return {};
}

std::string_view source_asset_dependency_kind_name_v1(AssetDependencyKind kind) noexcept {
    switch (kind) {
    case AssetDependencyKind::material_texture:
        return "material_texture";
    case AssetDependencyKind::scene_mesh:
        return "scene_mesh";
    case AssetDependencyKind::scene_material:
        return "scene_material";
    case AssetDependencyKind::scene_sprite:
        return "scene_sprite";
    case AssetDependencyKind::unknown:
    case AssetDependencyKind::shader_include:
    case AssetDependencyKind::ui_atlas_texture:
    case AssetDependencyKind::tilemap_texture:
    case AssetDependencyKind::sprite_animation_texture:
    case AssetDependencyKind::sprite_animation_material:
    case AssetDependencyKind::generated_artifact:
    case AssetDependencyKind::source_file:
        break;
    }
    return "unknown";
}

AssetDependencyKind parse_source_asset_dependency_kind_v1(std::string_view value) noexcept {
    if (value == "material_texture") {
        return AssetDependencyKind::material_texture;
    }
    if (value == "scene_mesh") {
        return AssetDependencyKind::scene_mesh;
    }
    if (value == "scene_material") {
        return AssetDependencyKind::scene_material;
    }
    if (value == "scene_sprite") {
        return AssetDependencyKind::scene_sprite;
    }
    return AssetDependencyKind::unknown;
}

std::vector<SourceAssetRegistryDiagnosticV1>
validate_source_asset_registry_document(const SourceAssetRegistryDocumentV1& document) {
    std::vector<SourceAssetRegistryDiagnosticV1> diagnostics;
    std::unordered_set<std::string> keys;
    std::unordered_set<std::uint64_t> ids;
    std::unordered_set<std::string> source_paths;
    std::unordered_set<std::string> imported_paths;
    std::unordered_map<std::string, AssetKind> kinds_by_valid_key;

    for (const auto& row : document.assets) {
        if (is_valid_asset_key(row.key.value)) {
            kinds_by_valid_key.emplace(row.key.value, row.kind);
        }
    }

    for (const auto& row : document.assets) {
        if (!is_valid_asset_key(row.key.value)) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_key, row);
        } else {
            if (!keys.insert(row.key.value).second) {
                add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::duplicate_key, row);
            }
            const auto id = asset_id_from_key_v2(row.key);
            if (!ids.insert(id.value).second) {
                add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::duplicate_asset_id, row);
            }
        }

        if (!is_supported_source_asset_kind_v1(row.kind)) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_kind, row);
        }

        if (!is_safe_repository_path(row.source_path)) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_source_path, row, row.source_path);
        } else if (!source_paths.insert(row.source_path).second) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::duplicate_source_path, row,
                           row.source_path);
        }

        if (row.source_format != expected_source_asset_format_v1(row.kind)) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_source_format, row);
        }

        if (!is_safe_repository_path(row.imported_path)) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_imported_path, row,
                           row.imported_path);
        } else if (!imported_paths.insert(row.imported_path).second) {
            add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::duplicate_imported_path, row,
                           row.imported_path);
        }

        std::unordered_set<std::string> dependencies;
        for (const auto& dependency : row.dependencies) {
            if (!dependency_kind_allowed_for_asset(row.kind, dependency.kind)) {
                add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_kind, row, {},
                               &dependency);
            }
            if (!is_valid_asset_key(dependency.key.value)) {
                add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_key, row, {},
                               &dependency);
            } else {
                const auto target_kind = kinds_by_valid_key.find(dependency.key.value);
                if (target_kind == kinds_by_valid_key.end()) {
                    add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::missing_dependency_key, row, {},
                                   &dependency);
                } else if (dependency.key.value == row.key.value ||
                           !dependency_target_kind_matches(dependency.kind, target_kind->second)) {
                    add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::invalid_dependency_target, row, {},
                                   &dependency);
                }
            }

            if (!dependencies.insert(dependency_sort_key(dependency)).second) {
                add_diagnostic(diagnostics, SourceAssetRegistryDiagnosticCodeV1::duplicate_dependency, row, {},
                               &dependency);
            }
        }
    }

    if (diagnostics.empty()) {
        const auto identity = project_source_asset_registry_identity_v2(document);
        if (!validate_asset_identity_document_v2(identity).empty()) {
            diagnostics.push_back(SourceAssetRegistryDiagnosticV1{
                .code = SourceAssetRegistryDiagnosticCodeV1::invalid_identity_projection,
                .key = {},
                .path = {},
                .dependency_kind = AssetDependencyKind::unknown,
                .dependency_key = {},
            });
        }

        try {
            (void)build_source_asset_import_metadata_registry(document);
        } catch (const std::exception&) {
            diagnostics.push_back(SourceAssetRegistryDiagnosticV1{
                .code = SourceAssetRegistryDiagnosticCodeV1::invalid_import_metadata,
                .key = {},
                .path = {},
                .dependency_kind = AssetDependencyKind::unknown,
                .dependency_key = {},
            });
        }
    }

    sort_diagnostics(diagnostics);
    return diagnostics;
}

std::string serialize_source_asset_registry_document(const SourceAssetRegistryDocumentV1& document) {
    const auto diagnostics = validate_source_asset_registry_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("source asset registry document is invalid");
    }

    std::ostringstream output;
    output << "format=" << source_asset_registry_format << '\n';
    for (std::size_t ordinal = 0; ordinal < document.assets.size(); ++ordinal) {
        const auto& row = document.assets[ordinal];
        output << "asset." << ordinal << ".key=" << row.key.value << '\n';
        output << "asset." << ordinal << ".id=" << asset_id_from_key_v2(row.key).value << '\n';
        output << "asset." << ordinal << ".kind=" << asset_kind_name(row.kind) << '\n';
        output << "asset." << ordinal << ".source=" << row.source_path << '\n';
        output << "asset." << ordinal << ".source_format=" << row.source_format << '\n';
        output << "asset." << ordinal << ".imported=" << row.imported_path << '\n';
        for (std::size_t dependency_ordinal = 0; dependency_ordinal < row.dependencies.size(); ++dependency_ordinal) {
            const auto& dependency = row.dependencies[dependency_ordinal];
            output << "asset." << ordinal << ".dependency." << dependency_ordinal
                   << ".kind=" << source_asset_dependency_kind_name_v1(dependency.kind) << '\n';
            output << "asset." << ordinal << ".dependency." << dependency_ordinal << ".key=" << dependency.key.value
                   << '\n';
        }
    }
    return output.str();
}

SourceAssetRegistryDocumentV1 parse_source_asset_registry_document_unvalidated_v1(std::string_view text) {
    bool saw_format = false;
    std::unordered_set<std::string> seen_keys;
    std::unordered_map<std::size_t, SourceAssetTextRow> rows;

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
            throw std::invalid_argument("source asset registry text contains carriage return");
        }
        const auto separator = line.find('=');
        if (separator == std::string_view::npos) {
            throw std::invalid_argument("source asset registry line is missing '='");
        }
        const auto key = line.substr(0, separator);
        const auto value = line.substr(separator + 1U);
        if (!seen_keys.insert(std::string(key)).second) {
            throw std::invalid_argument("source asset registry text contains duplicate keys");
        }
        if (key == "format") {
            saw_format = true;
            if (value != source_asset_registry_format) {
                throw std::invalid_argument("source asset registry format is unsupported");
            }
            continue;
        }

        parse_asset_row_value(rows, key, value);
    }

    if (!saw_format) {
        throw std::invalid_argument("source asset registry format is missing");
    }

    std::vector<std::size_t> ordinals;
    ordinals.reserve(rows.size());
    for (const auto& [ordinal, _] : rows) {
        ordinals.push_back(ordinal);
    }
    std::ranges::sort(ordinals);

    SourceAssetRegistryDocumentV1 document;
    document.assets.reserve(ordinals.size());
    std::size_t expected_ordinal = 0;
    for (const auto ordinal : ordinals) {
        if (ordinal != expected_ordinal) {
            throw std::invalid_argument("source asset registry row ordinals are not contiguous");
        }
        ++expected_ordinal;

        const auto& row = rows.at(ordinal);
        if (!row.has_key || !row.has_id || !row.has_kind || !row.has_source || !row.has_source_format ||
            !row.has_imported) {
            throw std::invalid_argument("source asset registry row is incomplete");
        }
        if (AssetId{row.id} != asset_id_from_key_v2(AssetKeyV2{row.key})) {
            throw std::invalid_argument("source asset registry row id does not match key");
        }

        std::vector<std::size_t> dependency_ordinals;
        dependency_ordinals.reserve(row.dependencies.size());
        for (const auto& [dependency_ordinal, _] : row.dependencies) {
            dependency_ordinals.push_back(dependency_ordinal);
        }
        std::ranges::sort(dependency_ordinals);

        std::vector<SourceAssetDependencyRowV1> dependencies;
        dependencies.reserve(dependency_ordinals.size());
        std::size_t expected_dependency = 0;
        for (const auto dependency_ordinal : dependency_ordinals) {
            if (dependency_ordinal != expected_dependency) {
                throw std::invalid_argument("source asset registry dependency ordinals are not contiguous");
            }
            ++expected_dependency;

            const auto& dependency = row.dependencies.at(dependency_ordinal);
            if (!dependency.has_kind || !dependency.has_key) {
                throw std::invalid_argument("source asset registry dependency row is incomplete");
            }
            dependencies.push_back(SourceAssetDependencyRowV1{
                .kind = dependency.kind,
                .key = AssetKeyV2{dependency.key},
            });
        }

        document.assets.push_back(SourceAssetRegistryRowV1{
            .key = AssetKeyV2{row.key},
            .kind = row.kind,
            .source_path = row.source,
            .source_format = row.source_format,
            .imported_path = row.imported,
            .dependencies = std::move(dependencies),
        });
    }

    return document;
}

SourceAssetRegistryDocumentV1 deserialize_source_asset_registry_document(std::string_view text) {
    auto document = parse_source_asset_registry_document_unvalidated_v1(text);
    const auto diagnostics = validate_source_asset_registry_document(document);
    if (!diagnostics.empty()) {
        throw std::invalid_argument("source asset registry document is invalid");
    }
    return document;
}

AssetIdentityDocumentV2 project_source_asset_registry_identity_v2(const SourceAssetRegistryDocumentV1& document) {
    AssetIdentityDocumentV2 identity;
    identity.assets.reserve(document.assets.size());
    for (const auto& row : document.assets) {
        identity.assets.push_back(AssetIdentityRowV2{
            .key = row.key,
            .kind = row.kind,
            .source_path = row.source_path,
        });
    }
    return identity;
}

AssetImportMetadataRegistry build_source_asset_import_metadata_registry(const SourceAssetRegistryDocumentV1& document) {
    AssetImportMetadataRegistry registry;
    std::unordered_map<std::string, AssetId> ids_by_key;
    ids_by_key.reserve(document.assets.size());
    for (const auto& row : document.assets) {
        ids_by_key.emplace(row.key.value, asset_id_from_key_v2(row.key));
    }

    for (const auto& row : document.assets) {
        const auto id = asset_id_from_key_v2(row.key);
        if (row.kind == AssetKind::texture) {
            registry.add_texture(TextureImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
                .color_space = TextureColorSpace::srgb,
                .generate_mips = true,
                .compression = TextureCompression::none,
            });
        } else if (row.kind == AssetKind::mesh) {
            registry.add_mesh(MeshImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
                .scale = 1.0F,
                .generate_lods = false,
                .generate_collision = false,
            });
        } else if (row.kind == AssetKind::morph_mesh_cpu) {
            registry.add_morph_mesh_cpu(MorphMeshCpuImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
            });
        } else if (row.kind == AssetKind::animation_float_clip) {
            registry.add_animation_float_clip(AnimationFloatClipImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
            });
        } else if (row.kind == AssetKind::animation_quaternion_clip) {
            registry.add_animation_quaternion_clip(AnimationQuaternionClipImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
            });
        } else if (row.kind == AssetKind::audio) {
            registry.add_audio(AudioImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
                .streaming = false,
            });
        } else if (row.kind == AssetKind::material) {
            std::vector<AssetId> texture_dependencies;
            for (const auto& dependency : row.dependencies) {
                if (dependency.kind == AssetDependencyKind::material_texture) {
                    texture_dependencies.push_back(ids_by_key.at(dependency.key.value));
                }
            }
            registry.add_material(MaterialImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
                .texture_dependencies = std::move(texture_dependencies),
            });
        } else if (row.kind == AssetKind::scene) {
            std::vector<AssetId> mesh_dependencies;
            std::vector<AssetId> material_dependencies;
            std::vector<AssetId> sprite_dependencies;
            for (const auto& dependency : row.dependencies) {
                if (dependency.kind == AssetDependencyKind::scene_mesh) {
                    mesh_dependencies.push_back(ids_by_key.at(dependency.key.value));
                } else if (dependency.kind == AssetDependencyKind::scene_material) {
                    material_dependencies.push_back(ids_by_key.at(dependency.key.value));
                } else if (dependency.kind == AssetDependencyKind::scene_sprite) {
                    sprite_dependencies.push_back(ids_by_key.at(dependency.key.value));
                }
            }
            registry.add_scene(SceneImportMetadata{
                .id = id,
                .source_path = row.source_path,
                .imported_path = row.imported_path,
                .mesh_dependencies = std::move(mesh_dependencies),
                .material_dependencies = std::move(material_dependencies),
                .sprite_dependencies = std::move(sprite_dependencies),
            });
        } else {
            throw std::invalid_argument("source asset registry row kind is unsupported for import metadata");
        }
    }

    return registry;
}

} // namespace mirakana
