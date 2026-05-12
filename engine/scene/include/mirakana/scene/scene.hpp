// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/math/transform.hpp"
#include "mirakana/scene/components.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct SceneNodeId {
    std::uint32_t value{0};

    friend constexpr bool operator==(SceneNodeId lhs, SceneNodeId rhs) noexcept {
        return lhs.value == rhs.value;
    }

    friend constexpr bool operator!=(SceneNodeId lhs, SceneNodeId rhs) noexcept {
        return !(lhs == rhs);
    }
};

inline constexpr SceneNodeId null_scene_node{};

struct ScenePrefabSourceLink {
    std::string prefab_name;
    std::string prefab_path;
    std::uint32_t source_node_index{0};
    std::string source_node_name;
};

struct SceneNode {
    SceneNodeId id;
    std::string name;
    Transform3D transform;
    SceneNodeComponents components;
    std::optional<ScenePrefabSourceLink> prefab_source;
    SceneNodeId parent{null_scene_node};
    std::vector<SceneNodeId> children;
};

class Scene {
  public:
    explicit Scene(std::string name);

    [[nodiscard]] std::string_view name() const noexcept;
    [[nodiscard]] SceneNodeId create_node(std::string name);
    [[nodiscard]] SceneNode* find_node(SceneNodeId id) noexcept;
    [[nodiscard]] const SceneNode* find_node(SceneNodeId id) const noexcept;
    [[nodiscard]] const std::vector<SceneNode>& nodes() const noexcept;

    void set_components(SceneNodeId node, SceneNodeComponents components);
    void set_parent(SceneNodeId child, SceneNodeId parent);

  private:
    [[nodiscard]] std::size_t index_for(SceneNodeId id) const;

    std::string name_;
    std::vector<SceneNode> nodes_;
};

[[nodiscard]] bool is_valid_scene_prefab_source_link(const ScenePrefabSourceLink& link) noexcept;
[[nodiscard]] std::string serialize_scene(const Scene& scene);
[[nodiscard]] Scene deserialize_scene(std::string_view text);

} // namespace mirakana
