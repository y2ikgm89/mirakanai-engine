// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/assets/asset_dependency_graph.hpp"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <utility>
#include <vector>

namespace mirakana {
namespace {

[[nodiscard]] bool valid_token(std::string_view value) noexcept {
    return !value.empty() && value.find('\n') == std::string_view::npos && value.find('\r') == std::string_view::npos;
}

[[nodiscard]] bool valid_dependency_kind(AssetDependencyKind kind) noexcept {
    switch (kind) {
    case AssetDependencyKind::shader_include:
    case AssetDependencyKind::material_texture:
    case AssetDependencyKind::scene_mesh:
    case AssetDependencyKind::scene_material:
    case AssetDependencyKind::scene_sprite:
    case AssetDependencyKind::ui_atlas_texture:
    case AssetDependencyKind::tilemap_texture:
    case AssetDependencyKind::sprite_animation_texture:
    case AssetDependencyKind::sprite_animation_material:
    case AssetDependencyKind::generated_artifact:
    case AssetDependencyKind::source_file:
        return true;
    case AssetDependencyKind::unknown:
        break;
    }
    return false;
}

[[nodiscard]] bool same_edge(const AssetDependencyEdge& lhs, const AssetDependencyEdge& rhs) noexcept {
    return lhs.asset == rhs.asset && lhs.dependency == rhs.dependency && lhs.kind == rhs.kind && lhs.path == rhs.path;
}

[[nodiscard]] std::vector<AssetDependencyEdge> sorted_edges(std::vector<AssetDependencyEdge> edges) {
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
    return edges;
}

} // namespace

bool is_valid_asset_dependency_edge(const AssetDependencyEdge& edge) noexcept {
    return edge.asset.value != 0 && edge.dependency.value != 0 && edge.asset != edge.dependency &&
           valid_dependency_kind(edge.kind) && valid_token(edge.path);
}

bool AssetDependencyGraph::try_add(AssetDependencyEdge edge) {
    if (!is_valid_asset_dependency_edge(edge) || has_edge(edge)) {
        return false;
    }
    edges_.push_back(std::move(edge));
    return true;
}

void AssetDependencyGraph::add(AssetDependencyEdge edge) {
    if (!try_add(std::move(edge))) {
        throw std::logic_error("asset dependency edge could not be added");
    }
}

std::size_t AssetDependencyGraph::edge_count() const noexcept {
    return edges_.size();
}

bool AssetDependencyGraph::has_edge(const AssetDependencyEdge& edge) const noexcept {
    return std::ranges::find_if(edges_, [&edge](const AssetDependencyEdge& existing) {
               return same_edge(existing, edge);
           }) != edges_.end();
}

std::vector<AssetDependencyEdge> AssetDependencyGraph::dependencies_of(AssetId asset) const {
    std::vector<AssetDependencyEdge> result;
    for (const auto& edge : edges_) {
        if (edge.asset == asset) {
            result.push_back(edge);
        }
    }
    return sorted_edges(std::move(result));
}

std::vector<AssetDependencyEdge> AssetDependencyGraph::dependents_of(AssetId dependency) const {
    std::vector<AssetDependencyEdge> result;
    for (const auto& edge : edges_) {
        if (edge.dependency == dependency) {
            result.push_back(edge);
        }
    }
    return sorted_edges(std::move(result));
}

} // namespace mirakana
