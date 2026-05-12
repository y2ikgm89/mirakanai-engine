# Editor Material GPU Preview Execution Evidence v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add retained editor-core evidence rows for host-owned selected-material GPU preview execution without moving RHI, SDL3, Dear ImGui, shader compiler, or native handle work into `editor/core`.

**Architecture:** Keep `mirakana_editor` as the only place that creates RHI resources, uploads material textures, renders the 128x128 preview surface, and displays the SDL/Dear ImGui texture. Add a plain `EditorMaterialGpuPreviewExecutionSnapshot` value that the host can feed into `EditorMaterialAssetPreviewPanelModel`, then expose deterministic retained `material_asset_preview.gpu.execution` rows for AI/editor diagnostics. This promotes the existing D3D12 material-preview execution evidence while keeping Vulkan display parity, Metal readiness, shader compiler execution, package streaming, native handles, and broad renderer/editor readiness out of scope.

**Tech Stack:** C++23, `mirakana_editor_core` material asset preview panel model, retained `mirakana_ui`, optional `mirakana_editor` SDL3/Dear ImGui adapter, `mirakana_editor_core_tests`, existing docs/manifest/static validation.

---

## Goal

Implement `editor-material-gpu-preview-execution-evidence-v1` as a narrow editor-productization slice: selected cooked material preview execution remains host-owned in `mirakana_editor`, but the editor-core retained model can report whether the optional host preview cache is unavailable, ready, rendered, or failed, including backend/display-path/frame-count diagnostics.

## Context

- `Editor Material Asset Preview Diagnostics v1` already reports selected material metadata, material factors, texture dependency rows, typed GPU payload rows, and D3D12/Vulkan material-preview shader readiness.
- `mirakana_editor` already has `MaterialPreviewGpuCache`, uploads texture payloads through `mirakana_runtime_rhi`, renders a small `RhiViewportSurface`, and displays an SDL/Dear ImGui image when the native RHI preview path is ready.
- The retained `make_material_asset_preview_panel_ui_model` does not currently expose host-owned GPU execution/cache evidence, so AI/editor diagnostics still describe only read-only readiness.
- `editor/core` must stay GUI-independent and must not execute RHI work or expose native handles.

## Constraints

- Do not move RHI resource creation, texture upload, pipeline creation, render submission, SDL texture work, shader compiler execution, process execution, or native handle access into `editor/core`.
- Keep the execution snapshot as caller-supplied plain data; invalid or absent snapshots must produce deterministic non-ready diagnostics instead of guessing.
- Keep D3D12 preview execution as the only promoted visible execution evidence. Vulkan display parity and Metal readiness remain follow-up work.
- Do not claim package streaming, broad material/shader graph production, live shader generation, renderer quality, or full editor productization.
- Update Codex and Claude editor guidance equivalently if behavior guidance changes.

## Done When

- `EditorMaterialAssetPreviewPanelModel` owns a caller-supplied GPU preview execution snapshot/status section without setting `executes=true` for editor-core behavior.
- `make_material_asset_preview_panel_ui_model` emits stable retained ids under `material_asset_preview.gpu.execution`.
- `mirakana_editor` maps its existing `MaterialPreviewGpuCache` into the snapshot after attempting preview rendering.
- Focused tests prove default host-required diagnostics and ready/rendered snapshot rows.
- Docs, master plan, plan registry, skills, manifest, and static checks distinguish host-owned D3D12 preview execution evidence from Vulkan/Metal parity or editor-core RHI execution.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Tasks

### Task 1: RED Tests

- [x] Add `editor material asset preview panel reports host gpu preview execution snapshot` to `tests/unit/editor_core_tests.cpp`.
- [x] Build a ready selected material preview model and assert the default execution status is host-required/non-rendered with retained `material_asset_preview.gpu.execution.status` rows.
- [x] Apply a ready snapshot with backend `D3D12`, display path `cpu-readback`, one rendered frame, and an empty diagnostic; assert the model and retained UI rows report ready/rendered evidence.
- [x] Add a failure snapshot case with diagnostic text so `attention` evidence remains deterministic.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests`.
- [x] Run `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` and record the expected RED failure before implementation.

### Task 2: Editor-Core Execution Evidence Model

- [x] Add `EditorMaterialGpuPreviewExecutionSnapshot` to `editor/core/include/mirakana/editor/material_asset_preview_panel.hpp`.
- [x] Add execution fields to `EditorMaterialAssetPreviewPanelModel`: status label, diagnostic, backend, display path, frame count, `host_owned`, `rendered`, and `ready`.
- [x] Add `apply_editor_material_gpu_preview_execution_snapshot(EditorMaterialAssetPreviewPanelModel&, const EditorMaterialGpuPreviewExecutionSnapshot&)`.
- [x] Keep the default model deterministic: no snapshot means host-owned preview execution is required/unavailable, not editor-core execution.
- [x] Add `material_asset_preview.gpu.execution` retained rows to `make_material_asset_preview_panel_ui_model`.
- [x] Run focused `mirakana_editor_core_tests` until green.

### Task 3: Visible Editor Adapter

- [x] Add a local helper in `editor/src/main.cpp` that maps `MaterialPreviewGpuCache` to `EditorMaterialGpuPreviewExecutionSnapshot` after `render_material_preview_gpu_cache`.
- [x] Keep existing ImGui image/status rendering, but ensure the model passed through the panel draw path can be augmented with the snapshot for retained diagnostics.
- [x] Do not add Vulkan display parity, Metal preview, shader compiler execution, package scripts, package streaming, or native handle exposure.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Skills, Static Checks

- [x] Update `docs/editor.md`, `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/testing.md`, `docs/architecture.md`, and `docs/roadmap.md`.
- [x] Update the master plan and plan registry.
- [x] Update `engine/agent/manifest.json`.
- [x] Update `.agents/skills/editor-change/SKILL.md` and `.claude/skills/gameengine-editor/SKILL.md`.
- [x] Update `tools/check-ai-integration.ps1` to require the new model/test/docs needles.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [x] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Pass/expected fail | `cmake --build --preset dev --target mirakana_editor_core_tests` failed before implementation because `EditorMaterialGpuPreviewExecutionSnapshot`, execution fields, and `apply_editor_material_gpu_preview_execution_snapshot` did not exist. |
| Focused `mirakana_editor_core_tests` | Pass | `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | Formatting check passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Pass | Public API boundary check accepted the material preview panel header changes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | Static AI integration checks cover the new snapshot API, retained ids, docs, manifest, and skills. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | Audit kept `unsupported_gaps=11` and `editor-productization` as `partly-ready`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Pass | Desktop GUI preset build succeeded and ran 46/46 tests. |
| `git diff --check` | Pass | No whitespace errors; Git reported line-ending warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Pass | Full repository validation passed with expected host-gated diagnostic lanes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Pass | Dev preset build completed successfully after validation. |
| Slice-closing commit | Pass | Commit created after final validation/build, staging only material GPU preview execution evidence slice files. |
