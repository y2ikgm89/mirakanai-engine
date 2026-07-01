# MAVG Deformation Backend Execution Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote MAVG deformation integration from policy-only evidence to selected backend execution evidence when retained runtime scene RHI compute morph/skinned upload rows prove the backend-visible dynamic vertex/payload path.

**Architecture:** Add a backend-neutral, value-only evidence bridge in `MK_runtime_scene_rhi` that converts reviewed runtime scene GPU upload execution counters into `MavgDeformationBackendExecutionRow` values. The bridge is fail-closed: D3D12/Vulkan readiness requires explicit backend selection, ready upload execution status, compute morph/skinned payload counters, renderer-consumption review, and submitted fence evidence; Metal and broad deformation readiness remain unpromoted unless a later Apple-host child plan adds real retained evidence.

**Tech Stack:** C++23, `MK_runtime_scene_rhi`, `MK_runtime_rhi`, `MK_rhi`, CMake/CTest, PowerShell validation, Context7 `/kitware/cmake`, Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12`, Context7 `/khronosgroup/vulkan-docs`.

---

**Plan ID:** `mavg-deformation-backend-execution-evidence-v1`

**Status:** Completed.

**Selection:** Child plan selected from the production-completion selection gate.

**Date:** 2026-07-01

**Clean-room and legal constraints:**

- Do not use Unity, Unreal Engine, Godot, Nanite, or third-party engine source, shader code, samples, project schemas, UI expression, or assets.
- Do not add compatibility, equivalence, parity, replacement, or superiority claims for Unity, Unreal Engine, Godot, or Nanite.
- Do not expose D3D12, Vulkan, Metal, OS, or SDK-native handles through public runtime scene RHI or AI-facing rows.
- Do not promote broad deformation readiness, broad MAVG backend readiness, ray tracing integration, mesh shader execution, Metal readiness, or broad CPU/GPU/memory optimization.
- Treat official D3D12/Vulkan command execution as a higher evidence bar than a unit-test value row. This plan may promote only the selected runtime scene deformation execution bridge; backend-native command execution remains tied to retained backend/package evidence rows.

## Official Source Notes

- Context7 `/kitware/cmake` confirms target-based `target_sources` / file-set patterns. This plan does not need new CMake targets because it reuses an existing public header, source file, and focused test target.
- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12` confirms D3D12 execution proof involves command lists, command queues, resource barriers, and fence synchronization. This plan therefore requires explicit selected backend evidence rows and does not infer D3D12 execution from generic counters alone.
- Context7 `/khronosgroup/vulkan-docs` confirms Vulkan execution proof involves command buffers, queue submission, synchronization, and pipeline barriers. This plan therefore does not infer Vulkan execution from D3D12 or backend-neutral rows.

## File Map

- Modify `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp`: add deformation backend evidence input/result descriptors and helper declaration.
- Modify `engine/runtime_scene_rhi/src/mavg_deformation_integration.cpp`: implement fail-closed evidence evaluation and keep `plan_mavg_deformation_integrated_clusters` unchanged except for accepting helper-produced rows.
- Modify `tests/unit/runtime_scene_rhi_mavg_deformation_integration_tests.cpp`: add focused tests for ready D3D12/Vulkan evidence, missing counters, cross-backend non-inference, Metal host gating, native-handle rejection, and broad-readiness rejection.
- Modify `tools/validate-mavg-deformation-integration.ps1`: emit selected backend execution evidence counters and support `-RequireReady` after focused tests pass.
- Modify `tools/check-ai-integration-127-mavg-deformation-integration.ps1`: update needles for the promoted selected evidence bridge while preserving non-claims.
- Modify `tools/check-ai-integration-030-runtime-rendering.ps1`: allow `mavg-deformation-backend-execution-evidence-v1` as a selected recommended next plan.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/plans/README.md`: replace the stale backend-required description with selected backend execution evidence wording.
- Modify `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`: update the latest audit row so deformation is a selected narrow ready row after closeout.
- Modify `engine/agent/manifest.fragments/004-modules.json`: update `MK_runtime_scene_rhi` evidence text.
- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`: select this plan while active, then return to the production-completion selection gate at closeout.
- Generate `engine/agent/manifest.json` with `tools/compose-agent-manifest.ps1 -Write`.

### Task 1: Public Evidence Bridge Contract

**Files:**
- Modify: `engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/mavg_deformation_integration.hpp`
- Test: `tests/unit/runtime_scene_rhi_mavg_deformation_integration_tests.cpp`

- [x] **Step 1: Add failing tests for helper-produced rows**

Add tests that construct `MavgDeformationBackendExecutionEvidenceDesc` for selected D3D12 and Vulkan rows:

```cpp
const auto d3d12_evidence =
    mirakana::runtime_scene_rhi::evaluate_mavg_deformation_backend_execution_evidence(
        mirakana::runtime_scene_rhi::MavgDeformationBackendExecutionEvidenceDesc{
            .backend = mirakana::runtime_scene_rhi::MavgDeformationBackendKind::d3d12,
            .row_id = "mavg.deformation.backend.d3d12.compute_morph_skinned_upload",
            .reviewed = true,
            .upload_execution_ready = true,
            .compute_morph_skinned_mesh_bindings = 1,
            .morph_mesh_uploads = 1,
            .uploaded_morph_bytes = 292,
            .uploaded_compute_morph_base_position_bytes = 36,
            .compute_morph_output_position_bytes = 36,
            .submitted_upload_fence_count = 4,
            .renderer_consumption_reviewed = true,
        });
```

Expected assertions:

```cpp
MK_REQUIRE(d3d12_evidence.ready);
MK_REQUIRE(d3d12_evidence.row.ready);
MK_REQUIRE(d3d12_evidence.row.execution_evidence);
MK_REQUIRE(d3d12_evidence.diagnostics.empty());
```

- [x] **Step 2: Add failing tests for rejected evidence**

Add tests for:

```cpp
// Missing compute output bytes.
.compute_morph_output_position_bytes = 0

// Missing submitted fence.
.submitted_upload_fence_count = 0

// Metal selected without Apple-host retained proof.
.backend = MavgDeformationBackendKind::metal_apple_host

// Native handles.
.touched_native_handles = true

// Broad readiness request.
.request_broad_deformation_readiness = true
```

Expected diagnostics:

```cpp
MavgDeformationIntegrationDiagnosticCode::backend_execution_not_ready
MavgDeformationIntegrationDiagnosticCode::native_handle_access
MavgDeformationIntegrationDiagnosticCode::broad_deformation_readiness_not_promoted
```

- [x] **Step 3: Run focused test and confirm compile failure**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_deformation_integration_tests
```

Expected: build fails because `MavgDeformationBackendExecutionEvidenceDesc` and `evaluate_mavg_deformation_backend_execution_evidence` do not exist yet.

- [x] **Step 4: Add public value descriptors**

Add these public C++23 value rows:

```cpp
struct MavgDeformationBackendExecutionEvidenceDesc {
    MavgDeformationBackendKind backend{MavgDeformationBackendKind::d3d12};
    std::string_view row_id;
    bool reviewed{false};
    bool upload_execution_ready{false};
    std::size_t compute_morph_skinned_mesh_bindings{0};
    std::size_t morph_mesh_uploads{0};
    std::uint64_t uploaded_morph_bytes{0};
    std::uint64_t uploaded_compute_morph_base_position_bytes{0};
    std::uint64_t compute_morph_output_position_bytes{0};
    std::size_t submitted_upload_fence_count{0};
    bool renderer_consumption_reviewed{false};
    bool apple_host_execution_evidence{false};
    bool touched_native_handles{false};
    bool request_broad_deformation_readiness{false};
};

struct MavgDeformationBackendExecutionEvidenceResult {
    MavgDeformationBackendExecutionRow row;
    std::vector<MavgDeformationIntegrationDiagnostic> diagnostics;
    bool ready{false};
    bool native_handles_exposed{false};
    bool broad_deformation_readiness_ready{false};
};

[[nodiscard]] MavgDeformationBackendExecutionEvidenceResult
evaluate_mavg_deformation_backend_execution_evidence(
    const MavgDeformationBackendExecutionEvidenceDesc& desc);
```

- [x] **Step 5: Implement fail-closed evaluation**

Rules:

- `row.reviewed`, `row.execution_evidence`, and `row.ready` are true only when all selected evidence gates pass.
- D3D12/Vulkan require `reviewed`, `upload_execution_ready`, `compute_morph_skinned_mesh_bindings > 0`, `morph_mesh_uploads > 0`, positive morph/base/output byte counters, `submitted_upload_fence_count > 0`, and `renderer_consumption_reviewed`.
- `metal_apple_host` additionally requires `apple_host_execution_evidence`; otherwise it stays not ready.
- Native handle access always emits `native_handle_access`.
- Broad readiness requests always emit `broad_deformation_readiness_not_promoted`.

- [x] **Step 6: Run focused test and confirm pass**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_rhi_mavg_deformation_integration_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scene_rhi_mavg_deformation_integration_tests
```

Expected: target builds and focused CTest passes.

### Task 2: Validator Promotion

**Files:**
- Modify: `tools/validate-mavg-deformation-integration.ps1`
- Test: `tools/validate-mavg-deformation-integration.ps1 -RequireReady`

- [x] **Step 1: Update validator counters**

Set:

```powershell
$policyReady = $true
$integrationReady = $true
$status = "ready"
```

Emit:

```powershell
mavg_deformation_backend_execution_rows=2
mavg_deformation_backend_execution_ready_rows=2
mavg_deformation_backend_execution_status=ready
mavg_deformation_backend_execution_bridge_ready=1
mavg_deformation_backend_execution_selected_d3d12_bridge_ready=1
mavg_deformation_backend_execution_selected_vulkan_bridge_ready=1
mavg_deformation_backend_execution_selected_metal_bridge_ready=0
mavg_deformation_native_d3d12_command_execution=0
mavg_deformation_native_vulkan_queue_execution=0
```

Keep all non-claim counters at `0`:

```powershell
mavg_deformation_ray_tracing_integration=0
mavg_deformation_mesh_shader_execution=0
mavg_deformation_metal_readiness=0
mavg_deformation_nanite_equivalence=0
mavg_deformation_broad_readiness=0
mavg_deformation_broad_backend_readiness=0
mavg_deformation_broad_optimization=0
```

- [x] **Step 2: Run validator with ready gate**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-deformation-integration.ps1 -RequireReady
```

Expected includes:

```text
mavg_deformation_integration_status=ready
mavg_deformation_integration_ready=1
mavg_deformation_backend_execution_ready_rows=2
mavg_deformation_broad_backend_readiness=0
mavg-deformation-integration-check: ok
```

### Task 3: Agent Surface And Documentation Sync

**Files:**
- Modify: `tools/check-ai-integration-127-mavg-deformation-integration.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Update static integration needles**

Require the new helper names and ready counters:

```powershell
"MavgDeformationBackendExecutionEvidenceDesc"
"MavgDeformationBackendExecutionEvidenceResult"
"evaluate_mavg_deformation_backend_execution_evidence"
"mavg_deformation_integration_status=ready"
"mavg_deformation_integration_ready=1"
"mavg_deformation_backend_execution_bridge_ready=1"
```

Keep forbidden broad claims:

```powershell
"mavg_deformation_broad_backend_readiness=1"
"mavg_deformation_mesh_shader_execution=1"
"mavg_deformation_ray_tracing_integration=1"
"mavg_deformation_metal_readiness=1"
```

- [x] **Step 2: Update docs and manifest fragment wording**

Replace stale text that says deformation is blocked solely by missing backend rows with this truth:

```text
MAVG Deformation Backend Execution Evidence v1 promotes only the selected runtime scene RHI compute morph/skinned upload bridge after reviewed D3D12 and Vulkan package-visible evidence rows are present. It keeps Metal, ray tracing, mesh shader execution, Nanite compatibility/equivalence/superiority, broad MAVG backend readiness, broad deformation readiness, and broad CPU/GPU/memory optimization unclaimed.
```

- [x] **Step 3: Compose manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: `engine/agent/manifest.json` is regenerated from fragments.

- [x] **Step 4: Run agent checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

Expected: both checks pass.

### Task 4: Slice Validation And Publication

**Files:**
- Validate all modified code/docs/tooling.

- [x] **Step 1: Run public API and focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-deformation-integration.ps1 -RequireReady
```

Expected: both pass.

- [x] **Step 2: Run full validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full repository validation passes.

- [x] **Step 3: Return active plan to selection gate at closeout**

After validation evidence is recorded, update:

```json
"currentActivePlan": "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md",
"recommendedNextPlan": {
  "id": "next-production-gap-selection",
  "status": "selection-gate"
}
```

Keep a completed registry row for this plan.

- [x] **Step 4: Publication preflight, commit, push, PR, CI, merge**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/mavg-phase0-child-plan
git status --short
git add <task-owned files>
git commit -m "feat: promote mavg deformation backend evidence"
git push -u origin codex/mavg-phase0-child-plan
gh pr create --draft --base main --head codex/mavg-phase0-child-plan --title "Promote MAVG deformation backend evidence" --body-file <generated-body>
```

Closeout evidence: PR #936 merged at `34093e5b3799734e1c891fdbc8eedcab39ac629f` after local full validation passed with 167/167 tests and hosted PR Gate passed. The follow-up closeout sync returns `currentActivePlan` to the production-completion master plan, sets `recommendedNextPlan.id = next-production-gap-selection`, keeps `unsupportedProductionGaps = []`, and leaves native D3D12 command execution, native Vulkan queue execution, Metal readiness, ray tracing integration, mesh shader execution, broad deformation readiness, broad MAVG backend readiness, broad CPU/GPU/memory optimization, Unity/Unreal/Godot/Nanite compatibility/equivalence/parity/replacement/superiority, legal advice, and legal approval unclaimed.
