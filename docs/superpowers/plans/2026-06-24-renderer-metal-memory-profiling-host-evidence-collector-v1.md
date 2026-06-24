# Renderer Metal Memory Profiling Host Evidence Collector Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a first-party, host-owned collector/importer that shapes Apple-host Metal heap/residency/capture artifacts into the existing `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` `evidence.json` contract without claiming broad backend parity, broad Metal readiness, commercial renderer readiness, or broad renderer quality.

**Architecture:** The collector is a PowerShell command surface with `Plan` and `Import` modes. `Plan` reports macOS/Xcode/`xcrun`/`metal`/`metallib`/`xctrace` availability and the intended evidence path without writing artifacts; `Import` accepts operator-owned capture/summary artifacts already stored under the selected evidence root, computes SHA-256, writes a schema-shaped `evidence.json`, and leaves all non-claim booleans false. The collector does not start Metal captures, does not execute arbitrary workloads, does not expose native handles, and does not make default Windows/Linux validation depend on Apple tooling.

**Tech Stack:** PowerShell 7, JSON Schema draft 2020-12, existing `tools/check-renderer-metal-memory-profiling-host-evidence.ps1`, `tools/apple-host-helpers.ps1`, Apple Metal `MTLHeap`, `MTLResidencySet`, `MTLCommandQueue.addResidencySet`, `MTLCaptureManager`, `MTLCaptureDescriptor`, `MTLCaptureScope`, Xcode `xcrun` / `xctrace`, CMake/CTest validation, repository manifest/static checks.

---

Plan ID: `renderer-metal-memory-profiling-host-evidence-collector-v1`
Status: Draft PR published; hosted validation pending.

## Official Source Checks

- Apple Developer Documentation: `MTLHeap` is a memory pool for suballocating Metal resources. <https://developer.apple.com/documentation/metal/mtlheap>
- Apple Developer Documentation: `MTLResidencySet` groups resource allocations that Metal can make GPU-accessible/resident, with `requestResidency()` and allocation staging APIs. <https://developer.apple.com/documentation/metal/mtlresidencyset>
- Apple Developer Documentation: `MTLCaptureManager`, `MTLCaptureDescriptor`, and `MTLCaptureScope` are the programmatic capture surfaces for Metal command data and capture scope boundaries. <https://developer.apple.com/documentation/metal/mtlcapturemanager> / <https://developer.apple.com/documentation/metal/mtlcapturedescriptor> / <https://developer.apple.com/documentation/metal/mtlcapturescope>
- Apple Developer Documentation: Xcode command-line tools include `xctrace` for recording, importing, exporting, and symbolicating Instruments `.trace` files. <https://developer.apple.com/documentation/xcode/xcode-command-line-tool-reference>

## Context

- `Renderer Backend Parity Apple Memory Profiling Proof Rows v1` added backend-local `memory_residency` and `profiling_capture` proof rows.
- `Renderer Metal Memory Profiling Host Evidence v1` added the retained `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` schema and validator.
- Default repository validation intentionally remains `renderer_metal_memory_profiling_status=host_evidence_required` and `renderer_metal_memory_profiling_ready=0` because real Apple-host artifacts are not checked in under `artifacts/renderer/metal-memory-profiling-host-evidence`.
- This plan adds only the missing operator tool for turning host-owned artifacts into validator-ready retained evidence.

## File Responsibilities

- Create `tools/collect-renderer-metal-memory-profiling-host-evidence.ps1`: `Plan` / `Import` collector that writes one `evidence.json` row beside an operator-owned capture artifact.
- Create `tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1`: self-test using ignored `out/` artifacts, verifying `Plan`, `Import`, generated JSON, validator readiness, and zero broad-claim counters.
- Create `tools/check-ai-integration-140-renderer-metal-memory-profiling-host-evidence-collector.ps1`: static drift guard requiring the collector, self-test, docs, manifest, validation, CI classifier, and non-claims.
- Modify `tools/validate.ps1`: include the collector self-test in static validation.
- Modify `tools/classify-pr-validation-tier.ps1` and `tools/check-ci-matrix.ps1`: classify collector/validator changes as Apple host evidence.
- Modify `engine/agent/manifest.fragments/002-commands.json`, `004-modules.json`, `010-aiOperableProductionLoop.json`, `014-gameCodeGuidance.json`, then run `tools/compose-agent-manifest.ps1 -Write`.
- Modify `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/testing.md`, `docs/superpowers/plans/README.md`, `.agents/skills/rendering-change/references/full-guidance.md`, `.agents/skills/gameengine-rendering/references/full-guidance.md`, and `.claude/skills/gameengine-rendering/references/full-guidance.md`.

## Constraints

- Do not execute or launch arbitrary workload commands.
- Do not start/stop Metal capture sessions from default validation.
- Do not make `xctrace`, Xcode, Metal tools, or Apple hardware a default validation dependency.
- Do not write outside the selected evidence root; reject absolute paths, backslashes, drive-qualified paths, colons, `..`, and path escapes.
- Do not add Unity, Unreal Engine, Godot, middleware UI/renderer APIs, public native handles, or external-engine parity claims.
- Do not change `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` schema shape unless a validator task explicitly updates it.
- Keep `renderer_backend_parity_ready=0`, `renderer_metal_broad_readiness=0`, `renderer_commercial_readiness=0`, `renderer_broad_quality_ready=0`, and `renderer_environment_ready=0`.

## Task 1: RED Static Guard And Plan Registration

**Files:**
- Create: `tools/check-ai-integration-140-renderer-metal-memory-profiling-host-evidence-collector.ps1`
- Modify: `docs/superpowers/plans/README.md`

- [ ] Add this plan to the plan registry as an active implementation slice.
- [ ] Add a static guard that fails until `tools/collect-renderer-metal-memory-profiling-host-evidence.ps1`, `tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1`, `tools/validate.ps1`, manifest fragments, docs, and rendering guidance all mention `renderer-metal-memory-profiling-host-evidence-collector-v1`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
```

Expected: FAIL on missing collector/self-test/docs needles.

## Task 2: Collector Command

**Files:**
- Create: `tools/collect-renderer-metal-memory-profiling-host-evidence.ps1`

- [ ] Implement parameters:
  - `-Mode Plan|Import`
  - `-EvidenceRoot artifacts/renderer/metal-memory-profiling-host-evidence`
  - required `-WorkloadId`, `-CaptureArtifactRelativePath`, `-MacosVersion`, `-XcodeVersion`, `-HeapResourceRows`, `-HeapAllocatedBytes`, `-ResidentBytes`, `-BudgetBytes`, `-ResidencySetAllocationRows`, `-MemoryPressureSampleRows`, `-MemoryPressureBudgetStatus within_budget|pressure_observed`, `-CaptureScopeLabel`, `-CaptureArtifactRows`
  - optional `-NoWrite`
- [ ] Use `common.ps1` and `apple-host-helpers.ps1`; report host/tool availability using existing helpers plus `Find-CommandOnCombinedPath "xctrace"`.
- [ ] In `Plan` mode, print deterministic counters and do not write files:

```text
validation_recipe=renderer-metal-memory-profiling-host-evidence
renderer_metal_memory_profiling_host_evidence_collector_mode=Plan
renderer_metal_memory_profiling_host_evidence_collector_plan_ready=1
renderer_metal_memory_profiling_host_evidence_collector_writes_evidence=0
renderer_metal_memory_profiling_host_evidence_collector_broad_backend_parity=0
renderer_metal_memory_profiling_host_evidence_collector_broad_metal_readiness=0
renderer_metal_memory_profiling_host_evidence_collector_commercial_renderer=0
renderer_metal_memory_profiling_host_evidence_collector_broad_renderer_quality=0
```

- [ ] In `Import` mode, require `CaptureArtifactRelativePath` to resolve under `EvidenceRoot`, compute SHA-256, create the parent directory, and write `evidence.json` in the same directory.
- [ ] The written JSON must use the exact existing constants:
  - `schema_version = GameEngine.RendererMetalMemoryProfilingHostEvidence.v1`
  - `claim_id = renderer-metal-memory-profiling-host-evidence-v1`
  - `host.platform = macos`
  - source ids `Apple-Metal-MTLHeap-2026-06-24`, `Apple-Metal-MTLResidencySet-2026-06-24`, `Apple-Metal-MTLResidencySet-requestResidency-2026-06-24`, `Apple-Metal-MTLCommandQueue-addResidencySet-2026-06-24`, `Apple-Metal-MTLCaptureManager-2026-06-24`, `Apple-Metal-ProgrammaticCapture-2026-06-24`
  - `memory_residency_row.proof_row_id = memory_residency`
  - `profiling_capture_row.proof_row_id = profiling_capture`
  - all `non_claims` booleans false
- [ ] In `Import` mode, emit:

```text
renderer_metal_memory_profiling_host_evidence_collector_mode=Import
renderer_metal_memory_profiling_host_evidence_collector_written=1
renderer_metal_memory_profiling_host_evidence_collector_capture_artifact_hash=<sha256>
renderer_metal_memory_profiling_host_evidence_collector_native_handles_exposed=0
renderer_metal_memory_profiling_host_evidence_collector_external_engine_api_parity=0
```

## Task 3: Collector Self-Test

**Files:**
- Create: `tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1`

- [ ] Create ignored synthetic artifacts under `out/renderer-metal-memory-profiling-host-evidence-collector-contract/$PID/sample_desktop_runtime_game/`.
- [ ] Run collector `Plan` mode with `-NoWrite` and assert the expected no-write and no-broad-claim lines.
- [ ] Run collector `Import` mode against a synthetic `capture-summary.txt` artifact and assert `written=1`, native handles `0`, external engine parity `0`, and a 64-character lower-case SHA-256.
- [ ] Read the generated `evidence.json` and assert `schema_version`, `claim_id`, `memory_residency_row.proof_row_id`, `profiling_capture_row.proof_row_id`, `capture_artifact_path`, and `capture_artifact_hash_sha256`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ArtifactRootRelative out/renderer-metal-memory-profiling-host-evidence-collector-contract/<pid> -RequireReady -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=ready renderer_metal_memory_profiling_ready=1 renderer_backend_parity_ready=0 renderer_metal_broad_readiness=0 renderer_commercial_readiness=0 renderer_broad_quality_ready=0
```

Expected: PASS.

## Task 4: CI And Validation Wiring

**Files:**
- Modify: `tools/validate.ps1`
- Modify: `tools/classify-pr-validation-tier.ps1`
- Modify: `tools/check-ci-matrix.ps1`

- [ ] Add `check-renderer-metal-memory-profiling-host-evidence-collector.ps1` to static validation.
- [ ] Add `collect-renderer-metal-memory-profiling-host-evidence`, `check-renderer-metal-memory-profiling-host-evidence`, and `check-renderer-metal-memory-profiling-host-evidence-collector` to the Apple host evidence classifier path list.
- [ ] Extend `tools/check-ci-matrix.ps1` with a representative `ChangedPath @("tools/collect-renderer-metal-memory-profiling-host-evidence.ps1")` case that expects `metal_host_evidence=true` and `ios_metal_evidence=true`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1
```

Expected: both PASS.

## Task 5: Docs, Manifest, And Skills

**Files:**
- Modify: `engine/agent/manifest.fragments/002-commands.json`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `.agents/skills/rendering-change/references/full-guidance.md`
- Modify: `.agents/skills/gameengine-rendering/references/full-guidance.md`
- Modify: `.claude/skills/gameengine-rendering/references/full-guidance.md`

- [ ] Add manifest command `rendererMetalMemoryProfilingHostEvidenceCollectorCheck`.
- [ ] Document that the collector shapes host-owned Apple artifacts into validator-ready evidence; it does not run capture, infer readiness, or claim broad parity.
- [ ] Record default validation behavior: no checked-in real artifacts means `renderer_metal_memory_profiling_status=host_evidence_required` and `renderer_metal_memory_profiling_ready=0`.
- [ ] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
```

Expected: all PASS.

## Task 6: Final Validation And Publication

**Files:**
- Modify this plan with final validation evidence and PR/merge evidence.

- [ ] Run focused checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

- [ ] Run full slice gate:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Run publication preflight:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1
```

- [ ] Commit only task-owned files, push `codex/renderer-metal-memory-profiling-host-evidence-collector`, open a draft PR, wait for hosted checks, mark ready through `tools/ready-task-pr.ps1`, and merge only after required gates pass.

## Done When

- The collector self-test proves `Plan` and `Import` behavior and validates generated `evidence.json` through the existing renderer Metal memory/profiling validator.
- `tools/validate.ps1` includes the collector guard.
- CI classifier selects Apple host evidence for collector/validator changes.
- Docs, manifest, plan registry, and rendering skills describe the collector and preserve non-claims.
- Full validation passes, PR checks pass, and the plan records final publication evidence.

## Validation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` before collector implementation | Failed as expected on missing `tools/collect-renderer-metal-memory-profiling-host-evidence.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence-collector.ps1` | PASS: Plan/Import collector self-test wrote ignored `out/` evidence, validated it through `tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -SkipFocusedRendererBuild -RequireReady`, and preserved broad readiness counters at `0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0` | PASS: focused renderer build and `MK_renderer_tests` passed; command found no default retained Apple-host artifacts. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-renderer-metal-memory-profiling-host-evidence.ps1 -SkipFocusedRendererBuild -ArtifactRootRelative artifacts/renderer/metal-memory-profiling-host-evidence -ExpectedEvidenceCounters renderer_metal_memory_profiling_status=host_evidence_required renderer_metal_memory_profiling_ready=0` | PASS: default artifact root remained `host_evidence_required` and `renderer_metal_memory_profiling_ready=0`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1` | PASS: collector changes select Apple host evidence lanes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS: regenerated `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS. |
| `git diff --check` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`, static checks passed, and CTest reported 157/157 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1` | PASS before commit and PASS before push. |
| `git commit -m "Add renderer Metal memory profiling collector"` | PASS: commit `8a99f219`. |
| `git push -u origin codex/renderer-metal-memory-profiling-host-evidence-collector` | PASS. |
| `gh pr create --draft --base main --head codex/renderer-metal-memory-profiling-host-evidence-collector` | PASS: opened draft PR #808. |
