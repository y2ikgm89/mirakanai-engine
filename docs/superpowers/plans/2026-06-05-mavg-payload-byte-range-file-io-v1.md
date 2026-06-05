# MAVG Payload Byte-Range File IO v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or focused inline TDD execution to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add first-party binary byte-range file IO and a runtime MAVG payload page file loader so validated payload page rows can read exact file byte ranges without claiming native DirectStorage execution.

**Architecture:** `MK_platform` owns clean binary and byte-range filesystem APIs behind `IFileSystem`; `MK_runtime` consumes those APIs to load selected MAVG payload page bytes from a caller-supplied blob path after validating the existing `GameEngine.MavgClusterPayload.v1` metadata against a graph document. The slice deliberately stops before native DirectStorage queues, fences, status arrays, async overlap, background workers, renderer/RHI upload, or GPU memory policy.

**Tech Stack:** C++23, `MK_platform`, `MK_runtime`, `MK_assets`, `IFileSystem`, `GameEngine.MavgClusterPayload.v1`, Microsoft Learn DirectStorage docs, Context7 Microsoft Learn lookup, CMake/CTest, PowerShell 7 validation scripts.

---

**Plan ID:** `mavg-payload-byte-range-file-io-v1`

**Status:** Completed stacked child.

Focused implementation, agent-surface validation, and full slice validation are green; published as stacked draft PR #459.

Completed candidate over `mavg-runtime-lod-milestone-v1`, stacked on `mavg-page-addressable-payload-schema-v1` / draft PR #456. The next active child is `mavg-directstorage-request-plan-v1`.

## Official Source Audit

Checked on 2026-06-05:

- Context7 selected `/websites/learn_microsoft_en-us` for Microsoft Learn, but the query returned broad Microsoft content rather than DirectStorage-specific API details. Direct Microsoft Learn pages are therefore the authoritative DirectStorage source for this slice.
- Microsoft Learn `DSTORAGE_SOURCE_FILE` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/ns-dstorage-dstorage_source_file`) documents `Source`, `Offset`, and `Size`; `Offset` is the byte offset in the file and `Size` is the number of bytes to read.
- Microsoft Learn `IDStorageFactory::OpenFile` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragefactory-openfile`) documents that DirectStorage opens a file for DirectStorage access.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue-enqueuerequest`) documents that requests are queued until `Submit` or queue-capacity behavior.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue3-enqueurequests`) documents request arrays synchronized with an `ID3D12Fence`.
- Microsoft Learn `DSTORAGE_ENQUEUE_REQUEST_FLAGS` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/ne-dstorage-dstorage_enqueue_request_flags`) documents fence wait points. This slice records the dependency but does not expose or use native fences.

## Scope

In scope:

- Add `IFileSystem::read_bytes`, `IFileSystem::read_byte_range`, and `IFileSystem::write_bytes` with default implementations that preserve existing subclass compatibility.
- Override binary/range APIs in `MemoryFileSystem` and `RootedFileSystem`.
- Preserve binary `NUL` bytes and fail closed for out-of-range byte requests.
- Add `RuntimeMavgPayloadPageFileLoadDesc`, `RuntimeMavgPayloadPageFileLoadRow`, `RuntimeMavgPayloadPageFileLoadResult`, diagnostics, and `load_runtime_mavg_payload_file_pages`.
- Validate graph and payload metadata before file reads.
- Read selected payload pages in request order from a caller-supplied blob path using validated page `byte_offset` / `byte_size`.
- Report explicit flags: file IO invoked, `used_native_directstorage=false`, no mount mutation, no background worker, no renderer/RHI handles, and no native DirectStorage.

Out of scope:

- Native DirectStorage factory/queue/status/fence objects.
- Win32 overlapped IO, autonomous worker ownership, async overlap or performance claims.
- Resident mount mutation, automatic eviction policy, GPU memory pressure integration.
- Renderer/RHI upload, native handles, Vulkan/Metal parity, mesh shaders, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization.

## Files

- Modify: `engine/platform/include/mirakana/platform/filesystem.hpp`
- Modify: `engine/platform/src/filesystem.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Modify: `engine/runtime/src/mavg_payload_pages.cpp`
- Test: `tests/unit/core_tests.cpp`
- Test: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`
- Create: `tools/check-ai-integration-113-mavg-payload-byte-range-file-io.ps1`

## Public API Contract

`IFileSystem` gains non-native binary file APIs:

```cpp
[[nodiscard]] virtual std::vector<std::uint8_t> read_bytes(std::string_view path) const;
[[nodiscard]] virtual std::vector<std::uint8_t> read_byte_range(std::string_view path, std::uint64_t byte_offset,
                                                                std::uint64_t byte_size) const;
virtual void write_bytes(std::string_view path, std::span<const std::uint8_t> bytes);
```

`MK_runtime` gains a file-backed MAVG page loader:

```cpp
struct RuntimeMavgPayloadPageFileLoadDesc {
    IFileSystem* filesystem{nullptr};
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::string_view payload_blob_path;
    std::span<const std::uint32_t> page_indices;
};

[[nodiscard]] RuntimeMavgPayloadPageFileLoadResult
load_runtime_mavg_payload_file_pages(const RuntimeMavgPayloadPageFileLoadDesc& desc);
```

## Tasks

- [x] Add RED tests for `MemoryFileSystem` and `RootedFileSystem` binary byte-range reads.
- [x] Add RED tests for runtime MAVG file page loading from a blob path and fail-closed short blob reads.
- [x] Run focused RED build for `MK_core_tests` and `MK_runtime_mavg_payload_pages_tests`; expected failure is missing binary filesystem/runtime file loader symbols.
- [x] Implement default `IFileSystem` binary APIs plus `MemoryFileSystem` and `RootedFileSystem` overrides.
- [x] Implement `load_runtime_mavg_payload_file_pages` using validated payload page rows and `IFileSystem::read_byte_range`.
- [x] Update docs/spec/registry/parent milestone/manifest fragments/static guards and compose manifest.
- [x] Run focused build and CTest for `MK_core_tests` and `MK_runtime_mavg_payload_pages_tests`.
- [x] Run public API boundary, targeted tidy, format, JSON/AI/agent checks, and `git diff --check`.
- [x] Run full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR over `codex/mavg-page-addressable-payload-schema-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_runtime_mavg_payload_pages_tests` | RED fails before implementation because binary filesystem/runtime file loader symbols are absent; later passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests|MK_runtime_mavg_payload_pages_tests"` | Focused platform/runtime tests pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public API checks pass with no native DirectStorage or renderer/RHI handle exposure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/src/filesystem.cpp,engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Targeted C++ static analysis passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Manifest is composed from fragment updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | JSON contracts pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | MAVG runtime LOD and byte-range file IO guard needles pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Agent-surface parity checks pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | C++ and tracked text formatting pass. |
| `git diff --check` | Whitespace is clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Full slice validation passes or records a concrete host/tool blocker. |

## Validation Evidence

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_runtime_mavg_payload_pages_tests` | RED failed before implementation on missing binary filesystem/runtime file loader symbols. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; wrote `engine/agent/manifest.json`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed: `agent-manifest-compose: ok`, `json-contract-check: ok`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed: `ai-integration-check: ok`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed: `public-api-boundary-check: ok`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_core_tests MK_runtime_mavg_payload_pages_tests` | Passed after implementation and tidy cleanup. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_core_tests\|MK_runtime_mavg_payload_pages_tests"` | Passed: 2/2 tests. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/platform/src/filesystem.cpp,engine/runtime/src/mavg_payload_pages.cpp,tests/unit/core_tests.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Passed: `tidy-check: ok (4 files)`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed: `agent-config-check: ok`. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed: `format-check: ok`. |
| 2026-06-05 | `git diff --check` | Passed with no whitespace errors. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed: `validate: ok`; Apple/Metal checks remained diagnostic/host-gated on the Windows host as expected. |
| 2026-06-05 | `gh pr create --draft --base codex/mavg-page-addressable-payload-schema-v1 --head codex/mavg-directstorage-byte-range-io-v1` | Published stacked draft PR #459. |

## Non-Claims

This plan does not claim native DirectStorage factory/queue/status/fence execution, Win32 async IO, autonomous background workers, async-overlap/performance, resident mount mutation, automatic eviction policy, GPU memory pressure enforcement, renderer/RHI residency, native handles, Vulkan/Metal readiness, mesh shaders, deformation, ray tracing, Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, or broad CPU/GPU/memory optimization.
