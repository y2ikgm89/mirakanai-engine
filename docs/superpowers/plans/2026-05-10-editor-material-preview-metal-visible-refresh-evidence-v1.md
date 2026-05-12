# Editor Material Preview Metal Visible Refresh Evidence v1 (2026-05-10)

**Plan ID:** `editor-material-preview-metal-visible-refresh-evidence-v1`  
**Gap:** `editor-productization`  
**Parent:** [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md)  
**Depends on:** [2026-05-10-editor-material-preview-vulkan-visible-refresh-evidence-v1.md](2026-05-10-editor-material-preview-vulkan-visible-refresh-evidence-v1.md), [2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md](2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md)  
**Status:** Completed  

## Goal

When `MK_editor` uses the **Metal** RHI backend for the viewport/material preview path, prove **at least one host-owned material GPU preview refresh completed** for the selected cooked material by surfacing deterministic evidence alongside the existing `EditorMaterialGpuPreviewExecutionSnapshot` contract: retained `material_asset_preview.gpu.execution` rows plus a new **`material_asset_preview.gpu.execution.metal_visible_refresh`** label derived only from snapshot fields (`backend_label`, `status`, `display_path_label`, `frames_rendered`, `diagnostic`), mirroring the Vulkan visible-refresh rules with Metal-specific backend matching.

**Out of scope:** pixel-identical parity with D3D12 or Vulkan, broad Metal editor readiness, native Metal presentation guarantees, or clearing `editor-productization.requiredBeforeReadyClaim` items that still require full Vulkan/Metal material-preview **display** parity beyond this narrow host-owned evidence row.

## Context

- `editor/core` owns `EditorMaterialGpuPreviewExecutionSnapshot`, `apply_editor_material_gpu_preview_execution_snapshot`, and `make_material_asset_preview_panel_ui_model` (no RHI execution in `MK_editor_core`).
- `MK_editor` builds snapshots after `render_material_preview_gpu_cache` the same way as the Vulkan slice.
- [2026-05-10-editor-material-preview-vulkan-visible-refresh-evidence-v1.md](2026-05-10-editor-material-preview-vulkan-visible-refresh-evidence-v1.md) completed the Vulkan analogue; Metal uses the same CPU readback display path contract where applicable ([2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md](2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md)).
- `EditorRenderBackend::metal` exists in the shell; Metal toolchain gates already surface in the editor UI elsewhere.

## Constraints

- No RHI, shader execution, or SDL/Dear ImGui in `MK_editor_core`; evidence is **computed** from the snapshot only.
- If the host lacks Metal material-preview shader artifacts or a Metal-capable editor shell, record a **blocker** in the validation table; do not promote gap status or remove `requiredBeforeReadyClaim` rows on incomplete evidence.
- Greenfield only: no backward-compatibility shims for legacy snapshot shapes.
- Keep ASCII enum values for the retained row: `complete` \| `pending` \| `not-applicable` (same vocabulary as Vulkan for operator consistency).

## Done when

- [x] This plan documents Goal / Context / Constraints / Done When and the validation evidence table (this file satisfies the pre-implementation record; mark **Status: Completed** only after implementation and green validation).
- [x] `apply_editor_material_gpu_preview_execution_snapshot` (or the single composition helper used for GPU execution rows) sets a model field and retained UI id `material_asset_preview.gpu.execution.metal_visible_refresh` with values `complete`, `pending`, or `not-applicable` (deterministic rules documented beside code, symmetric to Vulkan with Metal backend matching).
- [x] `MK_editor` shows the evidence string in the material GPU preview section when not `not-applicable`.
- [x] `MK_editor_core_tests` cover D3D12/Vulkan (`not-applicable`), Metal-ready with frames (`complete`), and at least one `pending` case; `tools/check-ai-integration.ps1` needles updated if new retained row ids are contract-tested.
- [x] `docs/editor.md` mentions the Metal visible-refresh line in one sentence next to the Vulkan line.
- [x] `engine/agent/manifest.json` `editor-productization` notes honestly mention this slice when landed, without claiming full display-parity gap closure.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the validation host, or blockers are recorded in the validation table below.

## Validation evidence

| Check | Host / command | Result | Notes |
| --- | --- | --- | --- |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | **PASS** | Run after implementation on the validation host. |
| Editor core unit tests | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | **PASS** | Same host as slice validation. |
| Metal visible smoke | Manual `MK_editor` with Metal backend + material-preview artifacts | **Not run on Windows** | Host-gated; typically macOS/full Xcode; CI remains D3D12/Vulkan/Null-oriented. |

## Contract labels

- Existing: `ge.editor.material_gpu_preview_execution.v1` (unchanged).
- Proposed retained row: `material_asset_preview.gpu.execution.metal_visible_refresh` with text values `complete` \| `pending` \| `not-applicable`.
