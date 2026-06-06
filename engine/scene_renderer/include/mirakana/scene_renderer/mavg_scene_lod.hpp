// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/renderer/mavg_lod_selection.hpp"
#include "mirakana/scene_renderer/scene_renderer.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgSceneLodDiagnosticCode : std::uint8_t {
    invalid_graph,
    invalid_selection,
    missing_mesh_binding,
    invalid_selected_cluster,
    invalid_material_partition,
    missing_material_binding,
    fallback_substitution,
};

struct MavgSceneLodDiagnostic {
    MavgSceneLodDiagnosticCode code{MavgSceneLodDiagnosticCode::invalid_graph};
    std::uint32_t cluster_index{0};
    AssetId asset;
    std::string message;
};

struct MavgSceneLodSubmitDesc {
    Transform3D transform;
    Color fallback_color{.r = 1.0F, .g = 1.0F, .b = 1.0F, .a = 1.0F};
    const SceneGpuBindingPalette* gpu_bindings{nullptr};
    std::uint32_t instance_count{1};
};

struct MavgSceneLodSubmitResult {
    std::vector<MeshCommand> mesh_commands;
    std::vector<MavgSceneLodDiagnostic> diagnostics;
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
