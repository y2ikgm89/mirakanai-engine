# Runtime RHI Compute Morph Pipelined Output Ring D3D12 v1 (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `runtime-rhi-compute-morph-pipelined-output-ring-d3d12-v1`  
**Status:** Completed  
**Parent:** [production-completion-master-plan-v1](2026-05-03-production-completion-master-plan-v1.md)

## Goal

Add the runtime/RHI compute morph output-ring prerequisite for D3D12 pipelined compute/graphics scheduling: compute can
write one output slot while graphics consumes a previously completed slot, using first-party descriptor/buffer contracts
and tests without claiming measured async overlap or package-visible performance.

## Architecture

Keep this inside `mirakana_runtime_rhi`, `mirakana_rhi`, D3D12-focused tests, and renderer/runtime RHI helpers. Preserve the existing
single-output `RuntimeMorphMeshComputeBinding` behavior for current callers while adding a clean multi-slot contract for
future pipelined scheduling. Do not expose native queues, fences, command lists, descriptor handles, query heaps,
timestamp resources, or backend-specific D3D12 objects through gameplay or runtime-host APIs.

## Tech Stack

C++23, `mirakana_rhi`, `mirakana_runtime_rhi`, `mirakana_rhi_d3d12`, D3D12 focused tests, and official Microsoft D3D12 multi-engine
synchronization guidance for compute/graphics pipelining.

---

## Context

- Runtime RHI Compute Morph Async Overlap Evidence D3D12 v1 added a deterministic diagnostic that classifies the current
  same-frame graphics wait as `not_proven_serial_dependency`.
- Microsoft's D3D12 pipelined compute/graphics guidance requires multiple versions of data passed from compute to
  graphics when graphics consumes compute results from an earlier frame.
- The current `RuntimeMorphMeshComputeBinding` owns one output POSITION buffer plus optional NORMAL/TANGENT buffers and
  one descriptor set, which forces the same output resource to serve compute writes and graphics reads.

## Constraints

- Do not claim async speedup, measured overlap, GPU performance improvement, Vulkan/Metal parity, frame graph scheduling,
  directional-shadow morph rendering, scene-schema compute-morph authoring, graphics morph+skin composition, generated
  package readiness, or broad renderer quality.
- Do not update generated `DesktopRuntime3DPackage` validation to require pipelined output rings in this slice.
- Keep the existing single-output compute morph APIs source-compatible within the current greenfield policy unless a
  cleaner non-backward-compatible edit is required by the implementation.
- Keep all native D3D12 handles and timestamp/query resources backend-private.

## Done When

- A RED runtime-RHI/D3D12 test fails first because there is no first-party multi-slot compute morph output-ring contract.
- `mirakana_runtime_rhi` can create and describe a compute morph output ring with at least two output slots, each with POSITION
  output buffers and descriptor state suitable for compute dispatch.
- Existing single-output compute morph tests still pass, and focused tests prove the ring can select distinct write/read
  slots without aliasing the same output buffer.
- Docs, manifest, plan registry, static checks, skills/subagents, and validation evidence describe this as pipelined
  output-ring readiness only, not measured overlap or package-visible performance.
- Focused tests plus `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, validate-scoped tidy, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`
  pass, or concrete host/tool blockers are recorded.

## Tasks

- [x] Inspect current `RuntimeMorphMeshComputeBinding` output buffer and descriptor-set construction.
- [x] Add a RED runtime-RHI/D3D12 test for missing multi-slot compute morph output-ring support.
- [x] Implement minimal output-ring contracts and helpers while preserving current single-slot callers.
- [x] Prove distinct output slots through NullRHI and D3D12 focused tests.
- [x] Update docs, manifest, plan registry, static checks, skills/subagents, and validation evidence.

## Validation Evidence

- RED: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` failed after adding the NullRHI output-ring test because
  `RuntimeMorphMeshComputeBindingOptions::output_slot_count`, `RuntimeMorphMeshComputeBinding::output_slots`, and the
  slot-indexed `make_runtime_compute_morph_output_mesh_gpu_binding` overload did not exist.
- GREEN focused build: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` passed after adding `RuntimeMorphMeshComputeOutputSlot`, output-slot descriptor
  arrays for POSITION output binding 3, slot-indexed mesh/skinned binding helpers, and output-slot release coverage in
  `mirakana_runtime_scene_rhi`.
- GREEN focused tests: `ctest --preset dev -R "mirakana_(runtime_rhi|d3d12_rhi)_tests" --output-on-failure` passed. The D3D12
  test writes slot 1 through a descriptor range at `u3/u4` and reads back the selected slot's POSITION bytes.
- Static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` passed after the
  docs/manifest/skill/subagent updates.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed. It reported `tidy-check: ok (1 files)`, rebuilt the `dev` preset, and
  passed CTest `29/29`. Metal shader packaging and Apple packaging remained Windows-host diagnostic-only blockers.
