# 2026-07-01 MAVG GPU DirectStorage GDeflate Policy v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party value-only policy/evidence gate for MAVG DirectStorage GPU destinations, GDeflate GPU decompression, and DirectStorage 1.4/Zstd preview non-promotion.

**Architecture:** Keep this slice in `MK_runtime_rhi` as a pure reviewed-row evaluator. It must not execute DirectStorage, expose `dstorage.h`, `D3D12`, `HANDLE`, `IUnknown`, `void*`, or backend-native objects, mutate packages, schedule IO, or promote package-visible MAVG backend readiness. The Win32 DirectStorage SDK adapter remains system-memory byte-range IO only; this plan only records what exact GPU destination/GDeflate evidence would be required before future execution rows can be accepted.

**Tech Stack:** C++23, `MK_runtime_rhi`, CMake/CTest, PowerShell 7 validation wrappers, JSON Schema draft 2020-12, Microsoft DirectStorage official documentation and SDK release notes.

---

**Plan ID:** `mavg-gpu-directstorage-gdeflate-policy-v1`

**Status:** Validated implementation ready for publication.

**Date:** 2026-07-01

## Official Source Refresh

Context7 status on 2026-07-01: `resolve-library-id` for Microsoft DirectStorage selected `/websites/learn_microsoft_en-us`, but the DirectStorage-specific query returned unrelated Microsoft Learn snippets. Treat Context7 as checked but insufficient for this SDK slice; use the Microsoft official fallback sources below.

Authoritative sources for this plan:

- Microsoft DirectStorage SDK/API landing page, `https://devblogs.microsoft.com/directx/directstorage-api-downloads/`: DirectStorage 1.1 adds GDeflate GPU decompression, DirectStorage 1.3 adds `EnqueueRequests` and `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE`, and the listed 1.4 package is preview for Zstd/GACL/CreatorID work.
- Microsoft DirectStorage 1.3 release post, `https://devblogs.microsoft.com/directx/directstorage-1-3-is-now-available/`: GPU work can be coordinated with fences, 1.3 adds the multiple-subresources destination for D3D12 textures, and corrupted GPU-decompressed GDeflate can surface as driver failure rather than the CPU decompression error path.
- Microsoft DirectStorage developer guidance, `https://github.com/microsoft/DirectStorage/blob/main/Docs/DeveloperGuidance.md`: GPU decompression uses VRAM staging buffers, `IDStorageFactory::SetStagingBufferSize`, and `IDStorageCompressionCodec` for GDeflate content conditioning.
- Microsoft DirectStorage 1.4/Zstd post, `https://devblogs.microsoft.com/directx/directstorage-1-4-release-adds-support-for-zstandard/`: Zstd/GACL is a public-preview content-pipeline and explicit API usage path; replacing DLLs or treating prior assets as ready does not enable it.

## Scope

This plan closes the missing policy/evidence row for DirectStorage GPU destinations and GDeflate. It does not add GPU destination execution, GDeflate decompression execution, DirectStorage 1.4/Zstd execution, performance proof, autonomous scheduler changes, native handle access, package-visible backend readiness, Metal readiness, Nanite compatibility/equivalence/superiority, or broad optimization.

## Files

- Create: `engine/runtime_rhi/include/mirakana/runtime_rhi/mavg_directstorage_gpu_decompression_policy.hpp`
- Create: `engine/runtime_rhi/src/mavg_directstorage_gpu_decompression_policy.cpp`
- Create: `tests/unit/runtime_rhi_mavg_directstorage_gpu_decompression_policy_tests.cpp`
- Create: `schemas/mavg-directstorage-gpu-decompression-policy.schema.json`
- Create: `tools/validate-mavg-directstorage-gpu-decompression-policy.ps1`
- Create: `tools/check-ai-integration-155-mavg-directstorage-gpu-decompression-policy.ps1`
- Modify: `engine/runtime_rhi/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify after compose: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`

## Task 1: TDD Public Contract

- [x] Add failing unit tests for `RuntimeMavgDirectStorageGpuDecompressionPolicyRow`, `RuntimeMavgDirectStorageGpuDecompressionPolicyResult`, `evaluate_runtime_mavg_directstorage_gpu_decompression_policy`, status labels, and diagnostics.
- [x] Add a CMake test target `MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests` and verify the initial build fails because the header is absent.
- [x] Implement the public header/source as a value-only evaluator.

Required behavior:

- A reviewed DirectStorage 1.3 GPU destination row plus a reviewed GDeflate row can make `mavg_directstorage_gpu_decompression_policy_ready=true` while keeping execution readiness false.
- Missing official source, SDK version, GPU destination review, D3D12 fence/resource review, or GDeflate review remains `review_required`.
- DirectStorage 1.4/Zstd preview rows remain `review_required` and set preview counters; they do not promote stable policy or execution readiness.
- Native handles, performance claims, broad MAVG backend readiness, broad optimization, Nanite claims, or execution-ready claims without retained artifact identity/hash are `blocked`.

## Task 2: Validation Surface

- [x] Add a JSON schema for reviewed policy rows with closed objects, stable DirectStorage 1.3 source ids, preview row fields, retained artifact ids/hashes, and explicit false claim fields.
- [x] Add `tools/validate-mavg-directstorage-gpu-decompression-policy.ps1` to configure/build/run the focused test, assert schema literals, and emit retained counters.
- [x] Add `mavgDirectStorageGpuDecompressionPolicyCheck` to the command manifest.
- [x] Add static AI integration guards for the public API, test target, validator, schema, docs, manifest, non-claims, and forbidden ready claims.

Expected default counters:

```text
validation_recipe=mavg-directstorage-gpu-decompression-policy
mavg_directstorage_gpu_decompression_policy_status=policy_ready
mavg_directstorage_gpu_decompression_policy_ready=1
mavg_directstorage_gpu_destination_policy_ready=1
mavg_directstorage_gdeflate_policy_ready=1
mavg_directstorage_gpu_destination_execution_ready=0
mavg_directstorage_gdeflate_execution_ready=0
mavg_directstorage_zstd_preview_ready=0
mavg_directstorage_zstd_preview_selected=0
mavg_directstorage_native_handles_exposed=0
mavg_directstorage_performance_ready=0
mavg_package_visible_backend_readiness_ready=0
mavg_broad_cpu_gpu_memory_optimization_ready=0
mavg_nanite_compatible=0
mavg_nanite_equivalent=0
mavg_nanite_superior=0
```

## Task 3: Docs, Manifest, And Closeout

- [x] Update current capabilities, roadmap, plan registry, and MAVG master plan with this policy-only retained evidence and exact non-claims.
- [x] Update `MK_runtime_rhi` manifest public headers/recent evidence/purpose, compose the manifest, and keep live production execution at the master selection gate after closeout.
- [x] Run focused validation, agent-surface checks, JSON contracts, public API checks, format checks, and full `tools/validate.ps1`.

## Validation Evidence

- TDD red check: `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests` failed while `mavg_directstorage_gpu_decompression_policy.hpp` was absent.
- Focused green check: `tools/cmake.ps1 --preset dev`, `tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests`, and `tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_mavg_ds_gpu_decomp_policy_tests` passed after adding the value-only public API.
- Focused validator: `tools/validate-mavg-directstorage-gpu-decompression-policy.ps1` passed with `mavg_directstorage_gpu_decompression_policy_ready=1`, `mavg_directstorage_gpu_destination_execution_ready=0`, `mavg_directstorage_gdeflate_execution_ready=0`, and `mavg_directstorage_zstd_preview_ready=0`.
- Agent-surface drift check: `tools/check-ai-integration.ps1` passed after adding `tools/check-ai-integration-155-mavg-directstorage-gpu-decompression-policy.ps1`.
- Static contract checks: `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, and `git diff --check` passed.
- Full validation: `tools/validate.ps1` passed, including 52 static checks and 168 CTest tests.
- Official-source split is explicit: DirectStorage 1.1 introduced GDeflate GPU decompression, DirectStorage 1.3 introduced `EnqueueRequests` and `DSTORAGE_DESTINATION_MULTIPLE_SUBRESOURCES_RANGE`, and DirectStorage 1.4/Zstd/GACL remains preview-only in this policy slice.

## Legal And Originality Boundary

This plan uses only public Microsoft documentation as constraints. It does not copy DirectStorage samples, GDeflate implementation code, Zstd shaders, GACL content-pipeline code, DirectStorage SDK headers into repository code, or any Unity/Unreal/Godot code/assets/UI/schema. It does not claim compatibility, parity, equivalence, replacement, or superiority with Unity, Unreal Engine, Godot, Nanite, or any external engine feature.
