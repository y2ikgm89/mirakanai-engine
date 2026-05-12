# Editor Viewport Vulkan RHI Bootstrap v1 (2026-05-10)

**Plan ID:** `editor-viewport-vulkan-rhi-bootstrap-v1`  
**Gap:** `editor-productization`  
**Status:** Completed.

## Goal

Enable `MK_editor` on Windows to select **Vulkan** as the viewport RHI backend using a headless `VulkanRuntimeDevice`, `minimal_irhi_device_mapping_plan`, and compiled viewport/material-preview **SPIR-V** artifacts, narrowing the manifest gap on Vulkan material-preview display parity without claiming Metal parity or full Unity-class editor UX.

## Context

- `EditorRenderBackendAvailability::vulkan` was never true; `create_viewport_rhi_device` returned `NullRhiDevice` for Vulkan.
- `VulkanRhiDevice` creation requires a complete `VulkanRhiDeviceMappingPlan`; `minimal_irhi_device_mapping_plan()` centralizes the same contract as `backend_scaffold_tests` (`ready_vulkan_rhi_mapping_plan`).
- Viewport and material-preview shader compile requests already emit `.spv` artifacts beside DXIL.

## Constraints

- Scope to Windows editor executable linking (`MK_EDITOR_ENABLE_VULKAN` with `MK_rhi_vulkan`).
- Do not claim Metal editor viewport parity.
- Keep `editor/core` free of Vulkan headers; shell-only wiring in `MK_editor` (`main.cpp`).

## Done When

- Loader probe + viewport SPIR-V readiness gates `availability.vulkan`.
- `create_viewport_rhi_device(Vulkan)` returns a non-null `IRhiDevice` when runtime device creation succeeds.
- Viewport and material GPU preview load SPIR-V when the active backend is Vulkan.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes.

## Validation Evidence

| Check | Command / note |
| --- | --- |
| Repository | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` — **ok** (2026-05-10); includes `build.ps1`, default `ctest --preset dev` (30 tests). Note: default `dev` preset leaves `MK_ENABLE_DESKTOP_GUI=OFF`, so `MK_editor.exe` is not built unless configured with `-DMK_ENABLE_DESKTOP_GUI=ON`; `mirakana_rhi_vulkan` and editor-dependent targets built and linked as part of validate. |
