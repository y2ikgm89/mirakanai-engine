# 2026-06-05 MAVG Package Streaming Residency Dispatch v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-package-streaming-residency-dispatch-v1`

**Status:** Completed.

**Execution State:** Completed through stacked draft PR #454. The next active child is `mavg-page-addressable-payload-schema-v1`.

**Goal:** Add a deterministic runtime MAVG page streaming dispatch planner that batches already-reviewed page streaming rows into caller-owned safe-point or caller-owned background-queue dispatch rows without executing file IO, mutating resident mounts, owning worker threads, or touching renderer/RHI handles.

**Architecture:** Keep `MK_runtime` as the owner of runtime package streaming dispatch rows and preserve the existing one-row safe-point drain as the only execution surface. The new planner converts `RuntimeMavgPageStreamingPlanRow` values plus caller-assigned mount ids, budget, overlay, and reviewed eviction/protection rows into copy-owned `RuntimeMavgPageStreamingDrainDesc` rows. Background mode is a value-only caller-owned queue annotation, not an autonomous worker or async-overlap claim.

**Tech Stack:** C++23, `MK_runtime`, existing package streaming/resident mount contracts, Microsoft Learn DirectStorage queue/status/submit docs as dispatch-boundary guidance, Context7 Microsoft Learn docs, PowerShell 7 validation scripts.

---

## Official Source Audit

Checked on 2026-06-05:

- Context7 `/websites/learn_microsoft_en-us`: selected Microsoft Learn as the official documentation source for DirectStorage/Windows API behavior.
- Microsoft Learn `IDStorageFactory`: DirectStorage has a factory that creates queues and opens files for DirectStorage access. This supports keeping queue creation/file IO out of this value-only planner.
- Microsoft Learn `IDStorageQueue`: DirectStorage queues enqueue read requests, status writes, fence writes, and `Submit` sends queued requests for execution. This supports a clean separation between dispatch row planning and actual execution.
- Microsoft Learn `IDStorageQueueX::EnqueueStatus`: status writes are signaled only after prior queued requests complete; automatic submission can happen when queue capacity thresholds are hit, and manual `Submit` is recommended when automatic submission is undesirable. This supports explicit, deterministic batching instead of hidden automatic execution.
- Microsoft Learn `IDStorageQueueX::Submit`: `Submit` controls when queued requests are handed to hardware and where CPU cost occurs. This slice keeps that control with the caller and does not introduce a backend or OS queue.

## Scope

In scope:

- `MK_runtime` value-only dispatch planning over already-reviewed `RuntimeMavgPageStreamingPlanRow` values.
- Deterministic row order preservation from the upstream streaming planner.
- Caller-assigned mount id validation with invalid, duplicate, and count-mismatch diagnostics.
- Dispatch row construction as copy-owned `RuntimeMavgPageStreamingDrainDesc` values that can later be passed to `execute_runtime_mavg_page_streaming_request_safe_point`.
- `caller_owned_safe_point` and `caller_owned_background_queue` dispatch modes as annotations only.
- Fail-closed rejection when a dispatch plan tries to bypass the required safe-point mutation boundary.
- Explicit side-effect flags for zero file IO, zero mount mutation, zero streaming execution, zero background worker execution, and zero renderer/RHI handle access.

Out of scope:

- DirectStorage, Win32 async IO, native queue/fence/status handles, or OS worker ownership.
- Autonomous background package streaming workers, thread pools, async overlap/performance evidence, or IO throughput claims.
- Automatic eviction policy beyond caller-reviewed candidate order and selected/fallback ancestor protection.
- Partial `.mavgpayload` byte-range schema/loader and GPU memory pressure integration.
- Renderer/RHI residency, GPU culling, indirect draw execution changes, Vulkan/Metal backend work, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, or broad optimization.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`
- Create: `tools/check-ai-integration-111-mavg-package-streaming-residency-dispatch.ps1`

## Tasks

- [x] Add RED tests in `tests/unit/runtime_mavg_page_streaming_tests.cpp`:
  - `runtime mavg page streaming dispatch planner builds safe point rows deterministically`
  - `runtime mavg page streaming dispatch planner applies deterministic max dispatch budget`
  - `runtime mavg page streaming dispatch planner records caller owned background queue without executing worker`
  - `runtime mavg page streaming dispatch planner rejects invalid dispatch rows before side effects`
  - `runtime mavg page streaming dispatch planner rejects unsafe no safe point mutation boundary`
- [x] Implement `RuntimeMavgPageStreamingDispatchMode`, `RuntimeMavgPageStreamingDispatchDesc`, `RuntimeMavgPageStreamingDispatchRow`, `RuntimeMavgPageStreamingDispatchPlan`, and `plan_runtime_mavg_page_streaming_dispatches`.
- [x] Validate plan rows for non-zero graph asset, finite priority, valid reason, and package candidate path/root before producing dispatch rows.
- [x] Validate caller mount ids for exact count, non-zero value, and uniqueness before producing dispatch rows.
- [x] Copy overlay, budget, reviewed eviction candidate order, and protected mount ids into every dispatch row's `RuntimeMavgPageStreamingDrainDesc`.
- [x] Preserve row order from `RuntimeMavgPageStreamingPlanResult::queued_page_requests`; apply `max_dispatch_rows` as a deterministic prefix budget with dropped-count evidence.
- [x] Keep `caller_owned_background_queue` as a value-only annotation and keep `executed_background_worker` false.
- [x] Reject `require_safe_point = false` because the current drain path mutates residency only at caller-owned safe points.
- [x] Update docs, architecture spec, parent milestone, plan registry, master plan, manifest fragments, composed manifest, and static guards.
- [x] Run focused runtime/MAVG validation.
- [x] Run full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR over `codex/mavg-d3d12-gpu-culling-execution-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` | RED fails after tests are added and before dispatch planner public API exists. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests MK_runtime_mavg_lod_residency_tests MK_runtime_package_streaming_resident_mount_tests` | Runtime MAVG page streaming, residency bridge, and package resident mount targets build after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests|MK_runtime_mavg_lod_residency_tests|MK_runtime_package_streaming_resident_mount_tests"` | Focused runtime/MAVG tests pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public API boundary checks pass with no native/renderer/RHI handle exposure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply` | Targeted C++ style/static analysis passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Manifest is composed from fragments after active plan and module claim updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | JSON contracts pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | MAVG runtime LOD and dispatch guard needles prove docs/manifest/static alignment. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Agent-surface parity checks pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | C++ and tracked text formatting pass. |
| `git diff --check` | Whitespace is clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Full slice validation passes or records a concrete host/tool blocker. |

## Validation Evidence

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` | RED failed before implementation on missing dispatch public API symbols and diagnostics. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; composed `engine/agent/manifest.json` from fragments. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after MAVG dispatch, parent milestone, JSON/static guard, and retained prerequisite evidence updates. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after registering `mavg-package-streaming-residency-dispatch-v1` as a MAVG runtime child. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed after tracked text line endings were normalized. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed; no native/renderer/RHI handle exposure added. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests MK_runtime_mavg_lod_residency_tests MK_runtime_package_streaming_resident_mount_tests` | Passed after final formatting. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_page_streaming_tests|MK_runtime_mavg_lod_residency_tests|MK_runtime_package_streaming_resident_mount_tests"` | Passed; 3/3 focused tests passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply` | Passed for 2 files. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed after `tools/format.ps1`. |
| 2026-06-05 | `git diff --check` | Passed; whitespace clean. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; 19 static checks, full build, targeted tidy smoke, and 107/107 CTest tests passed. Metal/Apple lanes remained diagnostic-only host-gated on Windows. |
| 2026-06-05 | Draft PR #454 | Published stacked draft PR over `codex/mavg-d3d12-gpu-culling-execution-v1`: `https://github.com/y2ikgm89/mirakanai-engine/pull/454`. |

## Non-Claims

This plan does not claim autonomous background workers, DirectStorage/Win32 IO integration, async overlap, package streaming performance, automatic eviction policy, partial `.mavgpayload` byte-range loading/schema, GPU memory pressure integration, renderer/RHI residency, native handles, Vulkan/Metal readiness, mesh shaders, deformation, ray tracing, Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, or broad CPU/GPU/memory optimization.
