// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include "mirakana/assets/asset_registry.hpp"
#include "mirakana/assets/mavg_cluster_graph.hpp"
#include "mirakana/runtime/resource_runtime.hpp"

#include <span>
#include <string>
#include <vector>

namespace mirakana::runtime {

struct RuntimeMavgLodResidencyDesc {
    AssetId graph_asset;
    const MavgClusterGraphDocument* graph{nullptr};
    const RuntimeResourceCatalogV2* catalog{nullptr};
};

struct RuntimeMavgLodResidencyResult {
    MavgLodResidentPageSet resident_pages;
    std::vector<MavgLodPageRequest> reviewed_page_requests;
    std::vector<std::string> diagnostics;
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_streaming{false};
    bool touched_renderer_or_rhi_handles{false};

    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeMavgLodResidencyResult
build_runtime_mavg_lod_residency(const RuntimeMavgLodResidencyDesc& desc,
                                 std::span<const MavgLodPageRequest> page_requests);

} // namespace mirakana::runtime
