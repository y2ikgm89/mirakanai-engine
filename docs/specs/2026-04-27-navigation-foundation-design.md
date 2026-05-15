# Navigation Foundation Design

## Goal

Add a first-party deterministic navigation foundation that game code can use without editor APIs, native handles, or third-party middleware.

## Scope

The first slice implements grid-based pathfinding. It does not claim production navmesh, crowd avoidance, steering, dynamic obstacle replanning, or behavior-tree AI. Those belong in later slices after the core path contract is validated.

## Architecture

Create `mirakana_navigation` under `engine/navigation`. The module is standard-library-only and exposes public headers under `mirakana/navigation`. It remains independent from renderer, platform, physics, scene, editor, and asset code.

The public contract models:

- `NavigationGridCoord` for integer cell coordinates.
- `NavigationGrid` for walkable cells and positive traversal cost.
- `NavigationPathRequest` for start, goal, and adjacency policy.
- `NavigationPathResult` for status, path cells, total cost, and visited node count.
- `find_navigation_path` for deterministic A* pathfinding.

## Behavior

The pathfinder validates bounds, rejects blocked start or goal cells, returns `no_path` when the goal is unreachable, and includes both start and goal in successful paths. Cost is deterministic and based on the destination cell's traversal cost. Tie-breaking is stable through a fixed neighbor order and ordered frontier metadata.

## Testing

Tests must cover:

- straight-line path success,
- obstacle detours,
- blocked or out-of-bounds endpoint failures,
- unreachable goals,
- lower-cost route selection.

## Follow-Up Slices

Later navigation work should add navmesh assets, agent radius constraints, dynamic obstacle updates, steering/avoidance, path smoothing, scene integration, editor visualization, and AI controller helpers.
