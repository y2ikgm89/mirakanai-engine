# Sprite Effects Particles v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add budgeted first-party sprite effects and particle planning for transient 2D visuals such as damage numbers, trails, hit flashes, and short-lived effects, then prove package-visible counters in the selected 2D runtime package.

**Plan ID:** `sprite-effects-particles-v1`

**Status:** Validated; PR pending.

**Gap:** `sprite-effects-particles-v1`

**Architecture:** Implemented as the fourth child of Candidate Backlog Burn-down v1. The runtime contract is value-only and backend-neutral, caller-owned active particle state advances through explicit requests, scene renderer integration is limited to reviewed `SpriteCommand` rows, and native/RHI handles stay out of gameplay APIs.

**Tech Stack:** C++23, `MK_runtime`, `MK_scene_renderer`, `MK_renderer` statistics as needed, repository `tools/*.ps1`, composed engine agent manifest fragments.

## Official Docs Review

- Use official CMake target-scoped patterns for new tests/targets.
- Use first-party engine contracts for authored effect rows and renderer statistics; do not add middleware or native backend types to public gameplay APIs.

## Implementation Summary

- Added `mirakana::runtime::plan_runtime_sprite_effect_particles` with explicit templates, active particle state, spawn events, render rows, budgets, deterministic ordering, lifetime fade, fail-closed diagnostics, and no renderer/RHI ownership.
- Added `mirakana::make_runtime_sprite_effect_particle_command` plus `submit_runtime_sprite_effect_particle_rows` as the thin `MK_scene_renderer` bridge from runtime rows to existing sprite submissions.
- Added `sample_2d_desktop_runtime_package --require-sprite-effects-particles` counters for spawn events, spawned/surviving/rendered/submitted rows, diagnostics, and budget diagnostics.
- Kept damage-number glyph/text rendering, GPU particles, editor authoring, source sprite parsing, scene mutation, strict interleaving with scene sorting, and native handles as non-goals.

## Non-Goals

- GPU particle simulation, editor timeline tooling, external VFX middleware, native/RHI handles, source image decoding, damage-number text/glyph rendering, Metal parity, subjective visual-quality claims, or broad renderer readiness.

## Validation Evidence

| Check | Result | Evidence |
| --- | --- | --- |
| TDD red | pass | `MK_runtime_sprite_effect_particles_tests` and `MK_scene_renderer_tests` first failed on the missing runtime header/bridge before implementation. |
| Focused build | pass | `tools/cmake.ps1 --build --preset dev --target MK_runtime_sprite_effect_particles_tests`, `MK_scene_renderer_tests`, and `sample_2d_desktop_runtime_package`. |
| Focused ctest | pass | `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_sprite_effect_particles_tests|MK_scene_renderer_tests|sample_2d_desktop_runtime_package"` completed 5/5 tests. |
| Package smoke | pass | `sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-sprite-effects-particles` emitted `sprite_effect_particles_ready=1`, `sprite_effect_particles_spawned_particles=3`, `sprite_effect_particles_render_rows=9`, and zero diagnostics. |
| Full closeout | pass | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1` completed with `validate: ok`; `test.ps1 -SkipBuild` reported 76/76 tests passed. |

## Done When

- Unit tests prove deterministic effect/particle rows, budget diagnostics, and stable ordering.
- `sample_2d_desktop_runtime_package --require-sprite-effects-particles` exits 0 with package-visible effect counters.
- Backlog row promoted; manifest/docs/static checks synced; focused validation and `validate.ps1` green; PR merged.
