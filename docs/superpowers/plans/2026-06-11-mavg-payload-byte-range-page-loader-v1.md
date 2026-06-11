# 2026-06-11 MAVG Payload Byte-Range Page Loader v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic partial `.mavgpayload` byte-range page schema and side-effect-free runtime loader over existing MAVG graph page rows.

**Architecture:** Keep page byte offsets and sizes in `MK_assets` graph validation, and put the runtime page extraction API in `MK_runtime` beside the existing MAVG page streaming planners. The loader consumes caller-owned payload bytes and reviewed page indices; it does not add filesystem byte-range reads, background workers, DirectStorage, GPU upload, RHI handles, or GPU memory pressure integration.

**Tech Stack:** C++23, `MK_assets`, `MK_runtime`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-payload-byte-range-page-loader-v1`

**Status:** Completed through PR #569 / merge commit `187d10f`. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

Completed prerequisites:

- `mavg-asset-graph-v1` already defines `MavgClusterGraphPage::byte_offset` and `byte_size`, `MavgClusterGraphDocument::cluster_payload_uri`, and deterministic graph serialization.
- MAVG page streaming follow-ups already convert reviewed `MavgLodPageRequest` rows into package candidates, drain one selected candidate at a safe point, protect selected/fallback page mounts, and support caller-supplied recency eviction order.
- Existing runtime package loading reads whole text files through `IFileSystem::read_text`; this plan intentionally avoids changing the filesystem interface until a later byte-range IO plan.

## Files

- Modify: `engine/assets/include/mirakana/assets/mavg_cluster_graph.hpp`
- Modify: `engine/assets/src/mavg_cluster_graph.cpp`
- Create: `engine/runtime/include/mirakana/runtime/mavg_payload_page_loader.hpp`
- Create: `engine/runtime/src/mavg_payload_page_loader.cpp`
- Create: `tests/unit/runtime_mavg_payload_page_loader_tests.cpp`
- Modify: `tests/unit/mavg_cluster_graph_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-115-mavg-payload-byte-range-page-loader.ps1`

## Task 1 - Plan Activation And RED Coverage

- [x] Add this plan to the plan registry as the next MAVG child slice without changing `currentActivePlan`.
- [x] Add RED runtime loader tests for exact byte extraction, duplicate/missing/out-of-bounds page requests, invalid payload format, and zero file IO / zero background worker / zero renderer-RHI side effects.
- [x] Add RED graph validation coverage for invalid page byte ranges.
- [x] Register `MK_runtime_mavg_payload_page_loader_tests` near existing MAVG runtime tests.
- [x] Run focused configure/build and confirm the expected missing API failure before implementation.

## Task 2 - Graph Schema Validation

- [x] Add a public `mavg_cluster_payload_format_v1()` schema literal.
- [x] Add `invalid_page_byte_range` diagnostics for overflowing or overlapping page byte ranges while preserving valid non-overlapping current fixtures.
- [x] Keep serialization format compatible with existing `page.N.byte_offset` and `page.N.byte_size` rows; no migration shim is needed.

## Task 3 - Runtime Page Loader

- [x] Add `RuntimeMavgPayloadPageLoadDesc`, `RuntimeMavgPayloadPageRow`, `RuntimeMavgPayloadPageLoadResult`, diagnostics, counters, and `load_runtime_mavg_payload_pages`.
- [x] Validate graph asset, graph pointer, graph validation, payload format header, requested page existence, duplicate requests, and byte-range bounds before returning rows.
- [x] Copy only the requested byte ranges from caller-owned payload bytes and expose deterministic counters.
- [x] Keep loader flags proving no filesystem IO, mount mutation, background worker execution, DirectStorage execution, GPU memory policy integration, or renderer/RHI/native-handle access.

## Task 4 - Docs, Manifest, Static Guards, And Publication

- [x] Record the exact ready claim: runtime can validate and copy reviewed `.mavgpayload` page byte ranges from caller-owned payload bytes using graph page metadata.
- [x] Keep non-claims explicit: whole-file package replacement remains the existing runtime path, and filesystem byte-range reads, autonomous/background streaming, DirectStorage, GPU memory pressure integration, package-visible MAVG backend readiness, mesh shaders, Metal readiness, ray tracing, deformation, Nanite equivalence/superiority, and broad optimization remain unclaimed.
- [x] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [x] Run focused tests, static guards, public API checks, and full `tools/validate.ps1`.
- [x] Run publication preflight and `git diff --check`.

## Non-Claims

This plan does not claim autonomous/background MAVG streaming workers, async overlap, filesystem byte-range IO, DirectStorage execution, GPU memory pressure integration, renderer/RHI upload, package-visible MAVG backend readiness, mesh shaders, Metal readiness, deformation, ray tracing, benchmark results, Nanite compatibility, Nanite equivalence, Nanite superiority, or broad CPU/GPU/memory optimization.
