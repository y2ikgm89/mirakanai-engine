# MAVG D3D12 Residency Action Execution v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Execute narrow D3D12 backend-private residency actions over engine-owned RHI resources without exposing native handles or claiming a full allocator/residency service.

**Architecture:** Add a backend-neutral `IRhiDevice` residency action row API that accepts first-party `BufferHandle` / `TextureHandle` rows and returns deterministic value evidence. The Null backend provides deterministic test evidence, D3D12 resolves those public handles to backend-private `ID3D12Pageable` committed resources and calls `ID3D12Device::MakeResident` or `ID3D12Device::Evict`, and unsupported backends fail closed until backend-local parity lands.

**Tech Stack:** C++23, `MK_rhi`, `MK_rhi_d3d12`, `IRhiDevice`, `BufferHandle`, `TextureHandle`, `ID3D12Device::MakeResident`, `ID3D12Device::Evict`, `MK_rhi_tests`, `MK_d3d12_rhi_tests`, Microsoft Learn D3D12 residency/memory management docs, Context7 `/microsoft/directx-graphics-samples`.

---

**Date:** 2026-06-06

**Plan ID:** `mavg-d3d12-residency-action-execution-v1`

**Status:** Published as stacked draft PR #491 over `mavg-dxgi-gpu-memory-pressure-evidence-v1`.

## Context

MAVG currently has value-only resident page eviction ordering, runtime-inferred recency/frequency evidence, caller-supplied GPU memory pressure ordering, and a D3D12/DXGI diagnostics-to-pressure-row adapter. It still does not execute any real GPU residency action. Microsoft Direct3D 12 guidance treats residency as application-owned memory management; `MakeResident` / `Evict` operate on supported pageable objects, must be ordered around GPU use with fences, and can fail even when the apparent budget looks sufficient. Context7's DirectX Graphics Samples likewise show D3D12 residency managers tracking managed objects and making required objects resident before command queue execution.

This child deliberately stops before allocator enforcement, automatic MAVG page-to-GPU resource mapping, DirectStorage IO, backend parity, or benchmark claims. It only creates a first-party action boundary and proves that D3D12 can execute reviewed residency actions over already-created engine-owned RHI buffer/texture resources.

## Files

- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify: `engine/rhi/src/null_rhi.cpp`
- Modify: `engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp`
- Modify: `engine/rhi/d3d12/src/d3d12_backend.cpp`
- Modify: `engine/rhi/vulkan/src/vulkan_backend.cpp`
- Modify: `tests/unit/rhi_tests.cpp`
- Modify: `tests/unit/d3d12_rhi_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generated: `engine/agent/manifest.json` via `tools/compose-agent-manifest.ps1 -Write`
- Create: `tools/check-ai-integration-126-mavg-d3d12-residency-action-execution.ps1`

## API Shape

Add value rows to `mirakana::rhi`:

```cpp
enum class RhiResidencyActionKind : std::uint8_t {
    make_resident = 0,
    evict,
};

enum class RhiResidencyResourceKind : std::uint8_t {
    buffer = 0,
    texture,
};

enum class RhiResidencyActionStatus : std::uint8_t {
    succeeded = 0,
    unsupported_backend,
    invalid_request,
    invalid_resource,
    native_call_failed,
};

struct RhiResidencyResourceRef {
    RhiResidencyResourceKind kind{RhiResidencyResourceKind::buffer};
    BufferHandle buffer;
    TextureHandle texture;
};

struct RhiResidencyActionDesc {
    RhiResidencyActionKind action{RhiResidencyActionKind::make_resident};
    std::span<const RhiResidencyResourceRef> resources;
};

struct RhiResidencyActionResult {
    RhiResidencyActionStatus status{RhiResidencyActionStatus::invalid_request};
    BackendKind backend{BackendKind::null};
    RhiResidencyActionKind action{RhiResidencyActionKind::make_resident};
    std::uint32_t requested_resource_count{0};
    std::uint32_t acted_resource_count{0};
    std::uint32_t invalid_resource_count{0};
    bool invoked_native_make_resident{false};
    bool invoked_native_evict{false};
    bool exposed_native_handles{false};
    bool enforced_allocator_budget{false};
    std::uint32_t native_error_code{0};
    std::string diagnostic;
};
```

Add to `IRhiDevice`:

```cpp
[[nodiscard]] virtual RhiResidencyActionResult execute_residency_action(const RhiResidencyActionDesc& desc);
```

The base implementation returns `unsupported_backend` so backends that do not override the method fail closed without a compatibility shim.

## Implementation

- [x] **Step 1: Add RED tests for backend-neutral residency action rows.**

Add `MK_rhi_tests` coverage proving:

```cpp
MK_TEST("null rhi residency action validates buffer and texture rows")
MK_TEST("null rhi residency action rejects invalid resource rows before action")
```

Expected RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
```

Expected failure: `RhiResidencyActionDesc`, `RhiResidencyActionResult`, and `IRhiDevice::execute_residency_action` are undefined.

- [x] **Step 2: Add RED tests for D3D12 native action execution.**

Add `MK_d3d12_rhi_tests` coverage proving:

```cpp
MK_TEST("d3d12 rhi residency action executes make resident and evict for committed resources")
MK_TEST("d3d12 rhi residency action rejects unknown resources before native calls")
```

Expected RED:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
```

- [x] **Step 3: Add public RHI residency action value API.**

Modify `engine/rhi/include/mirakana/rhi/rhi.hpp` with the value rows, result status, string label helpers, and `IRhiDevice::execute_residency_action`.

Rules:

- Empty resource span fails with `invalid_request`.
- Mixed buffer/texture rows are accepted.
- A row must name exactly one live handle matching its kind.
- Results are value-only and never expose `ID3D12Pageable`, `ID3D12Resource`, `ID3D12Heap`, or backend-private pointers.

- [x] **Step 4: Implement Null backend deterministic behavior.**

Modify `engine/rhi/src/null_rhi.cpp` so `NullRhiDevice::execute_residency_action` validates rows, records `requested_resource_count` / `acted_resource_count`, returns `succeeded` for live handles, and returns `invalid_resource` before action for unknown, released, or mismatched handles.

Do not mutate buffer/texture lifetime, memory diagnostics, or resource active state.

- [x] **Step 5: Implement D3D12 backend-private action execution.**

Modify `DeviceContext` and `D3d12RhiDevice` so D3D12:

- Validates all public handles before native calls.
- Resolves committed public buffer/texture handles to backend-private resources.
- Calls `ID3D12Device::MakeResident` for `make_resident`.
- Calls `ID3D12Device::Evict` for `evict`.
- Reports HRESULT as `native_error_code` on failure.
- Sets `invoked_native_make_resident` or `invoked_native_evict` only after the native call is attempted.

This slice acts only on committed public buffers/textures. Placed/transient resources, descriptor heaps, query heaps, residency priority, video-memory reservation, command-list integration, fence scheduling, and MAVG page-to-resource ownership are out of scope.

- [x] **Step 6: Add unsupported backend behavior.**

Modify Vulkan to return `unsupported_backend` for the new virtual method. Metal has no `IRhiDevice` implementation in this repo path today, so no Metal code change is expected unless a build error reveals one.

- [x] **Step 7: Run focused GREEN.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests
```

- [x] **Step 8: Update docs, manifest fragments, composed manifest, and static checks.**

Claim only:

- `RhiResidencyActionDesc`
- `RhiResidencyActionResult`
- `IRhiDevice::execute_residency_action`
- Null deterministic validation evidence
- D3D12 `MakeResident` / `Evict` action execution over committed public buffer/texture handles
- fail-closed invalid resource validation
- no native handle exposure
- no allocator/GPU budget enforcement
- no DirectStorage/file IO
- no Vulkan/Metal parity
- no async-overlap/performance or Nanite claim

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] **Step 9: Run focused/static/full validation.**

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/src/null_rhi.cpp,engine/rhi/d3d12/src/d3d12_backend.cpp,engine/rhi/vulkan/src/vulkan_backend.cpp,tests/unit/rhi_tests.cpp,tests/unit/d3d12_rhi_tests.cpp -ReuseExistingFileApiReply
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Validation evidence on 2026-06-06:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_rhi_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_rhi_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_d3d12_rhi_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_d3d12_rhi_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/rhi/src/null_rhi.cpp,engine/rhi/d3d12/src/d3d12_backend.cpp,engine/rhi/vulkan/src/vulkan_backend.cpp,tests/unit/rhi_tests.cpp,tests/unit/d3d12_rhi_tests.cpp -ReuseExistingFileApiReply` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `git diff --check` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed; all 109 CTest tests passed.

- [x] **Step 10: Publish stacked PR.**

Run publication preflight, commit the candidate, push `codex/mavg-d3d12-residency-action-execution-v1`, and create a draft PR over `codex/mavg-dxgi-gpu-memory-pressure-evidence-v1`.

Publication evidence:

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` passed.
- Commit `0d77d6f6` (`Add MAVG D3D12 residency action execution`) was pushed to `origin/codex/mavg-d3d12-residency-action-execution-v1`.
- Draft PR #491 was created over `codex/mavg-dxgi-gpu-memory-pressure-evidence-v1`.

## Done When

- `IRhiDevice` exposes a backend-neutral, value-only residency action execution boundary.
- Null tests prove validation and deterministic action evidence without mutating resource lifetimes.
- D3D12 WARP-backed tests prove `MakeResident` and `Evict` are actually invoked for committed RHI resources and invalid public handles fail before native calls.
- Docs, plan registry, manifest fragments, composed manifest, and static checks describe the exact D3D12 action scope and remaining non-claims.
- Full `tools/validate.ps1` passes or records a concrete host/tool blocker.

## Non-Claims

- No public `ID3D12Device`, `ID3D12Pageable`, `ID3D12Resource`, `ID3D12Heap`, `IDXGIAdapter3`, `IRhiDevice` internals, native/RHI pointer, or backend-private handle exposure.
- No allocator enforcement, video-memory reservation, memory-budget solver, residency priority management, automatic residency manager, MAVG page-to-resource residency service, or command submission residency-set integration.
- No DirectStorage native queue/status/fence/file IO execution.
- No Vulkan/Metal residency parity.
- No async-overlap/performance benchmark claim.
- No Nanite compatibility, equivalence, superiority, or benchmark-exceeds claim.
