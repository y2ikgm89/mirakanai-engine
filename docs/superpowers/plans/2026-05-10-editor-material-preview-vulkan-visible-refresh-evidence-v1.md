# Editor Material Preview Vulkan Visible Refresh Evidence v1 (2026-05-10)

**Plan ID:** `editor-material-preview-vulkan-visible-refresh-evidence-v1`  
**Gap:** `editor-productization`  
**Status:** Completed.

## Goal

When `MK_editor` uses the **Vulkan** RHI backend for the viewport/material preview path, prove **at least one host-owned material GPU preview refresh completed** for the selected cooked material by surfacing deterministic evidence alongside the existing `EditorMaterialGpuPreviewExecutionSnapshot` contract: retained `material_asset_preview.gpu.execution` rows plus a new **`material_asset_preview.gpu.execution.vulkan_visible_refresh`** label derived only from snapshot fields (`backend_label`, `status`, `display_path_label`, `frames_rendered`, `diagnostic`).

**Out of scope:** pixel-identical parity with D3D12, Metal, broad renderer/editor ready claims, or clearing `editor-productization.requiredBeforeReadyClaim` items that still require full Vulkan/Metal display parity beyond this narrow host-owned evidence row.

## Context

- `editor/core` owns `EditorMaterialGpuPreviewExecutionSnapshot`, `apply_editor_material_gpu_preview_execution_snapshot`, and `make_material_asset_preview_panel_ui_model` (no RHI execution in `MK_editor_core`).
- `MK_editor` (`editor/src/main.cpp`) builds snapshots via `make_material_gpu_preview_execution_snapshot` after `render_material_preview_gpu_cache`.
- [2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md](2026-05-10-editor-material-preview-display-path-contract-alignment-v1.md) aligned Vulkan material-preview snapshots to `display_path_label=cpu-readback` for the CPU readback display path (not D3D12 shared-texture interop).
- [2026-05-10-editor-viewport-vulkan-rhi-bootstrap-v1.md](2026-05-10-editor-viewport-vulkan-rhi-bootstrap-v1.md) completed Vulkan RHI bootstrap for the editor viewport when host/toolchain gates pass.

## Constraints

- No RHI, shader execution, or SDL/Dear ImGui in `MK_editor_core`; evidence is **computed** from the snapshot only.
- If the host lacks Vulkan + SPIR-V material-preview shader artifacts, record a **blocker** in the validation table below; do not promote gap status or remove `requiredBeforeReadyClaim` rows on incomplete evidence.
- Greenfield only: no backward-compatibility shims for legacy snapshot shapes.

## Done when

- [x] This plan documents Goal / Context / Constraints / Done When and the validation evidence table.
- [x] `apply_editor_material_gpu_preview_execution_snapshot` sets a model field and retained UI id `material_asset_preview.gpu.execution.vulkan_visible_refresh` with values `complete`, `pending`, or `not-applicable` (ASCII, deterministic rules documented in code).
- [x] `MK_editor` shows the same evidence string in the material GPU preview section when appropriate.
- [x] `MK_editor_core_tests` cover D3D12-ready (`not-applicable`), Vulkan-ready with frames (`complete`), and at least one `pending` case; `tools/check-ai-integration.ps1` needles updated; `docs/editor.md` updated with one sentence; `engine/agent/manifest.json` `editor-productization` notes honestly mention this slice without claiming full display-parity gap closure.
- [x] `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes on the validation host, or blockers are recorded below.

## Validation evidence

| Check | Host / command | Result | Notes |
| --- | --- | --- | --- |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | 2026-05-10 Windows host: license/agents/json/ai-integration/production-readiness/ci/dependency/vcpkg/toolchain/cpp-standard/coverage/shader-toolchain (diagnostic-only blockers)/mobile (diagnostic-only)/apple-host (host-gated)/public-api/tidy (1 file)/build.ps1 + full `ctest --preset dev` (47 tests). |
| Editor core unit tests | `ctest --preset dev -R MK_editor_core_tests --output-on-failure` | PASS | Same host as slice validation. |
| Vulkan visible smoke | Manual `MK_editor` with Vulkan backend + SPIR-V preview artifacts | Not run in CI slice | Host-gated optional row. |

## Contract labels

- Existing: `ge.editor.material_gpu_preview_execution.v1` (unchanged).
- Vulkan visible refresh values (retained row `material_asset_preview.gpu.execution.vulkan_visible_refresh` text): `complete` \| `pending` \| `not-applicable`.
