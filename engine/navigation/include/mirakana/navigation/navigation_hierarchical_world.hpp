// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace mirakana {

enum class NavigationHierarchicalWorldPathStatus : std::uint8_t {
    success = 0,
    invalid_request,
    invalid_world_graph,
    invalid_endpoint,
    no_path,
};

enum class NavigationHierarchicalWorldPathDiagnostic : std::uint8_t {
    none = 0,
    empty_regions,
    invalid_region_id,
    duplicate_region_id,
    invalid_world_region_ref,
    duplicate_world_region_ref,
    invalid_nav_data_ref,
    duplicate_nav_data_ref,
    invalid_traversal_cost,
    invalid_portal_id,
    duplicate_portal_id,
    missing_portal_region,
    portal_nav_data_region_mismatch,
    invalid_portal_endpoint,
    invalid_portal_cost,
    cost_overflow,
};

struct NavigationHierarchicalWorldRegion {
    std::string region_id;
    std::string world_region_ref;
    std::string nav_data_ref;
    std::uint32_t traversal_cost{1U};
};

struct NavigationHierarchicalWorldPortalEndpoint {
    std::string region_id;
    std::string nav_data_ref;
    std::uint32_t polygon_id{0U};
    std::string scene_ref;
};

struct NavigationHierarchicalWorldPortal {
    std::string portal_id;
    NavigationHierarchicalWorldPortalEndpoint from;
    NavigationHierarchicalWorldPortalEndpoint to;
    std::uint32_t cost{1U};
    bool bidirectional{true};
};

struct NavigationHierarchicalWorldPortalPathRow {
    std::string portal_id;
    std::string from_region_id;
    std::string to_region_id;
    std::string from_nav_data_ref;
    std::string to_nav_data_ref;
    std::uint32_t from_polygon_id{0U};
    std::uint32_t to_polygon_id{0U};
    std::string from_scene_ref;
    std::string to_scene_ref;
    std::uint32_t cost{0U};
};

struct NavigationHierarchicalWorldPathRequest {
    std::vector<NavigationHierarchicalWorldRegion> regions;
    std::vector<NavigationHierarchicalWorldPortal> portals;
    std::string start_region_id;
    std::string goal_region_id;
};

struct NavigationHierarchicalWorldPathResult {
    NavigationHierarchicalWorldPathStatus status{NavigationHierarchicalWorldPathStatus::invalid_request};
    NavigationHierarchicalWorldPathDiagnostic diagnostic{NavigationHierarchicalWorldPathDiagnostic::none};
    std::vector<std::string> region_path;
    std::vector<std::string> world_region_refs;
    std::vector<std::string> nav_data_refs;
    std::vector<std::string> portal_path;
    std::vector<NavigationHierarchicalWorldPortalPathRow> portal_rows;
    std::uint32_t total_cost{0U};
    std::size_t visited_region_count{0U};
    std::string failing_region_id;
    std::string failing_portal_id;

    [[nodiscard]] bool succeeded() const noexcept;
};

/// Plans a deterministic high-level region route over caller-authored world-region and nav-data references.
/// The helper is value-only: it does not bake navigation data, stream packages, touch renderer/RHI resources,
/// mutate scenes, start background jobs, or expose native handles.
[[nodiscard]] NavigationHierarchicalWorldPathResult
plan_navigation_hierarchical_world_path(const NavigationHierarchicalWorldPathRequest& request);

} // namespace mirakana
