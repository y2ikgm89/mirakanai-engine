# 2D Tilemap Editor Runtime UX v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `2d-tilemap-editor-runtime-ux-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Promote the existing deterministic `GameEngine.Tilemap.v1` package metadata into a narrow generated 2D runtime/editor UX
proof: a committed/generated `DesktopRuntime2DPackage` should expose package-visible tilemap runtime/editor counters or
diagnostics through first-party tilemap runtime sampling or application counters, and editor/tooling surfaces should
report tilemap package diagnostics from the same package rows, without
claiming source image decoding, production atlas packing, a full tilemap editor, package streaming execution, public
native/RHI handles, Metal readiness, or broad renderer quality.

## Architecture

Keep the tilemap contract data-first and host-feasible. Reuse `TilemapMetadataDocument`, existing tilemap package
payload inspection, cooked texture/material records, `SceneRenderPacket` sprite submission, package validation, and
editor-core diagnostic models before adding any new abstraction. If runtime sampling is needed, represent it as
deterministic first-party rows/counters over package data rather than a renderer-native atlas API. If editor UX is
needed, keep it in editor-core/tool diagnostics and visible panels backed by existing first-party models; do not make
runtime games depend on Dear ImGui or editor-private APIs.

## Context

- `2d-atlas-tilemap-package-authoring-v1` completed deterministic tilemap metadata rows, package entries, generated
  manifest rows, and runtime package inspection, but it remained data-only.
- `2d-native-sprite-batching-execution-v1` completed renderer-owned native sprite batch execution counters.
- `2d-sprite-animation-package-v1` completed first-party cooked sprite animation package sampling/application counters.
- The master plan still lists generated 2D production gaps around tilemap/atlas runtime/editor integration.

## Constraints

- Do not introduce third-party dependencies.
- Do not parse source PNG/JPEG or runtime source images.
- Do not claim production atlas packing, a full tilemap editing canvas, package streaming execution, Metal readiness,
  broad production sprite batching readiness, or broad renderer quality.
- Do not expose `IRhiDevice`, native handles, RHI handles, backend objects, atlas internals, or editor-private APIs to
  gameplay.
- Preserve sprite ordering and the existing native sprite batching execution contract.

## File Map

- `engine/assets/`, `engine/runtime/`, and `engine/scene_renderer/`: inspect tilemap metadata/package payload and render
  packet integration points before adding runtime sampling/application helpers.
- `engine/editor_core/`, `editor/`, and `engine/tools/`: inspect existing content browser, import diagnostics, and
  tilemap package authoring diagnostics before adding editor/tool UX rows.
- `games/sample_2d_desktop_runtime_package/` and `tools/new-game.ps1`: carry the committed proof and generated
  `DesktopRuntime2DPackage` scaffold together once the runtime/editor contract is stable.
- `tools/validate-installed-desktop-runtime.ps1`, `tools/check-json-contracts.ps1`, and
  `tools/check-ai-integration.ps1`: keep package-visible tilemap counters/diagnostics enforced without broadening
  unsupported claims.
- Docs, manifest, skills, subagents, and plan registry: keep active/completed pointers truthful.

## Done When

- A RED test or static/package check fails first because tilemap runtime/editor package counters or diagnostics are
  missing.
- The committed and/or generated 2D desktop runtime package proves deterministic tilemap package consumption through
  first-party package-visible counters or diagnostics.
- Editor/tooling surfaces can report the selected tilemap package diagnostics through first-party models without
  executing package scripts from editor core.
- Existing sprite animation counters, sprite batch planning telemetry, and native sprite batch execution counters remain
  unchanged.
- Unsupported claims remain explicit for source image decoding, production atlas packing, full tilemap editor UX,
  package streaming execution, public native/RHI handles, Metal, broad production sprite batching readiness, and broad
  renderer quality.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, focused runtime/editor/package tests, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public API changes, and
  `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect existing tilemap package payload, scene renderer, editor-core diagnostics, and generated package scaffold patterns.
- [x] Add RED checks for tilemap runtime/editor package counters or diagnostics.
- [x] Implement the narrow committed sample and generated `DesktopRuntime2DPackage` tilemap runtime/editor UX proof.
- [x] Update docs, manifest, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_runtime_tests"` failed before implementation because
  `sample_runtime_tilemap_visible_cells` was missing from `mirakana::runtime`.
- RED: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_editor_core_tests"` failed before implementation because
  `make_editor_tilemap_package_diagnostics_model` was missing from `mirakana::editor`.
- GREEN: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_runtime_tests"` passed after adding
  `RuntimeTilemapVisibleCellSampleResult` and `sample_runtime_tilemap_visible_cells`.
- GREEN: `cmd /c "set PATH=& cmake --build --preset dev --target mirakana_editor_core_tests"` passed after adding
  `EditorTilemapPackageDiagnosticsModel` and `make_editor_tilemap_package_diagnostics_model`.
- GREEN: `ctest --preset dev -R "mirakana_runtime_tests|mirakana_editor_core_tests" --output-on-failure` passed.
- GREEN: `cmd /c "set PATH=& cmake --build --preset desktop-runtime --target sample_2d_desktop_runtime_package"` passed.
- GREEN: `ctest --preset desktop-runtime -R "sample_2d_desktop_runtime_package_smoke" --output-on-failure` passed.
- GREEN: source-tree `sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-sprite-animation --require-tilemap-runtime-ux` passed and reported `tilemap_layers=1`, `tilemap_visible_layers=1`, `tilemap_tiles=2`, `tilemap_non_empty_cells=3`, `tilemap_cells_sampled=3`, and `tilemap_diagnostics=0`.
- GREEN: `ctest --preset desktop-runtime -R "sample_2d_desktop_runtime_package_(smoke|shader_artifacts_smoke)" --output-on-failure` passed.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` passed and installed validation accepted the same tilemap runtime UX counters.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed after static AI integration checks were updated.
