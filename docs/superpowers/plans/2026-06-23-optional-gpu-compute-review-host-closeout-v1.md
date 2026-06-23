# 2026-06-23 Optional GPU Compute Review Host Closeout v1 Implementation Plan

Plan ID: `optional-gpu-compute-review-host-closeout-v1`

## Goal

Close `optional-gpu-compute-review-host` for the selected `runtime-rendering-or-simulation-rhi-compute` review candidate using retained real strict Vulkan timestamp/validation artifacts, without adding CUDA/HIP/SYCL dependencies or claiming broad GPU compute readiness.

## Context

`Optional GPU Compute Review Collector v1` added the collector and checker, but intentionally left the host gate open until real complete candidate bundles existed. The selected closeout bundle reuses retained official-style strict Vulkan evidence from the environment weather simulation stress workload: Vulkan timestamp query data, `VK_LAYER_KHRONOS_validation` visibility, Chrome trace summary, data-transfer counters, memory-residency counters, queue/barrier synchronization wording, dependency gate text, and first-party RHI fallback evidence.

Official-source boundary:

- Vulkan queue work needs explicit ordering and synchronization; no cross-queue or cross-backend inference is allowed.
- D3D12 queue work needs explicit fence/timeline synchronization; D3D12 proof does not promote Vulkan or Metal.
- CUDA/HIP/SYCL remain optional private-adapter or tooling research unless selected with their own install/runtime/profiler evidence.

## Constraints

- Do not introduce CUDA/HIP/SYCL runtime dependencies, `vcpkg.json` features, CMake linkage, default validation dependencies, public native/vendor handles, broad GPU compute, async overlap, cross-vendor parity, cross-backend parity, or broad CPU/GPU/memory optimization claims.
- Do not mark offline acceleration, CUDA/HIP private adapter, SYCL private adapter, or default vendor runtime compute ready from the selected RHI evidence.
- Keep retained profiler artifacts under `artifacts/performance/optional-gpu-compute-review/2026-06-23-rhi-compute-vulkan-weather-simulation`.

## Done When

`tools/validate-optional-gpu-compute-review-host-gate.ps1` validates the selected evidence bundle with `tools/check-optional-gpu-compute-review-evidence.ps1 -RequiredCandidateIds runtime-rendering-or-simulation-rhi-compute -RequireReady`, emits `optional_gpu_compute_review_ready=1`, `optional_gpu_compute_review_missing_candidate_rows=0`, `optional_gpu_compute_review_profiler_artifact_rows=2`, and all side-effect counters at `0`; `optional-gpu-compute-review-host` becomes `ready` / `ready`; static checks, docs, manifest fragments, composed manifest, and validation recipe text match the retained evidence.

## Validation

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-optional-gpu-compute-review-host-gate.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-optional-gpu-compute-review-host-gate-closeout.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
