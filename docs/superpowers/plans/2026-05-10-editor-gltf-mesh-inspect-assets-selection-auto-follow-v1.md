# Editor glTF Mesh Inspect Assets Selection Auto-Follow v1 (2026-05-10)

**Plan ID:** `editor-gltf-mesh-inspect-assets-selection-auto-follow-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Depends on:** [2026-05-10-editor-gltf-mesh-inspect-snapshot-clipboard-v1.md](2026-05-10-editor-gltf-mesh-inspect-snapshot-clipboard-v1.md)  
**Status:** Completed  

## Goal

When the operator enables an explicit Inspector toggle, selecting a `.gltf` or `.glb` asset in the Assets panel updates the glTF mesh inspect path and refreshes the read-only inspect report without requiring the separate **Use Assets selection path** click.

## Context

The shell already supports manual path copy from Assets selection and `inspect_gltf_mesh_primitives` over project-store bytes. This slice only reduces friction for the reviewed inspect workflow.

## Constraints

- No importer execution, cooking, or scene mutation.
- Auto-follow is **opt-in** (default off) so browsing Assets does not surprise-overwrite the inspect path.
- Fail-closed diagnostics for missing files or parse errors remain unchanged.
- Do not broaden `editor-productization` ready claims beyond this narrow inspect UX.

## Done when

- Inspector exposes a checkbox controlling whether Assets selection drives glTF mesh inspect path refresh for `.gltf`/`.glb` selections.
- When enabled, a successful Assets list selection change for those extensions copies the project-relative path and runs the same refresh path as manual inspect.
- `editor_asset_path_supports_gltf_mesh_inspect` (or equivalent) lives in `MK_editor_core` with unit coverage for extension matching.
- `docs/editor.md` mentions the toggle in one sentence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
