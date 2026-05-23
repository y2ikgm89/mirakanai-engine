# Sprite Collision Hitbox v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic first-party runtime sprite hitbox/hurtbox planner that converts frame-authored collision rows into stable hit rows plus gameplay interaction events, then prove it through unit tests and `sample_2d_desktop_runtime_package --require-sprite-collision-hitbox`.

**Plan ID:** `sprite-collision-hitbox-v1`

**Status:** Completed.

**Gap:** `sprite-collision-hitbox-v1`

**Architecture:** Introduce a backend-neutral `MK_runtime` value API for frame/pose-scoped hitbox and hurtbox rows. The planner validates stable ids, frame/entity pose rows, finite positive AABB bounds, layer/mask policy, duplicate rows, and hit row budgets before returning deterministic hit rows and `RuntimeGameplayInteractionEvent` values for the existing gameplay interaction planner. Package samples consume only public runtime APIs and expose selected counters.

**Tech Stack:** C++23, `MK_runtime`, repository CMake/CTest wrappers, `sample_2d_desktop_runtime_package`, composed engine agent manifest fragments.

## Official Docs Review

- CMake target additions follow target-scoped `add_executable`, `target_link_libraries`, and `add_test` guidance from the official CMake documentation surfaced through Context7.
- Repository worktree and validation entrypoints follow the project-supported PowerShell wrapper surface rather than package-manager aliases.
- Public gameplay contracts stay first-party and backend-neutral; no physics middleware, renderer/RHI handle, SDL3, Dear ImGui, or editor API is exposed.

## Non-Goals

- Runtime source sprite parsing, authored editor hitbox painting, broad collision middleware, oriented/convex casts, scene mutation, gameplay scheduler execution, UI/audio/VFX feedback execution, renderer/RHI residency, native handles, Metal parity, or broad production collision readiness.

## Done When

- Unit tests prove deterministic hit rows/gameplay events, duplicate/invalid/missing frame diagnostics, inactive poses, layer/mask filtering, and fail-closed hit budget handling.
- `sample_2d_desktop_runtime_package --require-sprite-collision-hitbox` exits 0 with `sprite_collision_hitbox_*` counters that flow through `plan_runtime_gameplay_interactions`.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.

## Evidence Log

| Step | Result | Notes |
| --- | --- | --- |
| red runtime test | pass | `MK_runtime_sprite_collision_hitbox_tests` failed before `mirakana/runtime/sprite_collision_hitbox.hpp` existed. |
| red package smoke | pass | `sample_2d_desktop_runtime_package_smoke` failed before `--require-sprite-collision-hitbox` was implemented. |
| focused validation | pass | `MK_runtime_sprite_collision_hitbox_tests` and `sample_2d_desktop_runtime_package_smoke` passed through repository build/test wrappers. |
| validate.ps1 | pass | 75/75 tests passed; static checks passed; Metal/Apple lanes remained host-gated diagnostic-only on Windows. |
