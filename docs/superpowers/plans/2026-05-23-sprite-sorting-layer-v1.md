# Sprite Sorting Layer v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add explicit 2D sprite sorting layers and order-in-layer to scene sprite render packets with deterministic sort order and package-visible counters.

**Plan ID:** `sprite-sorting-layer-v1`

**Status:** Active.

**Gap:** `sprite-sorting-layer-v1`

**Architecture:** Extend `SpriteRendererComponent` with sorting fields, sort `SceneRenderPacket::sprites` in `build_scene_render_packet` before batch planning, and prove order through unit tests plus `sample_2d_desktop_runtime_package --require-sprite-sorting-layer`.

**Tech Stack:** C++23, `MK_scene`, `MK_scene_renderer`, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official docs review

- In-repo authority: `docs/cpp-style.md`, completed sprite batch planning contracts, backlog row in `04-developer-owned-engine-capability-backlog.md`.
- Sort key: `sorting_layer` ascending, then `order_in_layer` ascending, then `SceneNodeId` ascending for stable tie-break.

## Non-goals

- Transparency/overdraw policy breadth, Metal parity, public RHI handles, enabling `allow_sprite_reordering` in sprite batch planner v1.

## Done when

- Deterministic sort tests pass; scene IO round-trips sorting fields.
- `sample_2d_desktop_runtime_package` reports `sprite_sort_layers_applied` and `sprite_sorted_draws` under `--require-sprite-sorting-layer`.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.

## Evidence log

| Step | Result | Notes |
| --- | --- | --- |
| implementation | pending | |
