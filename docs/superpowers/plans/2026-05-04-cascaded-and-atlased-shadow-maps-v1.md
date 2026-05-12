# Cascaded and atlased shadow maps v1 (2026-05-04)

## Goal

Deliver production-grade directional shadow coverage using cascaded shadow maps (CSM) with an atlas-friendly layout, aligned with common engine practice (split scheme, stable light-space matrices, receiver resolve) and **no backward compatibility** with the temporary single-slice v0 path once v1 lands.

## Scope

- **Phase A (math, landed in this slice)**: `compute_practical_shadow_cascade_distances` — practical split (log/uniform blend, GPU Gems 3 Ch.10 style) returning `cascade_count + 1` monotonic view-space distances from `camera_near` to `camera_far`.
- **Phase B (RHI / frame graph)**: multiple shadow passes or layered depth targets, atlas packing of cascade tiles, and frame-graph resources wired for directional lights.
- **Phase C (shaders / resolve)**: cascade selection in receiver shaders, PCF / filtering policy, and integration with the existing `shadow.receiver.resolve` path or its v1 successor.
- **Validation**: unit tests for cascade math; renderer smoke / frame-graph tests for resource lifetime and diagnostics; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` green.

## Non-goals (v1)

- Point and spot shadow atlases (tracked separately once directional CSM is stable).
- Ray-traced shadows or hybrid RT/CSM.
- Dynamic quality scaling tied to GPU timing (future perf slice).

## References

- Zhang, "Parallel-Split Shadow Maps for Large-scale Virtual Environments" (GPU Gems 3, Ch.10) — practical split scheme.
- Foundation slice: `docs/superpowers/plans/2026-04-30-shadow-map-foundation-v0.md`.

## Verification

| Check | Command / artifact |
| --- | --- |
| Unit tests | `renderer_rhi_tests` cascade distance + shadow map / light-space / smoke cases |
| Repo gates | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` |

## Status

- **Phase A**: implemented (`compute_practical_shadow_cascade_distances` + tests); validated 2026-05-04.
- **Phase B**: implemented — directional shadow depth atlas (`ShadowMapDesc::directional_cascade_count`, `ShadowMapPlan` tile + atlas sizing), viewport/scissor cascade loop in `RhiDirectionalShadowSmokeFrameRenderer`, SDL passes atlas extent from `ShadowMapPlan`.
- **Phase C**: completed — receiver descriptor layout binding 2 uniform (`pack_shadow_receiver_constants` with per-cascade `clip_from_world`, camera `view_from_world` row for split distances, 768-byte cbuffer), `runtime_scene.hlsl` cascade selection + atlas UV + PCF, SDL smoke packs constants per scene camera, `RhiDirectionalShadowSmokeFrameRenderer` binds the cbuffer, D3D12 unit tests updated. Receiver depth compare uses **`saturate(light_clip.z / w)`** to match D3D12/Vulkan depth-buffer NDC Z in `[0, 1]` (not OpenGL-style `z*0.5+0.5`). **Vulkan**: recompile `tests/shaders/vulkan_shadow_receiver.hlsl` to SPIR-V and refresh `MK_VULKAN_TEST_SHADOW_RECEIVER_*_SPV` env artifacts before expecting the configured Vulkan shadow-receiver test to pass validation; use `tools/compile-vulkan-shadow-receiver-test-spirv.ps1` (see `docs/testing.md`).
- **Doc alignment (post-implementation)**: `docs/architecture.md` and `docs/roadmap.md` updated so the SDL package-visible shadow smoke path is described as consuming Stable Directional Light-Space Policy v0 data, atlased multi-cascade depth, and `pack_shadow_receiver_constants` (hardware comparison samplers, Metal shadow presentation, and editor shadow tooling remain out of scope for this plan).
