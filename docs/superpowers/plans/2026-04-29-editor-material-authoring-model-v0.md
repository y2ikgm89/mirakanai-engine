# Editor Material Authoring Model v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a GUI-independent editor material authoring model that can inspect, edit, validate, serialize, and stage first-party material definitions without depending on Dear ImGui, SDL3, RHI backends, or runtime game code.

**Architecture:** Keep authored material state in `mirakana_editor_core` and reuse `mirakana_assets` material definitions plus shader/material binding metadata. The Dear ImGui editor shell should remain an adapter that consumes tested editor-core panel/action models; it must not become the source of truth for material data, undo state, import settings, or generated-game material contracts.

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_editor_core`, optional `mirakana_editor` adapter, deterministic editor-core tests, JSON/text store helpers already present in the repository.

---

## Context

- Runtime material upload, base-color texture sampling, material factor payloads, and Lit Material v0 package smokes now prove the runtime/rendering side.
- Editor-core already has selected cooked material preview metadata and shader artifact readiness models, but not a dedicated authored material editing model with validation, dirty state, staged changes, and save/load workflow.
- This slice should improve the production editor authoring loop without introducing new UI middleware or exposing RHI/native handles.

## Constraints

- Do not add new third-party dependencies.
- Do not make `mirakana_assets`, `mirakana_runtime`, `mirakana_renderer`, or `mirakana_rhi` depend on editor modules.
- Do not expose Dear ImGui, SDL3, D3D12, Vulkan, Metal, or native handles through game public APIs.
- Do not implement a full material graph, shader graph, node editor, texture browser, or live GPU preview in this slice.
- Keep material authoring data first-party, deterministic, and testable headlessly.
- Editor/GUI changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`; public API changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- [x] `mirakana_editor_core` exposes a material authoring document/model that can be built from a first-party material definition, reports editable factor/texture rows, and tracks dirty/staged state.
- [x] Material edits validate factor ranges, texture asset ids, and sampler/binding expectations without invoking renderer/RHI backends.
- [x] Material authoring save/load uses existing project/text-store patterns and preserves deterministic first-party material serialization.
- [x] Undo/redo or editor command integration covers material factor and texture-slot edits at the editor-core level.
- [x] Dear ImGui shell wiring is intentionally deferred in v0; the tested editor-core model/actions are ready for a future adapter and no material business logic was added to the shell.
- [x] Docs, roadmap, gap analysis, manifest, skills, and Codex/Claude guidance describe this as editor material authoring model v0, not a full shader graph or runtime material system.
- [x] Focused editor-core tests, GUI build validation when touched, schema/agent/format checks, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or exact host blockers are recorded.

---

### Task 1: RED Editor-Core Material Authoring Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`
- Inspect: `editor/core/include/mirakana/editor/asset_pipeline.hpp`
- Inspect: `editor/core/include/mirakana/editor/project.hpp`

- [x] Add tests that create a material authoring model from a `mirakana::MaterialDefinition` with base color, emissive, metallic, roughness, and texture slots.
- [x] Assert deterministic editable rows, stable ids, clean initial state, dirty state after edits, and validation diagnostics for invalid factors or texture ids.
- [x] Add tests for save/load or project text-store integration only after identifying the existing editor-core persistence helper.

### Task 2: Material Authoring Model Implementation

**Files:**
- Add or modify under `editor/core/include/mirakana/editor/`
- Add or modify under `editor/core/src/`
- Modify: `editor/core/CMakeLists.txt` only if a new translation unit is added.

- [x] Add first-party value types for material authoring rows, validation diagnostics, edit requests, and authoring state.
- [x] Reuse `mirakana_assets` material definitions and binding metadata instead of inventing a parallel material schema.
- [x] Keep all APIs GUI-independent and backend-handle-free.

### Task 3: Editor Commands, Dirty State, And Undo/Redo

**Files:**
- Modify: editor-core command/project/undo files discovered during Task 1.
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] Add editor-core actions for factor edits and texture-slot edits.
- [x] Ensure undo/redo restores prior material authoring state deterministically.
- [x] Keep unrelated scene or import undo behavior unchanged.

### Task 4: Optional Dear ImGui Adapter

**Files:**
- Modify: `editor/src/` files that render the current material preview/asset inspector.
- Modify: GUI validation scripts only if they already enumerate panel smoke paths.

- [x] Defer selected-material edit UI to a future adapter slice; existing selected cooked material preview remains read-only.
- [x] Keep Dear ImGui-specific layout and controls in `mirakana_editor`; keep validation and serialization in `mirakana_editor_core`.
- [x] Do not add a node editor or shader graph UI in this slice.

### Task 5: Documentation And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md` if guidance changes.
- Modify: `.claude/skills/*` and `.codex/agents/*` / `.claude/agents/*` only where guidance would otherwise drift.

- [x] Mark editor material authoring model v0 honestly as implemented only after tests and validation pass.
- [x] Keep full material graphs, shader graph authoring, live GPU previews, prefab editing, and play-in-editor isolation as follow-up work.

### Task 6: Verification

- [x] Run focused editor-core tests.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` if `editor` or GUI adapter files changed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` if public headers changed.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- Focused editor-core validation: `cmake --build --preset dev --target mirakana_editor_core_tests` passed and `ctest --preset dev --output-on-failure -R "mirakana_editor_core_tests"` passed with 1/1 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: initially failed on clang-format differences in `material_authoring.cpp` and `editor_core_tests.cpp`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` applied formatting and the retry passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: sandboxed run hit the known vcpkg 7zip `CreateFileW stdin failed with 5` host/sandbox blocker; the same command rerun with elevated permissions passed, including desktop-gui build and 27/27 tests.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: passed. Metal shader/library packaging remains diagnostic-only because `metal` and `metallib` are missing on this Windows host. Apple packaging remains host-gated on macOS/Xcode. Android release signing is not configured. Android device smoke is not connected. Strict clang-tidy remains diagnostic-only when the `dev` Visual Studio generator compile database is unavailable before configure.
