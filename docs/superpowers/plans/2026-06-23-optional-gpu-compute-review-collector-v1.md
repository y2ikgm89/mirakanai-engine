# 2026-06-23 Optional GPU Compute Review Collector v1 Implementation Plan

Plan ID: `optional-gpu-compute-review-collector-v1`
Status: Completed.

## Goal

Add a first-party, host-owned Optional GPU Compute Review evidence collector/importer for the existing `host-optional-gpu-compute-review` recipe so operators can produce `evidence.json` bundles that `tools/check-optional-gpu-compute-review-evidence.ps1` validates without adding default vendor GPU dependencies or claiming GPU compute readiness.

## Context

The production loop already exposes `aiOperableProductionLoop.optionalGpuComputeReview`, `optional-gpu-compute-review-host`, and `host-optional-gpu-compute-review` as review-only, host-gated evidence contracts. Before this slice, the repository could validate attached bundles but did not provide a reviewed first-party script for shaping host-owned official GPU profiler artifacts into the required evidence row.

Official source checks:

- Context7 `/websites/nvidia_cuda` was queried for CUDA Toolkit tooling and device/profiler discovery surfaces such as `nvidia-smi`, `nvcc`, Nsight Systems, and Nsight Compute.
- Context7 `/khronosgroup/vulkan-tools` was queried for `vulkaninfo` host/runtime inspection and JSON/summary output.
- Context7 `/khronosgroup/sycl_reference` was queried for SYCL device enumeration, device aspects, queues, and event/profiling concepts.
- Existing manifest references retain NVIDIA CUDA Best Practices, AMD HIP asynchronous behavior, Khronos SYCL 2020, Khronos Vulkan queue-family documentation, and Microsoft D3D12 multi-engine synchronization as the official review categories.

## Constraints

- Do not execute arbitrary workload commands.
- Do not run GPU profilers, launch PIX/Nsight/ROCm tools, or start GPU captures from default validation.
- Do not make CUDA, HIP, SYCL, Vulkan, D3D12, PIX, Nsight, ROCm, or vendor-profiler installation a default validation dependency.
- Do not mark `optional-gpu-compute-review-host` ready from synthetic or local diagnostic-only evidence.
- Do not claim broad GPU compute, GPU async overlap, cross-vendor parity, cross-backend parity, RHI compute execution, CUDA/HIP/SYCL runtime dependency readiness, or broad CPU/GPU/memory optimization.
- Keep artifacts host-owned and referenced by safe repo-relative paths under the selected evidence root.

## Implementation

- [x] Add `tools/collect-optional-gpu-compute-review-evidence.ps1` with `Plan` and `Import` modes.
- [x] `Plan` mode reports host/tool availability and the target evidence path without writing evidence.
- [x] `Import` mode accepts an existing profiler summary artifact plus candidate classification, selected backend, workload recipe, synchronization, stream/event usage, dependency/install gate, and scalar/RHI fallback evidence, then writes one `evidence.json` row with explicit zero side-effect counters.
- [x] Add `tools/check-optional-gpu-compute-review-collector.ps1` as a default static validation guard using synthetic ignored artifacts under `out/` only.
- [x] Add the collector guard to `tools/validate.ps1`.
- [x] Update manifest recipe text, game-code guidance, docs, plan registry, and static checks.

## Done When

The collector self-test passes, generated synthetic evidence validates with `tools/check-optional-gpu-compute-review-evidence.ps1`, manifest/static checks require both the collector and validator surfaces, full validation passes, and `unsupportedProductionGaps` remains `[]`. This collector plan by itself does not mark `optional-gpu-compute-review-host` ready from synthetic evidence; the later `optional-gpu-compute-review-host-closeout-v1` plan closes the selected RHI candidate only through retained real strict Vulkan artifacts.

## Validation

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-optional-gpu-compute-review-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-optional-gpu-compute-review-evidence.ps1 -ExpectedEvidenceCounters optional_gpu_compute_review_ready=0 optional_gpu_compute_review_host_gated=1 optional_gpu_compute_review_broad_optimization_claim=0
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```
