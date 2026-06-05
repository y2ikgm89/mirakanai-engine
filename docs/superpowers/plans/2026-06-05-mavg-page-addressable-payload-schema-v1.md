# 2026-06-05 MAVG Page-Addressable Payload Schema v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-page-addressable-payload-schema-v1`

**Status:** Active.

**Execution State:** Stacked on `mavg-package-streaming-residency-dispatch-v1` / draft PR #454. This child promotes only deterministic `.mavgpayload` page table validation and caller-owned payload slice extraction from future work to implementation evidence.

**Goal:** Add a page-addressable `GameEngine.MavgClusterPayload.v1` schema so cooked MAVG payloads publish deterministic page byte ranges and runtime code can validate/extract selected page slices from caller-supplied payload text without file IO, worker ownership, renderer/RHI residency, or async-overlap claims.

**Architecture:** Keep MAVG payload parsing and validation in `MK_assets` because `GameEngine.MavgClusterPayload.v1` is an asset schema. `MK_tools` serializes the page table from the existing cooked graph rows, and `MK_runtime` exposes only a thin no-IO extraction helper over caller-supplied payload text plus a caller-owned graph document. DirectStorage/Win32 queues remain future execution work; this slice creates the metadata that such queues would later consume.

**Tech Stack:** C++23, `MK_assets`, `MK_tools`, `MK_runtime`, `GameEngine.MavgClusterGraph.v1`, `GameEngine.MavgClusterPayload.v1`, existing package streaming/resident mount contracts, Context7 Microsoft Learn documentation, Microsoft Learn DirectStorage `DSTORAGE_SOURCE_FILE` / `IDStorageQueue::EnqueueRequest` docs, CMake/CTest, PowerShell 7 validation scripts.

---

## Official Source Audit

Checked on 2026-06-05:

- Context7 `/websites/learn_microsoft_en-us`: selected Microsoft Learn as the official Microsoft documentation source. Context7 returned broad Microsoft Learn content and did not provide enough DirectStorage-specific byte-range details for the schema layer, so direct Microsoft Learn pages below are used as the primary source for DirectStorage API constraints.
- Microsoft Learn `DSTORAGE_SOURCE_FILE` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/ns-dstorage-dstorage_source_file`): DirectStorage file requests use explicit file, byte offset, and byte size fields. This supports making page byte ranges durable metadata before adding any DirectStorage queue implementation.
- Microsoft Learn `IDStorageQueue::EnqueueRequest` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nf-dstorage-idstoragequeue-enqueuerequest`): requests remain queued until `Submit` or queue-capacity behavior; this slice avoids hidden IO submission and only emits/extracts byte ranges.
- Microsoft Learn `IDStorageQueue` (`https://learn.microsoft.com/en-us/windows/win32/dstorage/dstorage/nn-dstorage-idstoragequeue`): queues own read/status/fence submission semantics. This slice keeps those OS/native objects out of public MAVG payload and runtime APIs.

## Scope

In scope:

- `MK_assets` `mavg_cluster_payload.hpp` / `.cpp` schema rows for `GameEngine.MavgClusterPayload.v1`.
- Payload page table rows with page index, byte offset, byte size, first cluster, cluster count, and deterministic `page.data_hex` logical page bytes.
- Cluster-to-page rows that mirror graph cluster page membership and draw ranges.
- Validation against caller-supplied `MavgClusterGraphDocument`: graph asset match, graph validity, duplicate/missing/unknown pages, byte-range overflow/out-of-bounds, overlapping page ranges, and cluster-page mismatch.
- `MK_tools` cook output that serializes page table rows deterministically from `MavgClusterGraphDocument::pages`.
- `MK_runtime` no-IO helper that validates a caller-supplied payload text and extracts selected page slices as decoded page payload bytes.
- Explicit non-claim flags for no file IO, no resident mount mutation, no background worker, no renderer/RHI handles, and no async/performance claim.

Out of scope:

- DirectStorage/Win32 async IO, native queue/fence/status handles, file opening, or OS worker ownership.
- Autonomous background package streaming workers or async-overlap/performance evidence.
- Automatic eviction policy or GPU memory pressure enforcement.
- Renderer/RHI residency, GPU upload, Vulkan/Metal backend proof, mesh shaders, deformation, ray tracing, Nanite compatibility/equivalence/superiority, benchmark superiority, or broad optimization.

## Files

- Create: `engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp`
- Create: `engine/assets/src/mavg_cluster_payload.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Modify: `engine/tools/asset/mavg_cluster_cook.cpp`
- Create: `engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp`
- Create: `engine/runtime/src/mavg_payload_pages.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Create: `tests/unit/mavg_cluster_payload_tests.cpp`
- Test: `tests/unit/tools_mavg_cluster_cook_tests.cpp`
- Create: `tests/unit/runtime_mavg_payload_pages_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-05-mavg-runtime-lod-milestone-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`
- Create: `tools/check-ai-integration-112-mavg-page-addressable-payload-schema.ps1`

## Public API Contract

`engine/assets/include/mirakana/assets/mavg_cluster_payload.hpp` owns the schema:

```cpp
namespace mirakana {

struct MavgClusterPayloadPage {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::uint32_t first_cluster{0};
    std::uint32_t cluster_count{0};
};

struct MavgClusterPayloadCluster {
    std::uint32_t cluster_index{0};
    std::uint32_t page_index{0};
    std::uint32_t first_index{0};
    std::uint32_t index_count{0};
    std::int32_t vertex_base{0};
};

struct MavgClusterPayloadDocument {
    AssetId asset;
    std::uint32_t vertex_count{0};
    std::uint32_t vertex_stride_bytes{0};
    std::string vertex_data_hex;
    std::uint32_t index_count{0};
    std::string index_format;
    std::string index_data_hex;
    std::string page_data_hex;
    std::vector<MavgClusterPayloadPage> pages;
    std::vector<MavgClusterPayloadCluster> clusters;
};

enum class MavgClusterPayloadDiagnosticCode : std::uint8_t {
    invalid_asset,
    graph_asset_mismatch,
    invalid_graph,
    invalid_payload_format,
    invalid_vertex_payload,
    invalid_index_payload,
    missing_page,
    duplicate_page_index,
    unknown_graph_page,
    missing_graph_page,
    invalid_page_range,
    overlapping_page_range,
    missing_cluster,
    duplicate_cluster_index,
    unknown_graph_cluster,
    missing_graph_cluster,
    cluster_page_mismatch,
    cluster_draw_range_mismatch,
};

struct MavgClusterPayloadValidationResult {
    std::vector<MavgClusterPayloadDiagnostic> diagnostics;
    [[nodiscard]] bool valid() const noexcept;
};

[[nodiscard]] std::string_view mavg_cluster_payload_format_v1() noexcept;
[[nodiscard]] MavgClusterPayloadDocument deserialize_mavg_cluster_payload_document(std::string_view text);
[[nodiscard]] std::string serialize_mavg_cluster_payload_document(const MavgClusterPayloadDocument& document);
[[nodiscard]] MavgClusterPayloadValidationResult
validate_mavg_cluster_payload(const MavgClusterPayloadDocument& payload, const MavgClusterGraphDocument& graph);

} // namespace mirakana
```

`engine/runtime/include/mirakana/runtime/mavg_payload_pages.hpp` owns the no-IO extraction helper:

```cpp
namespace mirakana::runtime {

struct RuntimeMavgPayloadPageSliceDesc {
    const MavgClusterGraphDocument* graph{nullptr};
    std::string_view payload_text;
    std::span<const std::uint32_t> page_indices;
};

struct RuntimeMavgPayloadPageSliceRow {
    std::uint32_t page_index{0};
    std::uint64_t byte_offset{0};
    std::uint64_t byte_size{0};
    std::vector<std::uint8_t> payload_bytes;
};

struct RuntimeMavgPayloadPageSliceResult {
    std::vector<RuntimeMavgPayloadPageSliceRow> pages;
    std::vector<RuntimeMavgPayloadPageSliceDiagnostic> diagnostics;
    std::size_t requested_page_count{0};
    std::size_t extracted_page_count{0};
    bool invoked_file_io{false};
    bool mutated_mount_set{false};
    bool executed_background_worker{false};
    bool touched_renderer_or_rhi_handles{false};
    [[nodiscard]] bool succeeded() const noexcept;
};

[[nodiscard]] RuntimeMavgPayloadPageSliceResult
extract_runtime_mavg_payload_page_slices(const RuntimeMavgPayloadPageSliceDesc& desc);

} // namespace mirakana::runtime
```

## Tasks

- [x] Add RED tests in `tests/unit/mavg_cluster_payload_tests.cpp` proving payload schema validation accepts deterministic page-addressable payload rows and rejects duplicate/overlapping/out-of-range rows against a graph.
- [x] Add RED tests in `tests/unit/tools_mavg_cluster_cook_tests.cpp` proving cook output includes `page.count`, page byte ranges, and cluster page metadata in `GameEngine.MavgClusterPayload.v1`.
- [x] Add RED tests in `tests/unit/runtime_mavg_payload_pages_tests.cpp` proving selected page slices are extracted in request order, fail closed for missing/duplicate/unknown page requests, and report no file IO, worker, mount mutation, or renderer/RHI side effects.
- [x] Run RED build for `MK_mavg_cluster_payload_tests`, `MK_tools_mavg_cluster_cook_tests`, and `MK_runtime_mavg_payload_pages_tests`; expected failure is missing payload schema/runtime symbols.
- [x] Implement `mavg_cluster_payload.hpp` / `.cpp` with canonical serialization, deserialization, and validation.
- [x] Update `engine/tools/asset/mavg_cluster_cook.cpp` to serialize payload documents through the new schema helper.
- [x] Implement `mavg_payload_pages.hpp` / `.cpp` using only caller-supplied `std::string_view` payload text and the validated graph document.
- [x] Register new source files and tests in CMake.
- [x] Update docs, parent milestone, registry, manifest fragments, composed manifest, and static guards.
- [x] Run focused C++ build, focused CTest, public API boundary, tidy, format, JSON/AI/agent checks, `git diff --check`, then full `tools/validate.ps1`.
- [x] Publish a validated stacked draft PR over `codex/mavg-package-streaming-residency-dispatch-v1`.

## Validation Plan

| Command | Expected Evidence |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_payload_tests MK_tools_mavg_cluster_cook_tests MK_runtime_mavg_payload_pages_tests` | RED fails before implementation because new schema/runtime symbols are absent; later passes after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_payload_tests|MK_tools_mavg_cluster_cook_tests|MK_runtime_mavg_payload_pages_tests"` | Focused asset/tool/runtime MAVG payload tests pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Public API checks pass with no native/renderer/RHI handle exposure. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/mavg_cluster_payload.cpp,engine/tools/asset/mavg_cluster_cook.cpp,engine/runtime/src/mavg_payload_pages.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Targeted C++ static analysis passes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Manifest is composed from fragments after active plan and module claim updates. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | JSON contracts pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | MAVG runtime LOD and payload schema guard needles prove docs/manifest/static alignment. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Agent-surface parity checks pass. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | C++ and tracked text formatting pass. |
| `git diff --check` | Whitespace is clean. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Full slice validation passes or records a concrete host/tool blocker. |

## Validation Evidence

| Date | Command | Result |
| --- | --- | --- |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | Passed before focused RED/GREEN build. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_payload_tests MK_tools_mavg_cluster_cook_tests MK_runtime_mavg_payload_pages_tests` | RED failed as expected before implementation because `mirakana/assets/mavg_cluster_payload.hpp` was absent. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_payload_tests MK_tools_mavg_cluster_cook_tests MK_runtime_mavg_payload_pages_tests` | Passed after implementation. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_payload_tests|MK_tools_mavg_cluster_cook_tests|MK_runtime_mavg_payload_pages_tests"` | Passed; 3/3 focused MAVG payload tests passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; composed `engine/agent/manifest.json` from fragment updates. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after restoring retained prerequisite evidence needles and adding the page-addressable payload schema guard. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed with no native/renderer/RHI handle exposure. |
| 2026-06-05 | `git diff --check` | Passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Initially failed on `engine/assets/src/mavg_cluster_payload.cpp`; `tools/format.ps1` was run and the check then passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_cluster_payload_tests MK_tools_mavg_cluster_cook_tests MK_runtime_mavg_payload_pages_tests` | Passed after formatting. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_mavg_cluster_payload_tests|MK_tools_mavg_cluster_cook_tests|MK_runtime_mavg_payload_pages_tests"` | Passed after formatting; 3/3 focused MAVG payload tests passed. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/assets/src/mavg_cluster_payload.cpp,engine/tools/asset/mavg_cluster_cook.cpp,engine/runtime/src/mavg_payload_pages.cpp,tests/unit/mavg_cluster_payload_tests.cpp,tests/unit/runtime_mavg_payload_pages_tests.cpp -ReuseExistingFileApiReply` | Passed; clang-tidy checked 5 files. |
| 2026-06-05 | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed; static checks, build, tidy smoke, and 109/109 tests passed. |
| 2026-06-05 | `gh pr create --draft --base codex/mavg-package-streaming-residency-dispatch-v1 --head codex/mavg-page-addressable-payload-schema-v1` | Created stacked draft PR #456. |

## Non-Claims

This plan does not claim DirectStorage/Win32 async IO integration, native queue/fence/status handles, file opening, autonomous background workers, async-overlap/performance, automatic eviction policy, GPU memory pressure enforcement, renderer/RHI residency, native handles, Vulkan/Metal readiness, mesh shaders, deformation, ray tracing, Nanite compatibility, Nanite equivalence, Nanite superiority, benchmark superiority, or broad CPU/GPU/memory optimization.
