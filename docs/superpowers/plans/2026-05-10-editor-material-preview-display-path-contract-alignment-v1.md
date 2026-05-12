# Editor Material Preview Display Path Contract Alignment v1 (2026-05-10)

**Plan ID:** `editor-material-preview-display-path-contract-alignment-v1`  
**Gap:** `editor-productization`  
**Status:** Completed.

## Goal

Align host-owned material GPU preview execution snapshots with the **actual** `MK_editor` display path: Vulkan (and any backend without D3D12 SDL shared-texture interop) uses **CPU readback** into an SDL texture, matching viewport fallback behavior and the existing `ge.editor.material_gpu_preview_execution.v1` contract vocabulary (`cpu-readback`, not incorrect `shared-texture` labels for Vulkan).

## Context

- `SdlViewportTexture::display_path_label()` remains human-readable for the Viewport panel (`CPU readback` / `D3D12 shared texture`).
- `make_material_gpu_preview_execution_snapshot` must feed **stable contract tokens** into `EditorMaterialGpuPreviewExecutionSnapshot::display_path_label` for retained `material_asset_preview.gpu.execution.display_path` rows.

## Constraints

- No editor/core RHI or shader execution changes.
- Do not claim Vulkan/Metal **visual** parity with D3D12.

## Done when

- `make_material_gpu_preview_execution_snapshot` uses explicit contract ids from the shell display texture.
- `mirakana_editor_core_tests` expects Vulkan snapshots to report `cpu-readback`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation evidence

| Step | Command | Result |
| --- | --- | --- |
| Tests | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | Pass |
| Repo | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
