# MAVG Ray Tracing Consistency Diagnostics v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add asset-level MAVG raster/ray-tracing consistency diagnostics so selected RT payload evidence is derived from the same cluster graph and fails closed before any backend RT execution claim.

**Architecture:** Keep this slice in `MK_assets` as value-only rows over a caller-owned `MavgClusterGraphDocument`. The implementation validates cluster membership, raster payload evidence, RT payload evidence, BLAS policy rows, fallback provenance, and explicit deformation-tier compatibility without creating D3D12/Vulkan acceleration structures, dispatching rays, mutating packages, or exposing native handles.

**Tech Stack:** C++23, `MK_assets`, CMake/CTest, PowerShell validation tools, official Microsoft DXR and Khronos Vulkan acceleration-structure terminology as constraints.

---

**Plan ID:** `mavg-ray-tracing-consistency-diagnostics-v1`

**Date:** 2026-06-12

**Context:** This is a focused child of `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md` Phase 8. It is independent from active draft PRs for backend draw execution, background streaming, DirectStorage page IO, upload-overlap evidence, mesh-shader capability planning, and deformation-tier diagnostics except for normal docs/manifest closeout conflicts.

**Constraints:**

- Do not add D3D12/Vulkan ray-tracing backend execution.
- Do not add acceleration-structure allocation, build, refit, serialization, native handles, shaders, package mutation, file IO, or renderer/RHI dependencies.
- Do not claim RT parity, Metal readiness, Nanite compatibility/equivalence/superiority, broad renderer readiness, or broad optimization.
- Keep the API deterministic and host-independent.

## Files

- Create: `engine/assets/include/mirakana/assets/mavg_ray_tracing_consistency.hpp`
- Create: `engine/assets/src/mavg_ray_tracing_consistency.cpp`
- Create: `tests/unit/mavg_ray_tracing_consistency_tests.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify at closeout: `docs/current-capabilities.md`
- Modify at closeout: `docs/roadmap.md`
- Modify at closeout: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify at closeout: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify at closeout: `docs/superpowers/plans/README.md`
- Modify at closeout: `engine/agent/manifest.fragments/004-modules.json`
- Modify at closeout: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate at closeout: `engine/agent/manifest.json`
- Create at closeout: `tools/check-ai-integration-122-mavg-ray-tracing-consistency-diagnostics.ps1`

### Task 1: RED Test And Target

- [x] **Step 1: Add a failing test target**

Add `MK_mavg_ray_tracing_consistency_tests` to the root `CMakeLists.txt` and create `tests/unit/mavg_ray_tracing_consistency_tests.cpp` that includes `mirakana/assets/mavg_ray_tracing_consistency.hpp`.

- [x] **Step 2: Verify RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_ray_tracing_consistency_tests
```

Expected: fail because `mirakana/assets/mavg_ray_tracing_consistency.hpp` does not exist.

### Task 2: Public Asset Contract

- [x] **Step 1: Add the public header**

Define:

- `MavgRayTracingPayloadPolicy`
- `MavgRayTracingDeformationTier`
- `MavgRayTracingConsistencyDiagnosticCode`
- `MavgRasterClusterPayloadRow`
- `MavgRayTracingClusterPayloadRow`
- `MavgRayTracingConsistencyDesc`
- `MavgRayTracingConsistencyRow`
- `MavgRayTracingConsistencyResult`
- `plan_mavg_ray_tracing_consistency`
- `has_mavg_ray_tracing_consistency_diagnostic`

- [x] **Step 2: Add minimal implementation**

Implement deterministic validation and row sorting by `graph_asset.value`, `cluster_index`, and policy enum value.

- [x] **Step 3: Verify GREEN**

Run the focused build and CTest target.

### Task 3: Consistency Diagnostics

- [x] **Step 1: Add tests for success**

Cover static clusters where raster and RT payload rows share the same graph asset, cluster index, material partition, draw range, payload page, fallback cluster, and supported BLAS build policy.

- [x] **Step 2: Add tests for fail-closed diagnostics**

Cover missing cluster graph, unknown cluster row, duplicate raster payload row, duplicate RT payload row, missing raster payload row, missing RT payload row, raster/RT payload mismatch, fallback mismatch, unsupported deformation tier for RT refit/rebuild, and unsupported dynamic displacement tier.

- [x] **Step 3: Verify focused tests**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_ray_tracing_consistency_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev -R MK_mavg_ray_tracing_consistency_tests --output-on-failure
```

### Task 4: Docs, Manifest, Static Guard

- [x] **Step 1: Update documentation**

Record the new asset-level RT consistency diagnostics and explicit non-claims in current capabilities, roadmap, MAVG architecture spec, master plan, and plan registry.

- [x] **Step 2: Update manifest fragments and compose**

Edit `engine/agent/manifest.fragments/004-modules.json` and `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`, then run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools\compose-agent-manifest.ps1 -Write
```

- [x] **Step 3: Add static guard**

Create `tools/check-ai-integration-122-mavg-ray-tracing-consistency-diagnostics.ps1` and wire it through existing numeric-prefix discovery by relying on `tools/check-ai-integration.ps1`.

- [x] **Step 4: Validate closeout**

Run focused build/test, `check-tidy -Files`, `check-ai-integration`, `check-json-contracts`, `check-public-api-boundaries`, `check-format`, `check-agents`, `git diff --check`, full `tools\validate.ps1`, publication preflight, commit, push, and draft PR.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\cmake.ps1 --build --preset dev --target MK_mavg_ray_tracing_consistency_tests` failed during RED before the public header existed, then passed after adding the API and implementation.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\ctest.ps1 --preset dev -R MK_mavg_ray_tracing_consistency_tests --output-on-failure`: `100% tests passed, 0 tests failed out of 1`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-tidy.ps1 -Files engine/assets/src/mavg_ray_tracing_consistency.cpp,tests/unit/mavg_ray_tracing_consistency_tests.cpp`: `tidy-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-ai-integration.ps1`: `ai-integration-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-format.ps1`: `text-format-check: ok`, `format-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-json-contracts.ps1`: `agent-manifest-compose: ok`, `json-contract-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-public-api-boundaries.ps1`: `public-api-boundaries-check: ok`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\check-agents.ps1`: `agent-surface-check: ok`.
- `git diff --check`: no whitespace errors.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools\validate.ps1`: `100% tests passed, 0 tests failed out of 119`; `validate: ok`. Diagnostic-only host gates remained expected: Metal tools unavailable on Windows, Apple host evidence requires macOS/Xcode, Android release signing not configured, and Android device smoke not connected.
