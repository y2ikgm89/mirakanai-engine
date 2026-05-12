# Editor glTF Mesh Inspect Retained Document Inspector Wiring v1 (2026-05-10)

**Plan ID:** `editor-gltf-mesh-inspect-retained-document-inspector-wiring-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Depends on:** [2026-05-10-editor-gltf-mesh-inspect-ui-document-contract-v1.md](2026-05-10-editor-gltf-mesh-inspect-ui-document-contract-v1.md)  
**Status:** Completed  

## Goal

Wire `make_gltf_mesh_inspect_ui_model` into the visible `MK_editor` Inspector so operators and agents can read the same serialized retained-mode snapshot as tests, alongside existing Dear ImGui inspect rows, without executing imports or broadening editor ready claims.

## Context

- `serialize_editor_ui_model` + `make_gltf_mesh_inspect_ui_model` are implemented in `MK_editor_core`; `MK_editor` already renders inspector rows from `gltf_mesh_inspect_report_to_inspector_rows`.

## Constraints

- Read-only display; no new filesystem or importer execution paths.
- Snapshot updates only when `refresh_gltf_mesh_inspect_report` completes (including failure diagnostics).
- Do not claim GPU mesh preview, cooking automation, or full editor productization.

## Done when

- Inspector shows a collapsible read-only block with `serialize_editor_ui_model(make_gltf_mesh_inspect_ui_model(report))` after refresh.
- `docs/editor.md` notes the visible wiring in one sentence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
