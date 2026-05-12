# Editor Material Preview Display Parity Checklist Row Bundle v1 (2026-05-11)

**Plan ID:** `editor-material-preview-display-parity-checklist-row-bundle-v1`  
**Gap:** `editor-productization`  
**Parent stream:** [2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md](2026-05-11-editor-productization-material-preview-display-parity-stream-v1.md)  
**Status:** Completed (see validation table)

## Goal

Add a **deterministic retained UI row bundle** under `material_asset_preview.gpu.execution` that binds **display-parity checklist** evidence to stable contract ids (`ge.editor.material_gpu_preview_display_parity_checklist.v1`), execution scope (backend, display path class), and existing Vulkan/Metal **visible refresh** snapshot gates — without widening editor-productization ready claims or clearing `requiredBeforeReadyClaim` rows for full Vulkan/Metal display parity.

## Context

- Material preview GPU execution already exposes `material_asset_preview.gpu.execution.contract` (`ge.editor.material_gpu_preview_execution.v1`), backend, display path, frames, and `vulkan_visible_refresh` / `metal_visible_refresh` snapshot-derived statuses.
- The material parity stream lists **child slice #1**: parity checklist row bundle (contract ids, backend scope, pending vs complete vs not-applicable).

## Constraints

- No gameplay API exposure of RHI or native handles; evidence remains host-owned and snapshot-derived where applicable.
- Preserve distinction between **cpu-readback** and **d3d12-shared-texture** display paths; do not treat checklist rows as proof of cross-backend visual equivalence.
- On non-Vulkan / non-Metal active backends, Vulkan/Metal gate rows must remain **`not-applicable`** per existing derivation rules.

## Done when

- `EditorMaterialAssetPreviewPanelModel` carries a stable ordered list of checklist rows populated whenever the execution snapshot fields are set (including default host-required model).
- `make_material_asset_preview_panel_ui_model` emits retained ids under `material_asset_preview.gpu.execution.parity_checklist` including `.contract` and `.rows.<row_id>.status` / `.rows.<row_id>.detail`.
- `tools/check-ai-integration.ps1` needles and `engine/agent/manifest.json` `editor-productization` notes reference the new retained ids.
- `MK_editor_core_tests` asserts representative serialized element ids and statuses for default, D3D12-ready, and Vulkan-ready snapshots.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the integration host.

## Validation evidence

| Check | Command / artifact | Result |
| --- | --- | --- |
| Unit tests | `ctest --preset dev` (includes `MK_editor_core_tests`) | **Passed** (47/47, 2026-05-11 host) |
| Repository validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | **Passed** (exit code 0) |
