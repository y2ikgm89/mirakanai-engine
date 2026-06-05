// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "mirakana/runtime/mavg_lod_residency.hpp"

#include <algorithm>
#include <cstdint>
#include <span>
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

[[nodiscard]] bool contains_sorted(const std::vector<std::uint32_t>& values, std::uint32_t value) {
    return std::ranges::binary_search(values, value);
}

[[nodiscard]] bool append_reviewed_page_requests(RuntimeMavgLodResidencyResult& result, AssetId graph_asset,
                                                 const std::vector<std::uint32_t>& resident_page_indices,
                                                 std::span<const MavgLodPageRequest> page_requests) {
    bool valid_requests = true;
    for (const auto& request : page_requests) {
        if (request.graph_asset != graph_asset) {
            add_diagnostic(result,
                           "invalid-page-request: reviewed page request graph asset does not match MAVG graph asset");
            valid_requests = false;
            continue;
        }
        if (!contains_sorted(resident_page_indices, request.page_index)) {
            add_diagnostic(result, "invalid-page-request: reviewed page request page is not in MAVG graph document");
            valid_requests = false;
            continue;
        }
        result.reviewed_page_requests.push_back(request);
    }
    return valid_requests;
}

} // namespace

bool RuntimeMavgLodResidencyResult::succeeded() const noexcept {
    return diagnostics.empty();
}

RuntimeMavgLodResidencyResult build_runtime_mavg_lod_residency(const RuntimeMavgLodResidencyDesc& desc,
                                                               std::span<const MavgLodPageRequest> page_requests) {
    RuntimeMavgLodResidencyResult result;

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

    auto resident_page_indices = resident_page_indices_from_graph(graph);
    if (!append_reviewed_page_requests(result, desc.graph_asset, resident_page_indices, page_requests)) {
        return result;
    }

    result.resident_pages.page_indices = std::move(resident_page_indices);
    return result;
}

} // namespace mirakana::runtime
