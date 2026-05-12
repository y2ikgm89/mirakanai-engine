# PBR Material and Lighting v1 Implementation Plan (2026-05-04)

> **For agentic workers:** Follow existing `mirakana_assets` / `mirakana_runtime_rhi` / `mirakana_scene_renderer` contracts. Use `gameengine-agent-integration` when changing `engine/agent/manifest.json` or static agent checks.

**Goal:** Align runtime lit materials with a single **per-frame PBR uniform** (`scene_pbr_frame`, **descriptor binding 6** in set 0; HLSL uses `cbuffer ScenePbrFrame : register(b6)` alongside `MaterialFactors` at `register(b0)` to match D3D12 root descriptor `BaseShaderRegister`), pack camera/light/ambient data from the scene renderer, share one scene UBO across scene-bound materials in `mirakana_runtime_scene_rhi`, and extend `MeshCommand` with `**world_from_node`** so vertex shaders can transform positions consistently with the packed frame matrices.

**Depends on:** [2026-05-04-shader-graph-and-generation-pipeline-v1.md](2026-05-04-shader-graph-and-generation-pipeline-v1.md) (Phase 4 sequencing; no hard compile-time dependency).

---

## Goal

- Material pipeline metadata exposes `scene_pbr_frame` (set 0, binding **6**, uniform, vertex+fragment).
- `pack_scene_pbr_frame_gpu` writes a 256-byte GPU struct; `SceneRenderSubmitDesc` accepts optional `ScenePbrGpuSubmitContext`.
- Runtime material binding creates or binds the scene UBO; scene GPU binding palette injects a **shared** scene buffer for all materials.
- Sample `runtime_scene.hlsl` paths evaluate lit shading using the new constants; desktop sample passes `pbr_gpu` from presentation.

## Constraints

- Do not renumber existing texture/sampler bindings; insert scene UBO after `material_factors` in metadata order only.
- Back-compat for old HLSL bytecode outside updated samples is explicitly **not** a goal for this slice (clean break in authored shaders under scope).

## Implementation Steps

- `build_material_pipeline_binding_metadata`: append `scene_pbr_frame`; `MaterialShaderStageMask` `operator|`.
- `pack_runtime_material_factors`: `shading_lit` flag at agreed offset; scene UBO size constant.
- `create_runtime_material_gpu_binding`: branch semantics; per-material or shared scene buffer + descriptor writes.
- `runtime_scene_rhi`: shared `scene_pbr_frame` buffer option/result; teardown of per-material scene buffers.
- `scene_renderer`: `pack_scene_pbr_frame_gpu`, `MeshCommand.world_from_node`, submit path `write_buffer` when `pbr_gpu` set.
- SDL3 presentation / sample game: expose scene UBO + device; wire `ScenePbrGpuSubmitContext`.
- HLSL: sample desktop runtime + generated material shader package `runtime_scene.hlsl`.
- Unit tests: binding counts, `MeshCommand` aggregate order, RHI stats (`descriptor_writes`, buffers, writes order).

## Tests

- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (includes CTest).

## Validation Evidence


| Command            | Result | Notes                   |
| ------------------ | ------ | ----------------------- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS   | Windows host 2026-05-04 |


## Follow-ups (completed in the same Phase-4 pass)

- Editor material preview HLSL / descriptor parity with `scene_pbr_frame` (`register(b6)`) and per-preview scene UBO uploads via `pack_scene_pbr_frame_gpu`.
- `emit_material_graph_reviewed_hlsl_v0` stub aligned with runtime material descriptor layout (`b0` + `b6` + `t1` / `s16`) and `VSMain` / `PSMain` entries.
