# MAVG Metal Mesh LOD Apple Host Execution Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fail-closed retained Apple-host evidence contract for MAVG Metal object/mesh shader LOD execution without claiming readiness from the current Windows host.

**Architecture:** Keep Task 9A's value-only `mavg-metal-mesh-lod-host-evidence-v1` / `metal_mavg_mesh_lod.hpp` gate intact and add a schema-backed host artifact validator around it. The validator may promote `mavg_metal_mesh_lod_ready=1` only from retained Apple-host evidence rows proving object/mesh shader pipeline creation, `drawMeshThreads` execution, deterministic readback hash, and package-visible output for a first-party MAVG workload. Missing artifacts keep the row `host_evidence_required`.

**Tech Stack:** C++23, `MK_rhi_metal`, PowerShell 7, JSON Schema draft 2020-12, Apple Metal object/mesh shaders, Metal Shading Language object payloads, Apple Metal Feature Set Tables, Xcode `metal` / `metallib`, `MTLRenderCommandEncoder` mesh/object resource binding and `drawMeshThreads`, CMake/CTest, `tools/check-ai-integration.ps1`.

---

**Plan ID:** `mavg-metal-mesh-lod-apple-host-execution-v1`

**Status:** Published via PR #773 and merged to `origin/main` at `d745eeab502405ef8324e1c8c424d77aef529766`. This is Task 9B under `MAVG Advanced Backend Evidence Closeout v1`; it does not change `currentActivePlan`.

**Date:** 2026-06-23

## Official Source Gate

- Context7 revalidated Metal Shading Language object-function syntax through `/dogukanveziroglu/metal-shading-language-specification`: object functions use `[[object]]`, export a `[[payload]]` object-data buffer to mesh functions, and can set `mesh_grid_properties` for downstream mesh work.
- Apple Developer's "Adjusting the level of detail using Metal mesh shaders" sample is the implementation shape: object shaders select LOD-dependent primitive and vertex counts for meshlets, then mesh shaders render point, line, or triangle variants.
- Apple Developer's mesh/object shader resource preparation commands require binding resources to object and mesh shader argument tables with Metal APIs such as `setMeshBuffer`.
- Apple Developer's `drawMeshThreads(_:threadsPerObjectThreadgroup:threadsPerMeshThreadgroup:)` command is the retained execution command row for this slice.
- Apple Metal Feature Set Tables must name the Apple GPU family row used for mesh shader support. D3D12/Vulkan proof, simulator-only output, or ray-tracing evidence cannot promote Metal mesh readiness.

## Non-Overlap Contract

- Task 9A already owns value-only prerequisite modeling in `metal_mavg_mesh_lod.hpp` and `evaluate_mavg_metal_mesh_lod_host_evidence`.
- The 9A API entry remains `MetalMavgMeshLodHostEvidenceDesc`, `MetalMavgMeshLodHostEvidenceResult`, `evaluate_mavg_metal_mesh_lod_host_evidence`, and `tools/check-mavg-metal-host-evidence.ps1`; the default repository root must continue to emit `mavg_metal_mesh_lod_status=host_evidence_required` and `mavg_metal_mesh_lod_ready=0`.
- D3D12 and Vulkan mesh LOD execution evidence remain backend-local and cannot satisfy Metal rows.
- Metal ray tracing is separate from Metal object/mesh shader execution.
- Nanite compatibility, equivalence, superiority, and marketing claims remain false.
- Package-visible broad `mavg_mesh_shader_lod_ready`, broad MAVG backend readiness, and broad CPU/GPU/memory optimization remain false.

## Files

- Create: `schemas/mavg-metal-mesh-lod-host-execution.schema.json`
- Create: `tests/fixtures/mavg/metal-mesh-lod-host-execution/ready/evidence.json`
- Create: `tests/fixtures/mavg/metal-mesh-lod-host-execution/ready/readback.bin`
- Modify: `tools/check-mavg-metal-host-evidence.ps1`
- Modify: `tools/check-ai-integration-129-mavg-metal-mesh-lod.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-06-21-mavg-advanced-backend-evidence-closeout-v1.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Regenerate: `engine/agent/manifest.json`

## Result Contract

`tools/check-mavg-metal-host-evidence.ps1` gains `-ArtifactRootRelative`, defaulting to `artifacts/mavg/metal-mesh-lod-host-execution`. It still builds and runs `MK_mavg_metal_mesh_lod_tests`. It scans `evidence.json` rows under the artifact root and emits these counters:

```text
mavg_metal_mesh_lod_execution_artifact_rows=N
mavg_metal_mesh_lod_execution_ready_rows=N
mavg_metal_mesh_lod_execution_invalid_rows=N
mavg_metal_mesh_lod_execution_missing_artifacts=N
mavg_metal_mesh_lod_object_mesh_pipeline_rows=N
mavg_metal_mesh_lod_object_mesh_dispatch_rows=N
mavg_metal_mesh_lod_draw_mesh_threads_rows=N
mavg_metal_mesh_lod_shader_payload_rows=N
mavg_metal_mesh_lod_readback_hash_rows=N
mavg_metal_mesh_lod_package_visible_output_rows=N
mavg_metal_mesh_lod_retained_apple_host_evidence=0|1
mavg_metal_mesh_lod_ready=0|1
mavg_mesh_shader_lod_ready=0
mavg_metal_ray_tracing_ready=0
mavg_metal_mesh_lod_nanite_compatible=0
mavg_metal_mesh_lod_nanite_equivalent=0
mavg_metal_mesh_lod_nanite_superior=0
mavg_metal_mesh_lod_broad_backend_readiness=0
mavg_metal_mesh_lod_broad_optimization=0
```

Ready requires at least one retained row with schema version `GameEngine.MavgMetalMeshLodHostExecutionEvidence.v1`, `platform=macos`, full Xcode and `metal`/`metallib` tool rows, Apple GPU family row, object and mesh shader support, first-party workload id, object/mesh pipeline row, `drawMeshThreads` row, command-buffer completion, deterministic readback SHA-256, matching retained readback artifact hash when an artifact path is present, package-visible output rows, and all non-claim booleans false.

## Tasks

### Task 1: RED Fixture And Validator Contract

- [x] Add a fixture evidence row under `tests/fixtures/mavg/metal-mesh-lod-host-execution/ready/evidence.json` plus `readback.bin`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1 -ArtifactRootRelative tests/fixtures/mavg/metal-mesh-lod-host-execution/ready -RequireReady`.
- [x] Expected RED before implementation: the script rejects the unknown `-ArtifactRootRelative` parameter or cannot emit retained execution counters.

### Task 2: Schema And Parser

- [x] Add `schemas/mavg-metal-mesh-lod-host-execution.schema.json` with required host, toolchain, feature, shader, execution, artifact, package, and non-claim fields.
- [x] Update the validator to scan `evidence.json` rows from `-ArtifactRootRelative`.
- [x] Reject missing/invalid schema version, non-macOS host rows, missing full Xcode/tool rows, missing Apple GPU family row, missing object/mesh shader support, missing `drawMeshThreads` row, missing readback hash, artifact path traversal, artifact hash mismatch, simulator-only rows, cross-backend inference, ray-tracing conflation, native handle exposure, Nanite claims, broad backend readiness, and broad optimization.
- [x] Verify the ready fixture passes with `-RequireReady`.

### Task 3: Default Fail-Closed Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1`.
- [x] Expected GREEN on Windows without retained artifacts: `mavg_metal_mesh_lod_status=host_evidence_required`, `mavg_metal_mesh_lod_ready=0`, and `mavg_metal_mesh_lod_execution_artifact_rows=0`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1 -RequireReady`.
- [x] Expected failure: readiness is incomplete until retained Apple-host evidence exists under the default artifact root.

### Task 4: Docs, Manifest, And Static Guards

- [x] Update docs and manifest fragments to describe the retained artifact contract and the default host-gated state.
- [x] Extend `tools/check-ai-integration-129-mavg-metal-mesh-lod.ps1` with schema, fixture, validator parameter, new counters, and non-claim needles.
- [x] Compose the manifest with `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-format.ps1`, `tools/check-text-format.ps1`, and `git diff --check`.

### Task 5: Slice Validation And Publication

- [x] Run the focused ready fixture validator:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1 -ArtifactRootRelative tests/fixtures/mavg/metal-mesh-lod-host-execution/ready -RequireReady
```

- [x] Run the default host-gated validator:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-mavg-metal-host-evidence.ps1
```

- [x] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` only after docs, manifest, schema, and static checks are green.
- [x] Run publication preflight, commit the validated candidate, push, open a draft PR, wait for hosted PR Gate, mark ready with `tools/ready-task-pr.ps1`, register auto-merge with the head SHA, verify `origin/main`, sync local `main`, and clean up the task worktree.

## Done When

- The ready fixture proves the validator can promote retained Apple-host object/mesh shader execution evidence.
- The default repository state remains host-gated and does not claim Metal readiness without retained artifacts.
- Docs, plan registry, manifest fragments, schema, and static guards preserve the non-claims.
- The candidate is validated locally, published as a PR, merged after CI, and root `main` is synchronized.
