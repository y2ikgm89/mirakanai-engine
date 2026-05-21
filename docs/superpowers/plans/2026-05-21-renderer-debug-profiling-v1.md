# Renderer Debug Profiling v1 (2026-05-21)

**Plan ID:** `renderer-debug-profiling-v1`
**Status:** Completed.
**Current pointer rule:** Completed; `engine/agent/manifest.json.aiOperableProductionLoop` points at `renderer-backend-parity-v1` as the next developer-owned renderer 1.x milestone.

## Goal

Strengthen renderer debugging and performance handoff with backend-neutral GPU timestamp/marker rows, capture request/evidence counters, frame diagnostics, and operator handoff before broader production profiling claims.

## Context

- Engine 1.0 closeout remains manifest-led and currently has `unsupportedProductionGaps = []`.
- `renderer-gpu-memory-v1` completed with backend-neutral GPU memory policy diagnostics, package-visible GPU memory counters, and selected D3D12 GPU memory execution evidence.
- Official references: Microsoft PIX on Windows, Vulkan debug utils guidance, Apple Metal capture / Xcode / Instruments guidance.

## Constraints

- Preserve `unsupportedProductionGaps = []`.
- Keep public renderer/game APIs backend-neutral; native capture tools stay backend-private.
- Do not claim crash telemetry, production flame graphs, or automatic external tool execution.
- Start behavior/API changes with RED tests or static guards.

## Phase 0: Pointer Sync

**Status:** Completed.

### Done When

- Plan registry, roadmap, master plan, and manifest identify this milestone as active during implementation.
- `unsupportedProductionGaps = []` preserved.

## Phase 1: Debug Profiling Policy

**Status:** Completed.

### Goal

Add backend-neutral `plan_debug_profiling_policy` with fail-closed diagnostics for unsupported automatic capture, flame graphs, and crash telemetry export.

### Done When

- `debug_profiling_policy.hpp/cpp` and `MK_renderer` wiring exist.
- `MK_renderer_tests` cover success and fail-closed diagnostics.

## Phase 2: Package-Visible SDL Presentation Counters

**Status:** Completed.

### Goal

Expose debug profiling policy and D3D12 execution reports through `SdlDesktopPresentationReport` and `sample_desktop_runtime_game` status fields.

### Done When

- `evaluate_sdl_desktop_presentation_debug_profiling_policy` and `evaluate_sdl_desktop_presentation_d3d12_debug_profiling_execution` exist.
- `MK_runtime_host_sdl3_tests` cover ready/blocked paths.
- `--require-debug-profiling-policy` and `--require-d3d12-debug-profiling-evidence` exist on the sample game.

## Phase 3: D3D12 Profiling Execution Evidence

**Status:** Completed.

### Goal

Record per-frame `mirakana.presentation` GPU debug markers and validate timestamp ticks, marker counters, and framegraph diagnostics on selected D3D12 package smoke.

### Done When

- Frame renderers insert GPU debug markers each active frame.
- `tools/validate-installed-desktop-runtime.ps1` validates installed package status fields.
- Agent surfaces, manifest fragments, and static checks stay aligned.

## Validation Evidence

| Check | Result |
| --- | --- |
| `MK_renderer_tests` | Pass |
| `MK_runtime_host_sdl3_tests` | Pass |
| `tools/check-agents.ps1` | Pass (after compose) |
| `tools/check-ai-integration.ps1` | Pass (after compose) |
| `tools/validate.ps1` | Pass (slice gate) |

## Non-Goals

- Automatic PIX/Vulkan/Metal capture execution from editor core.
- Production flame graphs and crash telemetry export.
- Vulkan/Metal debug profiling package parity promotion (tracked by `renderer-backend-parity-v1`).
