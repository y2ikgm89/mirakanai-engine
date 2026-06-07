# MAVG Runtime Inferred Page Use Generation v1

**Date:** 2026-06-07

**Plan ID:** `mavg-runtime-inferred-page-use-generation-v1`

## Goal

Add side-effect-free MAVG resident page use-generation inference so selected resident pages can update recency evidence before later LRU/recency eviction policy work.

## Context

MAVG Runtime LOD Milestone v1, MAVG Page Streaming Queue v1, MAVG Page Streaming Eviction Review v1, and MAVG Automatic Eviction Policy v1 already provide resident page rows, selected cluster rows, protected eviction review, deterministic fallback ordering, and one-row safe-point drain evidence. This slice adds only the value-only page-use generation row builder needed by future recency/LRU/frequency policies.

The implementation uses current repository C++23 patterns and the existing `mavg_page_streaming.hpp` boundary. No external SDK, platform API, third-party dependency, renderer/RHI object, file IO API, worker API, or official library reference is introduced; Context7 remains the preferred route when a touched library/SDK/toolchain API needs current documentation.

## Constraints

- Keep the surface value-only and host-independent.
- Do not mutate resident mounts, read files, execute streaming, start workers, or touch renderer/RHI/native handles.
- Do not claim LRU/recency/frequency eviction policy yet; this slice only emits recency evidence rows.
- Do not claim GPU memory pressure, DirectStorage/native IO, autonomous/background package streaming, Nanite compatibility/equivalence/superiority, or broad CPU/GPU/memory optimization.

## Tasks

- [x] Add `RuntimeMavgPageStreamingRecencyRow`.
- [x] Add `RuntimeMavgResidentPageUseGenerationDesc`.
- [x] Add `RuntimeMavgResidentPageUseGenerationResult`.
- [x] Add `infer_runtime_mavg_resident_page_use_generations`.
- [x] Update selected resident pages to `current_use_generation`.
- [x] Carry retained unselected resident recency rows.
- [x] Initialize new resident pages as cold generation zero.
- [x] Drop nonresident previous recency rows.
- [x] Fail closed on duplicate previous recency rows and non-monotonic retained generations.
- [x] Preserve side-effect flags proving no file IO, mount mutation, streaming execution, worker execution, renderer/RHI handle access, or broad optimization claim.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` failed because `RuntimeMavgResidentPageUseGenerationResult`, `RuntimeMavgPageStreamingRecencyRow`, `RuntimeMavgResidentPageUseGenerationDesc`, and `infer_runtime_mavg_resident_page_use_generations` were not defined.
- Static GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- Static GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- Static GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Static GREEN: `git diff --check` passed.
- API GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_mavg_page_streaming_tests` passed.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_mavg_page_streaming_tests` passed.
- Focused GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/mavg_page_streaming.cpp,tests/unit/runtime_mavg_page_streaming_tests.cpp -ReuseExistingFileApiReply` passed.
- Full GREEN: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including 107/107 CTest tests.

## Done When

- Public runtime API exposes deterministic MAVG resident page use-generation inference.
- Tests prove selected page update, unselected carry, nonresident row drop, cold new pages, duplicate recency rejection, non-monotonic generation rejection, and no side effects.
- Docs, plan registry, manifest fragments, composed manifest, and static AI integration checks match the implemented claim.
- Full validation passes or a concrete host/tool blocker is recorded.

## Non-Claims

- No LRU/recency/frequency eviction policy.
- No GPU memory pressure integration.
- No partial `.mavgpayload` byte-range page loading/schema.
- No file IO, mount mutation, background worker, package streaming execution, renderer/RHI handle access, DirectStorage execution, Metal readiness, Nanite compatibility/equivalence/superiority, or broad optimization claim.
