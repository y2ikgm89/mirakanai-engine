# MAVG Metal Host-Gated Capability Classifier Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a value-only MAVG Metal capability classifier that keeps Metal MAVG rows host-gated until explicit Apple-host evidence exists.

**Architecture:** Add a small renderer policy under `MK_renderer` that classifies first-party MAVG capability evidence rows by backend, capability kind, reviewed host recipe, and unsupported claim flags. The classifier does not execute GPU commands, expose native handles, infer Metal readiness from D3D12/Vulkan evidence, or claim Nanite equivalence.

**Tech Stack:** C++23, `MK_renderer`, `mirakana::rhi::BackendKind`, focused unit target `MK_mavg_metal_capability_policy_tests`, repository PowerShell validation wrappers.

---

## Context

This is a narrow child plan under the MAVG production master plan. It starts from `origin/main` and must not depend on open PRs #578-#585. The work is intentionally value-only: it records whether Metal MAVG capabilities are ready, host-gated, dependency-gated, unsupported, or invalid, and it rejects unsupported claims.

## Files

- Create: `engine/renderer/include/mirakana/renderer/mavg_metal_capability_policy.hpp`
- Create: `engine/renderer/src/mavg_metal_capability_policy.cpp`
- Create: `tests/unit/mavg_metal_capability_policy_tests.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Modify: `CMakeLists.txt`
- Modify: `docs/specs/2026-06-05-mavg-architecture-v1.md`
- Modify: `docs/superpowers/master-plans/2026-05-27-mirakana-adaptive-virtual-geometry-master-plan-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Create: `tools/check-ai-integration-123-mavg-metal-host-gated-capability-classifier.ps1`

## Non-Claims

- No Metal readiness without Apple-host evidence.
- No Apple-host proof from Windows.
- No mesh/object shader execution, backend draw execution, DirectStorage, persistent/autonomous streaming, async-overlap performance proof, ray tracing, deformation, quality-governor result, Nanite compatibility/equivalence/superiority, native handle exposure, or broad optimization.

## Tasks

### Task 1: RED Test And CMake Hook

- [x] Add `tests/unit/mavg_metal_capability_policy_tests.cpp` with tests for host-gated Metal classification, ready Apple-host evidence, cross-backend inference rejection, unsupported claims, deterministic replay hash, and side-effect counters.
- [x] Add `MK_mavg_metal_capability_policy_tests` to root `CMakeLists.txt`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_mavg_metal_capability_policy_tests
```

Result: build failed because `mirakana/renderer/mavg_metal_capability_policy.hpp` did not exist yet.

### Task 2: Minimal Public API And Implementation

- [x] Add `MavgMetalCapabilityStatus`, `MavgMetalCapabilityKind`, `MavgMetalCapabilityRowStatus`, `MavgMetalCapabilityDiagnosticCode`, row/request/diagnostic/plan structs, `plan_mavg_metal_capabilities`, and `mavg_metal_capability_diagnostic_message`.
- [x] Add `engine/renderer/src/mavg_metal_capability_policy.cpp` with deterministic sorting, diagnostics, replay hashing, and side-effect flags that remain false.
- [x] Add `src/mavg_metal_capability_policy.cpp` to `engine/renderer/CMakeLists.txt`.
- [x] Re-run the focused build and CTest target.

### Task 3: Agent Surface Sync

- [x] Update the MAVG architecture spec, master plan, active plan registry, roadmap, current capabilities, module fragment, production-loop fragment, and composed manifest.
- [x] Add static guard chapter `tools/check-ai-integration-123-mavg-metal-host-gated-capability-classifier.ps1`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
```

### Task 4: Verification And Publication

- [x] Run focused tidy/public API checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/renderer/src/mavg_metal_capability_policy.cpp,tests/unit/mavg_metal_capability_policy_tests.cpp
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
```

- [x] Run full validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight, commit, push, and open a draft PR:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
git add <task-owned files>
git commit -m "Add MAVG Metal host-gated capability policy"
git push -u origin codex/mavg-metal-host-gated-capability-classifier-v1
gh pr create --draft --base main --head codex/mavg-metal-host-gated-capability-classifier-v1 --title "Add MAVG Metal host-gated capability policy" --body-file <body>
```

## Validation Evidence

- RED: `tools/cmake.ps1 --build --preset dev --target MK_mavg_metal_capability_policy_tests` failed on missing `mirakana/renderer/mavg_metal_capability_policy.hpp` before implementation.
- Focused GREEN: `tools/cmake.ps1 --build --preset dev --target MK_mavg_metal_capability_policy_tests` and `tools/ctest.ps1 --preset dev --output-on-failure -R mavg_metal_capability_policy` passed.
- Review fix: `cpp-reviewer` found invalid-request readiness leakage, lowercase native-shaped evidence ids, and rows-present/required-empty diagnostics gaps; regression tests were added and passed after fixes.
- Static and surface checks passed: `tools/check-agents.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-public-api-boundaries.ps1`, `tools/check-format.ps1`, and focused `tools/check-tidy.ps1 -Files engine/renderer/src/mavg_metal_capability_policy.cpp,tests/unit/mavg_metal_capability_policy_tests.cpp`.
- Full gate passed: `tools/check-toolchain.ps1` and `tools/validate.ps1` completed with `validate: ok`, 119/119 tests passed. Metal/Apple checks remained diagnostic host gates on this Windows host.
