# MAVG DirectStorage GDeflate Execution Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party value-only evidence gate for reviewed DirectStorage GDeflate GPU decompression execution rows without promoting Zstd preview, performance, package-visible backend readiness, broad optimization, Nanite, or external-engine compatibility claims.

**Architecture:** The gate lives in `MK_runtime_rhi` beside the DirectStorage policy and GPU destination evidence gates. It accepts retained host evidence rows that prove stable DirectStorage 1.1 GDeflate format selection, stable DirectStorage 1.3 queue/destination synchronization, request option review, `GetCompressionSupport` GPU path review, GPU decompression staging, deterministic readback hash comparison, and package-visible output rows. The gate exposes no native DirectStorage, D3D12, OS, or SDK handles and does not copy Microsoft samples, GDeflate implementation code, Zstd shaders, GACL code, or external engine material.

**Tech Stack:** C++23, CMake `dev` preset, first-party test framework, PowerShell 7 validators/static guards, JSON Schema draft 2020-12, Context7 Microsoft Learn fallback, Microsoft DirectStorage official docs/blog/GitHub guidance, and clean-room legal/originality guardrails.

---

**Plan ID:** `mavg-directstorage-gdeflate-execution-evidence-v1`

**Status:** Implemented and validated locally in branch `codex/mavg-directstorage-gdeflate-execution-evidence`; hosted CI and merge are pending.

**Date:** 2026-07-02

**Public contract:** `mavg_directstorage_gdeflate_execution.hpp` exposes `RuntimeMavgDirectStorageGDeflateExecutionRow`, `RuntimeMavgDirectStorageGDeflateExecutionResult`, and `evaluate_runtime_mavg_directstorage_gdeflate_execution_evidence`. The retained schema is `GameEngine.MavgDirectStorageGDeflateExecutionEvidence.v1`, and the default validator is `tools/validate-mavg-directstorage-gdeflate-execution.ps1`.

## Official Source Refresh

Context7 status on 2026-07-02: `resolve-library-id` for Microsoft DirectStorage selected `/websites/learn_microsoft_en-us`, but the DirectStorage-specific query returned unrelated generic Microsoft Learn snippets. Treat Context7 as checked but insufficient for this SDK slice; use the official Microsoft fallback sources below.

Authoritative source rows for this plan:

- Microsoft Learn `DSTORAGE_COMPRESSION_FORMAT`: `DSTORAGE_COMPRESSION_FORMAT_GDEFLATE` is the built-in GDeflate format intended for GPU decompression.
- Microsoft Learn `DSTORAGE_REQUEST_OPTIONS`: request options carry `CompressionFormat`, `SourceType`, `DestinationType`, and reserved bits that must be zero.
- Microsoft Learn `IDStorageQueue2::GetCompressionSupport`: the DirectStorage runtime reports whether the selected decompression path is optimized GPU, fallback GPU shader, or CPU fallback.
- Microsoft Learn `DSTORAGE_COMPRESSION_SUPPORT`: GPU optimized/fallback and compute/copy queue support are distinct from CPU fallback.
- Microsoft Learn `IDStorageQueue3::EnqueueRequests`: enqueues arrays of DirectStorage requests synchronized with an `ID3D12Fence`.
- Microsoft Learn `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE`: D3D12 destination resources receive contiguous subresource output and the first subresource must be in `D3D12_RESOURCE_STATE_COMMON`.
- Microsoft DirectStorage 1.1 release post: GDeflate and GPU decompression are available in the stable 1.1 path.
- Microsoft DirectStorage 1.3 release post: `EnqueueRequests` and `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE` are stable 1.3 features.
- Microsoft DirectStorage developer guidance: GPU decompression uses VRAM input/output staging buffers sized through `IDStorageFactory::SetStagingBufferSize`.
- Microsoft DirectStorage 1.4/Zstd post: Zstd/GACL is a public-preview path that requires explicitly authored Zstd/GACL assets and explicit API usage, so it remains non-promoted here.

## Scope

This plan closes only a retained evidence contract for future host-supplied DirectStorage GDeflate execution artifacts. It does not implement live DirectStorage execution, add DirectStorage SDK dependencies, expose native handles, change package streaming, claim performance, promote package-visible MAVG backend readiness, or claim compatibility/equivalence/replacement/superiority with Unity, Unreal Engine, Godot, Nanite, or any external engine.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gdeflate_execution.hpp`
- Create: `engine/runtime_rhi/src/mavg_directstorage_gdeflate_execution.cpp`
- Create: `tests/unit/runtime_rhi_mavg_directstorage_gdeflate_execution_tests.cpp`
- Create: `schemas/mavg-directstorage-gdeflate-execution.schema.json`
- Create: `tools/validate-mavg-directstorage-gdeflate-execution.ps1`
- Create: `tools/check-ai-integration-157-mavg-directstorage-gdeflate-execution.ps1`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify after compose: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`

## Task 1: TDD Public Contract

- [x] Add `MK_runtime_rhi_mavg_ds_gdeflate_execution_tests` and a failing test file that includes `mirakana/runtime_rhi/mavg_directstorage_gdeflate_execution.hpp`.
- [x] Verify the red build fails because the public header is absent.
- [x] Implement the public value API and evaluator.

Required behavior:

- A reviewed DirectStorage 1.1 GDeflate row plus DirectStorage 1.3 GPU destination execution evidence can make `mavg_directstorage_gdeflate_execution_ready=true` and `mavg_directstorage_gpu_destination_execution_ready=true`.
- A `d3d12_multiple_subresources_range` row additionally promotes `mavg_directstorage_multiple_subresources_range_execution_ready=true`.
- `GetCompressionSupport` must be queried for GDeflate, and the row must report GPU optimized support or GPU fallback shader support plus a compute/copy queue path. CPU fallback is not accepted as GPU execution evidence.
- Missing request options, GDeflate compression format, source/destination type review, zeroed reserved bits, compression support review, staging buffer configuration, queue/fence synchronization, hash comparison, completion, status success, or package-visible output remains `host_evidence_required`.
- Invalid artifact hashes, mismatched byte counts, invalid subresource ranges, DirectStorage 1.4 preview selection, native handles, performance/package/broad/Nanite/external-engine claims are `blocked`.

## Task 2: Schema, Validator, Static Guard

- [x] Add `GameEngine.MavgDirectStorageGDeflateExecutionEvidence.v1` schema with closed rows, source ids, request option fields, compression support fields, byte counts, retained SHA-256 artifacts, summary counters, and explicit false unsupported claims.
- [x] Add `tools/validate-mavg-directstorage-gdeflate-execution.ps1` that builds/runs the focused test, checks docs/schema/manifest needles, and emits default host-gated counters with `mavg_directstorage_gdeflate_execution_ready=0`.
- [x] Add static guard chapter `157` to protect public API names, native-handle absence, schema literals, validator counters, docs/manifest non-claims, and forbidden ready claims outside the new GDeflate execution gate.

## Task 3: Docs, Manifest, And Closeout

- [x] Update current capabilities, roadmap, plan registry, and MAVG master plan with the retained host-gated GDeflate execution evidence contract and exact non-claims.
- [x] Add manifest command, `MK_runtime_rhi` public header/recent evidence, and retained production-loop evidence fragment rows, then run `tools/compose-agent-manifest.ps1 -Write`.
- [x] Run focused validator, public API boundary, agent, JSON contract, AI integration, full validation, and publication preflight.
- [ ] Commit, push, draft PR, hosted CI/PR Gate, ready conversion, and guarded auto-merge.

## Expected Default Counters

```text
validation_recipe=mavg-directstorage-gdeflate-execution-evidence
mavg_directstorage_gdeflate_execution_status=host_evidence_required
mavg_directstorage_gdeflate_execution_ready=0
mavg_directstorage_gpu_destination_execution_ready=0
mavg_directstorage_multiple_subresources_range_execution_ready=0
mavg_directstorage_zstd_preview_ready=0
mavg_directstorage_native_handles_exposed=0
mavg_directstorage_performance_ready=0
mavg_package_visible_backend_readiness_ready=0
mavg_broad_cpu_gpu_memory_optimization_ready=0
mavg_nanite_compatible=0
mavg_nanite_equivalent=0
mavg_nanite_superior=0
mavg_external_engine_compatibility=0
```

## Legal And Originality Boundary

This plan uses Microsoft official documentation only as constraints. It does not copy DirectStorage samples, SDK headers into public APIs, GDeflate reference implementation code, Zstd GPU shader code, GACL content-pipeline code, Unity/Unreal/Godot code/assets/UI/schema, or Nanite implementation details. It does not provide legal advice or legal approval.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` | Passed on 2026-07-02 with normalized configure/build environment, linked worktree, vcpkg junction, CMake 3.31.6, CTest 3.31.6, MSVC BuildTools, and clang-format 19.1.5 ready. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-mavg-directstorage-gdeflate-execution.ps1` | Passed on 2026-07-02 after fast-forwarding to `origin/main`; built and ran `MK_runtime_rhi_mavg_ds_gdeflate_execution_tests` with `1/1` passed and emitted default host-gated counters with `mavg_directstorage_gdeflate_execution_ready=0`, `mavg_directstorage_gpu_destination_execution_ready=0`, `mavg_directstorage_multiple_subresources_range_execution_ready=0`, `mavg_directstorage_zstd_preview_ready=0`, and all broad/non-clean-room claims at `0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed on 2026-07-02 with `agent-manifest-compose: ok` and `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed on 2026-07-02; chapter 157 protects the GDeflate execution API, schema, validator, manifest/docs needles, native-handle absence, and unsupported-claim zero rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed on 2026-07-02. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed on 2026-07-02. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed on 2026-07-02. |
| `git diff --check` | Passed on 2026-07-02. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed on 2026-07-02; static checks passed, build succeeded, and CTest reported `170/170` passed. |
