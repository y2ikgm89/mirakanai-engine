# Editor Scene/Prefab GUI Package Candidate Workflow v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Connect the visible desktop editor Scene, Inspector, and Assets panels to the existing `mirakana_editor_core` scene/prefab authoring model so authored scene changes, prefab operations, diagnostics, and package candidate rows are usable through the editor shell.

**Architecture:** Keep `mirakana_editor_core` as the source of persistent editor behavior and keep Dear ImGui as an adapter. The editor shell should hold a `mirakana::editor::SceneAuthoringDocument` plus a document-scoped `UndoStack`, route scene mutations through `make_scene_authoring_*_action`, and render model rows from `hierarchy_rows`, `validate_scene_authoring_references`, and `make_scene_package_candidate_rows`.

**Tech Stack:** Existing C++23 editor/core contracts, Dear ImGui shell in `mirakana_editor`, existing `AssetRegistry`, `FileTextStore`, `ProjectDocument`, `UndoStack`, `SceneAuthoringDocument`, focused GUI/default validation. No new third-party dependencies.

---

## Context

Editor Scene/Prefab Package Authoring v0 implemented GUI-independent authoring contracts in `mirakana_editor_core`, but `editor/src/main.cpp` still edits raw `mirakana::Scene` plus `selected_node_` in the Scene, Inspector, and Viewport paths. The next production gap is to make that model visible and usable in the actual editor shell without exposing Dear ImGui, SDL3, native handles, or RHI/backend objects through public game APIs.

## Constraints

- No new dependencies.
- Do not change public game APIs.
- Keep `editor/core` GUI-independent and default-preset buildable.
- Dear ImGui remains only the optional editor adapter.
- Keep scene/prefab authoring undo stacks scoped to the edited document lifetime.
- Do not automate manifest/CMake package registration in this v0 slice.
- Do not add prefab override/variant semantics in this v0 slice.
- If public editor headers change, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.

## Done When

- The editor shell owns a `mirakana::editor::SceneAuthoringDocument` for the active scene and clears document-scoped undo history when replacing it.
- Scene panel hierarchy rows come from `SceneAuthoringDocument::hierarchy_rows()` and expose select, create, rename, delete, duplicate, prefab save, and prefab instantiate actions through existing `mirakana_editor_core` actions.
- Inspector transform/component edits and viewport transform deltas route through `make_scene_authoring_transform_edit_action` / `make_scene_authoring_component_edit_action` instead of serializing and replacing raw scenes.
- Assets/package UI renders `validate_scene_authoring_references` and `make_scene_package_candidate_rows` so package candidates and broken references are visible before package registration automation.
- Docs, roadmap, gap analysis, manifest, Codex/Claude editor/game guidance, and relevant subagent guidance are synchronized.
- Focused validation and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass with existing host/toolchain gates recorded.

## Tasks

### Task 1: Register The Slice

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Create: this plan
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`

- [x] **Step 1: Create this dated focused plan.**
- [x] **Step 2: Move Generated Desktop Runtime Cooked Scene Package Scaffold v0 to completed and mark this plan as active.**
- [x] **Step 3: Update gap analysis so editor scene/prefab GUI wiring is the active Windows-host-feasible slice.**

### Task 2: Write The Failing Editor-Shell Contract

**Files:**
- Modify: `tools/check-ai-integration.ps1`
- Exercise: `editor/src/main.cpp`

- [x] **Step 1: Add static AI integration checks that require the visible editor shell to include and use `mirakana/editor/scene_authoring.hpp`.**

Expected strings in `editor/src/main.cpp` after implementation:

```text
#include "mirakana/editor/scene_authoring.hpp"
mirakana::editor::SceneAuthoringDocument
make_scene_authoring_create_node_action
make_scene_authoring_delete_node_action
make_scene_authoring_duplicate_subtree_action
make_scene_authoring_transform_edit_action
make_scene_authoring_component_edit_action
build_prefab_from_selected_node
save_prefab_authoring_document
load_prefab_authoring_document
validate_scene_authoring_references
make_scene_package_candidate_rows
```

- [x] **Step 2: Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` and confirm RED because the editor shell still edits raw scene state.**

Expected failure:

```text
ai-integration-check: editor shell missing SceneAuthoringDocument wiring
```

### Task 3: Make SceneAuthoringDocument The Active Scene Source

**Files:**
- Modify: `editor/src/main.cpp`

- [x] **Step 1: Add `mirakana/editor/scene_authoring.hpp` and replace the raw active scene ownership with a `SceneAuthoringDocument` member.**

Keep renderer/viewport reads on `scene_document_.scene()` through a small accessor, for example:

```cpp
[[nodiscard]] const mirakana::Scene& active_scene() const noexcept {
    return scene_document_.scene();
}
```

- [x] **Step 2: Replace `selected_node_` state with `scene_document_.selected_node()` and `scene_document_.select_node(...)`.**

Use `null_scene_node` only through the document contract; do not keep a second selected-node source of truth.

- [x] **Step 3: Replace `load_scene`, `save_current_scene`, and project bundle scene replacement with scene authoring load/save helpers.**

Required behavior:

```cpp
mirakana::editor::save_scene_authoring_document(project_store_, project_paths_.scene_path, scene_document_);
scene_document_ = mirakana::editor::SceneAuthoringDocument::from_scene(mirakana::deserialize_scene(text), project_paths_.scene_path);
history_.clear();
```

- [x] **Step 4: Clear `history_` before replacing the document so captured document references cannot outlive the document they mutate.**

### Task 4: Route Editor Mutations Through Core Actions

**Files:**
- Modify: `editor/src/main.cpp`

- [x] **Step 1: Add an `execute_scene_authoring_action` helper that wraps `history_.execute(...)`, marks `dirty_state_`, and logs rejected actions.**

Required behavior:

```cpp
bool execute_scene_authoring_action(mirakana::editor::UndoableAction action) {
    if (!history_.execute(std::move(action))) {
        log_.log(mirakana::LogLevel::warn, "editor", "Scene authoring action was rejected");
        return false;
    }
    dirty_state_.mark_dirty();
    return true;
}
```

- [x] **Step 2: Change Scene menu `Add Empty Node` and panel buttons to use `make_scene_authoring_create_node_action`.**
- [x] **Step 3: Change selected-node rename, delete, and duplicate controls to use `make_scene_authoring_rename_node_action`, `make_scene_authoring_delete_node_action`, and `make_scene_authoring_duplicate_subtree_action`.**
- [x] **Step 4: Change Inspector transform/component edits to use `make_scene_authoring_transform_edit_action` and `make_scene_authoring_component_edit_action`.**
- [x] **Step 5: Change viewport transform delta edits to build drafts from `active_scene()` and submit the transform action through the same helper.**

### Task 5: Surface Prefab And Package Candidate Workflow

**Files:**
- Modify: `editor/src/main.cpp`
- Modify: `docs/editor.md`

- [x] **Step 1: Add minimal prefab save/instantiate controls in the Scene panel.**

Use deterministic paths rooted under the active project for v0, for example:

```cpp
constexpr std::string_view default_prefab_path = "assets/prefabs/selected.prefab";
const auto prefab_path = current_prefab_path();
```

Save selected prefab with `build_prefab_from_selected_node` plus `save_prefab_authoring_document`; instantiate with `load_prefab_authoring_document` plus `make_scene_authoring_instantiate_prefab_action`.

- [x] **Step 2: Render scene reference diagnostics in the Assets panel through `validate_scene_authoring_references(active_scene(), assets_)`.**
- [x] **Step 3: Render package candidate rows through `make_scene_package_candidate_rows(scene_document_, current_cooked_scene_path(), current_package_index_path(), {current_prefab_path()})`.**
- [x] **Step 4: Keep rows read-only in this slice; package registration automation remains a follow-up.**

### Task 6: Sync Docs, Manifest, Skills, And Subagents

**Files:**
- Modify: `docs/editor.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/editor-change/SKILL.md`
- Modify: `.claude/skills/gameengine-editor/SKILL.md`
- Modify: `.codex/agents/engine-architect.toml`
- Modify: `.claude/agents/engine-architect.md`

- [x] **Step 1: Document that visible editor scene hierarchy, inspector edits, prefab buttons, diagnostics, and package candidate rows now adapt the `mirakana_editor_core` scene authoring model.**
- [x] **Step 2: Keep non-goals explicit: prefab overrides/variants, manifest/package registration automation, play-in-editor isolation, and shader/material generation remain follow-up work.**
- [x] **Step 3: Keep Codex and Claude guidance behaviorally equivalent.**

### Task 7: Validate

**Files:**
- Modify: this plan with validation evidence.
- Modify: `docs/superpowers/plans/README.md` after completion.

- [x] **Step 1: Run focused validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1
```

- [x] **Step 2: Run API boundary validation if public editor headers changed.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

- [x] **Step 3: Run required final validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [x] **Step 4: Run `cpp-reviewer` and fix actionable findings.**

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS. The static editor shell contract now requires `SceneAuthoringDocument`, `make_scene_authoring_*` actions, prefab save/load, scene reference diagnostics, package candidate rows, and project-rooted package/prefab path helpers.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS after `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build-gui.ps1`: PASS with the known sandbox vcpkg 7zip workaround. 32/32 GUI preset tests passed, including `mirakana_editor_core_tests`, SDL3 adapter tests, desktop runtime host tests, and generated desktop runtime package smokes.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: not required for this slice because no public headers or backend interop contracts changed; the default `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` still ran the public API boundary gate and passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. Existing diagnostic-only/host-gated results remain: Metal `metal`/`metallib` missing, Apple packaging blocked by missing macOS/Xcode tools, Android release signing not configured, Android device smoke not connected, and strict clang-tidy diagnostic-only because the Visual Studio dev preset does not emit the expected compile database before configure/build completes.
- `cpp-reviewer`: findings fixed. Prefab save/load and package candidate paths are now rooted under the active project, gameplay-builder Codex/Claude guidance no longer claims scene/prefab GUI wiring is follow-up work, `SceneAuthoringDocument` now outlives the document-scoped `UndoStack`, and this plan plus the registry record validation evidence have been updated.
