# Renderer Metal Memory Profiling Apple Host Artifacts v1 Implementation Plan

**Status:** Completed through PR #809 / merge commit `1ec10475e04410974e9fe0f25cbbd21e5fa370c4`.

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party Apple-host artifact producer that turns real macOS Metal heap, residency-set, and programmatic capture execution into validator-ready `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` evidence.

**Architecture:** Keep native Metal objects inside an Apple-only probe executable and the existing `mirakana::rhi::metal` private/test boundary. A PowerShell wrapper builds/runs the probe only on macOS with full Xcode, imports its artifact through the existing collector on `-RequireReady` capable hosts, validates retained evidence through the existing validator, and leaves Windows/Linux/default validation host-gated. GitHub-hosted macOS builds and runs the probe but may emit host-gated diagnostics when the hosted Metal device rejects `MTLResidencySet` creation. No public native handles, gameplay upload APIs, external-engine compatibility, broad backend parity, broad Metal readiness, commercial renderer readiness, broad renderer quality, or `environment_ready` claims are introduced.

**Tech Stack:** C++23, Objective-C++ on Apple hosts, Metal `MTLHeap`, `MTLResidencySet`, `MTLCommandQueue.addResidencySet`, `MTLCaptureManager`, `MTLCaptureDescriptor`, `MTLCaptureScope`, PowerShell 7, CMake/CTest, GitHub Actions macOS Metal lane.

---

## Official Source Review

- Apple Developer Documentation says `MTLHeap` is a memory pool for suballocating Metal resources: <https://developer.apple.com/documentation/metal/mtlheap>.
- Apple Developer Documentation says residency sets tell Metal which buffers, textures, and heaps to make resident/GPU-accessible: <https://developer.apple.com/documentation/metal/mtlresidencyset>.
- Apple Developer Documentation says `MTLResidencySetDescriptor.initialCapacity` is the number of allocations a new residency set can store without reallocating memory: <https://developer.apple.com/documentation/metal/mtlresidencysetdescriptor/initialcapacity>.
- Apple Developer Documentation says staged `MTLResidencySet.addAllocation(_:)` / `addAllocations(_:)` changes are applied by calling `MTLResidencySet.commit()`: <https://developer.apple.com/documentation/metal/mtlresidencyset/commit%28%29>.
- Apple Developer Documentation says `MTLResidencySet.requestResidency()` asks Metal to prepare allocations for residency: <https://developer.apple.com/documentation/metal/mtlresidencyset/requestresidency%28%29>.
- Apple Developer Documentation says a command queue can attach residency sets through `addResidencySet(_:)` / `addResidencySets`: <https://developer.apple.com/documentation/metal/mtlcommandqueue/addresidencyset%28_%3A%29>.
- Apple Developer Documentation says `MTLCaptureManager` captures Metal command data programmatically, and the Xcode guide documents `MTLCaptureDescriptor.outputURL` for GPU trace documents: <https://developer.apple.com/documentation/metal/mtlcapturemanager> and <https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-programmatically>.
- Apple Developer Documentation says `MTLCaptureScope` configures frame-capture scope boundaries: <https://developer.apple.com/documentation/metal/mtlcapturescope>.

Context7 was checked for Apple Metal SDK API docs. It only surfaced the Metal Shading Language specification and third-party binding docs, so Apple Developer Documentation remains the authoritative API source for this slice.

## Constraints

- Do not write retained artifacts into tracked `artifacts/`; generated artifacts stay under ignored runtime paths and are uploaded by CI only.
- Do not make `xctrace`, Xcode, Apple hardware, or `MTLCaptureManager` a default Windows/Linux validation dependency.
- Do not copy Apple samples, Stack Overflow snippets, Unity/Unreal/Godot APIs, or third-party implementation code.
- Do not add a public native handle API. Apple SDK types may appear only in Apple-only `.mm`/fixture implementation and private CMake wiring.
- If programmatic GPU trace document capture or `MTLResidencySet` creation is unavailable on a host, emit exact host-gated counters and fail only when `-RequireReady` is requested.

## Done When

- `tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1` reports host-gated counters on non-macOS hosts and, on macOS, builds/runs the Apple probe. A `-RequireReady` capable Apple host imports evidence with `tools/collect-renderer-metal-memory-profiling-host-evidence.ps1` and validates `renderer_metal_memory_profiling_status=ready` / `renderer_metal_memory_profiling_ready=1`; hosted or incomplete Apple hosts emit `renderer_metal_memory_profiling_host_artifacts_status=host_gated` with an exact host-gate reason.
- An Apple-only probe executable creates a real Metal device, command queue, heap allocation, heap resource allocation, residency set, residency request/commit, capture scope, command buffer, and capture artifact without exposing native handles.
- `tools/check-renderer-metal-memory-profiling-host-evidence.ps1` default validation remains `host_evidence_required` / `ready=0` unless generated artifacts are explicitly supplied.
- `.github/workflows/validate.yml` runs the producer without `-RequireReady` only on the existing macOS Metal host evidence lane and uploads generated ready artifacts or host-gated diagnostics for inspection.
- `tools/validate.ps1`, `tools/check-ai-integration.ps1`, `tools/check-json-contracts.ps1`, `tools/check-agents.ps1`, `tools/check-ci-matrix.ps1`, and focused producer/validator checks pass or record a concrete host blocker.
- `docs/superpowers/plans/README.md`, `docs/roadmap.md`, `docs/current-capabilities.md`, `engine/agent/manifest.fragments/*`, composed `engine/agent/manifest.json`, rendering guidance, and static checks agree on the new evidence producer and preserved non-claims.
- Branch publication is complete: validated commit, push, draft PR, selected CI lanes, ready wrapper, auto-merge with `--match-head-commit`, `origin/main` reachability, local main sync, and worktree cleanup.

## Task 1: RED Static Selection Gate

**Files:**
- Create: `tools/check-ai-integration-141-renderer-metal-memory-profiling-host-artifacts.ps1`
- Modify: `tools/check-ai-integration.ps1`

- [x] **Step 1: Add a failing static guard**

Require these literals before implementation exists:

```text
renderer-metal-memory-profiling-apple-host-artifacts-v1
tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1
MK_metal_memory_profiling_host_artifacts_probe
MTLHeap
MTLResidencySet
MTLCaptureManager
renderer_metal_memory_profiling_status=ready
renderer_backend_parity_ready=0
```

- [x] **Step 2: Run RED**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: FAIL on missing producer/probe/docs/manifest needles.

## Task 2: Apple Probe Executable

**Files:**
- Create: `tests/fixtures/metal_memory_profiling_host_artifacts_probe.mm`
- Modify: `CMakeLists.txt`

- [x] **Step 1: Add Apple-only executable**

Add an `APPLE`-only `MK_metal_memory_profiling_host_artifacts_probe` executable linked to `Foundation` and `Metal`, with `-fobjc-arc`.

- [x] **Step 2: Implement probe output**

The executable accepts:

```text
<output-directory>
```

It writes:

```text
capture.gputrace
capture-summary.txt
probe-summary.json
```

`probe-summary.json` contains at least:

```json
{
  "workload_id": "renderer_metal_memory_profiling_apple_host_probe",
  "capture_artifact": "capture-summary.txt",
  "heap_resource_rows": 1,
  "heap_allocated_bytes": 4096,
  "resident_bytes": 4096,
  "budget_bytes": 65536,
  "residency_set_allocation_rows": 1,
  "memory_pressure_sample_rows": 1,
  "memory_pressure_budget_status": "within_budget",
  "capture_scope_label": "GameEngine.RHI.Metal.MemoryProfiling",
  "capture_artifact_rows": 1,
  "raw_capture_bundle": "capture.gputrace"
}
```

`capture-summary.txt` is the stable retained validator artifact. `capture.gputrace` is kept as an uploaded raw inspection bundle when the host and Xcode capture path create it, but the schema row uses the summary leaf file so the existing validator can hash a deterministic artifact path.

- [x] **Step 3: Keep non-Apple builds clean**

The target must not exist on Windows/Linux, and default `dev` configure/build must remain unaffected.

## Task 3: Producer Wrapper

**Files:**
- Create: `tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1`
- Modify: `tools/validate.ps1`
- Modify: `tools/classify-pr-validation-tier.ps1`
- Modify: `tools/check-ci-matrix.ps1`
- Modify: `.github/workflows/validate.yml`

- [x] **Step 1: Add producer command**

On non-macOS, emit:

```text
renderer_metal_memory_profiling_host_artifacts_status=host_gated
renderer_metal_memory_profiling_host_artifacts_ready=0
renderer_metal_memory_profiling_ready=0
renderer_backend_parity_ready=0
```

On macOS with `-RequireReady`, build the probe using `ci-macos-appleclang`, run it under ignored `artifacts/renderer/metal-memory-profiling-host-evidence/<task>/`, import with the existing collector, and run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ArtifactRootRelative artifacts/renderer/metal-memory-profiling-host-evidence/<task> -RequireReady -SkipFocusedRendererBuild
```

- [x] **Step 2: Wire validation**

Add a default non-ready static/self-check path to `tools/validate.ps1`. Classifier changes to this producer must select `metal_host_evidence=true` and `ios_metal_evidence=true`, matching the current Apple evidence policy.

- [x] **Step 3: Wire hosted macOS lane**

In `.github/workflows/validate.yml`, run the producer with `-RequireReady` only when `metal_host_evidence == 'true'`, then upload `artifacts/renderer/metal-memory-profiling-host-evidence/**`.

## Task 4: Docs, Manifest, Agent Surfaces

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/testing.md`
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `.agents/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`

- [x] **Step 1: Record new evidence producer**

Document that this producer can make local Apple-host retained evidence ready for `renderer_metal_memory_profiling_ready=1`, while default repository validation and broad readiness counters remain unclaimed without generated artifacts.

- [x] **Step 2: Compose manifest**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

Expected: composed manifest updates without hand edits.

## Task 5: Validation and Publication

- [x] **Step 1: Focused validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

- [x] **Step 2: Slice validation**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

- [x] **Step 3: Publish**

Run publication preflight, commit task-owned files, push branch, open draft PR, wait for selected CI lanes, ready the PR through `tools/ready-task-pr.ps1`, auto-merge with `--match-head-commit`, verify head reaches `origin/main`, sync local `main`, and remove the worktree with guarded cleanup.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` before implementation | Failed as expected on missing `tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; wrote `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1` | Passed on Windows as host-gated with `renderer_metal_memory_profiling_ready=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0` | Passed after focused renderer build/CTest; default evidence remains host-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed. |
| `git diff --check` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `git diff --check`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after direct `newResidencySetWithDescriptor:error:`, `MTLResidencySet.commit()`, and pre-allocation residency-set creation fix | Passed on Windows; latest full validation log root `out/validation-logs/validate-20260625-015416-45984`. |
| GitHub Actions `macOS Metal CMake` on PR #809 head `74aaab17c19f774957219ebf118333ab4e54d4d7` | Failed on hosted `macos-15-arm64` / macOS `15.7.7` after successful configure/build because `MTLResidencySet creation failed`; this confirmed the hosted runner is a host gate for real residency-set artifacts, not a broad Metal readiness proof host. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after GitHub-hosted macOS host-gate handling | Passed on Windows; latest full validation log root `out/validation-logs/validate-20260625-021921-43304`. |
| GitHub Actions `macOS Metal CMake` on PR #809 head `20160842ec1d80823553cf67244cd65e24796fcb` | The producer correctly emitted `renderer_metal_memory_profiling_host_artifacts_status=host_gated`, `renderer_metal_memory_profiling_host_gate_reason=mtlresidencyset_unavailable`, and uploaded two `host-gate-summary.*` files, but the job still failed because PowerShell retained the probe native `$LASTEXITCODE=1` after the handled host gate. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/generate-renderer-metal-memory-profiling-host-artifacts.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`; `git diff --check`; `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` after explicit host-gated `$LASTEXITCODE = 0` handling | Passed on Windows; latest full validation log root `out/validation-logs/validate-20260625-024347-2224`. |
| `gh pr view 809 --json state,isDraft,mergedAt,mergeCommit,headRefOid,statusCheckRollup` | PASS: PR #809 is merged, head `bab101220633a622a0a68e7158fb0caeb8f32572` reached merge commit `1ec10475e04410974e9fe0f25cbbd21e5fa370c4`, and hosted PR Gate, Windows MSVC, Linux CMake/Coverage/ASan/Vulkan, static shards, macOS Metal CMake, iOS Metal, iOS Simulator smoke, and CodeQL passed. |
| `git merge-base --is-ancestor bab101220633a622a0a68e7158fb0caeb8f32572 origin/main`; `git merge-base --is-ancestor 1ec10475e04410974e9fe0f25cbbd21e5fa370c4 origin/main` | PASS: the reviewed head and merge commit reach `origin/main`; local `main` was synced to `1ec10475e04410974e9fe0f25cbbd21e5fa370c4`. |
