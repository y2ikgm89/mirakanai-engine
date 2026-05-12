# Prefab Variant / Override Authoring v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a dependency-free prefab variant and override foundation plus editor-core authoring helpers so derived prefabs can be reviewed, saved, validated, undone, and instantiated.

**Architecture:** `mirakana_scene` owns the runtime-safe value types, validation, deterministic serialization, and variant composition into a plain `PrefabDefinition`. `mirakana_editor_core` owns review rows, registry-backed diagnostics, `ITextStore` save/load helpers, and `UndoStack` actions; the visible editor remains a Dear ImGui adapter and is not required for this v0 slice. Gameplay consumes the composed prefab or cooked scene/package data, not editor APIs.

**Tech Stack:** C++23, existing `mirakana_scene`, `mirakana_editor_core`, first-party test harness, CMake, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`, and default validation. No new third-party dependencies.

---

## Context

`GameEngine.Prefab.v1` supports deterministic prefab serialization and instantiation, and `SceneAuthoringDocument` can save/load/instantiate prefabs. The missing production editor step is a stable override/variant model: authors need a deterministic way to express changed node names, transforms, and components against a base prefab without storing a full copied scene every time.

## Constraints

- Keep `mirakana_scene` independent from editor, renderer, RHI, platform, SDL3, Dear ImGui, JSON, and filesystem APIs.
- Keep `mirakana_editor_core` GUI-independent.
- Do not add third-party dependencies.
- Do not implement nested prefab propagation, instance links, conflict merge UI, or play-in-editor isolation in this slice.
- Keep generated gameplay on public `mirakana::` runtime/scene APIs; do not require editor headers at runtime.

## Done When

- `mirakana_scene` exposes prefab variant/override value types, validation diagnostics, deterministic composition into `PrefabDefinition`, and save/load text format helpers.
- `mirakana_editor_core` exposes a prefab variant authoring document with review rows, dirty tracking, registry-backed reference diagnostics, `ITextStore` save/load helpers, and undoable override edit actions.
- Focused unit tests prove variant composition, invalid override rejection, serialization round trip, editor save/load, reference diagnostics, and undo/redo behavior.
- Docs, roadmap, gap analysis, manifest, Codex/Claude skills, and subagent guidance are synchronized honestly.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host gates recorded.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Create: this plan

- [x] **Step 1: Move Desktop Runtime Package Registration Apply Tool v0 to completed and mark this plan as active.**
- [x] **Step 2: Update the gap analysis Immediate Next Slice to Prefab Variant / Override Authoring v0.**
- [x] **Step 3: Keep non-goals explicit: no nested prefab propagation, no editor shell command execution, no renderer/RHI coupling, and no play-in-editor isolation.**

### Task 2: Add RED Core Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] **Step 1: Include `mirakana/scene/prefab_overrides.hpp`.**
- [x] **Step 2: Add a failing test that composes a variant overriding a root name, child transform, and child mesh/material components.**
- [x] **Step 3: Add a failing test that rejects invalid base prefabs, invalid node indices, duplicate override keys, invalid names, and invalid components.**
- [x] **Step 4: Add a failing test that serializes and deserializes a prefab variant deterministically.**
- [x] **Step 5: Run focused core tests and confirm RED because the new header/API does not exist.**

### Task 3: Implement `mirakana_scene` Prefab Variants

**Files:**
- Create: `engine/scene/include/mirakana/scene/prefab_overrides.hpp`
- Create: `engine/scene/src/prefab_overrides.cpp`
- Modify: `engine/scene/CMakeLists.txt`
- Modify: `engine/agent/manifest.json`

- [x] **Step 1: Add `PrefabOverrideKind`, `PrefabVariantDiagnosticKind`, `PrefabNodeOverride`, `PrefabVariantDefinition`, and diagnostic/result structs.**
- [x] **Step 2: Implement validation with deterministic diagnostics for invalid base, invalid variant name, invalid node index, duplicate node/kind pairs, invalid names, and invalid components. Empty override lists are allowed so editors can create a clean variant draft before the first override.**
- [x] **Step 3: Implement `compose_prefab_variant` so overrides apply to a copy of the base prefab and return a status-bearing result.**
- [x] **Step 4: Implement deterministic `GameEngine.PrefabVariant.v1` serialization/deserialization using `base.<Prefab.v1 line>` prefixes and explicit override rows.**
- [x] **Step 5: Register the new source/header in CMake and manifest public headers.**
- [x] **Step 6: Run focused core tests and confirm GREEN.**

### Task 4: Add RED Editor-Core Tests

**Files:**
- Modify: `tests/unit/editor_core_tests.cpp`

- [x] **Step 1: Include the new editor prefab variant authoring header.**
- [x] **Step 2: Add a failing test for authoring rows, dirty tracking, save/load through `MemoryTextStore`, and composed prefab instantiation.**
- [x] **Step 3: Add a failing test for registry-backed mesh/material/sprite diagnostics on component override rows.**
- [x] **Step 4: Add a failing test for undoable name/transform/component override actions.**
- [x] **Step 5: Run focused editor-core tests and confirm RED because the editor-core API does not exist.**

### Task 5: Implement `mirakana_editor_core` Prefab Variant Authoring

**Files:**
- Create: `editor/core/include/mirakana/editor/prefab_variant_authoring.hpp`
- Create: `editor/core/src/prefab_variant_authoring.cpp`
- Modify: `editor/CMakeLists.txt`
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add `PrefabVariantAuthoringDocument` with base prefab, variant definition, composed prefab access, dirty tracking, rows, and `mark_saved`.**
- [x] **Step 2: Add row and diagnostic models with stable labels and registry-backed asset reference checks for component overrides.**
- [x] **Step 3: Add save/load helpers using `ITextStore` and the core variant text format.**
- [x] **Step 4: Add undoable actions for name, transform, and component override edits.**
- [x] **Step 5: Add static `agent-check` coverage so AI guidance cannot drift away from the new authoring API.**
- [x] **Step 6: Run focused editor-core tests and confirm GREEN.**

### Task 6: Sync Docs, Manifest, Skills, And Subagents

**Files:**
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/editor.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Document the core prefab variant format and editor-core authoring boundary.**
- [x] **Step 2: Keep runtime guidance honest: generated gameplay consumes composed prefabs/cooked packages, not editor authoring documents.**
- [x] **Step 3: Keep Codex and Claude guidance behaviorally equivalent.**

### Task 7: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests
ctest --preset dev --output-on-failure -R "mirakana_core_tests|mirakana_editor_core_tests"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

- [x] **Step 2: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer`; use `build-fixer` if validation exposes build or toolchain failures.**

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed as expected after core tests were added because `mirakana/scene/prefab_overrides.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `mirakana_scene` prefab variant/override API and implementation.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after core prefab variant tests were added.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed as expected after editor-core tests were added because `mirakana/editor/prefab_variant_authoring.hpp` did not exist.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding `PrefabVariantAuthoringDocument`, rows, diagnostics, save/load, and undo actions.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` after the initial sandboxed attempt hit the known vcpkg 7zip `CreateFileW stdin failed with 5` host-gate and the same command was rerun with approval.
- REVIEW: `cpp-reviewer` found three prefab variant validation/text IO blockers: unsupported `PrefabOverrideKind` values, non-finite transform overrides that could serialize into unreadable assets, and kind-mismatched text fields being accepted and dropped on reserialization.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding reviewer regression tests because `invalid_override_kind` and `invalid_transform` diagnostics did not exist yet.
- GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/test.ps1` passed after adding unsupported-kind diagnostics, finite/positive-scale transform validation, kind-matched text field validation, and editor transform guard coverage.
- PASS: focused `cmake --build --preset dev --target mirakana_core_tests mirakana_editor_core_tests` through repository tool resolution.
- PASS: focused `ctest --preset dev --output-on-failure -R "mirakana_core_tests|mirakana_editor_core_tests"` through repository tool resolution.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` after reviewer fixes.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` after the sandboxed attempt again hit the known vcpkg 7zip `CreateFileW stdin failed with 5` host-gate and the same command was rerun with approval.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Existing host/toolchain gates remain diagnostic-only: Metal `metal`/`metallib` missing, Apple packaging requires macOS/Xcode, Android release signing not configured, Android device smoke not connected, and strict clang-tidy remains diagnostic-only because the Visual Studio generator did not emit the expected compile database.
