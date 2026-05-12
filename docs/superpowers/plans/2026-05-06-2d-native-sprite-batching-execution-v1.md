# 2D Native Sprite Batching Execution v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `2d-native-sprite-batching-execution-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote the existing 2D sprite batch planning and package telemetry into a narrow renderer-owned native RHI execution
proof for adjacent compatible `SpriteCommand` runs without changing sprite order or exposing native/RHI handles.

## Architecture

Keep `mirakana_renderer` as the owner of the batching contract and native execution counters. Reuse `plan_sprite_batches` as
the canonical order-preserving batch plan, then route native 2D package sprite submission through renderer-owned RHI
overlay execution so package smokes can distinguish planned batches from executed native batches. Keep gameplay on
`IRenderer::draw_sprite`, keep cooked asset loading in the existing host/package path, and keep D3D12 as the primary
validation lane with Vulkan/Metal claims host-gated or unsupported.

## Tech Stack

C++23, `mirakana_renderer`, `mirakana_scene_renderer`, `mirakana_runtime_host_sdl3_presentation`, `sample_2d_desktop_runtime_package`,
PowerShell static/package checks, focused CTest targets, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Context

- `2d-sprite-batch-planning-contract-v1` completed `mirakana::plan_sprite_batches` as a host-independent value API over
  `SpriteCommand` rows.
- `2d-sprite-batch-package-telemetry-v1` completed `mirakana::plan_scene_sprite_batches` and package-visible
  `sprite_batch_plan_*` counters, but explicitly stopped before native GPU batch execution.
- `2d-native-rhi-sprite-package-proof-v1` completed a host-owned native 2D sprite/HUD package proof for the committed
  sample path, but it is not yet a broad production sprite batching readiness proof.
- The remaining 2D generated-game production gap in the master plan calls out native sprite batching execution or an
  equivalent renderer proof before broader 2D production readiness can be claimed.

## Constraints

- Do not sort sprites, change transparent draw ordering, or introduce material/atlas ordering policy.
- Do not expose `IRhiDevice`, RHI handles, OS handles, D3D12/Vulkan/Metal objects, or backend objects through gameplay
  APIs.
- Do not add third-party dependencies.
- Do not claim source/runtime image decoding, production atlas packing, tilemap editor UX, package streaming execution,
  Metal readiness, broad renderer quality, or broad generated 2D production readiness.
- Keep the first execution proof scoped to the existing cooked package texture/material path and adjacent compatible
  sprite runs.

## File Map

- `engine/renderer/include/mirakana/renderer/renderer.hpp`: add narrow renderer stats for native sprite batch execution if
  existing overlay counters are insufficient.
- `engine/renderer/src/rhi_native_ui_overlay.hpp` and `engine/renderer/src/rhi_native_ui_overlay.cpp`: make prepared
  native overlay draws report executed batch counts derived from `plan_sprite_batches`.
- `engine/renderer/src/rhi_frame_renderer.cpp`: accumulate native sprite batch execution counters during active frames.
- `tests/unit/renderer_rhi_tests.cpp`: add RED/GREEN coverage for adjacent compatible sprite batch execution counters.
- `games/sample_2d_desktop_runtime_package/main.cpp`: expose package-visible execution counters only after renderer
  stats prove native execution.
- `tools/validate-installed-desktop-runtime.ps1`, `tools/check-json-contracts.ps1`, and
  `tools/check-ai-integration.ps1`: require the narrow execution counters and keep unsupported claims blocked.
- Docs, manifest, plan registry, skills, and subagents: describe this as active execution proof work, not broad 2D
  renderer readiness.

## Done When

- A RED test or static check fails first because package/runtime native 2D sprite batching execution counters are absent
  or still telemetry-only.
- Native 2D sprite execution reports first-party batch execution counters derived from adjacent compatible
  `SpriteCommand` runs.
- `sample_2d_desktop_runtime_package` can require the new execution evidence in the installed package smoke without
  exposing native/RHI handles.
- Existing `sprite_batch_plan_*` telemetry remains order-preserving and unchanged.
- Unsupported claims remain explicit for atlas packing, runtime image decoding, Metal, public native/RHI handles,
  package streaming execution, and broad renderer quality.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused renderer/runtime tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public stats change, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Add RED renderer/package checks for native sprite batch execution counters.
- [x] Implement renderer-owned native sprite batch execution reporting from `plan_sprite_batches`.
- [x] Promote the committed 2D package smoke to require execution counters while preserving telemetry counters.
- [x] Update docs, manifest, static checks, skills/subagents, and validation evidence.

## Validation Evidence

| Command | Result | Evidence |
| --- | --- | --- |
| `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_renderer_tests"` | RED FAIL (expected) | Before implementation, `mirakana::RendererStats` had no `native_sprite_batches_executed`, `native_sprite_batch_sprites_executed`, `native_sprite_batch_textured_sprites_executed`, or `native_sprite_batch_texture_binds` fields. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | RED FAIL (expected) | Static checks required `native_2d_sprite_batches_executed` package fields before `sample_2d_desktop_runtime_package` and installed validation emitted them. |
| `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_renderer_tests"` | PASS | Renderer focused tests built after native overlay execution counters and batch draw recording landed. |
| `ctest --preset dev -R mirakana_renderer_tests --output-on-failure` | PASS | `mirakana_renderer_tests` passed, including adjacent-run native sprite batch execution coverage. |
| `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target sample_2d_desktop_runtime_package"` | PASS | Source-tree 2D desktop runtime package target built with updated package status fields. |
| `ctest --preset desktop-runtime -R "sample_2d_desktop_runtime_package_(smoke\|shader_artifacts_smoke)" --output-on-failure` | PASS | 2/2 source-tree 2D desktop runtime package smoke tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | Static checks accepted renderer/package native sprite batch execution counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` | PASS | Installed D3D12 package validation passed and reported positive `native_2d_sprite_batches_executed`, `native_2d_sprite_batch_sprites_executed`, `native_2d_sprite_batch_textured_sprites_executed`, and `native_2d_sprite_batch_texture_binds` counters. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | Repository format wrapper accepted the formatted C++ changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | PASS | Public API boundary check accepted the added renderer stats fields. |
| `git diff --check` | PASS | No whitespace errors; only expected CRLF conversion warnings were reported. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Full repository validation passed, including JSON/static checks, toolchain checks, build, tidy, and 29/29 dev CTest tests. |

## Next-Step Decision

This slice completes a narrow renderer-owned native RHI sprite batch execution proof for adjacent compatible
`SpriteCommand` runs. It does not claim atlas packing, runtime image decoding, Metal readiness, package streaming
execution, public native/RHI handle exposure, broad production sprite batching readiness, or broad renderer quality.
The next active slice is `2d-sprite-animation-package-v1`, which addresses the remaining generated 2D sprite animation
package workflow gap.
