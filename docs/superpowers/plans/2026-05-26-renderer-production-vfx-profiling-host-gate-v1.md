# 2026-05-26 Renderer Production VFX Profiling Host Gate v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the next developer-owned renderer production evidence slice by upgrading `production-rendering-vfx-profiling-v1` from value-only review to D3D12 and strict Vulkan host-evidence-backed package proof, while keeping Metal Apple-host-gated and avoiding broad renderer-readiness claims.

**Architecture:** Keep the public contract first-party and backend-neutral. Renderer production evidence is represented as explicit rows, diagnostics, and package-visible counters rather than native handles, implicit capture tooling, or backend-specific gameplay APIs. D3D12 and Vulkan evidence must be proven independently; Metal evidence remains blocked until an Apple host runs the required toolchain validation.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_runtime_host_sdl3`, `sample_generated_desktop_runtime_3d_package`, CMake/CTest, repository PowerShell validation tools, Direct3D 12, Vulkan, and host-gated Metal documentation.

---

**Plan ID:** `renderer-production-vfx-profiling-host-gate-v1`

**Status:** Completed.

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

## Immediate Next Work Queue

Execute the remaining work in this order. Each checkpoint should leave a reviewable diff and should not promote broad renderer readiness.

### Checkpoint 1 - Official Backend Evidence Requirements

**Files:**

- Modify: `docs/superpowers/plans/2026-05-26-renderer-production-vfx-profiling-host-gate-v1.md`

- [x] Record official-practice assumptions before changing runtime code:
  - D3D12 evidence must require explicit resource-state/barrier proof, command queue/fence synchronization proof, and command allocator reuse only after GPU completion is known.
  - Vulkan strict evidence must require synchronization2-style barrier/layout proof, explicit access/stage intent, queue-family ownership review when content survives across queues, validation-layer evidence, and SPIR-V validation evidence.
  - Metal remains Apple-host-gated until an Apple host supplies Metal shader/library and runtime validation evidence.
- [x] Do not add implementation fields that imply native handle access, capture execution, crash upload execution, backend performance parity, or broad renderer quality.

### Checkpoint 2 - RED Tests For Backend Evidence

**Files:**

- Modify: `tests/unit/renderer_production_vfx_profiling_tests.cpp`

- [x] Add failing tests for D3D12-ready evidence, strict Vulkan-ready evidence, and Metal-host-gated evidence as separate rows.
- [x] Add failing tests for missing synchronization proof, missing shader/tool validation proof, missing backend validation proof, missing host recipe proof, missing capture/crash handoff proof, duplicate backend rows, and cross-backend evidence transfer.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests
```

Expected at this checkpoint: FAIL because the public renderer evidence row shape and diagnostics do not yet expose the new contract.

### Checkpoint 3 - Renderer Evidence Contract Implementation

**Files:**

- Modify: `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- Modify: `engine/renderer/src/production_vfx_profiling.cpp`
- Modify: `tests/unit/renderer_production_vfx_profiling_tests.cpp`

- [x] Add clean-break value-only evidence rows/counters for backend timing, synchronization, shader/tool validation, backend validation, strict host recipe readiness, capture/crash handoff readiness, and per-backend host evidence readiness.
- [x] Add precise diagnostics for missing backend synchronization evidence, missing shader/tool validation, missing backend validation, missing host recipe evidence, missing capture handoff evidence, duplicate backend rows, and unsupported side-effect/native-handle requests.
- [x] Keep Metal host absence as host-gated evidence, not as D3D12/Vulkan failure and not as ready.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests"
```

Expected at this checkpoint: PASS for the unit target and focused CTest regex.

### Checkpoint 4 - Generated 3D Package Evidence

**Files:**

- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/README.md`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/check-ai-integration-097-rendering-vfx-profiling-pack.ps1`

- [x] Emit package-visible counters for three backend evidence rows: D3D12 ready, strict Vulkan ready, and Metal host-gated.
- [x] Require package validation fields for backend evidence row count, backend evidence ready count, backend evidence host-gated count, D3D12 host evidence ready, Vulkan strict host evidence ready, and Metal host evidence not ready.
- [x] Keep GPU command execution, native capture execution, and crash upload side-effect counters at zero.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests sample_generated_desktop_runtime_3d_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|sample_generated_desktop_runtime_3d_package_smoke"
```

Expected at this checkpoint: PASS for the renderer unit tests and selected generated 3D package smoke.

### Checkpoint 5 - Agent Surface Drift Sync

**Files:**

- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-26-renderer-production-vfx-profiling-host-gate-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`

- [x] Update docs and manifest fragments from "one host-validated backend row" to "D3D12 and strict Vulkan host evidence ready, Metal host-gated" wherever that is the durable contract.
- [x] Update `unsupportedProductionGaps`, `currentActivePlan`, and `recommendedNextPlan` only when closing the active slice; do not leave a completed plan selected.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected at this checkpoint: composed manifest matches fragments and AI integration needles pass.

### Checkpoint 6 - AGENTS, Skills, Rules, Subagents, And Tools Decision

**Files:**

- Inspect: `AGENTS.md`
- Inspect: `.agents/skills/`
- Inspect: `.claude/skills/`
- Inspect: `.cursor/skills/`
- Inspect: `.codex/rules/`
- Inspect: `.codex/agents/`
- Inspect: `.claude/agents/`
- Inspect: `.cursor/agents/`
- Inspect: `tools/`

- [x] Update AGENTS/skills/rules/subagents only if this slice changes durable workflow, permissions, validation entrypoints, or reusable agent behavior. A renderer evidence row/counter change alone should normally require docs, manifest fragments, game manifest, package validation, and static guards, not broad rule or subagent rewrites.
- [x] Update `tools/` files only for executable validation/static-check expectations that must enforce the new package-visible contract.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

Expected at this checkpoint: no stale agent-surface guidance, no stale tool needles, and no formatting or whitespace failures.

### Checkpoint 7 - Slice Validation, Commit, And PR

**Files:**

- Modify: `docs/superpowers/plans/2026-05-26-renderer-production-vfx-profiling-host-gate-v1.md`
- Modify any closing manifest/docs files selected by Checkpoint 5.

- [x] Run the full slice gate:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Expected: PASS, unless a concrete host/toolchain blocker is recorded in this plan with the exact failing command.

- [x] Record final focused and full validation evidence in this plan.
- [x] Commit the validated checkpoint on a `codex/` topic branch.
- [x] Publish through GitHub Flow with a reviewable PR; do not push directly to `main`, force-push, or mark the slice complete from local validation alone.

## Phase 1 - Lock Backend Evidence Contract Tests

- [x] Add or update tests in `tests/unit/renderer_production_vfx_profiling_tests.cpp` before implementation.
- [x] Cover D3D12 evidence success with backend timing, synchronization, debug/profiling, and package-review rows.
- [x] Cover strict Vulkan evidence success independently from D3D12 evidence.
- [x] Cover Metal absence as `host_evidence_required`, not `ready`.
- [x] Cover rejected cross-backend evidence transfer, missing synchronization proof, unsupported broad-quality claims, native-handle requests, external capture execution, and crash-upload execution.
- [x] Keep aggregate designated initializers in declaration order for all changed test aggregates.

## Phase 2 - Extend Renderer Production Evidence Rows

- [x] Extend `RendererProductionBackendTimingRow` or replace it with a clean-break row shape that records backend timing plus explicit synchronization and validation proof.
- [x] Record evidence fields for barrier/state/layout handling, queue waits or ownership transfer intent, shader/tool validation, debug layer or validation layer status, debug scopes/markers, and host validation.
- [x] Keep diagnostics precise enough to distinguish missing D3D12 evidence, missing Vulkan evidence, missing Metal host evidence, broad readiness claims, native handle requests, and external side-effect requests.
- [x] Keep the API first-party and value-only; no backend-native handle escape hatches.

## Phase 3 - Wire Generated 3D Package Evidence

- [x] Update `games/sample_generated_desktop_runtime_3d_package/main.cpp` to emit selected renderer production/VFX/profiling counters for D3D12 and strict Vulkan.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/game.agent.json` with the selected validation recipe expectations.
- [x] Update `games/sample_generated_desktop_runtime_3d_package/README.md` with the supported counters and non-goals.
- [x] Ensure package proof distinguishes D3D12 ready, strict Vulkan host evidence ready after explicit recipe execution, and Metal Apple-host-gated.

## Phase 4 - Sync Agent Surfaces And Static Guards

- [x] Update `docs/current-capabilities.md`, `docs/roadmap.md`, and the relevant production-completion ledger rows with the active/closed state.
- [x] Update `engine/agent/manifest.fragments/004-modules.json`, `010-aiOperableProductionLoop.json`, and `014-gameCodeGuidance.json` if capability, package, or game-guidance claims change.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` after fragment edits.
- [x] Update schemas or static checks only when the machine-readable contract changes.
- [x] Update `.agents`, `.claude`, `.cursor` skills/rules/subagents only if durable workflow guidance changes; otherwise record that no agent-surface update was required.

## Phase 5 - Validate And Close

- [x] Run the focused C++ loop:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests sample_generated_desktop_runtime_3d_package`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|sample_generated_desktop_runtime_3d_package_smoke"`
- [x] Run strict Vulkan package validation only on a host with the required Vulkan runtime, DXC SPIR-V CodeGen, and `spirv-val`.
- [x] Run agent-surface checks:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1`
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`
  - `git diff --check`
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` before claiming the implementation slice is complete.
- [x] Close this plan by recording validation evidence, updating the manifest pointer to the next selected plan or back to the master plan, and publishing through the normal GitHub Flow.

## Final Validation Evidence

| Gate | Evidence |
| --- | --- |
| Official docs | Context7 checks recorded D3D12 resource barriers, queue/fence synchronization, and command allocator lifetime expectations, plus Vulkan synchronization2 image barriers, layout transitions, access/stage masks, and queue-family ownership review. Metal stays Apple-host-gated until Apple host toolchain evidence exists. |
| RED test | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests"` failed on the new Metal partial-row regression before the all-rows backend evidence fix. |
| Focused C++ | `tools/cmake.ps1 --build --preset dev --target MK_renderer_production_vfx_profiling_tests sample_generated_desktop_runtime_3d_package` and `tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_production_vfx_profiling_tests|sample_generated_desktop_runtime_3d_package_smoke"` passed. |
| Package install | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package` passed with D3D12 package evidence, install validation, and CPack output. |
| Strict Vulkan package | `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package -RequireVulkanShaders` with reviewed Vulkan smoke args passed after removing D3D12-only compute morph async telemetry from the Vulkan lane. SPIR-V artifacts were compiled and validated with `spirv-val`, and installed validation proved `rendering_vfx_profiling_vulkan_strict_host_evidence_ready=1`. |
| Static/agent drift | `tools/check-toolchain.ps1`, scoped `tools/check-tidy.ps1 -Files ... -ReuseExistingFileApiReply`, `tools/check-public-api-boundaries.ps1`, `tools/check-json-contracts.ps1`, `tools/check-ai-integration.ps1`, `tools/check-agents.ps1`, `tools/check-format.ps1`, and `git diff --check` passed. |
| Agent surfaces | Docs, plan registry, production-completion master plan chapters, manifest fragments, composed manifest, game manifest, package validator, and scoped AI integration guard were updated. AGENTS, skills, rules, and subagent instructions did not need durable workflow changes; subagents used for review were closed after their findings were consumed. |
| Full validation | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-26 with `validate: ok` and 87/87 CTest tests passing. Diagnostic-only gates still report Metal/Apple host evidence as blocked on this Windows host, which is the intended host gate. |
