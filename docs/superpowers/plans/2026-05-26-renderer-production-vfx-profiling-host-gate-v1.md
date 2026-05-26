# 2026-05-26 Renderer Production VFX Profiling Host Gate v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the next developer-owned renderer production evidence slice by upgrading `production-rendering-vfx-profiling-v1` from value-only review to D3D12 and strict Vulkan host-evidence-backed package proof, while keeping Metal Apple-host-gated and avoiding broad renderer-readiness claims.

**Architecture:** Keep the public contract first-party and backend-neutral. Renderer production evidence is represented as explicit rows, diagnostics, and package-visible counters rather than native handles, implicit capture tooling, or backend-specific gameplay APIs. D3D12 and Vulkan evidence must be proven independently; Metal evidence remains blocked until an Apple host runs the required toolchain validation.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_runtime_host_sdl3`, `sample_generated_desktop_runtime_3d_package`, CMake/CTest, repository PowerShell validation tools, Direct3D 12, Vulkan, and host-gated Metal documentation.

---

**Plan ID:** `renderer-production-vfx-profiling-host-gate-v1`

**Status:** Active.

## Context

`General Purpose Game Production v1` already added `plan_renderer_production_vfx_profiling` and package-visible VFX/profiling review counters, but the completed milestone intentionally left renderer/VFX/profiling host evidence gated. `Engine 1.0 Gap Matrix v1` and the strict Vulkan follow-up closed the previous selection gate, so the next useful clean-break production slice is a renderer evidence plan that can be validated from the current Windows/D3D12/Vulkan host surface without pretending that Metal is complete.

Current implementation anchors:

- `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- `engine/renderer/src/production_vfx_profiling.cpp`
- `tests/unit/renderer_production_vfx_profiling_tests.cpp`
- `engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp`
- `engine/renderer/src/debug_profiling_policy.cpp`
- `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

## Official Practice Check

- D3D12 work must follow official resource-state, barrier, command allocator, queue, and fence rules. Do not replace explicit proof with broad assumptions about common-state promotion or decay.
- Vulkan work must follow official synchronization2, image-layout, usage-flag, queue-family ownership, and validation-layer expectations. Strict Vulkan evidence must stay toolchain/host gated.
- Metal readiness must use official Apple documentation and Apple-host toolchain evidence before promotion. Windows D3D12 or Vulkan proof must not mark Metal ready.

## Constraints

- Do not expose public D3D12, Vulkan, Metal, PIX, RenderDoc, Xcode, or native crash-reporting handles.
- Do not execute external capture, upload crash dumps, or mutate packages as a side effect of planning APIs.
- Do not add backward-compatibility aliases, deprecated shims, or duplicate APIs; this repository is greenfield.
- Do not claim broad renderer quality, broad VFX production readiness, or cross-platform parity from selected row/counter evidence.
- Keep runtime game UI and gameplay APIs independent from editor, Dear ImGui, SDL3 internals, and backend-native APIs.

## Done When

- `production-rendering-vfx-profiling-v1` has explicit D3D12 and strict Vulkan evidence rows/counters with fail-closed diagnostics for missing or cross-backend evidence.
- The generated 3D package can report selected renderer production/VFX/profiling readiness counters without native handle exposure or external capture side effects.
- Metal remains Apple-host-gated in docs, manifest fragments, generated manifest, and package evidence.
- Docs, plan registry, manifest fragments, composed manifest, schemas/static checks if needed, skills/rules if durable workflow changes, and validation evidence are synchronized.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` has passed for the implementation slice, or an explicit host/toolchain blocker is recorded.

## Phase 0 - Select The Active Plan

- [x] Create this dated plan and register it as the active `currentActivePlan`.
- [x] Update `docs/superpowers/plans/README.md` so the active work table points at this plan.
- [x] Update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` with `recommendedNextPlan.id = renderer-production-vfx-profiling-host-gate-v1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Run focused docs/agent-surface validation for the planning change:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `git diff --check`

Phase 0 evidence on 2026-05-26: all focused docs/agent-surface commands above passed after composing `engine/agent/manifest.json`.

## Phase 1 - Lock Backend Evidence Contract Tests

- [ ] Add or update tests in `tests/unit/renderer_production_vfx_profiling_tests.cpp` before implementation.
- [ ] Cover D3D12 evidence success with backend timing, synchronization, debug/profiling, and package-review rows.
- [ ] Cover strict Vulkan evidence success independently from D3D12 evidence.
- [ ] Cover Metal absence as `host_evidence_required`, not `ready`.
- [ ] Cover rejected cross-backend evidence transfer, missing synchronization proof, unsupported broad-quality claims, native-handle requests, external capture execution, and crash-upload execution.
- [ ] Keep aggregate designated initializers in declaration order for all changed test aggregates.

## Phase 2 - Extend Renderer Production Evidence Rows

- [ ] Extend `RendererProductionBackendTimingRow` or replace it with a clean-break row shape that records backend timing plus explicit synchronization and validation proof.
- [ ] Record evidence fields for barrier/state/layout handling, queue waits or ownership transfer intent, shader/tool validation, debug layer or validation layer status, debug scopes/markers, and host validation.
- [ ] Keep diagnostics precise enough to distinguish missing D3D12 evidence, missing Vulkan evidence, missing Metal host evidence, broad readiness claims, native handle requests, and external side-effect requests.
- [ ] Keep the API first-party and value-only; no backend-native handle escape hatches.

## Phase 3 - Wire Generated 3D Package Evidence

- [ ] Update `games/sample_generated_desktop_runtime_3d_package/main.cpp` to emit selected renderer production/VFX/profiling counters for D3D12 and strict Vulkan.
- [ ] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` with the selected validation recipe expectations.
- [ ] Update `games/sample_generated_desktop_runtime_3d_package/README.md` with the supported counters and non-goals.
- [ ] Ensure package proof distinguishes D3D12 ready, strict Vulkan host-gated ready after explicit recipe execution, and Metal Apple-host-gated.

## Phase 4 - Sync Agent Surfaces And Static Guards

- [ ] Update `docs/current-capabilities.md`, `docs/roadmap.md`, and the relevant production-completion ledger rows with the active/closed state.
- [ ] Update `engine/agent/manifest.fragments/004-modules.json`, `010-aiOperableProductionLoop.json`, and `014-gameCodeGuidance.json` if capability, package, or game-guidance claims change.
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after fragment edits.
- [ ] Update schemas or static checks only when the machine-readable contract changes.
- [ ] Update `.agents`, `.claude`, `.cursor` skills/rules/subagents only if durable workflow guidance changes; otherwise record that no agent-surface update was required.

## Phase 5 - Validate And Close

- [ ] Run the focused C++ loop:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests sample_generated_desktop_runtime_3d_package`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|sample_generated_desktop_runtime_3d_package_smoke"`
- [ ] Run strict Vulkan package validation only on a host with the required Vulkan runtime, DXC SPIR-V CodeGen, and `spirv-val`.
- [ ] Run agent-surface checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `git diff --check`
- [ ] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before claiming the implementation slice is complete.
- [ ] Close this plan by recording validation evidence, updating the manifest pointer to the next selected plan or back to the master plan, and publishing through the normal GitHub Flow.
