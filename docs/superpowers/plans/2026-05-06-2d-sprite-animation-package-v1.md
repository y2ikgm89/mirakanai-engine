# 2D Sprite Animation Package v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `2d-sprite-animation-package-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a narrow first-party generated `DesktopRuntime2DPackage` sprite animation package smoke so a cooked 2D package can
drive deterministic frame sampling and application for a sprite through engine-owned data and package-visible counters, without adding
runtime image decoding, atlas packing, tilemap editor UX, public native/RHI handles, Metal readiness, package streaming
execution, or broad renderer quality claims.

## Architecture

Keep gameplay and package code on public engine APIs. Prefer existing first-party animation/runtime package patterns and
the committed `sample_2d_desktop_runtime_package` path before adding new abstractions. If new data is needed, keep it as
deterministic first-party cooked rows and expose only package-visible counters such as frames sampled, sprite frames
applied, and diagnostics. Keep native renderer execution separate from animation sampling; the completed native sprite
batch execution proof remains a renderer-owned presentation path.

## Context

- `2d-native-sprite-batching-execution-v1` completed narrow native RHI sprite batch execution counters for adjacent
  compatible `SpriteCommand` runs.
- Current generated 2D package scaffolds load a cooked sprite texture/material/audio/tilemap/scene and can validate
  native 2D sprite presentation, but they do not yet prove sprite animation package data or frame application.
- The production master plan still lists `2d-sprite-animation-package-v1` as a remaining 2D generated-game production
  gap.

## Constraints

- Do not introduce third-party dependencies.
- Do not sort sprites or change transparent draw ordering.
- Do not expose `IRhiDevice`, native handles, RHI handles, backend objects, or atlas internals to gameplay APIs.
- Do not claim runtime source image decoding, production atlas packing, tilemap editor UX, Metal readiness, package
  streaming execution, broad production sprite batching readiness, or broad renderer quality.
- Keep any generated/package validation deterministic and host-feasible on the existing Windows desktop runtime lane.

## File Map

- `engine/animation/`, `engine/runtime/`, `engine/scene_renderer/`: inspect existing first-party animation/package
  payload patterns before adding a 2D sprite animation contract.
- `games/sample_2d_desktop_runtime_package/main.cpp`: add committed package smoke counters if the sample is the
  narrow proof lane.
- `tools/new-game.ps1`: carry the same generated `DesktopRuntime2DPackage` scaffold behavior once the committed sample
  proof is stable.
- `tools/validate-installed-desktop-runtime.ps1`, `tools/check-json-contracts.ps1`, and
  `tools/check-ai-integration.ps1`: require package-visible sprite animation counters without broadening unsupported
  claims.
- Docs, manifest, skills, subagents, and plan registry: keep active/completed pointers truthful.

## Done When

- A RED test or static/package check fails first because sprite animation package counters or data rows are missing.
- The committed and/or generated 2D desktop runtime package can prove deterministic sprite animation frame sampling and
  application through first-party package-visible counters.
- Existing sprite batch planning telemetry and native batch execution counters remain unchanged.
- Unsupported claims remain explicit for runtime image decoding, atlas packing, tilemap editor UX, public native/RHI
  handles, Metal, package streaming execution, broad production sprite batching readiness, and broad renderer quality.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused package/runtime tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public API changes, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing animation/runtime package payload patterns and choose the smallest first-party 2D sprite animation contract.
- [x] Add RED checks for package-visible sprite animation data/counters.
- [x] Implement committed sample and generated `DesktopRuntime2DPackage` sprite animation package smoke.
- [x] Update docs, manifest, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_runtime_tests"` failed before implementation
  because `mirakana::AssetKind::sprite_animation` and `mirakana::runtime::runtime_sprite_animation_payload` were missing.
- RED: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_scene_renderer_tests"` failed before implementation
  because `mirakana::runtime::RuntimeSpriteAnimationPayload` and
  `mirakana::sample_and_apply_runtime_scene_render_sprite_animation` were missing.
- GREEN: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_runtime_tests"` passed.
- GREEN: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_scene_renderer_tests"` passed after the
  parallel MSBuild PDB contention was rerun sequentially.
- GREEN: `ctest --preset dev -R "mirakana_runtime_tests|mirakana_scene_renderer_tests" --output-on-failure` passed.
- GREEN: `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target sample_2d_desktop_runtime_package"`
  passed.
- GREEN: `ctest --preset desktop-runtime -R "sample_2d_desktop_runtime_package_(smoke|shader_artifacts_smoke)" --output-on-failure`
  passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` passed and the
  installed smoke reported `sprite_animation_frames_sampled=3`, `sprite_animation_frames_applied=3`,
  `sprite_animation_selected_frame_sum=1`, `sprite_animation_diagnostics=0`, and `package_records=6`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after static scaffold and manifest contract updates.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after rerunning outside the sandbox because the first sandboxed attempt
  failed with `Operation not permitted`.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied clang-format, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- GREEN: `git diff --check` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Metal shader tools and Apple packaging remained diagnostic-only host gates.

## Closure Notes

The completed proof adds first-party cooked `sprite_animation` package rows, runtime payload parsing, runtime diagnostics
for sprite/material dependency edges, `mirakana_scene_renderer` deterministic frame sampling/application, committed
`sample_2d_desktop_runtime_package` counters, generated `DesktopRuntime2DPackage` scaffold parity, installed package
validation, and static contract checks. It intentionally does not add runtime source image decoding, atlas packing,
tilemap editor UX, package streaming execution, public native/RHI handles, Metal readiness, broad production sprite
batching readiness, or broad renderer quality claims.
