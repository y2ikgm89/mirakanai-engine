// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/math/transform.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/renderer/renderer.hpp"

#include <cstddef>
#include <span>
#include <string>
#include <vector>

namespace mirakana {

struct MavgSceneLodMeshBinding {
    AssetId graph_asset;
    MeshGpuBinding mesh_binding;
};

struct MavgSceneLodMaterialBinding {
    AssetId material;
    MaterialGpuBinding material_binding;
};

struct MavgSceneLodSubmitDesc {
    AssetId graph_asset;
    Transform3D transform;
    Color fallback_color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    const MavgSceneLodMeshBinding* mesh_binding{nullptr};
    std::span<const MavgSceneLodMaterialBinding> material_bindings;
};

struct MavgSceneLodSubmitResult {
    std::vector<MeshCommand> mesh_commands;
    std::vector<std::string> diagnostics;
    std::size_t submitted_cluster_count{0};
    std::size_t fallback_substitution_count{0};
    std::size_t missing_material_binding_count{0};
    bool rejected{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgSceneLodSubmitResult plan_mavg_scene_lod_mesh_commands(const MavgLodSelectionResult& selection,
                                                                         const MavgClusterGraphDocument& graph,
                                                                         const MavgSceneLodSubmitDesc& desc);

} // namespace mirakana
