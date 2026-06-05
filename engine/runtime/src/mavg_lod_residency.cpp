// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_lod_residency.hpp"

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace mirakana::runtime {
namespace {

void add_diagnostic(RuntimeMavgLodResidencyResult& result, std::string diagnostic) {
    result.diagnostics.push_back(std::move(diagnostic));
}

[[nodiscard]] bool catalog_contains_resident_graph(const RuntimeResourceCatalogV2& catalog, AssetId graph_asset,
                                                   RuntimeMavgLodResidencyResult& result) {
    bool found_asset = false;
    for (const auto& record : catalog.records()) {
        if (record.asset != graph_asset) {
            continue;
        }
        found_asset = true;
        if (record.kind == AssetKind::mavg_cluster_graph) {
            return true;
        }
        add_diagnostic(result, "invalid-catalog-kind: resident catalog row for graph asset is not mavg_cluster_graph");
    }

    if (!found_asset) {
        add_diagnostic(result, "missing-catalog-record: resident catalog has no row for MAVG cluster graph asset");
    }
    return false;
}

[[nodiscard]] std::vector<std::uint32_t> resident_page_indices_from_graph(const MavgClusterGraphDocument& graph) {
    std::vector<std::uint32_t> page_indices;
    page_indices.reserve(graph.pages.size());
    for (const auto& page : graph.pages) {
        page_indices.push_back(page.page_index);
    }
    std::ranges::sort(page_indices);
    const auto duplicates = std::ranges::unique(page_indices);
    page_indices.erase(duplicates.begin(), duplicates.end());
    return page_indices;
}

} // namespace

bool RuntimeMavgLodResidencyResult::succeeded() const noexcept {
    return diagnostics.empty();
}

RuntimeMavgLodResidencyResult build_runtime_mavg_lod_residency(const RuntimeMavgLodResidencyDesc& desc,
                                                               std::span<const MavgLodPageRequest> page_requests) {
    RuntimeMavgLodResidencyResult result;
    result.reviewed_page_requests.assign(page_requests.begin(), page_requests.end());

    bool invalid_inputs = false;
    if (desc.graph_asset.value == 0) {
        add_diagnostic(result, "invalid-graph-asset: MAVG graph asset id must be non-zero");
        invalid_inputs = true;
    }
    if (desc.graph == nullptr) {
        add_diagnostic(result, "missing-graph: MAVG cluster graph document is required");
        invalid_inputs = true;
    }
    if (desc.catalog == nullptr) {
        add_diagnostic(result, "missing-catalog: runtime resource catalog is required");
        invalid_inputs = true;
    }
    if (invalid_inputs) {
        return result;
    }

    const auto& graph = *desc.graph;
    const auto& catalog = *desc.catalog;
    if (graph.asset != desc.graph_asset) {
        add_diagnostic(result, "graph-asset-mismatch: MAVG graph document asset does not match requested asset");
    }

    const auto validation = validate_mavg_cluster_graph(graph);
    if (!validation.valid()) {
        add_diagnostic(result, "invalid-graph: MAVG cluster graph validation failed");
        for (const auto& diagnostic : validation.diagnostics) {
            add_diagnostic(result, "invalid-graph: " + diagnostic.message);
        }
    }

    if (!catalog_contains_resident_graph(catalog, desc.graph_asset, result) || !result.diagnostics.empty()) {
        return result;
    }

    result.resident_pages.page_indices = resident_page_indices_from_graph(graph);
    return result;
}

} // namespace mirakana::runtime
