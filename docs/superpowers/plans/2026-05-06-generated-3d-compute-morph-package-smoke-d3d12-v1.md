# Generated 3D Compute Morph Package Smoke D3D12 v1 (2026-05-06)

**Plan ID:** `generated-3d-compute-morph-package-smoke-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add a narrow generated `DesktopRuntime3DPackage` D3D12 package smoke that proves a selected cooked morph row can be
computed into a POSITION output buffer and rendered through the existing package host path.

## Context

- `runtime-rhi-compute-morph-d3d12-proof-v1` proved `RuntimeMorphMeshComputeBinding` and
  `create_runtime_morph_mesh_compute_binding` can dispatch a D3D12 compute shader and read back deterministic
  `base + weighted deltas` POSITION bytes.
- `runtime-rhi-compute-morph-renderer-consumption-d3d12-v1` added
  `make_runtime_compute_morph_output_mesh_gpu_binding` and proved `RhiFrameRenderer` can draw from that compute-written
  POSITION output through public RHI/renderer contracts.
- Generated 3D packages already ship cooked `morph_mesh_cpu` rows, morph-weight clips, generated graphics morph shader
  smokes, and host-owned morph palette counters. This slice adds the first package-visible D3D12 compute morph smoke.

## Constraints

- Keep generated gameplay code on public runtime/host/renderer contracts; do not expose `IRhiDevice`, descriptor heaps,
  command lists, swapchain frames, native D3D12 handles, or `SceneGpuBindingPalette` to gameplay.
- Keep the package smoke D3D12 primary-lane and explicit. Vulkan/Metal parity, async compute overlap, frame-graph
  scheduling, normal/tangent compute output, skin+morph composition, directional-shadow morph rendering, scene-schema
  compute-morph authoring, and broad generated 3D readiness remain out of scope.
- Reuse existing package file registration, shader artifact metadata, and smoke-result field patterns rather than
  creating ad hoc package paths.
- Add or update focused RED tests before production implementation.

## Done When

- A focused test/static smoke expectation fails first for generated D3D12 compute morph package readiness.
- The generated 3D package path can request compute morph, dispatch it through host-owned public RHI contracts, render
  the compute output, and emit deterministic package smoke fields proving the path was used.
- The generated scaffold, committed sample package metadata, docs, manifest, static checks, skills, and subagents all
  describe this as a D3D12 package smoke only.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or a concrete host/tool blocker is recorded.

## Tasks

- [x] Inspect the generated 3D package host path, shader artifact metadata, and existing morph package smoke fields to
  choose the smallest package-visible compute morph switch.
- [x] Add RED coverage for the generated package/scaffold/static smoke fields and D3D12 package path.
- [x] Implement host-owned compute morph dispatch plus renderer consumption in the selected D3D12 package path.
- [x] Update docs, manifest, static checks, AI guidance, and validation evidence.

## Validation Evidence

- RED: `ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure` failed before implementation because
  `make_runtime_compute_morph_output_mesh_gpu_binding` did not accept the generated package interleaved tangent-frame
  vertex payload as a compute morph source.
- RED: `cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests` failed before implementation
  because the SDL scene GPU binding injector did not expose compute morph output binding rows or related stats.
- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` failed before implementation because generated `DesktopRuntime3DPackage` scaffolds did not
  emit selected D3D12 `*_scene_compute_morph.vs.dxil` / `*_scene_compute_morph.cs.dxil` shader artifacts or
  `--require-compute-morph` package smoke expectations.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --preset desktop-runtime"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset dev --target mirakana_runtime_rhi_tests"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset dev -R mirakana_runtime_rhi_tests --output-on-failure"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_tests"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_tests --output-on-failure"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target mirakana_runtime_host_sdl3_public_api_compile"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& ctest --preset desktop-runtime -R mirakana_runtime_host_sdl3_public_api_compile --output-on-failure"`.
- PASS: `cmd.exe /d /c "set Path=%Path%&& set PATH=&& cmake --build --preset desktop-runtime --target sample_generated_desktop_runtime_material_shader_package"`.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -MaxFiles 1`.
- BLOCKED: full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1` exceeded the local 600s timeout after emitting existing unrelated warnings; the
  scoped tidy lane above proves the tidy setup is functional for this slice.
- PASS: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` (29/29 CTest tests passed; shader-toolchain Metal and Apple packaging entries remained
  diagnostic-only host gates on Windows).
