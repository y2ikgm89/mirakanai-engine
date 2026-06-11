# 2026-06-12 MAVG Deformation Tier Diagnostics v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use focused inline TDD execution or `superpowers:subagent-driven-development` for bounded follow-up tasks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add an `MK_assets` value-only MAVG deformation tier diagnostic contract for static, rigid, skinned, morph, and unsupported dynamic clusters.

**Architecture:** Keep this slice at the asset contract boundary. The new helper consumes an already validated `MavgClusterGraphDocument` plus caller-reviewed deformation rows, emits deterministic tier support/fallback diagnostics, and records explicit no-side-effect flags. Runtime upload/refit policy, renderer/RHI execution, mesh shaders, DirectStorage, background streaming, and package evidence remain follow-up work.

**Tech Stack:** C++23, `MK_assets`, CMake/CTest, PowerShell validation tools.

---

**Plan ID:** `mavg-deformation-tier-diagnostics-v1`

**Status:** Active local child candidate. Not selected as `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`.

## Context

This plan implements the first narrow checkpoint from MAVG Phase 7, not the full deformable cluster geometry milestone. It follows the master-plan tier taxonomy:

- Tier 0: static clusters.
- Tier 1: rigid instances.
- Tier 2: skinned clusters with conservative per-cluster bone bounds.
- Tier 3: morph clusters with precomputed delta bounds.
- Tier 4: runtime displacement or destruction with bounded dynamic update policy.

Khronos glTF 2.0 is the primary external reference for skin and morph terminology only. The implementation remains clean-room and value-only.

## Files

- Create: `engine/assets/include/mirakana/assets/mavg_deformation.hpp`
- Create: `engine/assets/src/mavg_deformation.cpp`
- Modify: `engine/assets/CMakeLists.txt`
- Create: `tests/unit/mavg_deformation_tests.cpp`
- Modify: `CMakeLists.txt`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Compose: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-121-mavg-deformation-tier-diagnostics.ps1`
- Modify: `tools/check-ai-integration.ps1`

## Task 1 - Plan Activation And RED Coverage

- [ ] Add this plan as the active local MAVG child slice in the plan registry.
- [ ] Add RED tests for static and rigid tier rows proving they are MAVG-ready with zero deterministic bounds expansion.
- [ ] Add RED tests proving skinned tier rows require matching conservative bone bounds.
- [ ] Add RED tests proving morph tier rows require matching delta bounds.
- [ ] Add RED tests proving dynamic/displacement rows fail closed to conventional fallback with explicit diagnostics.
- [ ] Add RED tests proving duplicate/out-of-order tier rows produce stable output rows and diagnostics.

## Task 2 - Asset-Level Deformation Diagnostics Contract

- [ ] Add `MavgDeformationTier`, `MavgDeformationDiagnosticCode`, `MavgDeformationTierDiagnosticsDesc`, `MavgDeformationTierDiagnosticsResult`, input rows, result rows, and `plan_mavg_deformation_tier_diagnostics`.
- [ ] Validate that every deformation row targets an existing cluster in the supplied graph.
- [ ] Sort output rows by `graph_asset`, `cluster_index`, and tier so replay output does not depend on caller input order.
- [ ] Mark Tier 0 static and Tier 1 rigid as supported by MAVG without runtime refit.
- [ ] Mark Tier 2 skinned as supported only with finite conservative bone bounds and positive influencing bone count.
- [ ] Mark Tier 3 morph as supported only with finite delta bounds and finite non-negative delta radius.
- [ ] Mark Tier 4 dynamic displacement/destruction as unsupported fallback-only in this slice.
- [ ] Keep renderer/RHI, runtime upload, mesh shader, DirectStorage, background streaming, native-handle, and broad optimization flags false.

## Task 3 - Docs, Manifest, Static Guards, And Validation

- [ ] Record the exact ready claim: MAVG has an asset-level deformation tier diagnostics value contract for static/rigid/skinned/morph/dynamic rows.
- [ ] Keep non-claims explicit: no runtime upload/refit, renderer/RHI execution, package evidence, mesh shader execution, DirectStorage, async-overlap/performance proof, Metal readiness, Nanite equivalence/superiority, or broad optimization.
- [ ] Update manifest fragments and compose `engine/agent/manifest.json`; do not hand-edit the composed manifest.
- [ ] Add static integration guard coverage for the new deformation literals and non-claims.
- [ ] Run focused tests, static guards, public API checks, full `tools/validate.ps1`, publication preflight, and `git diff --check`.

## Validation Evidence

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_deformation_tests`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev -R MK_mavg_deformation_tests --output-on-failure`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`
- `git diff --check`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1`

## Non-Claims

This plan does not claim runtime deformation upload/refit policy, package-visible skinned/morph MAVG samples, renderer/RHI execution, backend draw execution, mesh shader execution, DirectStorage, persistent/autonomous background streaming services, async-overlap or performance proof, ray tracing, Metal readiness, Nanite compatibility/equivalence/superiority, or broad CPU/GPU/memory optimization.
