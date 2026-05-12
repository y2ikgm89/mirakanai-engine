# Editor Scene Hierarchy Reparent UI v1 (2026-05-11)

**Plan ID:** `editor-scene-hierarchy-reparent-ui-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Completed  

## Goal

Expose reviewed Scene hierarchy **Reparent** controls in the visible `MK_editor` Scene panel by wiring existing `make_scene_authoring_reparent_node_action` through deterministic parent candidates from `MK_editor_core`, without Scene graph API changes (root promotion remains unsupported while `Scene::set_parent` requires a non-null parent).

## Context

- `make_scene_authoring_reparent_node_action` already rejects invalid parents (including descendant cycles and null parents).
- `MK_editor` previously offered hierarchy selection and edits but no explicit reparent UI.

## Constraints

- No importer execution, cooking, package mutation, or manifest edits.
- Candidate parents exclude the selected node, descendants of the selected node, and the **current** parent (no-op moves).
- Do not broaden `editor-productization` beyond this hierarchy UX lane.

## Done when

- `make_scene_authoring_reparent_parent_options` (or equivalent) lists deterministic, indented labels for valid targets.
- `MK_editor_core_tests` cover candidate filtering for representative hierarchies.
- Scene panel exposes parent combo + **Reparent** executing through `UndoStack` like other authoring actions.
- `docs/editor.md` mentions Reparent in one sentence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the development host.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
