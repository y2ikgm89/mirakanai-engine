# Renderer Metal Memory Profiling Host Evidence v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:executing-plans or superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a fail-closed retained Apple-host evidence contract for renderer Metal memory residency and profiling capture proof rows without claiming broad renderer, backend parity, Metal, commercial, or environment readiness.

**Architecture:** Keep the completed `BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc` / `make_backend_renderer_parity_apple_metal_memory_profiling_proofs` value mapping in `MK_renderer` intact. Add a schema-backed artifact validator that can promote only local retained evidence rows for `memory_residency` and `profiling_capture` when the retained Apple-host artifacts prove exact Metal heap allocation, residency-set request/commit, memory-pressure evidence, capture-manager/scope/descriptor use, capture boundaries, and a deterministic retained capture artifact. The validator must stay command-surface-only, avoid public native handles, avoid SDK types in public headers, and leave default repository validation `host_evidence_required` until real Apple-host artifacts exist under the default artifact root.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi_metal`, PowerShell 7, JSON Schema draft 2020-12, Apple Metal `MTLHeap`, `MTLResidencySet`, `MTLCommandQueue.addResidencySet`, `MTLCaptureManager`, `MTLCaptureDescriptor`, `MTLCaptureScope`, Xcode Metal tooling, CMake/CTest, `tools/check-ai-integration.ps1`.

---

**Plan ID:** `renderer-metal-memory-profiling-host-evidence-v1`

**Status:** Completed through PR #807 / merge commit `79ce2668c149a6295a6d846559c82bca6f887fcc`.

**Date:** 2026-06-24

## Official Source Gate

- Apple Developer Documentation `MTLHeap`: the memory evidence row must prove heap-backed allocation rather than generic buffer/texture creation.
- Apple Developer Documentation `MTLResidencySet`: the residency row must prove a residency set exists for resources or heaps.
- Apple Developer Documentation `MTLResidencySet.requestResidency()` and `MTLCommandQueue.addResidencySet(_:)`: ready residency evidence requires both request and command-queue commit rows.
- Apple Developer Documentation `MTLCaptureManager`, `MTLCaptureDescriptor`, and `MTLCaptureScope`: profiling evidence must prove programmatic capture setup, capture object/scope, start/stop boundaries, and retained capture artifact rows.
- Apple Developer Documentation "Capturing a Metal workload programmatically": the profiling contract follows Apple's programmatic command capture flow but stores only first-party counters and artifact hashes.

## Non-Overlap Contract

- `renderer-metal-apple-host-evidence` remains the selected environment-feature host recipe and may only feed synchronization, shader-validation, and package-evidence rows.
- This slice does not execute Metal on the current Windows host and does not claim real Apple-host readiness without retained artifacts.
- D3D12, Vulkan, Linux, Android, iOS, simulator-only, compile-only, or cross-backend evidence cannot satisfy these Metal memory/profiling rows.
- Public native handles, Unity/Unreal/Godot API or UI parity, broad backend parity, broad Metal readiness, commercial renderer readiness, broad renderer quality, and broad `environment_ready` remain false.
- The retained fixture may prove that the validator can accept complete rows; it is not a substitute for real production Apple-host artifacts under `artifacts/renderer/metal-memory-profiling-host-evidence`.

## Files

- Create: `schemas/renderer-metal-memory-profiling-host-evidence.schema.json`
- Create: `tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/evidence.json`
- Create: `tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready/capture-summary.txt`
- Create: `tools/check-renderer-metal-memory-profiling-host-evidence.ps1`
- Create: `tools/check-ai-integration-139-renderer-metal-memory-profiling-host-evidence.ps1`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.agents/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`
- Regenerate: `engine/agent/manifest.json`

## Result Contract

`tools/check-renderer-metal-memory-profiling-host-evidence.ps1` scans `evidence.json` rows under `-ArtifactRootRelative`, defaulting to `artifacts/renderer/metal-memory-profiling-host-evidence`, and emits:

```text
validation_recipe=renderer-metal-memory-profiling-host-evidence
renderer_metal_memory_profiling_status=host_evidence_required|blocked|ready
renderer_metal_memory_profiling_ready=0|1
renderer_metal_memory_profiling_retained_apple_host_evidence=0|1
renderer_metal_memory_residency_ready=0|1
renderer_metal_profiling_capture_ready=0|1
renderer_metal_memory_profiling_artifact_rows=N
renderer_metal_memory_profiling_ready_rows=N
renderer_metal_memory_profiling_invalid_rows=N
renderer_metal_memory_profiling_missing_artifacts=N
renderer_metal_memory_profiling_heap_rows=N
renderer_metal_memory_profiling_residency_set_rows=N
renderer_metal_memory_profiling_residency_commit_rows=N
renderer_metal_memory_profiling_pressure_rows=N
renderer_metal_memory_profiling_capture_scope_rows=N
renderer_metal_memory_profiling_capture_artifact_rows=N
renderer_backend_parity_ready=0
renderer_metal_broad_readiness=0
renderer_commercial_readiness=0
renderer_broad_quality_ready=0
renderer_environment_ready=0
```

Ready requires at least one retained row with schema version `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1`, claim id `renderer-metal-memory-profiling-host-evidence-v1`, `platform=macos`, full Xcode and `metal` / `metallib` tooling rows, official Apple source ids for heap, residency, and capture APIs, runtime and command-queue rows, `MTLHeap` allocation rows, resource/suballocation rows, `MTLResidencySet` rows, request and command-queue commit rows, memory pressure evidence rows, `MTLCaptureManager` / descriptor / scope rows, start/stop capture boundaries, command-buffer capture rows, matching retained capture artifact SHA-256, and all non-claim booleans false.

## Tasks

### Task 1: RED Static Contract

- [x] Add this plan and `tools/check-ai-integration-139-renderer-metal-memory-profiling-host-evidence.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Expected RED before implementation: missing validator, schema, fixture, command-surface, docs, manifest, and skill needles.

### Task 2: Schema And Fixture

- [x] Add the JSON schema for `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1`.
- [x] Add a ready fixture with one retained memory residency row and one retained profiling capture row.
- [x] Add a small deterministic `capture-summary.txt` fixture and record its SHA-256 in `evidence.json`.

### Task 3: Validator

- [x] Add `tools/check-renderer-metal-memory-profiling-host-evidence.ps1` with `-ArtifactRootRelative`, `-RequireReady`, and `-ExpectedEvidenceCounters`.
- [x] Reject missing/invalid schema version, non-macOS rows, missing full Xcode/tool rows, missing official source ids, missing heap rows, missing residency-set request/commit rows, missing pressure evidence, missing capture manager/descriptor/scope/boundary rows, missing command-buffer capture rows, unsafe artifact paths, artifact hash mismatch, simulator-only evidence, cross-backend inference, native-handle exposure, and broad readiness claims.
- [x] Verify the ready fixture passes with `-RequireReady`.
- [x] Verify the default repository path remains `host_evidence_required` and fails only when `-RequireReady` is set.

### Task 4: Docs, Manifest, And Agent Surface

- [x] Add `rendererMetalMemoryProfilingHostEvidenceCheck` to the manifest command surface.
- [x] Update module evidence, game guidance, current capabilities, roadmap, plan registry, and rendering skill guidance to describe the retained artifact contract and default host-gated state.
- [x] Compose `engine/agent/manifest.json`.
- [x] Keep `currentActivePlan` on the production-completion selection gate after the slice closes and keep `unsupportedProductionGaps = []`.

### Task 5: Validation And Publication

- [x] Run the ready fixture validator:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ArtifactRootRelative tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready -RequireReady
```

- [x] Run the default host-gated validator:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1
```

- [x] Run `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-text-format.ps1`, and `git diff --check`.
- [x] Run full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after docs, manifest, schema, and static checks are green.
- [x] Run publication preflight, commit, push, open a draft PR, wait for hosted PR Gate, mark ready with `tools/ready-task-pr.ps1`, register auto-merge with the head SHA, verify `origin/main`, sync local `main`, clean up the worktree, and reread the production-completion selection gate.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` before implementation | Failed as expected on missing `tools/check-renderer-metal-memory-profiling-host-evidence.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ArtifactRootRelative tests/fixtures/renderer/metal-memory-profiling-host-evidence/ready -RequireReady -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=ready renderer_metal_memory_profiling_ready=1 renderer_metal_memory_residency_ready=1 renderer_metal_profiling_capture_ready=1 renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_commercial_readiness=0 renderer_broad_quality_ready=0` | PASS: ready fixture emitted `renderer_metal_memory_profiling_ready=1` with broad readiness counters 0. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1` | PASS: default root emitted `renderer_metal_memory_profiling_status=host_evidence_required`, `renderer_metal_memory_profiling_ready=0`, and zero retained artifact rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -RequireReady` | Failed as expected on default root because retained Apple-host artifacts are absent. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS: regenerated `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS: `first-party-ui-clean-room: ok`; `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS: `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS: `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | PASS: `text-format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS: `text-format-check: ok`; `format-check: ok`. |
| `git diff --check` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`; 157/157 CTest tests passed. Local Windows host lacks Apple Metal tools, and Apple host evidence remained diagnostic/host-gated as expected. |
| `gh pr create --draft --base main --head codex/renderer-metal-memory-profiling-host-evidence` | PASS: opened draft PR #807. |
| `gh pr view 807 --json state,isDraft,mergedAt,mergeCommit,headRefOid,statusCheckRollup` | PASS: PR #807 merged on 2026-06-24T13:53:01Z as merge commit `79ce2668c149a6295a6d846559c82bca6f887fcc`; hosted PR Gate, Windows MSVC, Linux CMake, full static shards, macOS Metal CMake, iOS Simulator smoke, and CodeQL checks passed. |
| `git merge-base --is-ancestor 596853dc3300635e41f6c0e646791f8e4733e289 origin/main` | PASS: PR #807 head reaches `origin/main`. |

## Done When

- The ready fixture proves the validator can promote only retained Apple-host memory residency and profiling capture artifact rows.
- The default repository state remains host-gated and does not claim Metal memory/profiling readiness without retained artifacts.
- Docs, plan registry, manifest fragments, generated manifest, schema, static guards, and rendering skills preserve all non-claims.
- The candidate is locally validated, published as a PR, merged after CI, and root `main` is synchronized before selecting the next production-completion slice.
