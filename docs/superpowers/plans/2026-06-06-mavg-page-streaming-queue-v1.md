# 2026-06-06 MAVG Page Streaming Queue v1 Implementation Plan

**Plan ID:** `mavg-page-streaming-queue-v1`

**Status:** Completed.

**Goal:** Add the first caller-reviewed MAVG page streaming queue surface so selector-produced `MavgLodPageRequest` rows can become deterministic package candidate rows and one selected row can drain at a host-owned safe point.

**Context:** `MAVG Runtime LOD Milestone v1` is already closed for selected static conventional LOD readiness. This slice starts the `mavg-package-streaming-residency-v1` follow-up without reopening that completed milestone and without changing the production-completion master-plan selection gate.

**Constraints:**

- Keep `MK_runtime` independent from renderer/RHI/native handles.
- Use existing `RuntimeResidentPackageMountSetV2`, `RuntimeResidentCatalogCacheV2`, and reviewed eviction-safe candidate mount helpers.
- Do not create background workers, threads, automatic eviction policy, async-overlap/performance claims, partial `.mavgpayload` page schema, GPU memory pressure integration, GPU culling, indirect draw execution, mesh shaders, Metal readiness, or Nanite compatibility/equivalence/superiority claims.

## Tasks

### Task 1: Add Page Streaming Queue Tests

**Files:**

- Add: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `CMakeLists.txt`

- [x] Add `MK_runtime_mavg_page_streaming_tests` linked to `MK_runtime`.
- [x] Cover resident page skips, duplicate page request coalescing, deterministic priority/page ordering, `max_queued_pages` degradation, invalid request diagnostics, missing candidate diagnostics, one-row safe-point drain, and invalid mount id preservation.

### Task 2: Implement Runtime Page Streaming Queue

**Files:**

- Modify: `engine/runtime/CMakeLists.txt`
- Add: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Add: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`

- [x] Add `RuntimeMavgPageStreamingCandidateRow`, `RuntimeMavgPageStreamingPlanDesc`, `RuntimeMavgPageStreamingPlanRow`, `RuntimeMavgPageStreamingPlanResult`, `RuntimeMavgPageStreamingDrainDesc`, and `RuntimeMavgPageStreamingDrainResult`.
- [x] Add `plan_runtime_mavg_page_streaming_requests` as a pure planner over caller-reviewed `MavgLodPageRequest` rows and package index discovery candidates.
- [x] Add `execute_runtime_mavg_page_streaming_request_safe_point` as a one-row safe-point drain through `commit_runtime_package_candidate_resident_mount_with_reviewed_evictions_v2`.
- [x] Preserve explicit zero side-effect counters for planner file IO, mount mutation, streaming execution, background worker execution, and renderer/RHI handle access.

### Task 3: Sync Agent Surfaces And Validate

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Add: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`

- [x] Describe the narrow page streaming queue evidence without reopening `mavg-runtime-lod-milestone-v1`.
- [x] Run focused C++ validation for `MK_runtime_mavg_page_streaming_tests`, `MK_runtime_mavg_lod_residency_tests`, and `MK_runtime_package_streaming_resident_mount_tests`.
- [x] Run static/agent-surface validation and full `tools/validate.ps1`.
- [ ] Run publication preflight before staging/push/PR.

Evidence: focused validation passed with `tools/check-toolchain.ps1`, `tools/cmake.ps1 --preset dev`, targeted build for `MK_runtime_mavg_page_streaming_tests`, `MK_runtime_mavg_lod_residency_tests`, and `MK_runtime_package_streaming_resident_mount_tests`, targeted CTest passing 3/3, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/format.ps1`, `tools/check-format.ps1`, targeted `tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply`, and `git diff --check`. Full `tools/validate.ps1` passed with `validate: ok` and CTest `107/107` passing; Apple/Metal checks remain host-gated diagnostic-only on this Windows host.

## Done When

- Runtime planner output is deterministic and does not read files, mutate resident state, execute streaming, or touch renderer/RHI handles.
- Runtime safe-point drain delegates one reviewed queue row to existing reviewed candidate mount with reviewed evictions and preserves state on failure.
- Docs, registry, manifest fragments, composed manifest, and static checks describe exactly this narrow capability.
- Focused validation and full `tools/validate.ps1` pass, or a concrete host/tool blocker is recorded.

## Non-Claims

- No autonomous/background package streaming worker.
- No async-overlap/performance claim.
- No automatic eviction policy, partial `.mavgpayload` byte-range page loader/schema, or GPU memory pressure integration.
- No GPU culling, indirect draw execution, mesh shaders, deformation, ray tracing, Metal readiness, benchmark superiority, Nanite compatibility, Nanite equivalence, or Nanite superiority.
