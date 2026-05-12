// SPDX-FileCopyrightText: 2026 GameEngine contributors
// SPDX-License-Identifier: LicenseRef-Proprietary

#include "test_framework.hpp"

#include "mirakana/navigation/local_avoidance.hpp"
#include "mirakana/navigation/navigation_agent.hpp"
#include "mirakana/navigation/navigation_grid.hpp"
#include "mirakana/navigation/navigation_path_planner.hpp"
#include "mirakana/navigation/navigation_replan.hpp"
#include "mirakana/navigation/path_following.hpp"
#include "mirakana/navigation/path_smoothing.hpp"

#include <cmath>
#include <limits>
#include <vector>

MK_TEST("navigation grid finds a straight cardinal path") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 4, .height = 1});

    const auto result = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                              .goal = mirakana::NavigationGridCoord{.x = 3, .y = 0}});

    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
        mirakana::NavigationGridCoord{.x = 3, .y = 0},
    };
    MK_REQUIRE(result.status == mirakana::NavigationPathStatus::success);
    MK_REQUIRE(result.points == expected);
    MK_REQUIRE(result.total_cost == 3U);
}

MK_TEST("navigation grid routes around blocked cells deterministically") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);

    const auto result = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                              .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0}});

    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0}, mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 0, .y = 2}, mirakana::NavigationGridCoord{.x = 1, .y = 2},
        mirakana::NavigationGridCoord{.x = 2, .y = 2}, mirakana::NavigationGridCoord{.x = 2, .y = 1},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    MK_REQUIRE(result.status == mirakana::NavigationPathStatus::success);
    MK_REQUIRE(result.points == expected);
    MK_REQUIRE(result.total_cost == 6U);
}

MK_TEST("navigation grid rejects invalid endpoints") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 2});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);

    const auto blocked = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                              .goal = mirakana::NavigationGridCoord{.x = 1, .y = 1}});
    const auto out_of_bounds = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                              .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0}});

    MK_REQUIRE(blocked.status == mirakana::NavigationPathStatus::blocked_endpoint);
    MK_REQUIRE(blocked.points.empty());
    MK_REQUIRE(out_of_bounds.status == mirakana::NavigationPathStatus::invalid_endpoint);
    MK_REQUIRE(out_of_bounds.points.empty());
}

MK_TEST("navigation grid reports unreachable goals") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 2}, false);

    const auto result = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 1},
                                              .goal = mirakana::NavigationGridCoord{.x = 2, .y = 1}});

    MK_REQUIRE(result.status == mirakana::NavigationPathStatus::no_path);
    MK_REQUIRE(result.points.empty());
}

MK_TEST("navigation grid prefers lower traversal cost over fewer high cost cells") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 2});
    grid.set_traversal_cost(mirakana::NavigationGridCoord{.x = 1, .y = 0}, 50U);

    const auto result = mirakana::find_navigation_path(
        grid, mirakana::NavigationPathRequest{.start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                              .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0}});

    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0}, mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 1, .y = 1}, mirakana::NavigationGridCoord{.x = 2, .y = 1},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    MK_REQUIRE(result.status == mirakana::NavigationPathStatus::success);
    MK_REQUIRE(result.points == expected);
    MK_REQUIRE(result.total_cost == 4U);
}

MK_TEST("navigation grid path validation reports deterministic diagnostics") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});

    const std::vector<mirakana::NavigationGridCoord> valid_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    const auto valid =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                          .path = valid_path,
                                                      });
    MK_REQUIRE(valid.status == mirakana::NavigationGridPathValidationStatus::valid);
    MK_REQUIRE(valid.failing_index == 0U);
    MK_REQUIRE(valid.total_cost == 2U);

    const auto empty =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                          .path = {},
                                                      });
    MK_REQUIRE(empty.status == mirakana::NavigationGridPathValidationStatus::empty_path);
    MK_REQUIRE(empty.failing_index == 0U);

    const std::vector<mirakana::NavigationGridCoord> wrong_start{
        mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 1, .y = 1},
        mirakana::NavigationGridCoord{.x = 2, .y = 1},
    };
    const auto start_mismatch =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 1},
                                                          .path = wrong_start,
                                                      });
    MK_REQUIRE(start_mismatch.status == mirakana::NavigationGridPathValidationStatus::start_mismatch);
    MK_REQUIRE(start_mismatch.failing_index == 0U);
    MK_REQUIRE(start_mismatch.coord == (mirakana::NavigationGridCoord{0, 1}));

    const std::vector<mirakana::NavigationGridCoord> wrong_goal{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
    };
    const auto goal_mismatch =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                          .path = wrong_goal,
                                                      });
    MK_REQUIRE(goal_mismatch.status == mirakana::NavigationGridPathValidationStatus::goal_mismatch);
    MK_REQUIRE(goal_mismatch.failing_index == 1U);
    MK_REQUIRE(goal_mismatch.coord == (mirakana::NavigationGridCoord{1, 0}));

    const std::vector<mirakana::NavigationGridCoord> out_of_bounds_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 3, .y = 0},
    };
    const auto out_of_bounds =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 3, .y = 0},
                                                          .path = out_of_bounds_path,
                                                      });
    MK_REQUIRE(out_of_bounds.status == mirakana::NavigationGridPathValidationStatus::out_of_bounds_cell);
    MK_REQUIRE(out_of_bounds.failing_index == 1U);
    MK_REQUIRE(out_of_bounds.coord == (mirakana::NavigationGridCoord{3, 0}));

    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    const auto blocked =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                          .path = valid_path,
                                                      });
    MK_REQUIRE(blocked.status == mirakana::NavigationGridPathValidationStatus::blocked_cell);
    MK_REQUIRE(blocked.failing_index == 1U);
    MK_REQUIRE(blocked.coord == (mirakana::NavigationGridCoord{1, 0}));

    const std::vector<mirakana::NavigationGridCoord> diagonal_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 1},
    };
    const auto non_cardinal =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 1, .y = 1},
                                                          .path = diagonal_path,
                                                      });
    MK_REQUIRE(non_cardinal.status == mirakana::NavigationGridPathValidationStatus::non_cardinal_step);
    MK_REQUIRE(non_cardinal.failing_index == 1U);
    MK_REQUIRE(non_cardinal.coord == (mirakana::NavigationGridCoord{1, 1}));

    const auto unsupported =
        mirakana::validate_navigation_grid_path(grid, mirakana::NavigationGridPathValidationRequest{
                                                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                          .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                          .path = valid_path,
                                                          .adjacency = static_cast<mirakana::NavigationAdjacency>(99),
                                                      });
    MK_REQUIRE(unsupported.status == mirakana::NavigationGridPathValidationStatus::unsupported_adjacency);

    mirakana::NavigationGrid expensive_grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    expensive_grid.set_traversal_cost(mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                      std::numeric_limits<std::uint32_t>::max());
    const std::vector<mirakana::NavigationGridCoord> expensive_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    const auto cost_overflow = mirakana::validate_navigation_grid_path(
        expensive_grid, mirakana::NavigationGridPathValidationRequest{
                            .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                            .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                            .path = expensive_path,
                        });
    MK_REQUIRE(cost_overflow.status == mirakana::NavigationGridPathValidationStatus::cost_overflow);
    MK_REQUIRE(cost_overflow.failing_index == 2U);
    MK_REQUIRE(cost_overflow.coord == (mirakana::NavigationGridCoord{2, 0}));
}

MK_TEST("navigation grid replan reuses valid remaining paths") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 4, .height = 1});
    const std::vector<mirakana::NavigationGridCoord> remaining_path{
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
        mirakana::NavigationGridCoord{.x = 3, .y = 0},
    };

    const auto result =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 3, .y = 0},
                                                        .remaining_path = remaining_path,
                                                    });

    MK_REQUIRE(result.status == mirakana::NavigationGridReplanStatus::reused_existing_path);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::valid);
    MK_REQUIRE(result.total_cost == 2U);
    MK_REQUIRE(result.path == remaining_path);
    MK_REQUIRE(result.reused_existing_path);
    MK_REQUIRE(!result.replanned);
}

MK_TEST("navigation grid replan routes around newly blocked remaining cells") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    const std::vector<mirakana::NavigationGridCoord> stale_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };

    const auto result =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .remaining_path = stale_path,
                                                    });

    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0}, mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 1, .y = 1}, mirakana::NavigationGridCoord{.x = 2, .y = 1},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    MK_REQUIRE(result.status == mirakana::NavigationGridReplanStatus::replanned);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::blocked_cell);
    MK_REQUIRE(result.total_cost == 4U);
    MK_REQUIRE(result.path == expected);
    MK_REQUIRE(!result.reused_existing_path);
    MK_REQUIRE(result.replanned);
}

MK_TEST("navigation grid replan rejects unsupported adjacency before pathfinding") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    const std::vector<mirakana::NavigationGridCoord> remaining_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };

    const auto result =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .remaining_path = remaining_path,
                                                        .adjacency = static_cast<mirakana::NavigationAdjacency>(99),
                                                    });

    MK_REQUIRE(result.status == mirakana::NavigationGridReplanStatus::unsupported_adjacency);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::unsupported_adjacency);
    MK_REQUIRE(result.path.empty());
    MK_REQUIRE(result.total_cost == 0U);
    MK_REQUIRE(!result.reused_existing_path);
    MK_REQUIRE(!result.replanned);
}

MK_TEST("navigation grid replan forwards pathfinding failures") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    const std::vector<mirakana::NavigationGridCoord> stale_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };

    const auto no_path =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .remaining_path = stale_path,
                                                    });
    MK_REQUIRE(no_path.status == mirakana::NavigationGridReplanStatus::no_path);
    MK_REQUIRE(no_path.path.empty());
    MK_REQUIRE(no_path.total_cost == 0U);
    MK_REQUIRE(no_path.replanned);

    const auto invalid_endpoint =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = -1, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .remaining_path = stale_path,
                                                    });
    MK_REQUIRE(invalid_endpoint.status == mirakana::NavigationGridReplanStatus::invalid_endpoint);
    MK_REQUIRE(invalid_endpoint.path.empty());

    grid.set_walkable(mirakana::NavigationGridCoord{.x = 2, .y = 0}, false);
    const auto blocked_endpoint =
        mirakana::replan_navigation_grid_path(grid, mirakana::NavigationGridReplanRequest{
                                                        .current = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .remaining_path = stale_path,
                                                    });
    MK_REQUIRE(blocked_endpoint.status == mirakana::NavigationGridReplanStatus::blocked_endpoint);
    MK_REQUIRE(blocked_endpoint.path.empty());
}

MK_TEST("navigation grid path smoothing preserves direct paths") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    const std::vector<mirakana::NavigationGridCoord> source_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };

    const auto result =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .path = source_path,
                                                    });

    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };
    MK_REQUIRE(result.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::valid);
    MK_REQUIRE(result.path == expected);
    MK_REQUIRE(result.removed_point_count == 1U);
}

MK_TEST("navigation grid path smoothing leaves minimal valid paths unchanged") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 1});
    const std::vector<mirakana::NavigationGridCoord> single_point_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
    };
    const std::vector<mirakana::NavigationGridCoord> two_point_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
    };

    const auto single =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .path = single_point_path,
                                                    });
    const auto two =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                        .path = two_point_path,
                                                    });

    MK_REQUIRE(single.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(single.validation.status == mirakana::NavigationGridPathValidationStatus::valid);
    MK_REQUIRE(single.path == single_point_path);
    MK_REQUIRE(single.removed_point_count == 0U);
    MK_REQUIRE(two.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(two.validation.status == mirakana::NavigationGridPathValidationStatus::valid);
    MK_REQUIRE(two.path == two_point_path);
    MK_REQUIRE(two.removed_point_count == 0U);
}

MK_TEST("navigation grid path smoothing removes clear corners deterministically") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    const std::vector<mirakana::NavigationGridCoord> source_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0}, mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 1, .y = 1}, mirakana::NavigationGridCoord{.x = 2, .y = 1},
        mirakana::NavigationGridCoord{.x = 2, .y = 2},
    };

    const auto result =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 2},
                                                        .path = source_path,
                                                    });
    const auto replay =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 2},
                                                        .path = source_path,
                                                    });
    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 2},
    };

    MK_REQUIRE(result.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(result.path == expected);
    MK_REQUIRE(result.removed_point_count == 3U);
    MK_REQUIRE(result.path == replay.path);
}

MK_TEST("navigation grid path smoothing preserves corners around blocked line of sight") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);
    const std::vector<mirakana::NavigationGridCoord> source_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0}, mirakana::NavigationGridCoord{.x = 0, .y = 1},
        mirakana::NavigationGridCoord{.x = 0, .y = 2}, mirakana::NavigationGridCoord{.x = 1, .y = 2},
        mirakana::NavigationGridCoord{.x = 2, .y = 2},
    };

    const auto result =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 2},
                                                        .path = source_path,
                                                    });
    const std::vector<mirakana::NavigationGridCoord> direct_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 2},
    };
    const std::vector<mirakana::NavigationGridCoord> expected{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 0, .y = 2},
        mirakana::NavigationGridCoord{.x = 2, .y = 2},
    };

    MK_REQUIRE(result.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(result.path != direct_path);
    MK_REQUIRE(result.path == expected);
    MK_REQUIRE(result.removed_point_count == 2U);
}

MK_TEST("navigation grid path smoothing forwards invalid source diagnostics") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 3});
    const std::vector<mirakana::NavigationGridCoord> diagonal_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 1},
    };

    const auto result =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 1, .y = 1},
                                                        .path = diagonal_path,
                                                    });

    MK_REQUIRE(result.status == mirakana::NavigationGridPathSmoothingStatus::invalid_source_path);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::non_cardinal_step);
    MK_REQUIRE(result.path.empty());
    MK_REQUIRE(result.removed_point_count == 0U);
}

MK_TEST("navigation grid path smoothing rejects unsupported adjacency") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    const std::vector<mirakana::NavigationGridCoord> source_path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 0},
    };

    const auto result =
        mirakana::smooth_navigation_grid_path(grid, mirakana::NavigationGridPathSmoothingRequest{
                                                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                                                        .path = source_path,
                                                        .adjacency = static_cast<mirakana::NavigationAdjacency>(99),
                                                    });

    MK_REQUIRE(result.status == mirakana::NavigationGridPathSmoothingStatus::unsupported_adjacency);
    MK_REQUIRE(result.validation.status == mirakana::NavigationGridPathValidationStatus::unsupported_adjacency);
    MK_REQUIRE(result.path.empty());
    MK_REQUIRE(result.removed_point_count == 0U);
}

MK_TEST("navigation local avoidance passes through without relevant neighbors") {
    const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.25F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
    });

    MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(result.diagnostic == mirakana::NavigationLocalAvoidanceDiagnostic::none);
    MK_REQUIRE(result.adjusted_velocity == (mirakana::NavigationPoint2{1.0F, 0.25F}));
    MK_REQUIRE(result.applied_neighbor_count == 0U);
    MK_REQUIRE(!result.clamped_to_max_speed);
}

MK_TEST("navigation local avoidance pushes away from overlapping neighbors") {
    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 0.25F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };

    const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
        .neighbors = neighbors,
        .separation_weight = 2.0F,
        .prediction_time_seconds = 0.0F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(result.applied_neighbor_count == 1U);
    MK_REQUIRE(result.adjusted_velocity.x < 0.0F);
    MK_REQUIRE(result.adjusted_velocity.y == 0.0F);
}

MK_TEST("navigation local avoidance reacts to predicted head-on neighbors") {
    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 1.5F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = -1.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };

    const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
        .neighbors = neighbors,
        .separation_weight = 1.0F,
        .prediction_time_seconds = 0.75F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(result.applied_neighbor_count == 1U);
    MK_REQUIRE(result.adjusted_velocity.x < 1.0F);
}

MK_TEST("navigation local avoidance catches collisions inside the prediction window") {
    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = -1.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };

    const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
        .neighbors = neighbors,
        .separation_weight = 1.0F,
        .prediction_time_seconds = 2.0F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(result.applied_neighbor_count == 1U);
    MK_REQUIRE(result.adjusted_velocity.x < 1.0F);
}

MK_TEST("navigation local avoidance is deterministic for neighbor ordering") {
    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors_a{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 30U,
            .position = mirakana::NavigationPoint2{.x = 0.5F, .y = 0.25F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 0.5F, .y = -0.25F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> neighbors_b{
        neighbors_a[1],
        neighbors_a[0],
    };

    const mirakana::NavigationLocalAvoidanceAgentDesc agent{
        .id = 10U,
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
        .radius = 0.5F,
        .max_speed = 3.0F,
    };
    const auto first = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent = agent, .neighbors = neighbors_a, .separation_weight = 1.5F, .prediction_time_seconds = 0.25F});
    const auto second = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent = agent, .neighbors = neighbors_b, .separation_weight = 1.5F, .prediction_time_seconds = 0.25F});

    MK_REQUIRE(first.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(second.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(first.adjusted_velocity == second.adjusted_velocity);
    MK_REQUIRE(first.applied_neighbor_count == second.applied_neighbor_count);
}

MK_TEST("navigation local avoidance clamps adjusted velocity to max speed") {
    const auto result = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 4.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
    });

    MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::success);
    MK_REQUIRE(result.adjusted_velocity == (mirakana::NavigationPoint2{2.0F, 0.0F}));
    MK_REQUIRE(result.clamped_to_max_speed);
}

MK_TEST("navigation local avoidance reports deterministic invalid diagnostics") {
    const auto require_invalid = [](const mirakana::NavigationLocalAvoidanceResult& result,
                                    mirakana::NavigationLocalAvoidanceDiagnostic diagnostic) {
        MK_REQUIRE(result.status == mirakana::NavigationLocalAvoidanceStatus::invalid_request);
        MK_REQUIRE(result.diagnostic == diagnostic);
        MK_REQUIRE(result.adjusted_velocity == (mirakana::NavigationPoint2{0.0F, 0.0F}));
        MK_REQUIRE(result.applied_neighbor_count == 0U);
        MK_REQUIRE(!result.clamped_to_max_speed);
    };

    const auto invalid_agent = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 0U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = 2.0F,
            },
    });
    require_invalid(invalid_agent, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_agent_id);

    const auto invalid_position =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
        });
    require_invalid(invalid_position, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_position);

    const auto invalid_velocity =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity =
                        mirakana::NavigationPoint2{.x = std::numeric_limits<float>::infinity(), .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
        });
    require_invalid(invalid_velocity, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_desired_velocity);

    const auto invalid_radius =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = -0.5F,
                    .max_speed = 2.0F,
                },
        });
    require_invalid(invalid_radius, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_radius);

    const auto invalid_speed = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                .radius = 0.5F,
                .max_speed = -2.0F,
            },
    });
    require_invalid(invalid_speed, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_max_speed);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> invalid_neighbors{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 0U,
            .position = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const auto invalid_neighbor =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = invalid_neighbors,
        });
    require_invalid(invalid_neighbor, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_id);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> invalid_neighbor_position{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const auto neighbor_position =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = invalid_neighbor_position,
        });
    require_invalid(neighbor_position, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_position);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> invalid_neighbor_velocity{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::infinity(), .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const auto neighbor_velocity =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = invalid_neighbor_velocity,
        });
    require_invalid(neighbor_velocity, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_velocity);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> invalid_neighbor_radius{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = -0.5F,
        },
    };
    const auto neighbor_radius =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = invalid_neighbor_radius,
        });
    require_invalid(neighbor_radius, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_neighbor_radius);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> duplicate_neighbor_ids{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = -1.0F, .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const auto duplicate_neighbor =
        mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
            .agent =
                mirakana::NavigationLocalAvoidanceAgentDesc{
                    .id = 10U,
                    .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                    .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                    .radius = 0.5F,
                    .max_speed = 2.0F,
                },
            .neighbors = duplicate_neighbor_ids,
        });
    require_invalid(duplicate_neighbor, mirakana::NavigationLocalAvoidanceDiagnostic::duplicate_neighbor_id);

    const mirakana::NavigationLocalAvoidanceAgentDesc valid_agent{
        .id = 10U,
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .desired_velocity = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
        .radius = 0.5F,
        .max_speed = 2.0F,
    };
    const auto invalid_weight = mirakana::calculate_navigation_local_avoidance(
        mirakana::NavigationLocalAvoidanceRequest{.agent = valid_agent,
                                                  .neighbors = {},
                                                  .separation_weight = -1.0F,
                                                  .prediction_time_seconds = 1.0F,
                                                  .epsilon = 0.001F});
    require_invalid(invalid_weight, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_separation_weight);

    const auto invalid_prediction = mirakana::calculate_navigation_local_avoidance(
        mirakana::NavigationLocalAvoidanceRequest{.agent = valid_agent,
                                                  .neighbors = {},
                                                  .separation_weight = 1.0F,
                                                  .prediction_time_seconds = -1.0F,
                                                  .epsilon = 0.001F});
    require_invalid(invalid_prediction, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_prediction_time_seconds);

    const auto invalid_epsilon = mirakana::calculate_navigation_local_avoidance(
        mirakana::NavigationLocalAvoidanceRequest{.agent = valid_agent,
                                                  .neighbors = {},
                                                  .separation_weight = 1.0F,
                                                  .prediction_time_seconds = 1.0F,
                                                  .epsilon = 0.0F});
    require_invalid(invalid_epsilon, mirakana::NavigationLocalAvoidanceDiagnostic::invalid_epsilon);

    const std::vector<mirakana::NavigationLocalAvoidanceNeighborDesc> overflowing_neighbor{
        mirakana::NavigationLocalAvoidanceNeighborDesc{
            .id = 20U,
            .position = mirakana::NavigationPoint2{.x = -std::numeric_limits<float>::max(), .y = 0.0F},
            .velocity = mirakana::NavigationPoint2{.x = -std::numeric_limits<float>::max(), .y = 0.0F},
            .radius = 0.5F,
        },
    };
    const auto overflowing = mirakana::calculate_navigation_local_avoidance(mirakana::NavigationLocalAvoidanceRequest{
        .agent =
            mirakana::NavigationLocalAvoidanceAgentDesc{
                .id = 10U,
                .position = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::max(), .y = 0.0F},
                .desired_velocity = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::max(), .y = 0.0F},
                .radius = 0.5F,
                .max_speed = std::numeric_limits<float>::max(),
            },
        .neighbors = overflowing_neighbor,
        .separation_weight = 1.0F,
        .prediction_time_seconds = 2.0F,
        .epsilon = 0.001F,
    });
    require_invalid(overflowing, mirakana::NavigationLocalAvoidanceDiagnostic::calculation_overflow);
}

MK_TEST("navigation maps grid paths to point centers") {
    const std::vector<mirakana::NavigationGridCoord> path{
        mirakana::NavigationGridCoord{.x = 0, .y = 0},
        mirakana::NavigationGridCoord{.x = 2, .y = 1},
    };
    const auto points = mirakana::build_navigation_point_path(
        path, mirakana::NavigationGridPointMapping{.origin = mirakana::NavigationPoint2{.x = 10.0F, .y = 20.0F},
                                                   .cell_size = 2.0F,
                                                   .use_cell_centers = true});

    MK_REQUIRE(points.size() == 2);
    MK_REQUIRE(points[0] == (mirakana::NavigationPoint2{11.0F, 21.0F}));
    MK_REQUIRE(points[1] == (mirakana::NavigationPoint2{15.0F, 23.0F}));

    const auto corners = mirakana::build_navigation_point_path(
        path, mirakana::NavigationGridPointMapping{.origin = mirakana::NavigationPoint2{.x = 10.0F, .y = 20.0F},
                                                   .cell_size = 2.0F,
                                                   .use_cell_centers = false});
    MK_REQUIRE(corners[0] == (mirakana::NavigationPoint2{10.0F, 20.0F}));
    MK_REQUIRE(corners[1] == (mirakana::NavigationPoint2{14.0F, 22.0F}));

    const auto empty = mirakana::build_navigation_point_path_result(
        {}, mirakana::NavigationGridPointMapping{.origin = mirakana::NavigationPoint2{.x = 10.0F, .y = 20.0F},
                                                 .cell_size = 2.0F,
                                                 .use_cell_centers = true});
    MK_REQUIRE(empty.status == mirakana::NavigationPointPathBuildStatus::success);
    MK_REQUIRE(empty.points.empty());

    const std::vector<mirakana::NavigationGridCoord> overflowing_path{
        mirakana::NavigationGridCoord{.x = 1, .y = 0},
    };
    const auto invalid = mirakana::build_navigation_point_path_result(
        overflowing_path,
        mirakana::NavigationGridPointMapping{.origin = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                             .cell_size = std::numeric_limits<float>::max(),
                                             .use_cell_centers = true});
    MK_REQUIRE(invalid.status == mirakana::NavigationPointPathBuildStatus::invalid_mapping);
    MK_REQUIRE(invalid.points.empty());
}

MK_TEST("navigation grid agent path planner creates a smoothed moving agent path") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 4, .height = 4});
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 1}, false);
    grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 2}, false);

    const auto plan =
        mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                            .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                            .goal = mirakana::NavigationGridCoord{.x = 3, .y = 3},
                                                            .mapping =
                                                                mirakana::NavigationGridPointMapping{
                                                                    .origin = mirakana::NavigationPoint2{},
                                                                    .cell_size = 1.0F,
                                                                    .use_cell_centers = true,
                                                                },
                                                            .adjacency = mirakana::NavigationAdjacency::cardinal4,
                                                            .smooth_path = true,
                                                        });

    MK_REQUIRE(plan.status == mirakana::NavigationGridAgentPathStatus::ready);
    MK_REQUIRE(plan.diagnostic == mirakana::NavigationGridAgentPathDiagnostic::none);
    MK_REQUIRE(plan.path_result.status == mirakana::NavigationPathStatus::success);
    MK_REQUIRE(plan.smoothing_result.status == mirakana::NavigationGridPathSmoothingStatus::success);
    MK_REQUIRE(plan.point_path_result.status == mirakana::NavigationPointPathBuildStatus::success);
    MK_REQUIRE(plan.used_smoothing);
    MK_REQUIRE(plan.raw_grid_point_count == 7U);
    MK_REQUIRE(plan.planned_grid_point_count == 3U);
    MK_REQUIRE(plan.total_cost == 6U);
    MK_REQUIRE(plan.agent_state.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(plan.agent_state.position == (mirakana::NavigationPoint2{0.5F, 0.5F}));
    MK_REQUIRE(plan.agent_state.path == plan.point_path_result.points);
    MK_REQUIRE(plan.agent_state.path.size() == 3U);
    MK_REQUIRE(plan.agent_state.path.back() == (mirakana::NavigationPoint2{3.5F, 3.5F}));
}

MK_TEST("navigation grid agent path planner can preserve unsmoothed grid paths") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 3, .height = 1});

    const auto plan = mirakana::plan_navigation_grid_agent_path(
        grid, mirakana::NavigationGridAgentPathRequest{
                  .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                  .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                  .mapping =
                      mirakana::NavigationGridPointMapping{
                          .origin = mirakana::NavigationPoint2{.x = 10.0F, .y = 1.0F},
                          .cell_size = 2.0F,
                          .use_cell_centers = false,
                      },
                  .adjacency = mirakana::NavigationAdjacency::cardinal4,
                  .smooth_path = false,
              });

    MK_REQUIRE(plan.status == mirakana::NavigationGridAgentPathStatus::ready);
    MK_REQUIRE(!plan.used_smoothing);
    MK_REQUIRE(plan.raw_grid_point_count == 3U);
    MK_REQUIRE(plan.planned_grid_point_count == 3U);
    MK_REQUIRE(plan.agent_state.path == (std::vector<mirakana::NavigationPoint2>{
                                            mirakana::NavigationPoint2{10.0F, 1.0F},
                                            mirakana::NavigationPoint2{12.0F, 1.0F},
                                            mirakana::NavigationPoint2{14.0F, 1.0F},
                                        }));
}

MK_TEST("navigation grid agent path planner reports invalid mapping and unsupported adjacency") {
    mirakana::NavigationGrid grid(mirakana::NavigationGridSize{.width = 2, .height = 1});

    const auto invalid_mapping =
        mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                            .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                            .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                            .mapping =
                                                                mirakana::NavigationGridPointMapping{
                                                                    .origin = mirakana::NavigationPoint2{},
                                                                    .cell_size = 0.0F,
                                                                    .use_cell_centers = true,
                                                                },
                                                        });
    MK_REQUIRE(invalid_mapping.status == mirakana::NavigationGridAgentPathStatus::invalid_mapping);
    MK_REQUIRE(invalid_mapping.diagnostic == mirakana::NavigationGridAgentPathDiagnostic::invalid_mapping);
    MK_REQUIRE(invalid_mapping.point_path_result.status == mirakana::NavigationPointPathBuildStatus::invalid_mapping);
    MK_REQUIRE(invalid_mapping.agent_state.status == mirakana::NavigationAgentStatus::idle);

    const auto unsupported =
        mirakana::plan_navigation_grid_agent_path(grid, mirakana::NavigationGridAgentPathRequest{
                                                            .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                                                            .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                                                            .mapping = mirakana::NavigationGridPointMapping{},
                                                            .adjacency = static_cast<mirakana::NavigationAdjacency>(99),
                                                        });
    MK_REQUIRE(unsupported.status == mirakana::NavigationGridAgentPathStatus::unsupported_adjacency);
    MK_REQUIRE(unsupported.diagnostic == mirakana::NavigationGridAgentPathDiagnostic::unsupported_adjacency);
}

MK_TEST("navigation grid agent path planner maps blocked endpoints and no path") {
    mirakana::NavigationGrid blocked_goal(mirakana::NavigationGridSize{.width = 2, .height = 1});
    blocked_goal.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    const auto blocked = mirakana::plan_navigation_grid_agent_path(
        blocked_goal, mirakana::NavigationGridAgentPathRequest{
                          .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                          .goal = mirakana::NavigationGridCoord{.x = 1, .y = 0},
                      });
    MK_REQUIRE(blocked.status == mirakana::NavigationGridAgentPathStatus::blocked_endpoint);
    MK_REQUIRE(blocked.diagnostic == mirakana::NavigationGridAgentPathDiagnostic::path_blocked_endpoint);

    mirakana::NavigationGrid split_grid(mirakana::NavigationGridSize{.width = 3, .height = 1});
    split_grid.set_walkable(mirakana::NavigationGridCoord{.x = 1, .y = 0}, false);
    const auto no_path = mirakana::plan_navigation_grid_agent_path(
        split_grid, mirakana::NavigationGridAgentPathRequest{
                        .start = mirakana::NavigationGridCoord{.x = 0, .y = 0},
                        .goal = mirakana::NavigationGridCoord{.x = 2, .y = 0},
                    });
    MK_REQUIRE(no_path.status == mirakana::NavigationGridAgentPathStatus::no_path);
    MK_REQUIRE(no_path.diagnostic == mirakana::NavigationGridAgentPathDiagnostic::path_not_found);
    MK_REQUIRE(no_path.point_path_result.points.empty());
}

MK_TEST("navigation path follower advances through waypoints deterministically") {
    const std::vector<mirakana::NavigationPoint2> path{
        mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 2.0F, .y = 2.0F},
    };
    const mirakana::NavigationPathFollowRequest request{
        .points = path,
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target_index = 0,
        .max_step_distance = 3.0F,
        .arrival_radius = 0.01F,
    };

    const auto result = mirakana::advance_navigation_path_following(request);
    const auto replay = mirakana::advance_navigation_path_following(request);

    MK_REQUIRE(result.status == mirakana::NavigationPathFollowStatus::advanced);
    MK_REQUIRE(result.target_index == 2);
    MK_REQUIRE(result.position == (mirakana::NavigationPoint2{2.0F, 1.0F}));
    MK_REQUIRE(result.step_delta == (mirakana::NavigationPoint2{2.0F, 1.0F}));
    MK_REQUIRE(std::abs(result.advanced_distance - 3.0F) < 0.0001F);
    MK_REQUIRE(std::abs(result.remaining_distance - 1.0F) < 0.0001F);
    MK_REQUIRE(!result.reached_destination);
    MK_REQUIRE(replay.status == result.status);
    MK_REQUIRE(replay.target_index == result.target_index);
    MK_REQUIRE(replay.position == result.position);
}

MK_TEST("navigation path follower snaps arrivals and supports explicit path replacement") {
    const std::vector<mirakana::NavigationPoint2> path{
        mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
    };
    const auto reached = mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
        .points = path,
        .position = mirakana::NavigationPoint2{.x = 0.95F, .y = 0.0F},
        .target_index = 1,
        .max_step_distance = 10.0F,
        .arrival_radius = 0.1F,
    });

    MK_REQUIRE(reached.status == mirakana::NavigationPathFollowStatus::reached_destination);
    MK_REQUIRE(reached.reached_destination);
    MK_REQUIRE(reached.target_index == 1);
    MK_REQUIRE(reached.position == (mirakana::NavigationPoint2{1.0F, 0.0F}));
    MK_REQUIRE(std::abs(reached.step_delta.x - 0.05F) < 0.0001F);
    MK_REQUIRE(reached.step_delta.y == 0.0F);
    MK_REQUIRE(reached.remaining_distance == 0.0F);

    const std::vector<mirakana::NavigationPoint2> replacement{
        mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 1.0F, .y = 2.0F},
    };
    const auto replaced = mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
        .points = replacement,
        .position = reached.position,
        .target_index = 0,
        .max_step_distance = 1.0F,
        .arrival_radius = 0.01F,
    });
    MK_REQUIRE(replaced.status == mirakana::NavigationPathFollowStatus::advanced);
    MK_REQUIRE(replaced.target_index == 1);
    MK_REQUIRE(replaced.position == (mirakana::NavigationPoint2{1.0F, 1.0F}));
}

MK_TEST("navigation path follower preserves step clamp near arrival radius") {
    const std::vector<mirakana::NavigationPoint2> path{
        mirakana::NavigationPoint2{.x = 0.09F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
    };

    const auto result = mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
        .points = path,
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target_index = 0,
        .max_step_distance = 0.01F,
        .arrival_radius = 0.1F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationPathFollowStatus::advanced);
    MK_REQUIRE(result.target_index == 0);
    MK_REQUIRE(std::abs(result.position.x - 0.01F) < 0.000001F);
    MK_REQUIRE(result.position.y == 0.0F);
    MK_REQUIRE(std::abs(result.advanced_distance - 0.01F) < 0.000001F);
    MK_REQUIRE(!result.reached_destination);
}

MK_TEST("navigation path follower rejects invalid requests") {
    const std::vector<mirakana::NavigationPoint2> path{
        mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
    };

    MK_REQUIRE(mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{}).status ==
               mirakana::NavigationPathFollowStatus::invalid_path);
    MK_REQUIRE(mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
                                                               path,
                                                               mirakana::NavigationPoint2{0.0F, 0.0F},
                                                               4,
                                                               1.0F,
                                                               0.01F,
                                                           })
                   .status == mirakana::NavigationPathFollowStatus::invalid_request);
    MK_REQUIRE(mirakana::advance_navigation_path_following(
                   mirakana::NavigationPathFollowRequest{
                       path,
                       mirakana::NavigationPoint2{std::numeric_limits<float>::quiet_NaN(), 0.0F},
                       0,
                       1.0F,
                       0.01F,
                   })
                   .status == mirakana::NavigationPathFollowStatus::invalid_request);
    MK_REQUIRE(mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
                                                               path,
                                                               mirakana::NavigationPoint2{0.0F, 0.0F},
                                                               0,
                                                               0.0F,
                                                               0.01F,
                                                           })
                   .status == mirakana::NavigationPathFollowStatus::invalid_request);
    MK_REQUIRE(mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
                                                               path,
                                                               mirakana::NavigationPoint2{0.0F, 0.0F},
                                                               0,
                                                               1.0F,
                                                               -0.01F,
                                                           })
                   .status == mirakana::NavigationPathFollowStatus::invalid_request);
    MK_REQUIRE(mirakana::advance_navigation_path_following(
                   mirakana::NavigationPathFollowRequest{
                       std::vector<mirakana::NavigationPoint2>{
                           mirakana::NavigationPoint2{std::numeric_limits<float>::max(), 0.0F},
                           mirakana::NavigationPoint2{-std::numeric_limits<float>::max(), 0.0F},
                       },
                       mirakana::NavigationPoint2{std::numeric_limits<float>::max(), 0.0F},
                       1,
                       1.0F,
                       0.0F,
                   })
                   .status == mirakana::NavigationPathFollowStatus::invalid_request);
}

MK_TEST("navigation path follower handles tiny distances without false progress") {
    const std::vector<mirakana::NavigationPoint2> path{
        mirakana::NavigationPoint2{.x = 0.0000005F, .y = 0.0F},
    };

    const auto result = mirakana::advance_navigation_path_following(mirakana::NavigationPathFollowRequest{
        .points = path,
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target_index = 0,
        .max_step_distance = 0.0000002F,
        .arrival_radius = 0.0F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationPathFollowStatus::advanced);
    MK_REQUIRE(std::abs(result.position.x - 0.0000002F) < 0.00000005F);
    MK_REQUIRE(result.position.y == 0.0F);
    MK_REQUIRE(std::abs(result.advanced_distance - 0.0000002F) < 0.00000005F);
}

MK_TEST("navigation arrive steering clamps slows and reaches deterministically") {
    const auto far = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = 10.0F, .y = 0.0F},
        .max_speed = 4.0F,
        .slowing_radius = 5.0F,
        .arrival_radius = 0.1F,
    });
    MK_REQUIRE(far.status == mirakana::NavigationArriveSteeringStatus::moving);
    MK_REQUIRE(far.desired_velocity == (mirakana::NavigationPoint2{4.0F, 0.0F}));
    MK_REQUIRE(std::abs(far.desired_speed - 4.0F) < 0.0001F);
    MK_REQUIRE(std::abs(far.distance_to_target - 10.0F) < 0.0001F);

    const auto near = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = 2.5F, .y = 0.0F},
        .max_speed = 4.0F,
        .slowing_radius = 5.0F,
        .arrival_radius = 0.1F,
    });
    MK_REQUIRE(near.status == mirakana::NavigationArriveSteeringStatus::moving);
    MK_REQUIRE(near.desired_velocity == (mirakana::NavigationPoint2{2.0F, 0.0F}));
    MK_REQUIRE(std::abs(near.desired_speed - 2.0F) < 0.0001F);

    const auto reached = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = 0.05F, .y = 0.0F},
        .max_speed = 4.0F,
        .slowing_radius = 5.0F,
        .arrival_radius = 0.1F,
    });
    MK_REQUIRE(reached.status == mirakana::NavigationArriveSteeringStatus::reached_target);
    MK_REQUIRE(reached.reached_target);
    MK_REQUIRE(reached.desired_velocity == (mirakana::NavigationPoint2{0.0F, 0.0F}));

    const auto invalid = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
        .max_speed = -1.0F,
        .slowing_radius = 5.0F,
        .arrival_radius = 0.1F,
    });
    MK_REQUIRE(invalid.status == mirakana::NavigationArriveSteeringStatus::invalid_request);

    const auto tiny = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = 0.0000005F, .y = 0.0F},
        .max_speed = 1.0F,
        .slowing_radius = 1.0F,
        .arrival_radius = 0.0F,
    });
    MK_REQUIRE(tiny.status == mirakana::NavigationArriveSteeringStatus::moving);
    MK_REQUIRE(tiny.desired_velocity.x > 0.0F);
    MK_REQUIRE(tiny.desired_speed > 0.0F);

    const auto overflow = mirakana::calculate_navigation_arrive_steering(mirakana::NavigationArriveSteeringRequest{
        .position = mirakana::NavigationPoint2{.x = std::numeric_limits<float>::max(), .y = 0.0F},
        .target = mirakana::NavigationPoint2{.x = -std::numeric_limits<float>::max(), .y = 0.0F},
        .max_speed = 1.0F,
        .slowing_radius = 1.0F,
        .arrival_radius = 0.0F,
    });
    MK_REQUIRE(overflow.status == mirakana::NavigationArriveSteeringStatus::invalid_request);
}

MK_TEST("navigation agent idles without a path") {
    const auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 2.0F, .y = 3.0F});
    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 4.0F, .slowing_radius = 2.0F, .arrival_radius = 0.1F},
        .delta_seconds = 0.5F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::idle);
    MK_REQUIRE(result.state.status == mirakana::NavigationAgentStatus::idle);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{2.0F, 3.0F}));
    MK_REQUIRE(result.state.path.empty());
    MK_REQUIRE(result.state.target_index == 0);
    MK_REQUIRE(result.step_delta == (mirakana::NavigationPoint2{0.0F, 0.0F}));
    MK_REQUIRE(result.desired_velocity == (mirakana::NavigationPoint2{0.0F, 0.0F}));
    MK_REQUIRE(result.step_distance == 0.0F);
    MK_REQUIRE(result.remaining_distance == 0.0F);
    MK_REQUIRE(!result.reached_destination);
}

MK_TEST("navigation agent advances along a path with speed and delta clamp") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 2.0F, .y = 2.0F},
                                                           });

    const mirakana::NavigationAgentUpdateRequest request{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 2.0F, .slowing_radius = 0.5F, .arrival_radius = 0.01F},
        .delta_seconds = 0.75F,
    };
    const auto result = mirakana::update_navigation_agent(request);
    const auto replay = mirakana::update_navigation_agent(request);

    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(result.state.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(result.state.target_index == 1);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{1.5F, 0.0F}));
    MK_REQUIRE(result.step_delta == (mirakana::NavigationPoint2{1.5F, 0.0F}));
    MK_REQUIRE(result.desired_velocity == (mirakana::NavigationPoint2{2.0F, 0.0F}));
    MK_REQUIRE(std::abs(result.step_distance - 1.5F) < 0.0001F);
    MK_REQUIRE(std::abs(result.remaining_distance - 2.5F) < 0.0001F);
    MK_REQUIRE(!result.reached_destination);
    MK_REQUIRE(replay.status == result.status);
    MK_REQUIRE(replay.state.position == result.state.position);
    MK_REQUIRE(replay.state.target_index == result.state.target_index);
}

MK_TEST("navigation agent reports velocity from actual path movement across corners") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 2.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 2.0F, .y = 2.0F},
                                                           });

    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 3.0F, .slowing_radius = 1.0F, .arrival_radius = 0.01F},
        .delta_seconds = 1.0F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(result.state.target_index == 2);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{2.0F, 1.0F}));
    MK_REQUIRE(result.step_delta == (mirakana::NavigationPoint2{2.0F, 1.0F}));
    MK_REQUIRE(result.desired_velocity == (mirakana::NavigationPoint2{2.0F, 1.0F}));
}

MK_TEST("navigation agent slows near final destination") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                                           });

    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 4.0F, .slowing_radius = 2.0F, .arrival_radius = 0.01F},
        .delta_seconds = 0.25F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{0.5F, 0.0F}));
    MK_REQUIRE(result.desired_velocity == (mirakana::NavigationPoint2{2.0F, 0.0F}));
    MK_REQUIRE(std::abs(result.step_distance - 0.5F) < 0.0001F);
}

MK_TEST("navigation agent bases slowing on remaining path distance") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 10.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 10.0F, .y = 1.0F},
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 1.0F},
                                                           });

    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 4.0F, .slowing_radius = 5.0F, .arrival_radius = 0.01F},
        .delta_seconds = 0.25F,
    });

    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{1.0F, 0.0F}));
    MK_REQUIRE(result.desired_velocity == (mirakana::NavigationPoint2{4.0F, 0.0F}));
    MK_REQUIRE(std::abs(result.step_distance - 1.0F) < 0.0001F);
}

MK_TEST("navigation agent reaches destination and can replace path") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.9F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                                           });

    const auto reached = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 1.0F, .slowing_radius = 1.0F, .arrival_radius = 0.1F},
        .delta_seconds = 1.0F,
    });

    MK_REQUIRE(reached.status == mirakana::NavigationAgentStatus::reached_destination);
    MK_REQUIRE(reached.reached_destination);
    MK_REQUIRE(reached.state.position == (mirakana::NavigationPoint2{1.0F, 0.0F}));
    MK_REQUIRE(reached.state.target_index == 0);
    MK_REQUIRE(std::abs(reached.desired_velocity.x - 0.1F) < 0.0001F);
    MK_REQUIRE(reached.desired_velocity.y == 0.0F);
    MK_REQUIRE(std::abs(reached.step_distance - 0.1F) < 0.0001F);

    const auto replaced =
        mirakana::replace_navigation_agent_path(reached.state, std::vector<mirakana::NavigationPoint2>{
                                                                   mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                                                   mirakana::NavigationPoint2{.x = 1.0F, .y = 1.0F},
                                                               });
    MK_REQUIRE(replaced.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(replaced.target_index == 0);

    const auto moved = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = replaced,
        .config = mirakana::NavigationAgentConfig{.max_speed = 1.0F, .slowing_radius = 0.5F, .arrival_radius = 0.01F},
        .delta_seconds = 0.5F,
    });
    MK_REQUIRE(moved.status == mirakana::NavigationAgentStatus::moving);
    MK_REQUIRE(moved.state.target_index == 1);
    MK_REQUIRE(moved.state.position == (mirakana::NavigationPoint2{1.0F, 0.5F}));
}

MK_TEST("navigation agent cancel preserves cancelled status") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(state, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                                           });

    const auto cancelled = mirakana::cancel_navigation_agent_move(state);
    MK_REQUIRE(cancelled.status == mirakana::NavigationAgentStatus::cancelled);
    MK_REQUIRE(cancelled.path.empty());

    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = cancelled,
        .config = mirakana::NavigationAgentConfig{.max_speed = 1.0F, .slowing_radius = 1.0F, .arrival_radius = 0.01F},
        .delta_seconds = 1.0F,
    });
    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::cancelled);
    MK_REQUIRE(result.state.status == mirakana::NavigationAgentStatus::cancelled);
    MK_REQUIRE(result.state.position == (mirakana::NavigationPoint2{0.0F, 0.0F}));
}

MK_TEST("navigation agent path replacement rejects invalid paths") {
    auto state = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    state = mirakana::replace_navigation_agent_path(
        state, std::vector<mirakana::NavigationPoint2>{
                   mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                   mirakana::NavigationPoint2{.x = std::numeric_limits<float>::quiet_NaN(), .y = 0.0F},
               });

    MK_REQUIRE(state.status == mirakana::NavigationAgentStatus::invalid_request);

    const auto result = mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
        .state = state,
        .config = mirakana::NavigationAgentConfig{.max_speed = 1.0F, .slowing_radius = 1.0F, .arrival_radius = 0.01F},
        .delta_seconds = 1.0F,
    });
    MK_REQUIRE(result.status == mirakana::NavigationAgentStatus::invalid_request);
}

MK_TEST("navigation agent rejects invalid update inputs") {
    auto valid = mirakana::make_navigation_agent_state(mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F});
    valid = mirakana::replace_navigation_agent_path(valid, std::vector<mirakana::NavigationPoint2>{
                                                               mirakana::NavigationPoint2{.x = 0.0F, .y = 0.0F},
                                                               mirakana::NavigationPoint2{.x = 1.0F, .y = 0.0F},
                                                           });

    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     mirakana::NavigationAgentState{mirakana::NavigationPoint2{
                                                         std::numeric_limits<float>::quiet_NaN(), 0.0F}},
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, 0.0F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     valid,
                                                     mirakana::NavigationAgentConfig{0.0F, 1.0F, 0.0F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     valid,
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, 0.0F},
                                                     0.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     valid,
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, -0.01F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     mirakana::NavigationAgentState{
                                                         mirakana::NavigationPoint2{0.0F, 0.0F},
                                                         std::vector<mirakana::NavigationPoint2>{
                                                             mirakana::NavigationPoint2{0.0F, 0.0F},
                                                         },
                                                         8,
                                                         mirakana::NavigationAgentStatus::moving,
                                                     },
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, 0.0F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(
                   mirakana::NavigationAgentUpdateRequest{
                       valid,
                       mirakana::NavigationAgentConfig{std::numeric_limits<float>::max(), 1.0F, 0.0F},
                       2.0F,
                   })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     mirakana::NavigationAgentState{
                                                         mirakana::NavigationPoint2{0.0F, 0.0F},
                                                         std::vector<mirakana::NavigationPoint2>{
                                                             mirakana::NavigationPoint2{0.0F, 0.0F},
                                                             mirakana::NavigationPoint2{1.0F, 0.0F},
                                                         },
                                                         0,
                                                         mirakana::NavigationAgentStatus::idle,
                                                     },
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, 0.0F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
    MK_REQUIRE(mirakana::update_navigation_agent(mirakana::NavigationAgentUpdateRequest{
                                                     mirakana::NavigationAgentState{
                                                         mirakana::NavigationPoint2{0.0F, 0.0F},
                                                         std::vector<mirakana::NavigationPoint2>{
                                                             mirakana::NavigationPoint2{0.0F, 0.0F},
                                                             mirakana::NavigationPoint2{1.0F, 0.0F},
                                                         },
                                                         0,
                                                         static_cast<mirakana::NavigationAgentStatus>(99),
                                                     },
                                                     mirakana::NavigationAgentConfig{1.0F, 1.0F, 0.0F},
                                                     1.0F,
                                                 })
                   .status == mirakana::NavigationAgentStatus::invalid_request);
}

int main() {
    return mirakana::test::run_all();
}
