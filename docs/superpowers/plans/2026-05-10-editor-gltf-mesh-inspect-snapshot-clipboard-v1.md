# Editor glTF Mesh Inspect Snapshot Clipboard v1 (2026-05-10)

**Plan ID:** `editor-gltf-mesh-inspect-snapshot-clipboard-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Depends on:** [2026-05-10-editor-gltf-mesh-inspect-retained-document-inspector-wiring-v1.md](2026-05-10-editor-gltf-mesh-inspect-retained-document-inspector-wiring-v1.md)  
**Status:** Completed  

## Goal

Let operators and agents copy the serialized retained `MK_ui` glTF mesh inspect snapshot to the system clipboard from the visible `MK_editor` Inspector, using the same Dear ImGui clipboard path already used elsewhere in the shell.

## Constraints

- Copy only the existing `gltf_mesh_inspect_ui_snapshot_` string; no new network or file paths.
- Disable the control when the snapshot is empty (before first Inspect).

## Done when

- Inspector exposes a **Copy retained snapshot** control under the glTF retained snapshot section that calls `ImGui::SetClipboardText` when non-empty.
- `docs/editor.md` mentions the control in one sentence.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Repository gate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS |
