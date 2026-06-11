# MAVG Streaming Upload Overlap Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a narrow value-only evidence row that can verify caller-supplied temporal overlap between reviewed MAVG background package loads and streamed-cluster GPU upload results.

**Architecture:** This slice starts from `origin/main` and stays independent of open DirectStorage, backend draw, and persistent streaming service PRs. The new `MK_runtime_rhi` helper consumes existing closeout, safe-point adoption, and streamed GPU upload result rows plus caller-owned timing windows through `RuntimeMavgStreamingUploadOverlapEvidenceDesc`, `RuntimeMavgStreamingUploadOverlapEvidenceResult`, `background_load_window`, `gpu_upload_window`, and `plan_runtime_mavg_streaming_upload_overlap_evidence`; it records `recorded_temporal_overlap_evidence` and `overlap_tick_count` without executing work, mutating mounts/catalogs, touching native handles, or claiming speedup.

**Tech Stack:** C++23, `MK_runtime_rhi`, `MK_runtime`, `MK_renderer`, CMake/CTest, PowerShell validation scripts.

---

**Plan ID:** `mavg-streaming-upload-overlap-evidence-v1`

## Context

Current completed evidence includes `mavg-background-streaming-dispatch-v1`, `mavg-cluster-streaming-residency-closeout-v1`, `mavg-cluster-streaming-safe-point-adoption-v1`, and `mavg-streamed-cluster-gpu-upload-v1`. Remaining unclaimed work includes persistent/autonomous streaming services, async-overlap/performance proof, DirectStorage execution, backend draw execution, mesh shaders, Metal readiness, Nanite equivalence/superiority, and broad optimization.

This plan only records deterministic temporal overlap evidence from caller-owned timing windows. It keeps `claimed_speedup=false` and `proved_async_overlap_performance=false`; it does not measure speedup, execute asynchronous work, execute DirectStorage, execute backend execution, run mesh shaders, touch native handles, claim Metal readiness, claim Nanite equivalence/superiority, or promote broad async-overlap/performance proof or broad optimization.

## Task 1: Runtime-RHI Overlap Evidence Contract

**Files:**
- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp`
- Create: `engine/runtime_rhi/src/mavg_streaming_upload_overlap_evidence.cpp`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Test: `tests/unit/runtime_rhi_mavg_streaming_upload_overlap_evidence_tests.cpp`

- [x] Write a failing test that builds successful closeout/adoption/upload value rows plus overlapping background/upload windows and expects `recorded_temporal_overlap_evidence=true`, positive overlap ticks, copied counters, and all unsupported execution flags false.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests` and confirm the test target fails to compile because the new API does not exist.
- [x] Add the public header, implementation, and CMake wiring with minimal validation and deterministic interval intersection.
- [x] Run the focused build and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests --output-on-failure`.
- [x] Add fail-closed tests for missing input evidence, non-overlapping windows, invalid windows, mismatched window clock domains, source graph mismatches, failed closeout evidence, missing row counters, and speedup-proof requests without measured budget rows.

## Task 2: Docs, Manifest, And Static Guards

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Modify or add: `tools/check-ai-integration-*.ps1`

- [x] Document the narrow overlap evidence claim and keep persistent service, DirectStorage, backend draw, mesh shader, broad async/performance, and broad optimization non-claims intact.
- [x] Update manifest fragments and compose with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Add static needles for the new header, result flags, docs, manifest text, and non-claims.
- [x] Run `check-ai-integration`, `check-json-contracts`, `check-public-api-boundaries`, focused tidy, and full `tools/validate.ps1`.

## Validation Evidence

- TDD red: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests` failed before the API existed with missing include `mirakana/runtime_rhi/mavg_streaming_upload_overlap_evidence.hpp`.
- Focused build/test: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests` passed, then `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests --output-on-failure` passed with 1/1 tests and 0 failures.
- Drift/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `git diff --check` passed.
- Focused static/C++ checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files "engine/runtime_rhi/src/mavg_streaming_upload_overlap_evidence.cpp,tests/unit/runtime_rhi_mavg_streaming_upload_overlap_evidence_tests.cpp"` passed; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_streaming_upload_overlap_evidence_tests` and focused CTest passed after formatting.
- Full slice gate: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed with `validate: ok`; CTest reported 119/119 tests passed, 0 failed. Diagnostic-only host gates remained expected for Metal/Apple tooling on this Windows host.
