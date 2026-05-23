# Navigation Hierarchical World v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add value-only hierarchical navigation and world-region reference diagnostics so generated games can reason about region graphs, portal links, streaming-safe nav data references, and path-cache readiness without claiming open-world streaming execution or automatic nav baking.

**Plan ID:** `navigation-hierarchical-world-v1`

**Status:** Implemented; local full validation passed; PR pending.

**Gap:** `navigation-hierarchical-world-v1`

**Architecture:** Candidate Backlog Burn-down v1 child. `MK_navigation` owns the deterministic hierarchical region graph planner, while `MK_runtime` owns value-only world-region package residency and path-cache review. The boundary intentionally keeps streaming execution, background baking, middleware, renderer/RHI ownership, and native handles outside this slice.

**Tech Stack:** C++23, `MK_runtime` and existing navigation/world-region contracts, selected unit tests, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use project value-planning conventions and official CMake/test entrypoints.
- Use existing first-party navigation/path diagnostics where possible; do not introduce third-party navigation middleware or host streaming workers in this slice.

## Non-Goals

- Open-world streaming execution, automatic navmesh/navgrid baking, background workers, renderer/RHI integration, native handles, third-party navigation middleware, persistence migration, multiplayer replication, or broad production open-world readiness.

## Implementation Summary

- Added `NavigationHierarchicalWorldPathRequest`, `NavigationHierarchicalWorldPathResult`, `NavigationHierarchicalWorldPortalPathRow`, and `plan_navigation_hierarchical_world_path` in `MK_navigation` for deterministic region/portal routing over reviewed world-region refs, nav-data refs, and endpoint scene refs.
- Added `RuntimeWorldRegionNavigationRefReviewRequest`, `RuntimeWorldRegionNavigationRefReviewResult`, `review_runtime_world_region_navigation_refs`, `RuntimeWorldRegionNavigationPathCacheReviewRequest`, `RuntimeWorldRegionNavigationPathCacheReviewResult`, and `review_runtime_world_region_navigation_path_cache` in `MK_runtime` for value-only route residency rows and fail-closed retained path-cache readiness.
- Reviewer hardening split ref-review and cache-review result types, rejected empty routes and route/portal count mismatches, and made unrefreshed or mount-generation-stale `RuntimeResidentCatalogCacheV2` state fail closed before `cache_ready`.

## Execution Checklist

- [x] Define minimal external contracts and tests first.
- [x] Implement navigation planner and runtime route/cache review.
- [x] Run focused build and CTest loops for `MK_navigation_tests` and `MK_runtime_world_region_streaming_tests`.
- [x] Sync docs, manifest fragments, and static guards.
- [x] Run `check-ai-integration`, `check-json-contracts`, and full `tools/validate.ps1`.
- [ ] Commit, push, open PR, wait for hosted checks, merge, and clean the worktree through repository wrappers.

## Validation Evidence

| Gate | Status | Evidence |
| --- | --- | --- |
| TDD red | pass | `MK_navigation_tests` first failed on missing `mirakana/navigation/navigation_hierarchical_world.hpp`; `MK_runtime_world_region_streaming_tests` first failed on missing runtime navigation review types/functions. |
| Focused build | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_navigation_tests --parallel 4`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_world_region_streaming_tests --parallel 4`. |
| Focused CTest | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_navigation_tests" --timeout 120 --parallel 1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_world_region_streaming_tests" --timeout 120 --parallel 1`. |
| C++ review | pass after fixes | `cpp-reviewer` found cache-readiness fail-open, route-shape validation, result-type ambiguity, and one brittle test assertion; all were addressed before static/full validation. |
| Focused static | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/navigation/src/navigation_hierarchical_world.cpp,engine/runtime/src/world_region_streaming.cpp -Jobs 2`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`. |
| Full validation | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` completed with 76/76 CTest passing and diagnostic-only Apple/Metal host gates unchanged. |
| Hosted PR failure hardening | in progress | PR #203 initial Windows MSVC failed before validation in `actions/cache` restore for `external/vcpkg`; the branch updates Windows lanes to non-blocking `actions/cache/restore` plus guarded `actions/cache/save` for tool/package/install/build caches and extends `tools/check-ci-matrix.ps1` to keep that contract from drifting. |

## Done When

- Unit tests prove deterministic hierarchical region/portal/path-cache diagnostics and fail-closed malformed references.
- Selected package-facing counters or retained runtime diagnostics expose the reviewed rows without mutating world packages.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.
