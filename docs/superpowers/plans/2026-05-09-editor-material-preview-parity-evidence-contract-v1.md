# Editor Material Preview Parity Evidence Contract v1 (2026-05-09)

**Plan ID:** `editor-material-preview-parity-evidence-contract-v1`  
**Parent wave:** [2026-05-09-phase-4-5-continuation-wave-1-v1.md](2026-05-09-phase-4-5-continuation-wave-1-v1.md)  
**Status:** Completed.

## Goal

Give operators and automated diagnostics a **stable, versioned contract id** in the host-owned material GPU preview execution UI so **D3D12 vs Vulkan vs (future) Metal** snapshots can be compared row-for-row without claiming display parity or promoting `editor-productization` gap status.

## Context

- [`EditorMaterialGpuPreviewExecutionSnapshot`](../../../editor/core/include/mirakana/editor/material_asset_preview_panel.hpp) already carries `backend_label`, `display_path_label`, and `frames_rendered`.
- [Tracking checklist](2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md) covers manual lanes; this slice adds **machine-stable contract metadata** only.

## Constraints

- No RHI handles, uploads, or shader execution in `mirakana_editor_core`.
- Do not assert Vulkan/Metal **visual parity** in CI; contract row must be backend-agnostic.

## Done when

- Retained `mirakana_ui` material preview document exposes `material_asset_preview.gpu.execution.contract` with fixed id `ge.editor.material_gpu_preview_execution.v1`.
- `mirakana_editor_core_tests` covers D3D12- and Vulkan-labeled snapshots mapping through `apply_editor_material_gpu_preview_execution_snapshot`.

## Validation

| Step | Command | Result |
| --- | --- | --- |
| Tests | `ctest --preset dev -R mirakana_editor_core_tests --output-on-failure` | Pass |
| Repo | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | OK |
