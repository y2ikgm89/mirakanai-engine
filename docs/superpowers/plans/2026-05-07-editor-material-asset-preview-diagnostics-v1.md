# Editor Material Asset Preview Diagnostics v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent read-only material asset preview diagnostics model that visible `mirakana_editor` and AI/editor tooling can share.

**Architecture:** Keep material payload parsing and shader artifact readiness in `mirakana_editor_core`, represented as a deterministic retained `mirakana_ui` model. Keep actual RHI uploads, preview rendering, SDL/Dear ImGui display textures, shader compilation execution, and backend native handles in the visible editor shell.

**Tech Stack:** C++23, `mirakana_editor_core`, `mirakana_ui`, existing `AssetRegistry`/`IFileSystem`, `EditorMaterialPreview`, `EditorMaterialGpuPreviewPlan`, and `ViewportShaderArtifactState`.

---

## Goal

Expose selected material preview readiness as a first-party editor-core contract over:

- selected cooked material artifact status and factor rows;
- texture dependency rows and runtime texture payload readiness;
- GPU preview payload plan status without doing GPU work;
- D3D12/Vulkan material-preview shader artifact readiness for factor-only and textured variants;
- retained `mirakana_ui` rows suitable for AI/editor tooling and visible editor adapters.

## Context

- `make_editor_material_preview`, `make_editor_selected_material_preview`, and `make_editor_material_gpu_preview_plan` already parse material data and texture payloads without GUI/RHI dependencies.
- `ViewportShaderArtifactState` already reports material-preview shader artifact readiness by shader id.
- `mirakana_editor` already owns RHI upload/display for selected material GPU previews in the Assets panel.
- The preceding Content Browser Import Diagnostics slice moved the broader Assets panel into a read-only retained model, but material preview specifics are still rendered directly from lower-level rows in `mirakana_editor`.

## Constraints

- `editor/core` must not depend on Dear ImGui, SDL3, renderer/RHI backends, native handles, shader compiler execution, or package scripts.
- Do not execute imports, recooks, hot reload, validation recipes, arbitrary shell, shader compilation, package streaming, or free-form manifest edits.
- Do not claim Vulkan editor material preview display parity, Metal readiness, broad editor readiness, or general renderer quality.
- Preserve existing explicit GUI-owned `Import Assets`, `Simulate Hot Reload`, material GPU preview render, and shader compile actions.

## Done When

- `mirakana_editor_core` exposes `EditorMaterialAssetPreviewPanelModel` plus `make_editor_material_asset_preview_panel_model` and `make_material_asset_preview_panel_ui_model`.
- Unit tests prove ready and failure/retained-UI paths, including texture payload rows and material-preview shader readiness.
- The visible `mirakana_editor` Assets panel renders selected material preview diagnostics from the new model while keeping RHI rendering in GUI code.
- Docs, manifest, skills, and `tools/check-ai-integration.ps1` describe and enforce the new contract.
- Focused checks, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` pass before the slice commit.

## Files

- Create: `editor/core/include/mirakana/editor/material_asset_preview_panel.hpp`
- Create: `editor/core/src/material_asset_preview_panel.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tests/unit/editor_core_tests.cpp`
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration.ps1`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`

---

### Task 1: RED Tests For Material Asset Preview Diagnostics

- [x] Add tests to `tests/unit/editor_core_tests.cpp` for a selected cooked material with a resolved texture payload, shader artifact readiness rows, a ready GPU payload plan, and `mutates == false` / `executes == false`.
- [x] Add a retained `mirakana_ui` test that expects stable ids under `material_asset_preview`, including `material_asset_preview.material.path`, `material_asset_preview.factors.base_color`, `material_asset_preview.textures.base_color.status`, `material_asset_preview.gpu.status`, and `material_asset_preview.shaders.textured.d3d12.status`.
- [x] Run `cmake --build --preset dev --target mirakana_editor_core_tests` and confirm the missing header/symbol failure before production code exists.

### Task 2: Editor-Core Model

- [x] Add `material_asset_preview_panel.hpp/cpp` with row types for selected material metadata, factor values, texture dependency status, GPU payload texture rows, shader artifact readiness rows, and model status.
- [x] Implement `make_editor_material_asset_preview_panel_model(IFileSystem&, const AssetRegistry&, AssetId, const ViewportShaderArtifactState&)` by reusing existing material preview and GPU preview planning functions.
- [x] Implement `make_material_asset_preview_panel_ui_model` with deterministic retained ids and no execution hooks.
- [x] Register the new source in `editor/CMakeLists.txt`.
- [x] Run focused `mirakana_editor_core_tests`.

### Task 3: Visible Editor Wiring

- [x] Include the new header in `editor/src/main.cpp`.
- [x] Build the model for the selected material asset in `draw_assets_panel`.
- [x] Render selected material factor, texture, GPU payload plan, and shader readiness rows from the model.
- [x] Keep `ensure_material_preview_gpu_cache`, RHI uploads, render pass, display texture, and cache invalidation as explicit GUI-owned code outside editor core.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.

### Task 4: Docs, Manifest, Static Checks

- [x] Document Editor Material Asset Preview Diagnostics v1 as a read-only material preview diagnostics contract.
- [x] Keep non-ready claims explicit: no shader execution, no RHI upload/display in editor core, no package scripts, no arbitrary shell, no package streaming, no native handles, no Vulkan/Metal parity claim, and no broad renderer/editor readiness.
- [x] Add static checks for the new header/source/tests, visible editor wiring, docs, manifest, and Codex/Claude skill guidance.

### Task 5: Validation And Commit

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run focused build/test for `mirakana_editor_core_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`.
- [x] Run `git diff --check`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- [ ] Create a slice-closing commit after validation/build passes, staging only relevant files.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| RED focused build/test | Failed as expected | `cmake --build --preset dev --target mirakana_editor_core_tests` failed on missing `mirakana/editor/material_asset_preview_panel.hpp` before production code existed. |
| Focused `mirakana_editor_core_tests` | Passed | `cmake --build --preset dev --target mirakana_editor_core_tests` and `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` passed after the model, visible wiring, and static-check updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | Repository formatter check passed after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public boundary check passed for the new editor-core header and unchanged backend/native-handle boundaries. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `tools/check-ai-integration.ps1` validates the new material asset preview diagnostics contract across header/source/tests, visible editor wiring, docs, manifest, and Codex/Claude skill guidance. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | Current audit still reports 11 known unsupported gaps and keeps editor productization partly ready with material preview execution/parity unsupported beyond read-only diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | `desktop-gui` configure/build succeeded and 46/46 GUI-lane CTest tests passed. |
| `git diff --check` | Passed | Exit 0; Git reported CRLF conversion warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Default validation passed; 29/29 dev CTest tests passed, Metal/Apple checks remained diagnostic host gates on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Default dev build completed successfully after validation. |
| Slice-closing commit | Pending |  |
