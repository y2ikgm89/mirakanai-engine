# Production Completion v1 - Renderer Advanced Production Track

Source index: [Production Completion Master Plan v1](../2026-05-03-production-completion-master-plan-v1.md). Load this chapter only when selecting or reviewing renderer-related post-1.0 work.

## Purpose

This chapter is the renderer projection over the canonical backlog in [04-developer-owned-engine-capability-backlog.md](04-developer-owned-engine-capability-backlog.md). It owns renderer implementation rules and evidence boundaries, not the canonical list of capability rows.

## Renderer Rules

- Keep public gameplay, scene, material, UI, and package APIs backend-neutral.
- Keep D3D12, Vulkan, Metal, PIX, RenderDoc, Xcode/Metal capture, descriptor heaps, command queues, command buffers, fences, semaphores, heaps, argument buffers, and native handles behind backend-private adapters or an accepted opaque interop design.
- Treat each backend independently. D3D12 package proof does not imply Vulkan or Metal readiness.
- Cite current official backend documentation in the selected dated plan before changing synchronization, memory, shader, or presentation behavior.
- Performance claims require timestamp/profile evidence, deterministic budgets, and failure diagnostics.
- Keep runtime package renderer proof, editor preview, viewport tooling, capture handoff, and material-authoring UX as separate claims.

## Renderer Projection

| Renderer concern | Canonical rows | Evidence boundary |
| --- | --- | --- |
| Modern materials and shader authoring | `renderer-modern-materials-v1`, `material-shader-graph-production-v1` | Backend-specific shader compiler evidence, package-visible material counters, pipeline-cache diagnostics, and reviewed compile requests. |
| Lighting and shadows | `renderer-lighting-shadows-v1` | Backend-neutral light-list contracts, per-backend package proof, GPU counters, and explicit resource-state/layout evidence. |
| Postprocess and image quality | `renderer-postprocess-v1` | Frame graph pass rows, render-pass/load-store policy, texture lifetime/aliasing evidence, package-visible quality counters, and backend shader validation. |
| Scene scale and draw throughput | `renderer-scene-scale-v1`, `engine-entity-scale-and-culling-v1` | Deterministic culling/LOD tests, draw-call/instance counters, CPU/GPU timing rows, and no public native handles. |
| GPU memory and residency | `renderer-gpu-memory-v1`, `world-streaming-and-large-scenes-v1` | Heap/resource evidence per backend, package-visible budget diagnostics, and safe failure/remediation rows. |
| Backend parity | `renderer-backend-parity-v1` | Strict Vulkan validation/SPIR-V evidence and Apple-host Metal/Xcode evidence per promoted feature. |
| Debug and profiling | `renderer-debug-profiling-v1` | PIX, Vulkan debug utils / validation layers, Apple Metal/Xcode capture evidence, and first-party trace/profile export rows. |
| 2D sprite renderer production | `sprite-batching-renderer-v1`, `sprite-sorting-layer-v1`, `sprite-9slice-and-tiled-v1`, `sprite-effects-particles-v1` | Atlas/material grouping, draw/instance budgets, render-pass evidence, and per-backend package counters. |

When a renderer row is selected, create or update a dated capability/milestone plan. Do not broaden ready claims, expose native handles, or mark host-gated backend work as ready without focused validation evidence.
