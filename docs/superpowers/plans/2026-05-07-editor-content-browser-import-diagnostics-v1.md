# Editor Content Browser Import Diagnostics v1 Implementation Plan (2026-05-07)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent editor content/import diagnostics panel model that unifies Content Browser rows, import progress, diagnostics, dependencies, thumbnail requests, and material preview rows for the visible Assets panel and AI/editor tooling.

**Architecture:** Keep the existing asset import execution and Content Browser state as the sources of truth, then add a read-only `mirakana_editor_core` model over those states. The optional `mirakana_editor` Dear ImGui shell renders the same model it already derives from `ContentBrowserState`, `AssetPipelineState`, and `AssetImportPlan`, while retained `mirakana_ui` output gives AI/editor integrations a stable panel contract without executing imports or shell commands.

**Tech Stack:** C++23, `mirakana_editor_core`, `ContentBrowserState`, `AssetPipelineState`, `mirakana_ui` retained model output, `mirakana_editor` Dear ImGui adapter, `mirakana_editor_core_tests`, `desktop-gui` validation lane.

---

## Context

- `editor-productization` still lists content browser/import diagnostics as follow-up work before broad editor readiness can be claimed.
- `mirakana_editor_core` already owns `ContentBrowserState`, `AssetPipelineState`, import progress/dependency/thumbnail/material preview helpers, and imported asset record registration helpers.
- `mirakana_editor` already renders a visible Assets panel directly from those states, but there is no single deterministic panel model that captures the browser rows, selected row, import queue, diagnostics, dependencies, thumbnail requests, material previews, and retained `mirakana_ui` output as one editor/AI-facing contract.

## Constraints

- Add or update tests before production behavior.
- Keep `editor/core` GUI-independent and process-execution-free.
- The new panel model is read-only. It must not execute asset imports, recooks, hot reload, validation recipes, package scripts, shell commands, shader compilers, or manifest mutations.
- Do not add SDL3, Dear ImGui, renderer, RHI, native handles, filesystem writes, dynamic module loading, or package streaming to editor-core contracts.
- Preserve existing Content Browser filtering/selection semantics and existing `AssetPipelineState` import execution result mapping.
- Keep material GPU preview upload/display ownership in `mirakana_editor`; the editor-core model may expose only existing material preview planning rows.

## Done When

- `mirakana_editor_core` exposes `EditorContentBrowserImportPanelModel` plus status/asset/import row types and `make_editor_content_browser_import_panel_model`.
- The model reports deterministic status, asset rows, selected asset details, import queue rows, import progress, diagnostics, dependencies, thumbnail requests, material preview rows, and hot-reload summary rows from existing editor-core state.
- `make_content_browser_import_panel_ui_model` emits retained first-party `mirakana_ui` rows for the panel without execution controls.
- The visible `mirakana_editor` Assets panel renders from the new panel model while keeping import/recook execution in existing explicit GUI button handlers.
- Docs, registry, master plan, manifest, static checks, and Codex/Claude editor guidance describe Content Browser Import Diagnostics v1 without claiming source import automation beyond existing reviewed buttons, package streaming, renderer/RHI/native handles, or broad editor readiness.
- Relevant validation passes, including RED evidence, focused `mirakana_editor_core_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1`.
- The slice closes with a validated commit checkpoint after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 && pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passes, staging only files owned by this slice.

## Commit Policy

- Use one slice-closing commit after code, docs, manifest, static checks, GUI build, and validation evidence are complete.
- Do not commit RED-test or otherwise known-broken intermediate states.
- Keep unrelated pre-existing guidance changes out of the commit.

## Tasks

### Task 1: RED Tests For Content Browser Import Panel

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add model tests proving `make_editor_content_browser_import_panel_model` summarizes filtered asset rows, selected asset details, import progress, failed import diagnostics, dependency rows, thumbnail requests, material preview rows, and hot-reload summaries deterministically.
- [x] Add retained UI tests proving `make_content_browser_import_panel_ui_model` exposes browser, selection, import queue, diagnostics, dependency, thumbnail, material preview, and hot-reload sections.
- [x] Run focused build/test and confirm failure because `mirakana/editor/content_browser_import_panel.hpp`, `EditorContentBrowserImportPanelModel`, and `make_editor_content_browser_import_panel_model` do not exist yet.

### Task 2: Editor-Core Content Browser Import Panel Model

**Files:**
- Create: `editor/core/include/mirakana/editor/content_browser_import_panel.hpp`
- Create: `editor/core/src/content_browser_import_panel.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] Add row/model types for panel status, visible assets, selected asset details, import queue rows, hot-reload summary rows, and retained diagnostics.
- [x] Implement `make_editor_content_browser_import_panel_model` over `ContentBrowserState`, `AssetPipelineState`, `AssetImportPlan`, and optional preview material definitions.
- [x] Reuse existing import progress, dependency, thumbnail, import diagnostics, and material preview helpers instead of duplicating import logic.
- [x] Implement `make_content_browser_import_panel_ui_model` with first-party `mirakana_ui` rows and no GUI/process/filesystem/native dependencies.

### Task 3: Visible Editor Assets Panel Wiring

**Files:**
- Modify: `editor/src/main.cpp`

- [x] Include the new editor-core panel model header.
- [x] Build the panel model inside `draw_assets_panel` from current `content_browser_`, `asset_pipeline_`, and `asset_import_plan_`.
- [x] Render asset rows, selection details, import queue, progress, diagnostics, dependencies, thumbnails, material previews, and hot-reload summaries from the panel model.
- [x] Keep `Import Assets`, `Simulate Hot Reload`, material GPU preview rendering, and recook execution as existing explicit GUI-owned actions outside the retained model.

### Task 4: Docs, Manifest, Static Checks

**Files:**
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

- [x] Describe Editor Content Browser Import Diagnostics v1 as a visible/read-only panel model over existing Content Browser and asset pipeline state.
- [x] Keep non-ready claims explicit: no automatic import execution from editor-core, arbitrary shell, package scripts, validation recipe execution, free-form manifest edits, package streaming, renderer/RHI/native handles, dynamic game-module Play-In-Editor, or broad editor readiness.
- [x] Add static checks for `EditorContentBrowserImportPanelModel`, retained `make_content_browser_import_panel_ui_model`, visible `mirakana_editor` wiring, docs, manifest, and AI guidance markers.

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
| RED focused build/test | Failed as expected | `cmake --build --preset dev --target mirakana_editor_core_tests` failed on missing `mirakana/editor/content_browser_import_panel.hpp` before production code existed. |
| Focused `mirakana_editor_core_tests` | Passed | `cmake --build --preset dev --target mirakana_editor_core_tests` rebuilt `mirakana_editor_core_tests`; `ctest --preset dev --output-on-failure -R mirakana_editor_core_tests` reported 1/1 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed | `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | `public-api-boundary-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Passed | `production-readiness-audit-check: ok`; current audit still reports 11 unsupported gaps. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` | Passed | GUI configure/build completed and CTest reported 46/46 tests passed. |
| `git diff --check` | Passed | Exit 0; Git emitted CRLF normalization warnings only. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | `validate: ok`; dev CTest reported 29/29 tests passed. Metal/Apple diagnostics remained host-gated on this Windows host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Build completed for the dev preset. |
| Slice-closing commit | Pending |  |
