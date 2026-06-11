# 2026-06-11 MAVG Payload Filesystem Byte-Range IO v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic filesystem-backed `.mavgpayload` byte-range page loading path that reads only reviewed page ranges from `IFileSystem`.

**Architecture:** Extend the first-party platform filesystem contract with exact binary range reads, implemented by `MemoryFileSystem` and `RootedFileSystem`. Add a narrow `MK_runtime` MAVG helper that validates the graph and payload format, uses byte-range reads for only the requested page ranges, and returns the existing `RuntimeMavgPayloadPageLoadResult` shape without background workers, DirectStorage, GPU memory policy, renderer/RHI handles, or resident mount mutation.

**Tech Stack:** C++23, `MK_platform`, `MK_runtime`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-payload-filesystem-byte-range-io-v1`

**Status:** Active local child candidate. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

Completed prerequisites:

- `mavg-payload-byte-range-page-loader-v1` validates graph page byte ranges and extracts requested pages from caller-owned payload bytes.
- `MavgClusterGraphDocument::cluster_payload_uri` already identifies the reviewed `.mavgpayload` file path.
- `IFileSystem` currently supports text reads and rooted path validation; it does not yet expose binary byte-range reads.

## Files

- Modify: `engine/platform/include/mirakana/platform/filesystem.hpp`
- Modify: `engine/platform/src/filesystem.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/mavg_payload_page_loader.hpp`
- Modify: `engine/runtime/src/mavg_payload_page_loader.cpp`
- Modify: `tests/unit/core_tests.cpp`
- Modify: `tests/unit/runtime_mavg_payload_page_loader_tests.cpp`
- Modify: test-only `IFileSystem` implementations under `tests/unit/`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create or modify: focused static guard for MAVG filesystem byte-range IO if existing checks do not cover the new contract.

## Task 1 - Plan Activation And RED Coverage

- [x] Add this plan to the plan registry and mark the previous MAVG payload byte-range page loader slice completed through PR #569 / merge commit `187d10f`.
- [x] Add RED platform filesystem tests for exact binary range reads from `MemoryFileSystem` and `RootedFileSystem`, including out-of-bounds rejection.
- [x] Add RED runtime MAVG tests proving filesystem page loading reads only the format prefix and requested page byte ranges, preserves request order, and rejects duplicate, missing, and out-of-bounds pages without returning partial rows.
- [x] Run focused builds/tests and confirm failures are due to missing binary range IO and missing runtime filesystem loader API.

## Task 2 - Platform Binary Range Contract

- [x] Add `IFileSystem::read_binary_range(std::string_view path, std::uint64_t byte_offset, std::uint64_t byte_size)`.
- [x] Implement `MemoryFileSystem::read_binary_range` with exact range slicing over stored bytes.
- [x] Implement `RootedFileSystem::read_binary_range` with rooted path validation, binary open, file-size bounds checks, seek, exact read, and no whole-file load.
- [x] Keep test-only filesystem implementations fail-closed through the base contract unless a test explicitly overrides range reads.

## Task 3 - Runtime MAVG Filesystem Loader

- [x] Add `RuntimeMavgPayloadFilesystemPageLoadDesc` with graph asset, graph pointer, filesystem pointer, payload path, and requested page indices.
- [x] Add `load_runtime_mavg_payload_pages_from_filesystem`.
- [x] Validate graph asset, graph pointer, filesystem pointer, payload path, graph asset match, graph validation, payload format prefix, requested page existence, duplicate page requests, byte-range overflow, and filesystem range failures before returning page rows.
- [x] Populate `invoked_file_io=true`, exact loaded page rows, requested/loaded counters, and read byte counters while keeping background worker, DirectStorage, GPU memory policy, renderer/RHI handle, and mount mutation flags false.

## Task 4 - Docs, Manifest, Static Guards, And Publication

- [x] Record the exact ready claim: runtime can load reviewed `.mavgpayload` pages via first-party filesystem binary byte-range reads.
- [x] Keep non-claims explicit: no autonomous/background streaming, async overlap/performance proof, DirectStorage, GPU memory pressure integration, package-visible MAVG backend readiness, mesh shaders, Metal readiness, ray tracing, deformation, benchmark results, Nanite equivalence/superiority, or broad optimization.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, publication preflight, and `git diff --check`.

## Non-Claims

This plan does not claim autonomous/background MAVG streaming workers, async overlap, DirectStorage execution, GPU memory pressure integration, resident mount mutation, renderer/RHI upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
