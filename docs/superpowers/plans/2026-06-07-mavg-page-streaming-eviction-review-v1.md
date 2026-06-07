# MAVG Page Streaming Eviction Review v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `mavg-page-streaming-eviction-review-v1`

**Status:** Completed.

**Goal:** Add a caller-reviewed MAVG page eviction review surface that protects selected visible cluster pages and resident fallback ancestors before delegating to existing reviewed resident package eviction planning.

**Architecture:** This is a post-milestone runtime-only follow-up over `mavg-page-streaming-queue-v1`. The new API stays value-only in `MK_runtime`, observes a caller-owned `MavgClusterGraphDocument`, selected cluster rows, resident page-to-mount rows, and reviewed candidate unmount order, then returns protected mount ids and the delegated `plan_runtime_resident_package_evictions_v2` result. It does not infer an automatic eviction policy, mutate mounts, read files, spawn workers, or touch renderer/RHI/native handles.

**Tech Stack:** C++23, `MK_runtime`, `MK_assets` MAVG graph rows, existing runtime resident package eviction planner, CMake/CTest, repository PowerShell validation tools.

---

## Context

`MAVG Runtime LOD Milestone v1` and `MAVG Page Streaming Queue v1` are completed. This slice should not reopen those completed plans and should not select a new production-completion gap. It narrows the next streaming/residency step to explicit eviction review invariants only.

## Constraints

- Keep `MK_runtime` independent from renderer/RHI/native handles and background worker ownership.
- Use existing `RuntimeResidentPackageMountSetV2` and `plan_runtime_resident_package_evictions_v2`.
- Require caller-reviewed resident page mount rows and caller-reviewed candidate unmount order; do not infer LRU/frequency/GPU-memory policy here.
- Preserve visible selected pages and resident fallback ancestor pages from eviction.
- Report diagnostics for graph mismatch, unknown clusters/pages, invalid/duplicate/missing page mounts, invalid/duplicate caller-protected mounts, and delegated eviction-plan failures.
- Do not claim autonomous/background streaming, automatic eviction, partial `.mavgpayload` byte-range loading, GPU memory pressure integration, GPU culling, indirect draw execution, mesh shaders, Metal readiness, Nanite equivalence, or broad optimization.

## Files

- Modify: `engine/runtime/include/mirakana/runtime/mavg_page_streaming.hpp`
- Modify: `engine/runtime/src/mavg_page_streaming.cpp`
- Modify: `tests/unit/runtime_mavg_page_streaming_tests.cpp`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify: `tools/check-ai-integration-104-mavg-runtime-lod.ps1`

## Tasks

### Task 1: Add Eviction Review Tests

- [x] Add test helpers for mounting resident page packages without file IO.
- [x] Add a passing-shape test proving selected visible page and fallback ancestor mounts are protected while a reviewed unrelated candidate can be planned for eviction.
- [x] Add a rejection test proving a reviewed candidate that targets a protected visible page fails through the delegated eviction planner without mutating the mount set.
- [x] Add a rejection test proving invalid selected clusters and missing resident page mounts fail before eviction planning.

### Task 2: Implement Runtime Eviction Review API

- [x] Add diagnostic enum values for selected graph mismatch, unknown cluster, page mount validation, protected mount validation, and eviction plan failure.
- [x] Add `RuntimeMavgResidentPageMountRow`, `RuntimeMavgPageStreamingSelectedClusterRow`, `RuntimeMavgPageStreamingEvictionReviewDesc`, and `RuntimeMavgPageStreamingEvictionReviewResult`.
- [x] Implement `review_runtime_mavg_page_streaming_evictions` as a pure planner that validates graph and resident page mount rows, computes protected visible/fallback mount ids, delegates to `plan_runtime_resident_package_evictions_v2`, and preserves zero side-effect flags.
- [x] Keep selected/fallback protection deterministic and duplicate-protection accounting explicit.

### Task 3: Sync Docs, Manifest, And Static Checks

- [x] Update current capabilities, roadmap, architecture spec, and plan registry with the narrow eviction review evidence.
- [x] Update `MK_runtime` manifest fragment and production-loop fragment without reopening the completed MAVG runtime LOD milestone.
- [x] Compose `engine/agent/manifest.json`.
- [x] Extend `tools/check-ai-integration-104-mavg-runtime-lod.ps1` needles for the eviction review API, tests, docs, and manifest.

### Task 4: Validate And Publish

- [x] Run focused build/test for `MK_runtime_mavg_page_streaming_tests`, `MK_runtime_mavg_lod_residency_tests`, and `MK_runtime_package_streaming_resident_mount_tests`.
- [x] Run `tools/check-public-api-boundaries.ps1`, targeted `tools/check-tidy.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, `git diff --check`, and full `tools/validate.ps1`.
- [x] Run `tools/check-publication-preflight.ps1` before staging, push, PR creation, ready conversion, and auto-merge registration.
- [x] Open a PR from `codex/mavg-page-streaming-eviction-review-main-v1` with validation evidence.

## Done When

- Selected visible cluster pages and resident fallback ancestor pages become protected mount ids before reviewed eviction planning.
- Invalid graph, selected cluster, page mount, protected mount, and delegated eviction failure cases are deterministic and tested.
- The API remains side-effect-free: no file IO, mount mutation, background worker, renderer/RHI, native handle, automatic policy, or GPU memory pressure claim.
- Docs, plan registry, manifest fragments, composed manifest, and static checks reflect exactly the new eviction review capability.
- Focused validation, static/agent checks, full validation, hosted PR checks, and publication flow pass or record a concrete blocker.
