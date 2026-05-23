# Navigation Hierarchical World v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add value-only hierarchical navigation and world-region reference diagnostics so generated games can reason about region graphs, portal links, streaming-safe nav data references, and path-cache readiness without claiming open-world streaming execution or automatic nav baking.

**Plan ID:** `navigation-hierarchical-world-v1`

**Status:** Planned.

**Gap:** `navigation-hierarchical-world-v1`

**Architecture:** Planned next child of Candidate Backlog Burn-down v1. Prefer existing `MK_runtime`, `MK_physics`, and game-facing package counter patterns; keep region/world data as deterministic value rows and keep streaming execution, background baking, middleware, and native handles outside this slice.

**Tech Stack:** C++23, `MK_runtime` and existing navigation/world-region contracts, selected unit tests, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use project value-planning conventions and official CMake/test entrypoints.
- Use existing first-party navigation/path diagnostics where possible; do not introduce third-party navigation middleware or host streaming workers in this slice.

## Non-Goals

- Open-world streaming execution, automatic navmesh/navgrid baking, background workers, renderer/RHI integration, native handles, third-party navigation middleware, persistence migration, multiplayer replication, or broad production open-world readiness.

## Done When

- Unit tests prove deterministic hierarchical region/portal/path-cache diagnostics and fail-closed malformed references.
- Selected package-facing counters or retained runtime diagnostics expose the reviewed rows without mutating world packages.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.
