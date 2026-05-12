// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/scene/components.hpp"
#include "mirakana/scene/scene.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace mirakana {

struct PrefabNodeTemplate {
    std::string name;
    Transform3D transform;
    // 1-based index into PrefabDefinition::nodes; 0 marks a prefab root.
    std::uint32_t parent_index{0};
    SceneNodeComponents components;
};

struct PrefabDefinition {
    std::string name;
    std::vector<PrefabNodeTemplate> nodes;
};

struct PrefabInstantiateDesc {
    PrefabDefinition prefab;
    std::string source_path;
};

struct PrefabInstance {
    std::vector<SceneNodeId> nodes;
};

[[nodiscard]] bool is_valid_prefab_definition(const PrefabDefinition& prefab) noexcept;
[[nodiscard]] std::optional<PrefabDefinition> build_prefab_from_scene_subtree(const Scene& scene, SceneNodeId root,
                                                                              std::string name);
[[nodiscard]] PrefabInstance instantiate_prefab(Scene& scene, const PrefabInstantiateDesc& desc);
[[nodiscard]] PrefabInstance instantiate_prefab(Scene& scene, const PrefabDefinition& prefab);
[[nodiscard]] std::string serialize_prefab_definition(const PrefabDefinition& prefab);
[[nodiscard]] PrefabDefinition deserialize_prefab_definition(std::string_view text);

} // namespace mirakana
