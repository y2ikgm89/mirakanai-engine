# Phase 4–5 Production Closure Milestone — Evidence Index v1 (2026-05-09)

**Plan ID:** `phase-4-5-milestone-closure-evidence-index-v1`  
**Parent:** [2026-05-09-phase-4-5-closure-milestone-v1.md](2026-05-09-phase-4-5-closure-milestone-v1.md)  
**Status:** Completed.

## Purpose

Map [Phase 4 / Phase 5 milestone completion definitions](2026-05-09-phase-4-5-closure-milestone-v1.md) to **repository evidence** (plans, tests, manifests, docs). This index **does not** upgrade manifest unsupported-gap rows to “ready” without the cited validation; host-gated editor/GPU lanes remain explicitly gated.

## Phase 4 — evidence mapping

| Item | Milestone requirement (summary) | Evidence |
| --- | --- | --- |
| **4.1** | Quality/diagnostics tied to first-step renderer package quality path | Renderer Package Quality Gates v1 (`evaluate_sdl_desktop_presentation_quality_gate`, package-visible `renderer_quality_*` / presentation reports); see `docs/current-capabilities.md`, `engine/agent/manifest.json` authoring surfaces. |
| **4.2** | MaterialGraph → Material package binding re-validatable | `plan_material_graph_package_update` / `apply_material_graph_package_update` surfaces in `engine/agent/manifest.json` `authoringSurfaces`; regression coverage in `tests/unit/tools_tests.cpp` (material graph package update planning). Host-gated desktop smoke remains optional lane-specific (`DesktopRuntimeMaterialShaderPackage` template / package recipes per manifest). |
| **4.3** | Editor preview Vulkan/Metal parity **tracked** | [2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md](2026-05-09-phase-4-5-milestone-editor-material-preview-vulkan-parity-tracking-v1.md); prior editor preview diagnostics/GPU evidence plans (2026-05-07). Execution proofs remain **host-gated**. |
| **4.4** | Frame graph / upload foundation narrow series | Frame Graph v0/v1 compile-only contracts and schedules (`tests/unit/renderer_rhi_tests.cpp`); Frame Graph RHI execution plans elsewhere; RHI upload stale-generation diagnostics ([2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md](2026-05-08-rhi-upload-stale-generation-diagnostics-v1.md)); RHI resource lifetime retired-handle contract ([2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md](2026-05-09-rhi-resource-lifetime-retired-handle-invalid-contract-v1.md)). |

## Phase 5 — evidence mapping

| Item | Milestone requirement (summary) | Evidence |
| --- | --- | --- |
| **5.1** | Registered-source cook alignment + reproducible explicit / closure paths | `schemas/game-agent.schema.json` + `engine/agent/manifest.json` `registeredSourceAssetCookTargets` / `cook-registered-source-assets`; `tests/unit/tools_tests.cpp` (explicit selection, registry closure expansion, validation failures). |
| **5.2** | Reviewed codec import coherence + diagnostics + E2E-style tool coverage | `ExternalAssetImportAdapters` routing and executor tests in `tests/unit/tools_tests.cpp` (`asset import executor routes external sources…`, PNG/gltf/wav fixtures); editor Content Browser import diagnostics plans (see `docs/current-capabilities.md`). Full importer productization remains **out of scope** per milestone. |
| **5.3** | 2D vertical-slice production tooling boundary | Explicitly **out of scope** for this milestone (see milestone §Out of scope); deterministic cook/static recipes only. |

## Milestone-wide validation

| Step | Command | Result |
| --- | --- | --- |
| Repository validate | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Required green at milestone close |
