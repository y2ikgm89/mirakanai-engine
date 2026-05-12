# Editor glTF Mesh Inspect UI Document Contract v1 (2026-05-10)

**Plan ID:** `editor-gltf-mesh-inspect-ui-document-contract-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Status:** Completed  

## Goal

Expose read-only glTF mesh primitive inspect results as a retained-mode `mirakana::ui::UiDocument` with stable element ids and an explicit contract token so AI/editor diagnostics can consume the same facts as Dear ImGui inspector rows without claiming importer execution, cooking, scene mutation, or broad editor productization.

## Context

- `mirakana::inspect_gltf_mesh_primitives` and `gltf_mesh_inspect_report_to_inspector_rows` already exist ([2026-05-06-gltf-mesh-editor-ux-v1.md](2026-05-06-gltf-mesh-editor-ux-v1.md)).
- Other editor surfaces (for example material preview) expose parallel `make_*_ui_model` builders with retained ids and contract labels.

## Constraints

- No change to glTF parsing semantics; document emission must follow `gltf_mesh_inspect_report_to_inspector_rows` row ids and labels.
- Serialize-safe label/value text (reject `=`, newlines in strict fields; sanitize visible text like other editor UI models).
- Do not claim SDL/GPU mesh preview, runtime mesh upload, or package streaming.

## Done when

- `make_gltf_mesh_inspect_ui_model` exists under `mirakana::editor`, root id `gltf_mesh_inspect`, contract label `ge.editor.gltf_mesh_inspect.v1`, row nodes `gltf_mesh_inspect.rows.<row.id>.caption` / `.text`.
- `MK_editor_core_tests` cover failed inspect and importer-enabled triangle inspect document ids plus serialized snapshot substring checks.
- `docs/editor.md` notes the retained document contract in one sentence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Focused tests | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | PASS |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS (Windows dev preset, 2026-05-10) |
