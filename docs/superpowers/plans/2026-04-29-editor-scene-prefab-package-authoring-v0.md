# Editor Scene/Prefab Package Authoring v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add GUI-independent editor scene and prefab authoring models that can edit/save authored scenes, create/instantiate prefabs, validate asset references, and produce package file candidate rows for cooked package workflows.

**Architecture:** Keep durable authoring state in `mirakana_editor_core`, with existing `mirakana_scene` owning runtime/editor-neutral scene and prefab data. Add deterministic prefab text IO in `mirakana_scene` only if needed by editor/core tests. Keep Dear ImGui as a future adapter over the new retained models; do not expose editor APIs to runtime games.

**Tech Stack:** C++23, `mirakana_scene`, `mirakana_editor_core`, `UndoStack`, `ITextStore`, `AssetRegistry`, existing unit tests, no new third-party dependencies.

---

## Context

Automatic Cooked Package Assembly v0 made `.geindex` assembly available after asset imports. The next production gap is the authoring loop before package registration: users and AI agents need a tested model for scene hierarchy edits, prefab creation/instantiation, deterministic scene/prefab persistence, asset-reference validation, and package file rows that later build-system integration can mirror into `runtimePackageFiles` and `PACKAGE_FILES`.

## Constraints

- No new dependencies.
- No renderer, RHI, SDL3, Dear ImGui, OS window, GPU handle, or backend interop exposure.
- `mirakana_scene` must remain independent of editor/platform/renderer/RHI.
- `mirakana_editor_core` can depend on `mirakana_scene`, `mirakana_assets`, and existing editor text stores.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- GUI wiring is not required for this slice unless the headless model is complete first.

## Done When

- `mirakana_editor_core` exposes a `SceneAuthoringDocument` with deterministic hierarchy rows, selected node state, load/save through `ITextStore`, dirty tracking, and undoable create/rename/reparent/delete/duplicate/transform/component edits.
- `mirakana_editor_core` exposes prefab authoring helpers for selected subtree creation, deterministic prefab save/load, prefab instantiation into a scene document, and undo/redo.
- Scene authoring validation reports missing or wrong-kind mesh/material/sprite references through `AssetRegistry`.
- Package authoring rows expose source/cooked/index file candidates for scene and prefab authoring workflows without invoking CMake or platform packaging.
- Focused headless tests cover scene edit rows, undo/redo, prefab roundtrip/instantiation, asset reference validation, and package candidate rows.
- Docs, plan registry, roadmap, gap analysis, manifest, Codex/Claude skills, and subagent guidance are synchronized.
- Focused validation, API boundary, schema/agent/format/tidy diagnostics, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` have been run.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Mark this plan as current active work in the plan registry.**

### Task 2: Write Failing Editor-Core Tests

**Files:**
- Test: `tests/unit/editor_core_tests.cpp`
- Create: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Create: `editor/core/src/scene_authoring.cpp`

- [x] **Step 1: Add scene authoring document tests.**

Expected behavior:
- `SceneAuthoringDocument::from_scene` builds depth-sorted hierarchy rows.
- Selection is stable until the selected node is deleted.
- Undoable create, rename, reparent, delete, duplicate, transform, and component edit actions mutate the document and undo/redo through `UndoStack`.
- Invalid names, invalid parents, deleting missing nodes, and cyclic reparent requests are rejected without mutation.

- [x] **Step 2: Add prefab authoring tests.**

Expected behavior:
- A selected subtree can be converted to a prefab definition.
- Prefab definitions save/load deterministically.
- Instantiating a prefab into a scene document creates the expected hierarchy and can be undone.

- [x] **Step 3: Add validation/package-row tests.**

Expected behavior:
- Mesh/material/sprite references are validated against an `AssetRegistry`.
- Package candidate rows expose scene source path, cooked scene output path, and package index path for later generated-game package registration.

- [x] **Step 4: Run focused tests and observe expected compile failures before implementation.**

Run:

```powershell
. .\tools\common.ps1
$tools = Assert-CppBuildTools
Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^mirakana_editor_core_tests$"
```

Expected:
- Build fails because `mirakana/editor/scene_authoring.hpp` and related functions do not exist yet.

### Task 3: Implement Prefab Text IO

**Files:**
- Modify: `engine/scene/include/mirakana/scene/prefab.hpp`
- Modify: `engine/scene/src/prefab.cpp`
- Test: `tests/unit/core_tests.cpp`

- [x] **Step 1: Add deterministic prefab serialization declarations.**

Add `serialize_prefab_definition(const PrefabDefinition&)` and `deserialize_prefab_definition(std::string_view)`.

- [x] **Step 2: Implement text IO.**

Use a first-party `GameEngine.Prefab.v1` key/value format with stable node order, parent indices, transforms, and component fields matching existing scene component semantics.

- [x] **Step 3: Add roundtrip and rejection tests.**

Cover nested prefab roundtrip, invalid format, invalid parent index, missing fields, and invalid components.

### Task 4: Implement Scene Authoring Document

**Files:**
- Create: `editor/core/include/mirakana/editor/scene_authoring.hpp`
- Create: `editor/core/src/scene_authoring.cpp`
- Modify: `editor/CMakeLists.txt`

- [x] **Step 1: Add public editor-core types.**

Add `SceneAuthoringDocument`, `SceneAuthoringNodeRow`, `SceneAuthoringDiagnostic`, and `ScenePackageCandidateRow`.

- [x] **Step 2: Implement hierarchy, persistence, validation, and package rows.**

Load/save scenes and prefabs through `ITextStore`; build deterministic row models; validate asset references through `AssetRegistry`; produce package candidate rows without calling build/package scripts.

- [x] **Step 3: Implement undoable scene/prefab actions.**

Use snapshot-based `UndoableAction` helpers scoped to the document lifetime, matching `MaterialAuthoringDocument` patterns.

### Task 5: Sync Docs And AI Guidance

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] **Step 1: Record the new editor-core scene/prefab authoring capability honestly.**
- [x] **Step 2: Keep remaining gaps explicit: GUI wiring depth, play-in-editor isolation, build-system generated-game package integration, and broader scene/physics/navigation integration.**

### Task 6: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
. .\tools\common.ps1
$tools = Assert-CppBuildTools
Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests mirakana_scene_renderer_tests mirakana_runtime_scene_tests
Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^(mirakana_editor_core_tests|mirakana_scene_renderer_tests|mirakana_runtime_scene_tests)$"
```

- [x] **Step 2: Run required checks.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 3: Run `cpp-reviewer` and fix actionable findings.**

## Validation Evidence

- RED confirmed before implementation: `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests mirakana_core_tests` failed because `mirakana/editor/scene_authoring.hpp` did not exist.
- Focused implementation tests passed: `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests mirakana_core_tests` and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^(mirakana_editor_core_tests|mirakana_core_tests)$"`.
- Reviewer blockers fixed: delete selection remapping now tracks the same node after id rebuilds, scene hierarchy/subtree traversals tolerate corrupted child cycles, scene parenting rejects cycles, prefab subtree build rejects corrupted cycles, and asset diagnostics remain field-specific for repeated asset ids.
- Final focused validation passed: `Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_editor_core_tests mirakana_core_tests mirakana_scene_renderer_tests mirakana_runtime_scene_tests` and `Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^(mirakana_editor_core_tests|mirakana_core_tests|mirakana_scene_renderer_tests|mirakana_runtime_scene_tests)$"` reported 4/4 tests passed.
- Required checks passed: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` reported config ok with the existing diagnostic-only blocker: `compile_commands.json` missing for the Visual Studio `dev` preset at `out\build\dev\compile_commands.json`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1` first failed inside the sandbox while vcpkg extracted 7zip with `CreateFileW stdin failed with 5`; the same command passed when rerun outside the sandbox and reported 30/30 desktop-gui tests passed.
- `cpp-reviewer` found no remaining code blockers after fixes; the direct corrupted child-cycle undo-action coverage gap was closed with `editor scene authoring undo actions tolerate corrupted child cycles`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. Existing diagnostic-only gates remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy analysis gated by the missing compile database.
