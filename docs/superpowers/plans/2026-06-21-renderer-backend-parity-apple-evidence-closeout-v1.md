# Renderer Backend Parity Apple Evidence Closeout v1 Implementation Plan

**Plan ID:** `renderer-backend-parity-apple-evidence-closeout-v1`
**Status:** Completed.
**Closeout:** Completed through PR #710 / merge commit `e74841d47489db8948cdda05b10fb231dd1606c7`; `currentActivePlan` returned to the production-completion master plan with `recommendedNextPlan.id = next-production-gap-selection`.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Select and implement the next clean-break Apple/Metal evidence step for `renderer-backend-parity-v1` by connecting Apple-host Metal feature evidence to backend-local parity proof rows without promoting broad renderer, Metal, or native-handle readiness by inference.

**Architecture:** Keep `MK_renderer` backend-neutral and keep Apple SDK / Objective-C++ work inside `MK_rhi_metal` and reviewed validation recipes. The active slice uses value-only `BackendRendererParityProofRow` evidence and `renderer-metal-apple-host-evidence` counters as the boundary between Metal host proof and renderer parity policy. It does not introduce compatibility aliases, public native handles, SDL3, or a direct `MK_renderer` dependency on `MK_rhi_metal`.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_rhi_metal`, Metal / Metal Shading Language, Xcode `xcrun metal` / `metallib`, CMake presets, PowerShell 7 repository validation wrappers, Context7 `/dogukanveziroglu/metal-shading-language-specification`, and Apple Metal Developer Documentation.

---

## Official Source Gate

The implementation must keep these source facts visible in code review evidence:

- Apple Metal command submission is device/queue/command-buffer/encoder based. A command buffer owns render, compute, and blit encoders; only one encoder is active on a command buffer at a time, and encoders must end before the next encoder starts. Source: <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Cmd-Submiss/Cmd-Submiss.html>.
- Render evidence must include a `MTLRenderPipelineState` path and render-pass attachment path before a render proof row is ready. Source: <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Render-Ctx/Render-Ctx.html>.
- Compute evidence must include a compute encoder, compute pipeline, resource arguments, and dispatch proof before a compute proof row is ready. Source: <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Compute-Ctx/Compute-Ctx.html>.
- Resource evidence must treat textures and buffers as Metal-owned GPU memory objects and require explicit copy/readback paths for host-observable proof. Source: <https://developer.apple.com/library/archive/documentation/Miscellaneous/Conceptual/MetalProgrammingGuide/Mem-Obj/Mem-Obj.html>.
- Feature evidence must be capability-family specific. The May 21, 2026 Apple Metal Feature Set Tables record ASTC, cube-map arrays, function specialization, memory barriers, memory coherence, command barriers, and Metal 4 capabilities by Apple GPU family; a proof row must name the selected feature and must not infer broader families. Source: <https://developer.apple.com/metal/capabilities/>.
- Context7 MSL evidence: use `[[vertex]]`, `[[fragment]]`, and `[[kernel]]` entry attributes; use `device`, `constant`, and `threadgroup` address spaces as appropriate; resource bindings and function constants must use explicit indices; unsupported MSL features such as RTTI, exceptions, `new`/`delete`, virtual classes, and `dynamic_cast` must not appear in generated proof shaders. Source: Context7 `/dogukanveziroglu/metal-shading-language-specification`.

## Current Boundary

- Existing `BackendRendererParityProofRow::host_validation_recipe_id` already fails closed with `missing_host_validation_recipe` and `unreviewed_host_validation_recipe`.
- Existing reviewed Apple host recipe id is `renderer-metal-apple-host-evidence`.
- Existing Metal environment feature evidence in `MK_rhi_metal` produces seven selected rows: `physical_sky`, `height_fog`, `cloud_layer`, `precipitation`, `volumetric_fog`, `volumetric_cloud`, and `environment_lighting_ibl`.
- Existing recipe output includes `metal_environment_runtime_ready=1`, `metal_environment_command_queue_ready=1`, `metal_environment_metallib_valid=1`, `metal_environment_render_pipeline_ready=1`, `metal_environment_compute_pipeline_ready=1`, `metal_environment_render_pass_ready=1`, `metal_environment_cube_texture_ready=1`, `metal_environment_hdr_texture_ready=1`, `metal_environment_depth_texture_ready=1`, `metal_environment_particle_buffer_ready=1`, `metal_environment_synchronization_evidence_ready=1`, `metal_environment_render_readback_nonzero=1`, `metal_environment_compute_readback_nonzero=1`, and `metal_environment_native_handle_access=0`.
- Broad `BackendRendererParityPolicyStatus::ready`, broad Metal readiness, broad renderer quality, platform parity, commercial readiness, and broad `environment_ready` stay out of scope for this plan unless a later task explicitly supplies every required backend-local row and hosted evidence.

## File Map

- Modify `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`: select this plan, keep `unsupportedProductionGaps = []`, and keep retained closeout evidence honest.
- Modify `engine/agent/manifest.json`: generated only by `tools/compose-agent-manifest.ps1 -Write`.
- Modify `docs/superpowers/plans/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`: active-plan navigation and exact non-claims.
- Modify `tools/check-ai-integration-030-runtime-rendering.ps1`: static guard for this selected plan id and source/evidence needles.
- Future implementation task modifies `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp` and `engine/renderer/src/backend_renderer_parity_policy.cpp` only if a backend-neutral helper is needed; it must not include `mirakana/rhi/metal/metal_backend.hpp`.
- Future focused tests live in `tests/unit/renderer_rhi_tests.cpp`.

## Task 1: Select Active Plan And Static Guard

**Files:**
- Create: `docs/superpowers/plans/2026-06-21-renderer-backend-parity-apple-evidence-closeout-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`

- [ ] **Step 1: Select the plan in the manifest fragment**

Set:

```json
"currentActivePlan": "docs/superpowers/plans/2026-06-21-renderer-backend-parity-apple-evidence-closeout-v1.md",
"id": "renderer-backend-parity-apple-evidence-closeout-v1",
"title": "Renderer Backend Parity Apple Evidence Closeout v1",
"status": "active-plan",
"path": "docs/superpowers/plans/2026-06-21-renderer-backend-parity-apple-evidence-closeout-v1.md"
```

The `reason` text must include these exact needles:

```text
Renderer Backend Parity Apple Evidence Closeout v1
renderer-backend-parity-v1
renderer-metal-apple-host-evidence
Apple/Metal host evidence
Windows/Vulkan proof must not promote Metal readiness
native handles remain hidden
unsupportedProductionGaps = []
```

- [ ] **Step 2: Add the static guard branch**

In `tools/check-ai-integration-030-runtime-rendering.ps1`, add `renderer-backend-parity-apple-evidence-closeout-v1` to the `recommendedNextPlan.id` allowlist and add an `elseif` branch that asserts the Step 1 needles from the combined `latestCloseoutEvidence`, `completedContext`, and `reason` strings.

- [ ] **Step 3: Compose the manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: command exits 0 and updates `engine/agent/manifest.json` only from fragments.

- [ ] **Step 4: Run focused static checks**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: each command exits 0 with `json-contract-check: ok`, `ai-integration-check: ok`, and `text-format-check: ok`.

## Task 2: Backend-Neutral Apple Evidence Row Mapping

**Files:**
- Modify: `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`
- Modify: `engine/renderer/src/backend_renderer_parity_policy.cpp`
- Test: `tests/unit/renderer_rhi_tests.cpp`

- [ ] **Step 1: Add failing test for selected Metal proof rows**

Add a test named:

```cpp
MK_TEST("backend renderer parity maps selected Apple Metal environment evidence to backend local proof rows")
```

The test must construct a backend-neutral descriptor with these booleans set to true:

```cpp
runtime_ready
command_queue_ready
shader_library_ready
render_pipeline_ready
compute_pipeline_ready
render_pass_ready
resource_evidence_ready
synchronization_evidence_ready
package_evidence_ready
render_readback_nonzero
compute_readback_nonzero
```

It must require:

```cpp
proofs.size() == 3U
proofs[0].feature == BackendRendererParityFeatureKind::synchronization
proofs[1].feature == BackendRendererParityFeatureKind::shader_validation
proofs[2].feature == BackendRendererParityFeatureKind::package_evidence
row.selected_backend == mirakana::rhi::BackendKind::metal
row.proof_backend == mirakana::rhi::BackendKind::metal
row.reviewed
row.host_validated
!row.host_gate_required
row.host_validation_recipe_id == "renderer-metal-apple-host-evidence"
!row.request_native_handle_access
```

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests
```

Expected before implementation: compile fails because the new descriptor/helper does not exist.

- [ ] **Step 2: Add backend-neutral helper**

Add:

```cpp
struct BackendRendererParityAppleMetalEnvironmentEvidenceDesc {
    bool runtime_ready{false};
    bool command_queue_ready{false};
    bool shader_library_ready{false};
    bool render_pipeline_ready{false};
    bool compute_pipeline_ready{false};
    bool render_pass_ready{false};
    bool resource_evidence_ready{false};
    bool synchronization_evidence_ready{false};
    bool package_evidence_ready{false};
    bool render_readback_nonzero{false};
    bool compute_readback_nonzero{false};
    bool native_handle_access{false};
    std::string host_validation_recipe_id{"renderer-metal-apple-host-evidence"};
};

[[nodiscard]] std::vector<BackendRendererParityProofRow>
make_backend_renderer_parity_apple_metal_environment_proofs(
    const BackendRendererParityAppleMetalEnvironmentEvidenceDesc& desc);
```

The helper returns exactly three proof rows only when all required booleans are ready and the recipe id is `renderer-metal-apple-host-evidence`; otherwise it returns host-gated Metal rows for the same three feature kinds or an empty vector when native handles are requested.

- [ ] **Step 3: Run focused renderer tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_renderer_tests
```

Expected after implementation: `MK_renderer_tests` passes.

## Task 3: Validation Recipe And Documentation Evidence

**Files:**
- Modify: `tools/validate-renderer-metal-apple.ps1`
- Modify: `tools/run-validation-recipe-plans.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`

- [ ] **Step 1: Keep recipe counters exact**

Ensure `tools/validate-renderer-metal-apple.ps1` still emits these counters:

```text
metal_environment_host_validation_recipe_id=renderer-metal-apple-host-evidence
metal_environment_ready_rows=7
metal_environment_host_gated_rows=0
metal_environment_native_handle_access=0
metal_environment_broad_environment_ready_claimed=0
```

Add no broad `metal_ready`, `backend_parity_ready`, `renderer_quality_ready`, or public native-handle counter.

- [ ] **Step 2: Update docs and skills**

Add a short paragraph to docs/skills saying selected Apple-host `renderer-metal-apple-host-evidence` can feed backend-local `synchronization`, `shader_validation`, and `package_evidence` proof rows through the new helper, while `memory_residency` and `profiling_capture` remain separate proof rows.

- [ ] **Step 3: Run agent-surface validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1
```

Expected: all checks pass, and production readiness still reports `unsupported_gaps=0`.

## Task 4: Slice Closeout

**Files:**
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`

- [ ] **Step 1: Run full validation for C++/runtime/public contract changes**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: full validation exits 0. If an Apple-host lane cannot execute locally, record the local blocker and rely only on hosted `macOS Metal CMake` checks for Apple proof.

- [ ] **Step 2: Publish through GitHub Flow**

Run before staging, push, PR creation, ready conversion, and auto-merge:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

Expected: `publication-preflight: ok`.

- [ ] **Step 3: Close active pointer after merge**

After hosted checks pass and the PR merges, return:

```json
"currentActivePlan": "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md",
"id": "next-production-gap-selection",
"title": "Production Completion Selection Gate",
"status": "selection-gate",
"path": "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
```

Keep the completed plan as retained evidence and keep `unsupportedProductionGaps = []`.

## Done When

- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` selects this plan during implementation and returns to the master plan only after the slice is merged.
- `renderer-metal-apple-host-evidence` remains the only reviewed recipe for the selected Apple-host renderer Metal environment proof helper.
- Backend-local Apple proof rows cover only `synchronization`, `shader_validation`, and `package_evidence`.
- `memory_residency`, `profiling_capture`, broad backend parity, broad Metal readiness, broad renderer quality, native handles, commercial readiness, and broad `environment_ready` remain unclaimed unless a later exact proof row supplies evidence.
- Focused renderer tests, agent/static checks, and final slice validation pass or record a concrete host/tool blocker.
