// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_package.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos &&
           value.find('\0') == std::string_view::npos;
}

[[nodiscard]] bool valid_asset_kind(AssetKind kind) noexcept {
    switch (kind) {
    case AssetKind::texture:
    case AssetKind::mesh:
    case AssetKind::morph_mesh_cpu:
    case AssetKind::animation_float_clip:
    case AssetKind::animation_quaternion_clip:
    case AssetKind::sprite_animation:
    case AssetKind::skinned_mesh:
    case AssetKind::material:
    case AssetKind::scene:
    case AssetKind::audio:
    case AssetKind::script:
    case AssetKind::shader:
    case AssetKind::ui_atlas:
    case AssetKind::tilemap:
    case AssetKind::physics_collision_scene:
        return true;
    case AssetKind::unknown:
        break;
    }
    return false;
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

[[nodiscard]] AssetKind parse_asset_kind(std::string_view value) {
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
    throw std::invalid_argument("asset package entry kind is unknown");
}

[[nodiscard]] std::string_view dependency_kind_name(AssetDependencyKind kind) noexcept {
    switch (kind) {
    case AssetDependencyKind::shader_include:
        return "shader_include";
    case AssetDependencyKind::material_texture:
        return "material_texture";
    case AssetDependencyKind::scene_mesh:
        return "scene_mesh";
    case AssetDependencyKind::scene_material:
        return "scene_material";
    case AssetDependencyKind::scene_sprite:
        return "scene_sprite";
    case AssetDependencyKind::ui_atlas_texture:
        return "ui_atlas_texture";
    case AssetDependencyKind::tilemap_texture:
        return "tilemap_texture";
    case AssetDependencyKind::sprite_animation_texture:
        return "sprite_animation_texture";
    case AssetDependencyKind::sprite_animation_material:
        return "sprite_animation_material";
    case AssetDependencyKind::generated_artifact:
        return "generated_artifact";
    case AssetDependencyKind::source_file:
        return "source_file";
    case AssetDependencyKind::unknown:
        break;
    }
    return "unknown";
}

[[nodiscard]] AssetDependencyKind parse_dependency_kind(std::string_view value) {
    if (value == "shader_include") {
        return AssetDependencyKind::shader_include;
    }
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
    if (value == "ui_atlas_texture") {
        return AssetDependencyKind::ui_atlas_texture;
    }
    if (value == "tilemap_texture") {
        return AssetDependencyKind::tilemap_texture;
    }
    if (value == "sprite_animation_texture") {
        return AssetDependencyKind::sprite_animation_texture;
    }
    if (value == "sprite_animation_material") {
        return AssetDependencyKind::sprite_animation_material;
    }
    if (value == "generated_artifact") {
        return AssetDependencyKind::generated_artifact;
    }
    if (value == "source_file") {
        return AssetDependencyKind::source_file;
    }
    throw std::invalid_argument("asset package dependency kind is unknown");
}

void sort_asset_ids(std::vector<AssetId>& ids) {
    std::ranges::sort(ids, [](AssetId lhs, AssetId rhs) { return lhs.value < rhs.value; });
}

[[nodiscard]] bool valid_dependencies(const std::vector<AssetId>& dependencies, AssetId owner) noexcept {
    for (std::size_t index = 0; index < dependencies.size(); ++index) {
        if (dependencies[index].value == 0 || dependencies[index] == owner) {
            return false;
        }
        for (std::size_t other = index + 1U; other < dependencies.size(); ++other) {
            if (dependencies[index] == dependencies[other]) {
                return false;
            }
        }
    }
    return true;
}

void sort_entries(std::vector<AssetCookedPackageEntry>& entries) {
    std::ranges::sort(entries, [](const AssetCookedPackageEntry& lhs, const AssetCookedPackageEntry& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        return lhs.asset.value < rhs.asset.value;
    });
}

void sort_edges(std::vector<AssetDependencyEdge>& edges) {
    std::ranges::sort(edges, [](const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) {
        if (lhs.path != rhs.path) {
            return lhs.path < rhs.path;
        }
        if (lhs.asset.value != rhs.asset.value) {
            return lhs.asset.value < rhs.asset.value;
        }
        if (lhs.dependency.value != rhs.dependency.value) {
            return lhs.dependency.value < rhs.dependency.value;
        }
        return static_cast<int>(lhs.kind) < static_cast<int>(rhs.kind);
    });
}

[[nodiscard]] bool same_edge(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

[[nodiscard]] const AssetCookedPackageEntry* find_package_entry(const std::vector<AssetCookedPackageEntry>& entries,
                                                                AssetId asset) noexcept {
    const auto it =
        std::ranges::find_if(entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == entries.end() ? nullptr : &(*it);
}

[[nodiscard]] bool has_declared_dependency(const AssetCookedPackageEntry& entry, AssetId dependency) noexcept {
    return std::ranges::find(entry.dependencies, dependency) != entry.dependencies.end();
}

void validate_package_dependency_edges(const std::vector<AssetCookedPackageEntry>& entries,
                                       const std::vector<AssetDependencyEdge>& dependencies) {
    std::unordered_set<AssetId, AssetIdHash> assets;
    assets.reserve(entries.size());
    for (const auto& entry : entries) {
        if (!is_valid_asset_cooked_package_entry(entry)) {
            throw std::invalid_argument("asset cooked package entry is invalid");
        }
        if (!assets.insert(entry.asset).second) {
            throw std::invalid_argument("asset cooked package entry asset is duplicated");
        }
    }

    for (const auto& edge : dependencies) {
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("asset cooked package dependency edge is invalid");
        }
        const auto* owner = find_package_entry(entries, edge.asset);
        if (owner == nullptr) {
            throw std::invalid_argument("asset cooked package dependency edge references an unknown asset");
        }
        if (assets.find(edge.dependency) == assets.end()) {
            throw std::invalid_argument("asset cooked package dependency edge references an unknown dependency");
        }
        if (!has_declared_dependency(*owner, edge.dependency)) {
            throw std::invalid_argument("asset cooked package dependency edge is not declared by entry");
        }
    }
}

[[nodiscard]] bool same_edges(std::vector<AssetDependencyEdge> lhs, std::vector<AssetDependencyEdge> rhs) {
    sort_edges(lhs);
    sort_edges(rhs);
    if (lhs.size() != rhs.size()) {
        return false;
    }
    for (std::size_t index = 0; index < lhs.size(); ++index) {
        if (!same_edge(lhs[index], rhs[index])) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] std::vector<AssetDependencyEdge> dependencies_for(const AssetCookedPackageIndex& index, AssetId asset) {
    std::vector<AssetDependencyEdge> result;
    for (const auto& edge : index.dependencies) {
        if (edge.asset == asset) {
            result.push_back(edge);
        }
    }
    sort_edges(result);
    return result;
}

[[nodiscard]] const AssetCookedPackageEntry* find_entry(const AssetCookedPackageIndex& index, AssetId asset) noexcept {
    const auto it = std::ranges::find_if(
        index.entries, [asset](const AssetCookedPackageEntry& entry) { return entry.asset == asset; });
    return it == index.entries.end() ? nullptr : &*it;
}

[[nodiscard]] std::string join_asset_ids(const std::vector<AssetId>& ids) {
    std::ostringstream output;
    for (std::size_t index = 0; index < ids.size(); ++index) {
        if (index != 0U) {
            output << ',';
        }
        output << ids[index].value;
    }
    return output.str();
}

[[nodiscard]] std::uint64_t parse_u64(std::string_view value, std::string_view field) {
    std::uint64_t result = 0;
    bool any = false;
    for (const char character : value) {
        if (character < '0' || character > '9') {
            throw std::invalid_argument(std::string(field) + " must be an unsigned integer");
        }
        any = true;
        result = (result * 10U) + static_cast<std::uint64_t>(character - '0');
    }
    if (!any) {
        throw std::invalid_argument(std::string(field) + " is required");
    }
    return result;
}

[[nodiscard]] std::vector<AssetId> parse_asset_id_list(std::string_view value) {
    std::vector<AssetId> ids;
    if (value.empty()) {
        return ids;
    }

    std::size_t begin = 0;
    while (begin <= value.size()) {
        const auto end = value.find(',', begin);
        const auto token = value.substr(begin, end == std::string_view::npos ? std::string_view::npos : end - begin);
        ids.push_back(AssetId{parse_u64(token, "asset package dependency id")});
        if (end == std::string_view::npos) {
            break;
        }
        begin = end + 1U;
    }
    return ids;
}

using KeyValues = std::unordered_map<std::string, std::string>;

[[nodiscard]] KeyValues parse_key_values(std::string_view text) {
    KeyValues values;
    std::size_t begin = 0;
    while (begin < text.size()) {
        auto end = text.find('\n', begin);
        if (end == std::string_view::npos) {
            end = text.size();
        }
        const auto line = text.substr(begin, end - begin);
        if (!line.empty()) {
            const auto separator = line.find('=');
            if (separator == std::string_view::npos || separator == 0U) {
                throw std::invalid_argument("asset package index line is invalid");
            }
            values.emplace(std::string(line.substr(0, separator)), std::string(line.substr(separator + 1U)));
        }
        begin = end + 1U;
    }
    return values;
}

[[nodiscard]] const std::string& required_value(const KeyValues& values, const std::string& key) {
    const auto it = values.find(key);
    if (it == values.end()) {
        throw std::invalid_argument("asset package index is missing key: " + key);
    }
    return it->second;
}

[[nodiscard]] AssetCookedPackageRecookDecision
make_decision(AssetId asset, std::string path, AssetCookedPackageRecookDecisionKind kind, std::string diagnostic) {
    return AssetCookedPackageRecookDecision{
        .asset = asset,
        .path = std::move(path),
        .kind = kind,
        .diagnostic = std::move(diagnostic),
    };
}

} // namespace

std::uint64_t hash_asset_cooked_content(std::string_view content) noexcept {
    std::uint64_t hash = 14695981039346656037ULL;
    for (const char character : content) {
        hash ^= static_cast<unsigned char>(character);
        hash *= 1099511628211ULL;
    }
    return hash == 0 ? 1 : hash;
}

bool is_valid_asset_cooked_artifact(const AssetCookedArtifact& artifact) noexcept {
    return artifact.asset.value != 0 && valid_asset_kind(artifact.kind) && valid_token(artifact.path) &&
           !artifact.content.empty() && artifact.source_revision != 0 &&
           valid_dependencies(artifact.dependencies, artifact.asset);
}

bool is_valid_asset_cooked_package_entry(const AssetCookedPackageEntry& entry) noexcept {
    return entry.asset.value != 0 && valid_asset_kind(entry.kind) && valid_token(entry.path) &&
           entry.content_hash != 0 && entry.source_revision != 0 && valid_dependencies(entry.dependencies, entry.asset);
}

AssetCookedPackageIndex build_asset_cooked_package_index(std::vector<AssetCookedArtifact> artifacts,
                                                         std::vector<AssetDependencyEdge> dependencies) {
    AssetCookedPackageIndex index;
    std::unordered_set<AssetId, AssetIdHash> assets;
    index.entries.reserve(artifacts.size());

    for (auto& artifact : artifacts) {
        if (!is_valid_asset_cooked_artifact(artifact)) {
            throw std::invalid_argument("asset cooked artifact is invalid");
        }
        if (!assets.insert(artifact.asset).second) {
            throw std::invalid_argument("asset cooked artifact asset is duplicated");
        }
        sort_asset_ids(artifact.dependencies);
        index.entries.push_back(AssetCookedPackageEntry{
            .asset = artifact.asset,
            .kind = artifact.kind,
            .path = std::move(artifact.path),
            .content_hash = hash_asset_cooked_content(artifact.content),
            .source_revision = artifact.source_revision,
            .dependencies = std::move(artifact.dependencies),
        });
    }

    for (auto& edge : dependencies) {
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("asset cooked package dependency edge is invalid");
        }
        if (assets.find(edge.asset) == assets.end()) {
            throw std::invalid_argument("asset cooked package dependency edge references an unknown asset");
        }
        index.dependencies.push_back(std::move(edge));
    }
    validate_package_dependency_edges(index.entries, index.dependencies);
    sort_entries(index.entries);
    sort_edges(index.dependencies);
    const auto duplicate_tail = std::ranges::unique(index.dependencies, same_edge);
    index.dependencies.erase(duplicate_tail.begin(), duplicate_tail.end());
    return index;
}

std::vector<AssetCookedPackageRecookDecision>
build_asset_cooked_package_recook_decisions(const AssetCookedPackageIndex& previous,
                                            const AssetCookedPackageIndex& current) {
    std::vector<AssetCookedPackageRecookDecision> decisions;
    decisions.reserve(current.entries.size());

    for (const auto& entry : current.entries) {
        if (!is_valid_asset_cooked_package_entry(entry)) {
            throw std::invalid_argument("current asset cooked package entry is invalid");
        }
        const auto* previous_entry = find_entry(previous, entry.asset);
        if (previous_entry == nullptr) {
            decisions.push_back(make_decision(entry.asset, entry.path,
                                              AssetCookedPackageRecookDecisionKind::missing_from_previous_index,
                                              "asset is missing from previous cooked package index"));
            continue;
        }
        if (!is_valid_asset_cooked_package_entry(*previous_entry)) {
            throw std::invalid_argument("previous asset cooked package entry is invalid");
        }
        if (previous_entry->content_hash != entry.content_hash) {
            decisions.push_back(make_decision(entry.asset, entry.path,
                                              AssetCookedPackageRecookDecisionKind::content_hash_changed,
                                              "asset cooked content hash changed"));
            continue;
        }
        if (previous_entry->source_revision != entry.source_revision) {
            decisions.push_back(make_decision(entry.asset, entry.path,
                                              AssetCookedPackageRecookDecisionKind::source_revision_changed,
                                              "asset source revision changed"));
            continue;
        }
        if (previous_entry->dependencies != entry.dependencies ||
            !same_edges(dependencies_for(previous, entry.asset), dependencies_for(current, entry.asset))) {
            decisions.push_back(make_decision(entry.asset, entry.path,
                                              AssetCookedPackageRecookDecisionKind::dependencies_changed,
                                              "asset cooked dependency set changed"));
            continue;
        }

        decisions.push_back(make_decision(entry.asset, entry.path, AssetCookedPackageRecookDecisionKind::up_to_date,
                                          "asset cooked package entry is up to date"));
    }

    return decisions;
}

std::string serialize_asset_cooked_package_index(const AssetCookedPackageIndex& index) {
    std::ostringstream output;
    output << "format=GameEngine.CookedPackageIndex.v1\n";
    output << "entry.count=" << index.entries.size() << '\n';
    for (std::size_t ordinal = 0; ordinal < index.entries.size(); ++ordinal) {
        const auto& entry = index.entries[ordinal];
        if (!is_valid_asset_cooked_package_entry(entry)) {
            throw std::invalid_argument("asset cooked package entry is invalid for serialization");
        }
        output << "entry." << ordinal << ".asset=" << entry.asset.value << '\n';
        output << "entry." << ordinal << ".kind=" << asset_kind_name(entry.kind) << '\n';
        output << "entry." << ordinal << ".path=" << entry.path << '\n';
        output << "entry." << ordinal << ".content_hash=" << entry.content_hash << '\n';
        output << "entry." << ordinal << ".source_revision=" << entry.source_revision << '\n';
        output << "entry." << ordinal << ".dependencies=" << join_asset_ids(entry.dependencies) << '\n';
    }

    validate_package_dependency_edges(index.entries, index.dependencies);
    output << "dependency.count=" << index.dependencies.size() << '\n';
    for (std::size_t ordinal = 0; ordinal < index.dependencies.size(); ++ordinal) {
        const auto& edge = index.dependencies[ordinal];
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("asset cooked package dependency edge is invalid for serialization");
        }
        output << "dependency." << ordinal << ".asset=" << edge.asset.value << '\n';
        output << "dependency." << ordinal << ".dependency=" << edge.dependency.value << '\n';
        output << "dependency." << ordinal << ".kind=" << dependency_kind_name(edge.kind) << '\n';
        output << "dependency." << ordinal << ".path=" << edge.path << '\n';
    }
    return output.str();
}

AssetCookedPackageIndex deserialize_asset_cooked_package_index(std::string_view text) {
    const auto values = parse_key_values(text);
    if (required_value(values, "format") != "GameEngine.CookedPackageIndex.v1") {
        throw std::invalid_argument("asset cooked package index format is unsupported");
    }

    AssetCookedPackageIndex index;
    const auto entry_count = parse_u64(required_value(values, "entry.count"), "entry.count");
    index.entries.reserve(static_cast<std::size_t>(entry_count));
    for (std::uint64_t ordinal = 0; ordinal < entry_count; ++ordinal) {
        const auto prefix = "entry." + std::to_string(ordinal) + ".";
        auto dependencies = parse_asset_id_list(required_value(values, prefix + "dependencies"));
        sort_asset_ids(dependencies);
        AssetCookedPackageEntry entry{
            .asset = AssetId{parse_u64(required_value(values, prefix + "asset"), prefix + "asset")},
            .kind = parse_asset_kind(required_value(values, prefix + "kind")),
            .path = required_value(values, prefix + "path"),
            .content_hash = parse_u64(required_value(values, prefix + "content_hash"), prefix + "content_hash"),
            .source_revision =
                parse_u64(required_value(values, prefix + "source_revision"), prefix + "source_revision"),
            .dependencies = std::move(dependencies),
        };
        if (!is_valid_asset_cooked_package_entry(entry)) {
            throw std::invalid_argument("asset cooked package entry is invalid after deserialization");
        }
        index.entries.push_back(std::move(entry));
    }

    const auto dependency_count = parse_u64(required_value(values, "dependency.count"), "dependency.count");
    index.dependencies.reserve(static_cast<std::size_t>(dependency_count));
    for (std::uint64_t ordinal = 0; ordinal < dependency_count; ++ordinal) {
        const auto prefix = "dependency." + std::to_string(ordinal) + ".";
        AssetDependencyEdge edge{
            .asset = AssetId{parse_u64(required_value(values, prefix + "asset"), prefix + "asset")},
            .dependency = AssetId{parse_u64(required_value(values, prefix + "dependency"), prefix + "dependency")},
            .kind = parse_dependency_kind(required_value(values, prefix + "kind")),
            .path = required_value(values, prefix + "path"),
        };
        if (!is_valid_asset_dependency_edge(edge)) {
            throw std::invalid_argument("asset cooked package dependency edge is invalid after deserialization");
        }
        index.dependencies.push_back(std::move(edge));
    }

    sort_entries(index.entries);
    sort_edges(index.dependencies);
    validate_package_dependency_edges(index.entries, index.dependencies);
    return index;
}

} // namespace mirakana
