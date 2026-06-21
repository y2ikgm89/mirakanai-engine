# 2026-06-21 MAVG Win32 DirectStorage SDK Adapter v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party Windows DirectStorage SDK adapter that implements the existing `IByteRangeIoExecutor` contract for reviewed MAVG `.mavgpayload` byte ranges.

**Architecture:** Keep Microsoft DirectStorage SDK objects private to `MK_platform_win32`; expose only the existing backend-neutral `IByteRangeIoExecutor` / `ByteRangeIoBackendKind::direct_storage` runtime boundary. The adapter uses DirectStorage system-memory destinations only, feeds the completed `load_runtime_mavg_payload_pages_from_direct_storage` path, and remains optional, Windows-only, dependency-gated, and package-evidence-gated before any readiness claim.

**Tech Stack:** C++23, `MK_platform`, `MK_platform_win32`, `MK_runtime`, CMake, vcpkg manifest feature `directstorage-win32`, vcpkg port `dstorage` 1.3.0, Microsoft DirectStorage SDK 1.3.0, `dstorage.h`, `Microsoft::DirectStorage`, PowerShell validation wrappers, and focused CTest lanes.

---

**Plan ID:** `mavg-win32-directstorage-sdk-adapter-v1`

**Status:** Completed.

**Date:** 2026-06-21

**Closeout:** The implemented slice returns live execution to the production-completion selection gate:
`engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points back to
`docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md` after
manifest composition. The retained evidence is this completed plan plus the package-visible
`mavg_win32_directstorage_sdk_adapter_*` counters.

## Official Source Refresh

Context7 status on 2026-06-21: `resolve-library-id` found Microsoft Learn as `/websites/learn_microsoft_en-us`, but the DirectStorage-specific query returned unrelated Windows App SDK storage-picker pages. Treat Context7 as checked but insufficient for this SDK slice; use Microsoft official fallback sources below.

Authoritative sources for this plan:

- Microsoft DirectX Developer Blog DirectStorage SDK landing page: `https://devblogs.microsoft.com/directx/directstorage-api-downloads/`. Use stable `Microsoft.Direct3D.DirectStorage` 1.3.0 for implementation; do not build this plan on 1.4 preview-only APIs.
- Microsoft NuGet package `Microsoft.Direct3D.DirectStorage` 1.3.0: `https://www.nuget.org/packages/Microsoft.Direct3D.DirectStorage/1.3.0`. The package contains SDK headers, import library, redistributable DLLs, and package license files.
- Microsoft Learn DirectStorage API reference: `https://learn.microsoft.com/en-us/windows/win32/dstorage/`. Required API shapes are `DStorageGetFactory`, `IDStorageFactory`, `IDStorageQueue`, `IDStorageStatusArray`, `IDStorageQueue::EnqueueRequest`, `IDStorageQueue::EnqueueStatus`, `IDStorageQueue::Submit`, and `IDStorageStatusArray::GetHResult`.
- Microsoft DirectStorage GitHub developer guidance: `https://github.com/microsoft/DirectStorage/blob/main/Docs/DeveloperGuidance.md`. Use its queue/source/destination/status guidance as design constraints only; do not copy sample code.
- Repository vcpkg baseline check: `external/vcpkg/ports/dstorage/vcpkg.json` declares port `dstorage`, version `1.3.0`, supports `windows & !arm32 & !uwp & !xbox`; `external/vcpkg/ports/dstorage/usage` declares `find_package(dstorage CONFIG REQUIRED)` and `target_link_libraries(<target> PRIVATE Microsoft::DirectStorage)`.

## Scope

This plan selects only the first-party Win32 SDK adapter missing after `mavg-directstorage-page-io-execution-v1`. It does not add GPU destinations, GDeflate/GPU decompression, DirectStorage 1.4 preview scheduling, autonomous streaming policy, measured async-overlap proof, backend draw execution, mesh shaders, Metal readiness, Nanite equivalence/superiority, or broad optimization.

## Files

- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_directstorage_byte_range_io.hpp`
- Create: `engine/platform/win32/src/win32_directstorage_byte_range_io.cpp`
- Create: `tests/unit/win32_directstorage_byte_range_io_tests.cpp`
- Create: `tools/validate-mavg-win32-directstorage-sdk-adapter.ps1`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `CMakePresets.json`
- Modify: `vcpkg.json`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Modify static guards only when validation identifies stale needles: `tools/check-ai-integration-*.ps1`, `tools/check-json-contracts-*.ps1`

## Task 0: Selection And Drift Guard

**Files:**
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `tools/check-ai-integration-020-engine-manifest.ps1`
- Modify: `tools/check-ai-integration-030-runtime-rendering.ps1`
- Modify: `tools/check-json-contracts-040-agent-surfaces.ps1`

- [x] **Step 1: Select this active child plan**

Set:

```json
"currentActivePlan": "docs/superpowers/plans/2026-06-21-mavg-win32-directstorage-sdk-adapter-v1.md",
"recommendedNextPlan": {
  "id": "mavg-win32-directstorage-sdk-adapter-v1",
  "title": "MAVG Win32 DirectStorage SDK Adapter v1",
  "status": "active-child-plan",
  "path": "docs/superpowers/plans/2026-06-21-mavg-win32-directstorage-sdk-adapter-v1.md"
}
```

- [x] **Step 2: Preserve completed MAVG evidence**

Keep the completed `mavg-directstorage-page-io-execution-v1`, `mavg-streaming-upload-overlap-evidence-v1`, MAVG GPU upload/backend draw evidence, environment commercial evidence, and renderer backend parity evidence as retained evidence only. Fix stale `Active local implementation slice` rows for completed MAVG overlap evidence to completed retained evidence.

- [x] **Step 3: Run selection validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
git diff --check
```

Expected: all commands pass. If an agent-surface guard fails because it still assumes the selection gate, update the guard to the exact active child plan id rather than weakening the guard.

## Task 1: Dependency And Legal Gate

**Files:**
- Modify: `vcpkg.json`
- Modify: `tools/bootstrap-deps.ps1`
- Modify: `tools/check-dependency-policy.ps1`
- Modify: `docs/dependencies.md`
- Modify: `docs/legal-and-licensing.md`
- Modify: `THIRD_PARTY_NOTICES.md`

- [x] **Step 1: Add the optional vcpkg feature**

Add exactly:

```json
"directstorage-win32": {
  "description": "Optional Windows-only Microsoft DirectStorage SDK adapter for reviewed MAVG byte-range IO.",
  "dependencies": [
    "dstorage"
  ]
}
```

Do not add `dstorage` to default dependencies.

- [x] **Step 2: Add bootstrap support**

Extend `tools/bootstrap-deps.ps1` so an explicit `-Feature directstorage-win32` or `-All` installs `directstorage-win32`. The wrapper must still be the only repository entrypoint that runs vcpkg.

- [x] **Step 3: Add dependency policy checks**

Update `tools/check-dependency-policy.ps1` to require:

- `vcpkg.json.features.directstorage-win32.dependencies` contains only `dstorage`.
- `directstorage-win32` is not a default dependency.
- CMake presets keep `VCPKG_MANIFEST_INSTALL=OFF`.
- Docs and third-party notices mention `Microsoft DirectStorage SDK`, `dstorage`, version `1.3.0`, Windows-only support, private adapter isolation, and non-leakage of `dstorage.h` types into public gameplay/runtime APIs.

- [x] **Step 4: Verify dependency records**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

Expected: both pass without running vcpkg install.

## Task 2: Adapter Contract

**Files:**
- Create: `engine/platform/win32/include/mirakana/platform/win32/win32_directstorage_byte_range_io.hpp`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `tests/unit/win32_directstorage_byte_range_io_tests.cpp`

- [x] **Step 1: Write the failing public-contract test**

Create `tests/unit/win32_directstorage_byte_range_io_tests.cpp` with tests that include only the new first-party header plus `mirakana/platform/byte_range_io.hpp`, then assert:

```cpp
static_assert(std::derived_from<mirakana::win32::Win32DirectStorageByteRangeExecutor,
                                mirakana::IByteRangeIoExecutor>);
static_assert(!std::copy_constructible<mirakana::win32::Win32DirectStorageByteRangeExecutor>);
static_assert(!std::copy_assignable<mirakana::win32::Win32DirectStorageByteRangeExecutor>);
```

Also assert `backend_kind() == ByteRangeIoBackendKind::direct_storage` and no public method returns `IDStorage*`, `HANDLE`, `IUnknown*`, `void*`, `D3D12`, `DXGI`, or `dstorage` SDK objects.

- [x] **Step 2: Run the failing test build**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_directstorage_byte_range_io_tests
```

Expected: fail because the target and header do not exist.

- [x] **Step 3: Add the header**

Expose only this first-party shape:

```cpp
namespace mirakana::win32 {

struct Win32DirectStorageByteRangeExecutorOptions {
    std::uint32_t queue_capacity{128};
    std::chrono::milliseconds completion_timeout{std::chrono::seconds{30}};
};

struct Win32DirectStorageReadDiagnostics {
    bool factory_ready{false};
    bool queue_ready{false};
    bool status_array_ready{false};
    bool submitted{false};
    std::uint32_t request_count{0};
    std::int32_t last_hresult{0};
};

class Win32DirectStorageByteRangeExecutor final : public IByteRangeIoExecutor {
  public:
    explicit Win32DirectStorageByteRangeExecutor(Win32DirectStorageByteRangeExecutorOptions options = {});
    ~Win32DirectStorageByteRangeExecutor() override;
    Win32DirectStorageByteRangeExecutor(const Win32DirectStorageByteRangeExecutor&) = delete;
    Win32DirectStorageByteRangeExecutor& operator=(const Win32DirectStorageByteRangeExecutor&) = delete;
    Win32DirectStorageByteRangeExecutor(Win32DirectStorageByteRangeExecutor&&) noexcept;
    Win32DirectStorageByteRangeExecutor& operator=(Win32DirectStorageByteRangeExecutor&&) noexcept;

    [[nodiscard]] ByteRangeIoBackendKind backend_kind() const noexcept override;
    [[nodiscard]] bool available() const noexcept override;
    [[nodiscard]] const Win32DirectStorageReadDiagnostics& diagnostics() const noexcept;
    [[nodiscard]] std::vector<ByteRangeIoReadRow>
    read_ranges(std::span<const ByteRangeIoReadRequest> requests) override;

  private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace mirakana::win32
```

## Task 3: DirectStorage SDK Execution

**Files:**
- Create: `engine/platform/win32/src/win32_directstorage_byte_range_io.cpp`
- Modify: `engine/platform/win32/CMakeLists.txt`
- Modify: `tests/unit/win32_directstorage_byte_range_io_tests.cpp`

- [x] **Step 1: Write execution tests**

Add tests that create a temporary binary file with two non-overlapping byte ranges, execute `read_ranges`, and verify:

- Returned rows preserve request order.
- Returned `path`, `byte_offset`, `byte_size`, and `bytes` match the requests.
- Empty request spans return an empty vector without submitting.
- Missing files return empty or mismatched rows that cause existing runtime MAVG loader diagnostics, not filesystem fallback.
- `diagnostics().submitted` becomes true only after `IDStorageQueue::Submit`.
- No public native handles are exposed.

- [x] **Step 2: Implement SDK-backed reads**

Implement with `Microsoft::WRL::ComPtr`, `DStorageGetFactory`, `IDStorageFactory::OpenFile`, `IDStorageFactory::CreateQueue`, `IDStorageFactory::CreateStatusArray`, system-memory `DSTORAGE_REQUEST` destinations, `IDStorageQueue::EnqueueRequest`, `IDStorageQueue::EnqueueStatus`, `IDStorageQueue::Submit`, and `IDStorageStatusArray::GetHResult` polling until `S_OK`, failure `HRESULT`, or timeout.

- [x] **Step 3: Keep GPU and decompression paths out**

The adapter must not use `ID3D12Resource`, `ID3D12Fence`, `DSTORAGE_DESTINATION_BUFFER`, `DSTORAGE_DESTINATION_TEXTURE_REGION`, `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE`, GDeflate, Zstd, DirectStorage 1.4 preview-only APIs, or `IDStorageQueue3::EnqueueRequests`.

- [x] **Step 4: Run focused adapter validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_win32_directstorage_byte_range_io_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_win32_directstorage_byte_range_io_tests"
```

Expected: pass on a Windows host with `directstorage-win32` bootstrapped; skip with an explicit host/dependency blocker elsewhere.

## Task 4: MAVG Package Evidence

**Files:**
- Modify: `games/sample_desktop_runtime_game/main.cpp`
- Modify: `tools/validate-mavg-win32-directstorage-sdk-adapter.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

- [x] **Step 1: Add package-visible counters**

Add a selected smoke path such as `--require-mavg-win32-directstorage-sdk-adapter` that emits:

```text
mavg_win32_directstorage_sdk_adapter_status=ready
mavg_win32_directstorage_sdk_adapter_ready=1
mavg_win32_directstorage_sdk_adapter_sdk_version=1.3.0
mavg_win32_directstorage_sdk_adapter_requests=2
mavg_win32_directstorage_sdk_adapter_completed_ranges=2
mavg_win32_directstorage_sdk_adapter_native_handles_exposed=0
mavg_win32_directstorage_sdk_adapter_gpu_destinations=0
mavg_win32_directstorage_sdk_adapter_gdeflate=0
mavg_win32_directstorage_sdk_adapter_async_overlap_performance_proof=0
```

- [x] **Step 2: Add validator**

`tools/validate-mavg-win32-directstorage-sdk-adapter.ps1 -RequireReady` must fail closed unless host OS is Windows, `directstorage-win32` dependency files are present, the adapter tests pass, the package smoke emits the exact counters above, and the active plan/docs/manifest preserve non-claims.

- [x] **Step 3: Run closeout validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-win32-directstorage-sdk-adapter.ps1 -RequireReady
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: pass before claiming the adapter ready. If DirectStorage runtime, vcpkg `dstorage`, or Windows host evidence is missing, record a dependency/host blocker and do not mark the adapter ready.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 -Feature directstorage-win32` installed the pinned optional `dstorage` 1.3.0 feature.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1` passed after dependency/legal records were updated.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset directstorage-win32` configured the optional SDK lane.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset directstorage-win32 --target MK_win32_directstorage_byte_range_io_tests` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset directstorage-win32 --output-on-failure -R "MK_win32_directstorage_byte_range_io_tests"` passed.
- `sample_desktop_runtime_game.exe --smoke --max-frames 1 --require-mavg-win32-directstorage-sdk-adapter` emitted `mavg_win32_directstorage_sdk_adapter_ready=1`, SDK version `1.3.0`, two requests, two completed ranges, and zero native handle / GPU destination / GDeflate / async-overlap-performance claims.

## Done When

- `directstorage-win32` is an optional vcpkg feature over `dstorage` 1.3.0, installed only by `tools/bootstrap-deps.ps1`.
- `MK_platform_win32` owns the only `dstorage.h` includes and links only through `Microsoft::DirectStorage` when the optional feature is selected.
- Public runtime/gameplay APIs expose only `IByteRangeIoExecutor`, `ByteRangeIoBackendKind::direct_storage`, and first-party diagnostics.
- `load_runtime_mavg_payload_pages_from_direct_storage` can consume the adapter and produce existing deterministic MAVG page rows.
- Package-visible counters prove adapter invocation without native handle exposure, GPU destinations, GDeflate, preview APIs, or performance claims.
- Docs, manifest fragments, composed manifest, static checks, dependency/legal records, and validation wrappers match the implemented scope.

## Remaining Non-Claims

GPU DirectStorage destinations, GDeflate/GPU decompression, Zstd/DirectStorage 1.4 preview APIs, DirectStorage-to-D3D12 fence scheduling, RHI upload integration, backend draw execution, autonomous streaming service policy, measured async-overlap/performance proof, mesh shader execution, Metal readiness, Vulkan/Metal parity, ray tracing, deformation, Nanite compatibility/equivalence/superiority, and broad CPU/GPU/memory optimization remain unclaimed.
