# MAVG Streamed Cluster GPU Upload v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow runtime-RHI bridge that turns committed MAVG cluster streaming safe-point adoption rows into package-visible GPU upload/binding evidence.

**Architecture:** Build a new `MK_runtime_rhi` helper beside `mavg_conventional_upload` and `mavg_cluster_streaming_safe_point_adoption`. The helper validates a completed safe-point adoption result, resident catalog visibility, graph/page/cluster ranges, and caller-owned per-page runtime mesh payload rows, then delegates actual buffer creation to the existing public `upload_runtime_mesh` path without mutating streaming state again.

**Tech Stack:** C++23, `MK_runtime`, `MK_runtime_rhi`, `MK_rhi` Null RHI, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-streamed-cluster-gpu-upload-v1`

**Status:** Active implementation slice.

**Date:** 2026-06-11

**Context:** PR #576 completed MAVG Cluster Streaming Safe Point Adoption v1 through merge commit `7810ca10`, giving caller-owned `RuntimeResidentPackageMountSetV2` / `RuntimeResidentCatalogCacheV2` adoption evidence. This slice depends on that committed safe-point result and must not reload candidates or re-mutate mounts/catalog state.

**Evidence Contract:** MAVG Streamed Cluster GPU Upload v1 (`mavg-streamed-cluster-gpu-upload-v1`) exposes `mavg_streamed_cluster_gpu_upload.hpp`, `RuntimeMavgStreamedClusterGpuUploadResult`, and `upload_runtime_mavg_streamed_cluster_pages` so committed `RuntimeMavgClusterStreamingSafePointAdoptionResult` rows plus `RuntimeResidentCatalogCacheV2` resident mesh rows and caller-owned page payloads can publish page-level `MeshGpuBinding` rows.

**Constraints:**

- No DirectStorage, autonomous/background streaming service, async-overlap/performance proof, backend execution, backend draw execution, mesh shaders, mesh shader execution, native handle exposure, Nanite compatibility/equivalence/superiority, or broad optimization claim.
- Use TDD: add the new unit test file first and verify the target fails before adding production code.
- Keep the API value-oriented and public-header safe; no renderer/RHI native handles in result rows.
- Update docs, manifest fragments, composed manifest, and static guards only for the narrow claim.

**Done When:**

- `upload_runtime_mavg_streamed_cluster_pages` succeeds only after a committed safe-point adoption result and valid page payload rows produce package-visible `MeshGpuBinding` rows through existing runtime upload.
- Fail-closed tests cover uncommitted adoption, graph/page mismatch, missing resident catalog row, invalid cluster draw range, and no native-handle/backend/mesh-shader/DirectStorage claims.
- `MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests` builds and passes.
- Docs/specs/registry/master-plan/manifest/static checks describe the exact new evidence and preserve non-claims.
- Focused validation and final `tools/validate.ps1` pass before publication.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp`
- Create: `engine/runtime_rhi/src/mavg_streamed_cluster_gpu_upload.cpp`
- Create: `tests/unit/runtime_rhi_mavg_streamed_cluster_gpu_upload_tests.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.json` through `tools/compose-agent-manifest.ps1 -Write`
- Modify: `tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1`

## Tasks

### Task 1: Red Test And CMake Target

**Files:**
- Create: `tests/unit/runtime_rhi_mavg_streamed_cluster_gpu_upload_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add tests that include `mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp` and call `upload_runtime_mavg_streamed_cluster_pages`.
- [x] Register `MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests` linked to `MK_runtime_rhi`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests`.
- [x] Expected RED: build fails because the header/API does not exist.

### Task 2: Minimal Public API And Implementation

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_streamed_cluster_gpu_upload.hpp`
- Create: `engine/runtime_rhi/src/mavg_streamed_cluster_gpu_upload.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`

- [x] Add descriptor/result/diagnostic/page-payload structs and `upload_runtime_mavg_streamed_cluster_pages`.
- [x] Validate graph asset, graph document, committed safe-point adoption, resident catalog cache, and page payload rows before upload.
- [x] Upload each adopted page payload with `upload_runtime_mesh` and publish page-level `MeshGpuBinding` rows only after all validations for that page pass.
- [x] Run the target build and CTest until GREEN:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_streamed_cluster_gpu_upload_tests`

### Task 3: Docs, Manifest, And Static Guard

**Files:**
- Modify: docs/manifest/static-check files listed above

- [x] Record the slice as active/completed evidence without broad MAVG backend readiness.
- [x] Compose manifest with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Add static guard needles for the new header, result, upload function, and test target.
- [x] Run `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, and `tools/check-public-api-boundaries.ps1`.

### Task 4: Slice Validation And Publication

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run focused build/test/static checks.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`.
- [ ] Commit, push, and open a draft PR.
