// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class AssetDependencyKind : std::uint8_t {
    unknown,
    shader_include,
    material_texture,
    scene_mesh,
    scene_material,
    scene_sprite,
    ui_atlas_texture,
    tilemap_texture,
    sprite_animation_texture,
    sprite_animation_material,
    generated_artifact,
    source_file,
};

struct AssetDependencyEdge {
    AssetId asset;
    AssetId dependency;
    AssetDependencyKind kind{AssetDependencyKind::unknown};
    std::string path;
};

[[nodiscard]] bool is_valid_asset_dependency_edge(const AssetDependencyEdge& edge) noexcept;

class AssetDependencyGraph {
  public:
    bool try_add(AssetDependencyEdge edge);
    void add(AssetDependencyEdge edge);

    [[nodiscard]] std::size_t edge_count() const noexcept;
    [[nodiscard]] bool has_edge(const AssetDependencyEdge& edge) const noexcept;
    [[nodiscard]] std::vector<AssetDependencyEdge> dependencies_of(AssetId asset) const;
    [[nodiscard]] std::vector<AssetDependencyEdge> dependents_of(AssetId dependency) const;

  private:
    std::vector<AssetDependencyEdge> edges_;
};

} // namespace mirakana
