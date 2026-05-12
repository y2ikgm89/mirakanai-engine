# Editor Prefab Variant GUI Wiring v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Wire the existing prefab variant authoring model into the visible desktop editor so authors can create, inspect, save/load, validate, undo, and instantiate prefab variants through the Dear ImGui shell without moving durable behavior out of `mirakana_editor_core`.

**Architecture:** `mirakana_scene` remains the owner of dependency-free `PrefabVariantDefinition` validation, text IO, and composition. `mirakana_editor_core` remains the owner of `PrefabVariantAuthoringDocument`, rows, diagnostics, text-store helpers, and undo actions. `mirakana_editor` adds only adapter state, menus/panels, and ImGui controls that call those existing contracts. Gameplay continues to consume composed prefabs or cooked packages, not editor APIs.

**Tech Stack:** C++23, existing `mirakana_scene`, `mirakana_editor_core`, optional SDL3 + Dear ImGui `mirakana_editor`, CMake, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, static agent checks, and default validation. No new third-party dependencies.

---

## Context

Prefab Variant / Override Authoring v0 added the durable model, deterministic `GameEngine.PrefabVariant.v1` text format, registry-backed diagnostics, and `UndoStack` actions. The production editor gap is now the visible authoring loop: the desktop editor can already adapt `SceneAuthoringDocument`, but it does not yet expose `PrefabVariantAuthoringDocument` as a real panel/workflow.

## Constraints

- Keep `mirakana_scene` and `mirakana_editor_core` as the durable model owners.
- Keep Dear ImGui, SDL3, native windows, renderer/RHI handles, and shell-only state out of game public APIs.
- Do not add JSON manifest mutation or package registration apply buttons in this slice.
- Do not implement nested prefab propagation, instance link tracking, conflict/merge UX, play-in-editor isolation, or material/shader generation.
- Keep variant undo history scoped to the active variant document and clear it when replacing the document.

## Done When

- `mirakana_editor` exposes a visible Prefab Variant workflow backed by `PrefabVariantAuthoringDocument`.
- Authors can create a variant from a loaded/saved base prefab path, load/save `.prefabvariant` files, inspect override rows and diagnostics, edit name/transform/component overrides through undo actions, and instantiate the composed prefab into the active scene document.
- Static AI integration checks prove the editor shell is using `PrefabVariantAuthoringDocument` and not a parallel model.
- Docs, roadmap, gap analysis, manifest, Codex/Claude skills, and subagent guidance are synchronized honestly.
- Focused GUI validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host/toolchain gates recorded.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Create: this plan

- [x] **Step 1: Move Prefab Variant / Override Authoring v0 to completed and mark this plan as active.**
- [x] **Step 2: Update the gap analysis Immediate Next Slice to Editor Prefab Variant GUI Wiring v0.**
- [x] **Step 3: Keep non-goals explicit: no nested propagation, no conflict UX, no manifest mutation, no renderer/RHI coupling, and no play-in-editor isolation.**

### Task 2: Add RED Static/GUI Coverage

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Modify as needed: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Add static needles requiring `editor/src/main.cpp` to include and use `PrefabVariantAuthoringDocument`, variant save/load helpers, variant undo actions, and composed prefab instantiation.**
- [x] **Step 2: Add no editor-core tests because the existing `PrefabVariantAuthoringDocument` model already supplied the needed GUI operations.**
- [x] **Step 3: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm RED until the visible editor wiring exists.**

### Task 3: Wire The Visible Editor Adapter

**Files:**
- Modify: `editor/src/main.cpp`
- Modify as needed: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Modify as needed: `editor/core/src/prefab_variant_authoring.cpp`
- Modify as needed: `editor/CMakeLists.txt`

- [x] **Step 1: Add editor-shell state for active prefab variant document, variant path, base prefab path, variant undo stack, edit buffers, and diagnostics display without changing game public APIs.**
- [x] **Step 2: Add a Prefab Variant section reachable from the existing Assets panel.**
- [x] **Step 3: Wire New/Load/Save using `ITextStore`, `load_prefab_variant_authoring_document`, and `save_prefab_variant_authoring_document`.**
- [x] **Step 4: Render override rows, document dirty/valid state, registry-backed diagnostics, and composed-prefab review from the editor-core model.**
- [x] **Step 5: Route name, transform, and component override edits through `make_prefab_variant_*_override_action` and the variant-scoped `UndoStack`.**
- [x] **Step 6: Add an action to instantiate `document.composed_prefab()` into the active `SceneAuthoringDocument` through existing scene/prefab actions.**

### Task 4: Sync Docs, Manifest, Skills, And Subagents

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/editor.md`
- Modify: `docs/architecture.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`

- [x] **Step 1: Document that the visible editor adapts the prefab variant authoring model and keeps Dear ImGui as an adapter.**
- [x] **Step 2: Keep remaining follow-ups honest: nested propagation, conflict/merge UX, instance links, and play-in-editor isolation are still not implemented.**
- [x] **Step 3: Keep Codex and Claude guidance behaviorally equivalent.**

### Task 5: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

- [x] **Step 2: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer`; use `build-fixer` if validation exposes build or toolchain failures.**

## Validation Evidence

- PASS: Task 1 registration completed while finishing Prefab Variant / Override Authoring v0. `docs/superpowers/plans/README.md` marks this plan active; the gap analysis points to Editor Prefab Variant GUI Wiring v0; the previous slice has final `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` evidence.
- PASS: Visible editor adapter implemented in `editor/src/main.cpp`: the Assets panel now owns a `PrefabVariantAuthoringDocument`, variant-scoped `UndoStack`, path/name/edit buffers, New/Load/Save controls, override rows, registry-backed diagnostics, composed-prefab review, name/transform/component override actions, and guarded composed-prefab instantiation into the active `SceneAuthoringDocument`.
- PASS: Reviewer follow-up hardening added `reset_prefab_variant_document()` and resets variant document/history/path/edit buffers when creating or opening projects; composed-prefab instantiation is gated by `model(assets_).valid()` and logs unresolved diagnostics instead of instantiating invalid variants.
- PASS: Focused validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`. The first sandboxed `build-gui` hit the known vcpkg 7zip `CreateFileW stdin failed with 5` host issue; the approved rerun passed 32/32 GUI-lane tests.
- PASS: `cpp-reviewer` re-review found no blocking C++ behavior issue. Follow-up drift findings were fixed by strengthening `tools/check-ai-integration.ps1` with ordered reset/instantiate patterns and syncing plan registry/gap/guidance status.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed after implementation. Existing diagnostic-only host gates remain: Metal `metal`/`metallib` missing, Apple `xcodebuild`/`xcrun` unavailable on this Windows host, Android release signing not configured, Android device smoke not connected, and strict tidy blocked by missing `dev` compile database before configure.
