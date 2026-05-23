# Sprite 9-Slice and Tiled v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add 9-slice and tiled sprite draw metadata to scene sprites, expand them into multiple renderer commands with correct UV sub-rects, and prove expansion through unit tests plus `sample_2d_desktop_runtime_package --require-sprite-9slice-tiled`.

**Plan ID:** `sprite-9slice-and-tiled-v1`

**Status:** Completed.

**Gap:** `sprite-9slice-and-tiled-v1`

**Architecture:** Extend `SpriteRendererComponent` with `draw_mode`, normalized `slice_border`, and tiled `tile_size`, expand in `expand_scene_sprite_commands` used by `plan_scene_sprite_batches` and `submit_scene_render_packet`, round-trip through scene IO, validate nine-slice borders/tile sizing in components plus tooling helpers, and expose package counters from `SceneSpriteExpansionStats` (`source_sprite_count`, `expanded_sprite_count`, `nine_slice_count`, `tiled_count`).

**Tech Stack:** C++23, `MK_scene`, `MK_scene_renderer`, `MK_renderer`, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official docs review

- In-repo authority: cap-01 pattern in `2026-05-23-sprite-sorting-layer-v1.md`, backlog row in `04-developer-owned-engine-capability-backlog.md`.
- Border insets are normalized fractions of the source sprite rect; `nine_slice` stretches center/edge regions, `tiled` repeats edge/center patches.

## Non-goals

- Runtime source image decoding, UI middleware, public RHI handles, editor preview tooling, Metal parity, or production atlas packing workflows.

## Done when

- Unit tests prove nine-slice (9 quads, corner UVs), tiled (deterministic repeated quad count), and invalid-border fail-closed expansion.
- Scene IO round-trips `draw_mode`, `slice_border`, and tiled `tile_size`.
- `sample_2d_desktop_runtime_package --require-sprite-9slice-tiled` exits 0 with engine-backed `sprite_9slice_expanded_quads`, `sprite_tiled_expanded_quads`, and `sprite_9slice_tiled_source_sprites`.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.

## Evidence log

| Step | Result | Notes |
| --- | --- | --- |
| implementation | pass | Engine draw_mode/slice_border, expand_scene_sprite_commands, scene IO, tools border validation, package counters |
| validate.ps1 | pass | 74/74 tests, sample_2d_desktop_runtime_package_smoke with --require-sprite-9slice-tiled exit 0 |
| PR | merged | PR #197 merged as `e4dcbbac2dd2114b8441d5de8f967f2f1b03d1f8`. |
