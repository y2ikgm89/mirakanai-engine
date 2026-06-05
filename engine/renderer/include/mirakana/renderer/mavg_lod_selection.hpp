// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/math/mat4.hpp"
#include "mirakana/math/vec.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class MavgLodSelectionDiagnosticCode : std::uint8_t {
    invalid_graph,
    invalid_view,
    missing_root_cluster,
    missing_resident_fallback,
    missing_page,
    budget_degraded,
    hysteresis_reused_previous,
};

struct MavgLodViewDesc {
    Mat4 clip_from_world{Mat4::identity()};
    Vec3 camera_world_position{};
    float viewport_height_pixels{0.0F};
    float target_error_pixels{1.0F};
    float hysteresis_pixels{0.25F};
    std::uint32_t max_selected_clusters{0};
};

struct MavgLodResidentPageSet {
    std::vector<std::uint32_t> page_indices;
};

struct MavgLodPreviousSelection {
    std::vector<std::uint32_t> cluster_indices;
};

struct MavgLodSelectedCluster {
    AssetId graph_asset;
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t lod_level{0};
    std::uint32_t material_partition{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
    bool fallback_substitution{false};
};

struct MavgLodPageRequest {
    AssetId graph_asset;
    std::uint32_t page_index{0};
    float priority{0.0F};
    std::string reason;
};

struct MavgLodSelectionDiagnostic {
    MavgLodSelectionDiagnosticCode code{MavgLodSelectionDiagnosticCode::invalid_graph};
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::string message;
};

struct MavgLodSelectionResult {
    std::vector<MavgLodSelectedCluster> selected_clusters;
    std::vector<MavgLodPageRequest> page_requests;
    std::vector<MavgLodSelectionDiagnostic> diagnostics;
    std::size_t fallback_substitution_count{0};
    std::size_t missing_page_count{0};
    bool budget_degraded{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] MavgLodSelectionResult select_mavg_lod_clusters(const MavgClusterGraphDocument& graph,
                                                              const MavgLodViewDesc& view,
                                                              const MavgLodResidentPageSet& resident_pages,
                                                              const MavgLodPreviousSelection& previous = {});

} // namespace mirakana
