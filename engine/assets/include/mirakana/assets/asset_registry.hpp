// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace mirakana {

struct AssetId {
    std::uint64_t value{0};

    [[nodiscard]] static AssetId from_name(std::string_view name) noexcept;

    friend constexpr bool operator==(AssetId lhs, AssetId rhs) noexcept {
        return lhs.value == rhs.value;
    }
};

struct AssetIdHash {
    [[nodiscard]] std::size_t operator()(AssetId id) const noexcept {
        return std::hash<std::uint64_t>{}(id.value);
    }
};

enum class AssetKind {
    unknown,
    texture,
    mesh,
    morph_mesh_cpu,
    animation_float_clip,
    animation_quaternion_clip,
    sprite_animation,
    skinned_mesh,
    material,
    scene,
    audio,
    script,
    shader,
    ui_atlas,
    tilemap,
    physics_collision_scene
};

struct AssetRecord {
    AssetId id;
    AssetKind kind{AssetKind::unknown};
    std::string path;
};

class AssetRegistry {
  public:
    bool try_add(AssetRecord record);
    void add(AssetRecord record);

    [[nodiscard]] const AssetRecord* find(AssetId id) const noexcept;
    [[nodiscard]] std::size_t count() const noexcept;
    [[nodiscard]] std::vector<AssetRecord> records() const;

  private:
    std::unordered_map<AssetId, AssetRecord, AssetIdHash> records_;
};

} // namespace mirakana
