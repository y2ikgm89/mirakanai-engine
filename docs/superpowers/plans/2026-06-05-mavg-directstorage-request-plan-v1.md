# MAVG DirectStorage Request Plan v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic runtime MAVG DirectStorage request-plan contract that converts validated selected payload pages into ordered file-source request rows without executing native DirectStorage or claiming async performance.

**Architecture:** `MK_runtime` stays SDK-link-free and plans request data from `GameEngine.MavgClusterPayload.v1` page rows after validating them against `MavgClusterGraphDocument`. The result mirrors official DirectStorage source offset/size, destination size, request arrays, and fence wait-point concepts as first-party value rows while leaving `dstorage.h`, queues, status arrays, native fences, background workers, and performance evidence to later host/dependency-gated slices.

**Tech Stack:** C++23, `MK_runtime`, `MK_assets`, `GameEngine.MavgClusterPayload.v1`, Microsoft Learn DirectStorage API docs, Context7 Microsoft Learn lookup, NuGet `Microsoft.Direct3D.DirectStorage` package audit, CMake/CTest, PowerShell 7 validation scripts.

---

**Plan ID:** `mavg-directstorage-request-plan-v1`

**Status:** Completed stacked child.

Focused child over `mavg-runtime-lod-milestone-v1`, stacked on `mavg-payload-byte-range-file-io-v1` / draft PR #459, and published as draft PR #460.

## Official Source Audit

Checked on 2026-06-05:

- Context7 selected `/websites/learn_microsoft_en-us` for Microsoft Learn. The returned results were broad Microsoft Learn snippets, so direct Microsoft Learn pages remain the DirectStorage authority for exact API fields.
- Microsoft Learn DirectStorage portal documents DirectStorage for games using high-speed storage with small reads and APIs declared in `dstorage.h`.
- Microsoft Learn `IDStorageFactory` documents the factory as the object used to create queues and open files for DirectStorage access.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` documents that a request remains queued until `Submit` or queue behavior.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests` documents array enqueue synchronized with an optional `ID3D12Fence` and value.
- Microsoft Learn `DSTORAGE_ENQUEUE_REQUEST_FLAGS` documents fence wait points: none, before GPU work, and before source access.
- Microsoft Learn GDK DirectStorage overview records the request model with file source offset/size and destination size, notes 64 KiB-ish NVMe-friendly read guidance, and recommends enqueuing requests promptly rather than title-side buffering.
- NuGet `Microsoft.Direct3D.DirectStorage` 1.3.0 is the official native SDK package and includes redistributable binaries. This slice deliberately avoids adding that dependency or claiming native execution; SDK availability remains a later host/dependency-gated validation requirement.

## Scope

In scope:

- Add `RuntimeMavgPayloadDirectStorageRequestPlanDesc`, `RuntimeMavgPayloadDirectStorageRequestRow`, `RuntimeMavgPayloadDirectStorageRequestPlanResult`, diagnostics, and `plan_runtime_mavg_payload_directstorage_requests`.
- Validate graph and payload metadata before planning.
- Reject duplicate, unknown, and missing selected page requests before planning rows.
- Convert selected page `byte_offset` / `byte_size` into ordered file-source request rows.
- Produce contiguous caller-owned destination offsets from `destination_base_offset` in request order.
- Record DirectStorage-style fence wait-point intent without exposing native `ID3D12Fence`.
- Fail closed when page sizes cannot fit DirectStorage `UINT32` request size fields or destination ranges overflow.
- Record explicit zero side-effect flags: no file IO, no native DirectStorage use, no enqueue/submit/fence signal, no resident mount mutation, no background worker, no renderer/RHI handles.

Out of scope:

- Adding or bootstrapping the DirectStorage NuGet package.
- Including `dstorage.h`, linking DirectStorage libraries, or redistributing DirectStorage DLLs.
- Native `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `ID3D12Fence`, or `DSTORAGE_REQUEST` execution.
- Win32 overlapped IO / IOCP execution.
- Autonomous background workers, async-overlap/performance evidence, automatic eviction, GPU memory pressure, renderer/RHI upload, Vulkan/Metal parity, mesh shaders, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Test: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-113-mavg-payload-byte-range-file-io.ps1`
- Create: `tools/check-ai-integration-114-mavg-directstorage-request-plan.ps1`

## Public API Contract

`MK_runtime` gains value-only DirectStorage request planning:

```cpp
enum class RuntimeMavgPayloadDirectStorageFenceWaitPoint : std::uint8_t {
    before_destination_write = 0,
    before_gpu_work,
    before_source_access,
};

struct RuntimeMavgPayloadDirectStorageRequestPlanDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::string_view payload_blob_path;
    std::span<const std::uint32_t> page_indices;
    std::uint64_t destination_base_offset{0};
    RuntimeMavgPayloadDirectStorageFenceWaitPoint fence_wait_point{
        RuntimeMavgPayloadDirectStorageFenceWaitPoint::before_destination_write};
    bool synchronize_with_fence{false};
};

[[nodiscard]] RuntimeMavgPayloadDirectStorageRequestPlanResult
plan_runtime_mavg_payload_directstorage_requests(const RuntimeMavgPayloadDirectStorageRequestPlanDesc& desc);
```

Successful plans report `requires_native_directstorage_sdk=true` and `used_native_directstorage=false`; execution-side flags for file IO, native enqueue, native submit, native fence signal, background workers, resident mount mutation, and renderer/RHI handle access remain false.

## Tasks

- [x] Add RED tests for ordered DirectStorage request planning, duplicate selected page rejection, and destination overflow rejection.
- [x] Run focused RED build for `MK_runtime_mavg_payload_pages_tests`; expected failure is missing request-plan symbols.
- [x] Implement request-plan structs, diagnostics, and planner function without native SDK includes or file IO.
- [x] Run focused build and CTest for `MK_runtime_mavg_payload_pages_tests`.
- [x] Update docs/spec/registry/parent milestone/manifest fragments/static guards and compose manifest.
- [x] Run public API boundary, targeted tidy, format, JSON/AI/agent checks, and `git diff --check`.
- [x] Run full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR from `codex/mavg-directstorage-request-plan-v1` with base `codex/mavg-directstorage-byte-range-io-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests` | RED fails before implementation because request-plan symbols are absent; later passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"` | Focused runtime MAVG payload page tests pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public API checks pass with no native DirectStorage or renderer/RHI handle exposure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Targeted C++ static analysis passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Manifest is composed from fragment updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | JSON contracts pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | MAVG runtime LOD, byte-range file IO retained evidence, and DirectStorage request-plan guard needles pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Agent-surface parity checks pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | C++ and tracked text formatting pass. |
| `git diff --check` | Whitespace is clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Full slice validation passes or records a concrete host/tool blocker. |

## Validation Evidence

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed; generated `out/build/dev` for the new worktree. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests` | RED failed as expected because request-plan symbols were absent. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_payload_pages_tests` | Passed after implementation. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_mavg_payload_pages_tests"` | Passed: 1/1 focused runtime MAVG payload test target. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; regenerated `engine/agent/manifest.json` from fragments. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed: `ai-integration-check: ok`. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed: `agent-config-check: ok`. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed: `public-api-boundary-check: ok`. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Passed: `tidy-check: ok (2 files)`. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed: `format-check: ok`. |
| 2026-06-06 | `git diff --check` | Passed with no whitespace diagnostics. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: `validate: ok`, including 109/109 CTest targets. |
| 2026-06-06 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`; `git push -u origin codex/mavg-directstorage-request-plan-v1`; `gh pr create --draft --base codex/mavg-directstorage-byte-range-io-v1 --head codex/mavg-directstorage-request-plan-v1` | Passed; published draft PR #460 at `https://github.com/y2ikgm89/mirakanai-engine/pull/460` with head `24274498ce4b2bf7709b1704f67b4f4ca9408f00`. |

## Non-Claims

This plan does not claim DirectStorage SDK installation, native DirectStorage factory/queue/status/fence execution, `dstorage.h` compile readiness, DirectStorage DLL redistribution, Win32 async IO, autonomous background workers, async-overlap/performance, resident mount mutation, automatic eviction policy, GPU memory pressure enforcement, renderer/RHI residency, native handles, Vulkan/Metal readiness, mesh shaders, deformation, ray tracing, Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, or broad CPU/GPU/memory optimization.
