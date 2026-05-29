# Renderer Production Quality And Backend Parity v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote renderer production quality from selected evidence rows into backend-local D3D12, strict Vulkan, and Apple-host-gated Metal production gates with broad profiling, residency, frame-graph, shader, package, and non-claim evidence.

**Architecture:** Keep renderer, RHI, runtime upload, scene rendering, and package smoke contracts backend-neutral at public boundaries while each backend proves synchronization, shader validation, residency, timing, and package evidence independently. Use clean breaking changes when existing value rows or package counters are too narrow; update all callers, tests, manifests, docs, and static checks in the same slice.

**Tech Stack:** C++23, `MK_renderer`, `MK_rhi`, `MK_runtime_rhi`, `MK_runtime_scene_rhi`, `MK_scene_renderer`, generated desktop runtime package samples, CMake/CTest, PowerShell validation wrappers, D3D12 resource barriers/fences/residency, Vulkan synchronization/validation/SPIR-V tools, Apple Metal resource synchronization/capability host gates, and first-party diagnostics/profile capture.

---

**Plan ID:** `renderer-production-quality-backend-parity-v1`

**Status:** Active.

Implementation/docs/static checks are closeout-ready, but publication remains blocked by the local Git worktree ACL until staging, commit, push, and PR creation can use the normal index path.

Selected child plan of `clean-break-broad-production-readiness-master-plan-v1`.

**Date:** 2026-05-27

## Execution Discipline

- This active child plan remains the selected `currentActivePlan` until closeout. Do not change `currentActivePlan`, `recommendedNextPlan`, or `unsupportedProductionGaps` during Tasks 1-5; update those pointers only in Task 6 closeout after implemented evidence, docs, manifest fragments, composed manifest, and validation agree.
- Execute implementation in a linked worktree. Before configure inside that worktree, run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1`, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev`.
- Candidate checkpoints are review boundaries, not plan boundaries: Task 1 is an audit-only/no-production-code checkpoint; Tasks 2-3 are the renderer quality contract/test checkpoint; Task 4 is the profiling/residency checkpoint; Tasks 5-6 are the package/docs/manifest closeout checkpoint. Create validated commits and PRs at those boundaries when the candidate is independently reviewable; group tightly coupled checkpoints in one PR with small validated commits.
- Do not resolve validation blockers by bootstrapping or adding SDL3. Keep the default `dev` lane dependency-free and route remaining legacy desktop runtime/editor/audio removal to the first-party native desktop replacement plan.

## Context

Existing renderer evidence is useful but still narrow:

- `renderer-general-quality-matrix-v1` records backend-local selected D3D12/Vulkan rows and Metal host-gated rows.
- `production-rendering-vfx-profiling-v1`, `renderer-gpu-memory-v1`, `renderer-debug-profiling-v1`, and `renderer-backend-parity-v1` have selected counters and package evidence.
- Current docs still correctly reject broad production renderer quality, full backend parity, and broad profiling claims.

This child plan is the first clean-break implementation candidate for the broad production readiness master plan. It does not reopen Engine 1.0; it strengthens post-1.0 renderer breadth.

## Official Practice Check

Before code changes in each task, re-check and record the exact docs touched by that task:

- D3D12 resource barriers: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12>
- D3D12 multi-engine synchronization: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>
- D3D12 residency: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/residency>
- Vulkan synchronization guide: <https://docs.vulkan.org/guide/latest/synchronization.html>
- Vulkan validation overview: <https://docs.vulkan.org/guide/latest/validation_overview.html>
- Vulkan memory allocation guide: <https://docs.vulkan.org/guide/latest/memory_allocation.html>
- Apple Metal resource synchronization: <https://developer.apple.com/documentation/metal/resource-synchronization>
- Apple Metal capabilities: <https://developer.apple.com/metal/capabilities/>

2026-05-29 Task 1 re-check:

- Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12`: resource barriers, multi-engine fence waits/signals, and residency evidence must remain backend-local D3D12 proof.
- Context7 `/khronosgroup/vulkan-docs`: synchronization2 barriers, image layout transitions, queue-family ownership, and validation/SPIR-V evidence must remain strict Vulkan proof.
- Apple Metal resource synchronization and capabilities remain Apple-host-gated official anchors; do not promote Metal readiness from Windows, D3D12, or Vulkan evidence.

## Constraints

- No backend-native handles in public gameplay, scene, material, UI, or package APIs.
- D3D12, Vulkan, and Metal evidence is independent. No backend inherits another backend's readiness.
- Metal remains host-gated until macOS/Xcode/Metal validation lands.
- Performance claims require deterministic timing/profile rows, budget diagnostics, and package-visible counters. Do not claim measured frame-rate parity from synthetic or value-only rows.
- Renderer package counters must distinguish ready, host-gated, dependency-gated, and unsupported rows.
- Any public aggregate changes must update designated initializers in tests in declaration order.

## Files

- Modify: `engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp`
- Modify: `engine/renderer/src/renderer_quality_matrix.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`
- Modify: `engine/renderer/src/backend_renderer_parity_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp`
- Modify: `engine/renderer/src/debug_profiling_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp`
- Modify: `engine/renderer/src/gpu_memory_policy.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- Modify: `engine/renderer/src/production_vfx_profiling.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp`
- Modify: `engine/renderer/src/frame_graph_rhi.cpp`
- Modify: `engine/rhi/include/mirakana/rhi/rhi.hpp`
- Modify as discovered: backend implementation files under `engine/rhi/`, `platform/`, or backend-specific renderer folders.
- Modify: `tests/unit/renderer_quality_matrix_tests.cpp`
- Modify: `tests/unit/renderer_rhi_tests.cpp`
- Modify: `tests/unit/renderer_production_vfx_profiling_tests.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify as needed: `tools/check-ai-integration*.ps1`, `tools/check-json-contracts*.ps1`
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Modify: `engine/agent/manifest.fragments/*.json`
- Generate: `engine/agent/manifest.json`

## Task 1 - Baseline Renderer Evidence Audit

- [x] Read the current renderer/RHI/package tests and identify which broad production claims are still unsupported.
- [x] Add a short evidence table to this plan with columns `claim/feature`, `backend`, `evidence category`, `current status`, `proof source`, and `missing gate/blocker`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/prepare-worktree.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_quality_matrix_tests MK_renderer_production_vfx_profiling_tests MK_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_quality_matrix|renderer_production_vfx_profiling|MK_renderer_tests"
```

Expected: current baseline passes or records exact pre-existing tool/host blocker before implementation.

### Task 1 Baseline Evidence Table

| claim/feature | backend | evidence category | current status | proof source | missing gate/blocker |
| --- | --- | --- | --- | --- | --- |
| Renderer quality matrix selected rows | D3D12/Vulkan/Metal | Backend production evidence taxonomy | D3D12 and strict Vulkan selected rows are ready; Metal is host-gated; broad general renderer quality remains unclaimed. | `tests/unit/renderer_quality_matrix_tests.cpp`; `RendererQualityMatrixPlan` reports 21 rows, 14 ready, 7 host-gated; `cpp23-eval` build/CTest passed `MK_renderer_quality_matrix_tests`. | Row taxonomy still lacks first-class `dependency-gated` and `unsupported` row statuses, so package counters cannot yet distinguish every broad readiness outcome required by this plan. |
| Renderer backend parity | D3D12/Vulkan/Metal | Backend-local synchronization, shader/tool validation, residency, package evidence | Partial selected-row proof only; backend-local evidence is represented by booleans and selected package readiness, while the standalone parity helper only checks selected backend equals proof backend. | `engine/renderer/include/mirakana/renderer/renderer_quality_matrix.hpp`; `engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp`; `cpp23-eval` build/CTest passed `MK_renderer_tests`. | Must reject inferred parity through explicit category completeness per backend, including synchronization, shader/tool validation, backend validation, memory/residency, package counters, and host gate evidence. |
| Production VFX/profiling backend evidence | D3D12/Vulkan/Metal | CPU/GPU profiling, budgets, package counters, capture handoff | D3D12 and strict Vulkan backend evidence rows are ready; Metal is host-gated; broad VFX/profiling readiness remains unclaimed. | `tests/unit/renderer_production_vfx_profiling_tests.cpp`; `RendererProductionVfxProfilingPlan` reports 3 backend evidence rows, 2 ready, 1 host-gated; `cpp23-eval` build/CTest passed `MK_renderer_production_vfx_profiling_tests`. | Must add package-visible CPU profile/budget/counter rows and trace/capture handoff evidence before any broad profiling readiness claim. |
| Gameplay-facing renderer evidence notes | Public package/gameplay surface | Backend-neutral API hygiene | Native-handle requests and native tokens in ids/counter ids are rejected; no gameplay-facing row notes field exists yet. | `tests/unit/renderer_quality_matrix_tests.cpp`; `RendererQualityMatrixDiagnosticCode::unsupported_native_handle_claim`; `RendererQualityMatrixDiagnosticCode::invalid_quality_row`. | Add a first-party notes/evidence text surface only if needed, and reject backend/platform-native token strings without exposing implementation handles. |
| Baseline validation lane | Linked worktree | Toolchain/configure/build/test evidence | `check-toolchain` and `prepare-worktree` passed; initial default `dev` configure exposed stale legacy desktop dependency inheritance; dependency-free `cpp23-eval` focused renderer build/CTest passed; `dev --fresh` passed after removing that inheritance. | `tools/check-toolchain.ps1`: ok, linked-worktree=true; `tools/prepare-worktree.ps1`: ok; initial `tools/cmake.ps1 --preset dev`: missing legacy desktop middleware package config; `tools/cmake.ps1 --preset cpp23-eval`, build, and CTest passed 3/3; `tools/cmake.ps1 --fresh --preset dev` passed after default `dev` was made dependency-free. | Default `dev` must stay SDL3-free. Remaining optional SDL3 lanes are legacy replacement/removal work, not future renderer readiness evidence. |

## Task 2 - RED Tests For Production Quality Gate Expansion

- [x] Add tests that fail until renderer quality rows distinguish production-ready, host-gated, dependency-gated, and unsupported status per backend and feature.
- [x] Add tests that reject inferred backend parity when a Vulkan or Metal row is missing backend-local synchronization/shader/validation evidence.
- [x] Add tests that reject broad performance profiling readiness without CPU profile rows, GPU timestamp availability rows, budget rows, package counters, and trace/capture handoff rows.
- [x] Add tests that reject public native-handle leakage and backend-native token strings in gameplay-facing row notes.
- [x] Run the focused tests and record the expected RED failures in this plan.

2026-05-29 RED evidence:

| test surface | expected failure before implementation | command/evidence |
| --- | --- | --- |
| Renderer quality matrix row taxonomy | Missing `RendererQualityRowStatus`, `RendererQualityEvidenceCategory`, dependency/unsupported proof kinds, row ids/notes, and plan counts. | `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset cpp23-eval --target MK_renderer_quality_matrix_tests MK_renderer_production_vfx_profiling_tests` failed as expected before implementation. |
| Production VFX/profile evidence | Missing `RendererProductionCpuProfileRow`, `RendererProductionPackageCounterRow`, request/plan vectors, counts, and missing-row diagnostics. | Same RED build caught the absent public contract before implementation. |

## Task 3 - Backend-Local Renderer Quality Contracts

- [x] Extend renderer quality value rows with explicit evidence categories: synchronization, shader/tool validation, memory/residency, render-pass/frame-graph behavior, profiling, package evidence, host gate, dependency gate, unsupported claim.
- [x] Implement fail-closed diagnostics for missing categories, duplicate feature/backend rows, unsupported broad claims, backend inference, and native handle leakage.
- [x] Keep any aggregate changes clean-break and update all designated initializers in tests.
- [x] Run focused renderer quality tests and record GREEN evidence.

2026-05-29 GREEN evidence:

| command | result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_quality_matrix_tests MK_renderer_production_vfx_profiling_tests MK_renderer_tests` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_quality_matrix|renderer_production_vfx_profiling|MK_renderer_tests"` | Passed 3/3. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |

Full `tools/validate.ps1` note: two attempts timed out at the outer command limit after static checks had started (`out/validation-logs/validate-20260529-125536-16776` at 900s and `out/validation-logs/validate-20260529-131116-9440` at 1800s). The closeout pass later traced the hang to validation wrapper child-process/stream waits and local Android device probing, fixed both, and completed full validation.

## Task 4 - Profiling And Residency Evidence Expansion

- [x] Extend debug profiling and GPU memory policy rows so broad profiling requires CPU zones, GPU timestamp capability, memory budget, residency pressure, package counter, and trace/capture handoff evidence.
- [x] Keep actual PIX/RenderDoc/Xcode capture execution host/operator-gated; the engine may publish request/review rows only.
- [x] Add package-visible counters to selected desktop runtime samples without a single broad `renderer_ready` flag.
- [x] Run focused profiling, memory, and package tests.

2026-05-29 GREEN evidence:

| command | result |
| --- | --- |
| Context7 `/websites/learn_microsoft_en-us_windows_win32_direct3d12`, `/khronosgroup/vulkan-docs`, and `/websites/developer_apple` lookups | Official guidance confirmed GPU timestamp/counter APIs, D3D12 video-memory budget/residency responsibility, Vulkan performance/debug/memory evidence, and Metal counter/timestamp capture evidence. Capture execution remains host/operator-gated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed after RED failure and implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests"` | Passed 1/1. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_host_sdl3_tests` | Target absent because default `dev` is intentionally SDL3-free; no SDL3 bootstrap or legacy lane promotion was performed. Package-visible counters were guarded through source/static validation instead. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; `engine/agent/manifest.json` regenerated. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed after `tools/format.ps1`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `git diff --check` | Passed. |

## Task 5 - Package And Validation Recipe Promotion

- [x] Update generated 3D package smoke and installed validation to require exact renderer production quality fields.
- [x] Update validation recipes and manifest rows only for evidence implemented in Tasks 2-4.
- [x] Keep Metal Apple-host-gated and keep strict Vulkan validation/toolchain-gated where local host evidence is absent.
- [x] Run package and installed validation.

2026-05-29 package/recipe evidence:

| command | result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after synchronizing `sample_desktop_runtime_game/game.agent.json`, `sample_generated_desktop_runtime_3d_package/game.agent.json`, validation-script expected fields, and composed manifest JSON. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed; static package recipe checks cover generated package manifest rows, sample stdout field names, installed validation script needles, and SDL3-free default dev policy. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| Installed runtime smoke execution | Not run in this slice because default `dev` is intentionally SDL3-free and no installed desktop runtime package was produced. The validation script now checks the exact new fields when a legacy installed package lane is explicitly built. |

## Task 6 - Docs, Manifest, Static Checks, And Closeout

- [x] Update current capabilities, roadmap, plan registry, backlog, projection chapter, manifest fragments, schema/static checks if literals changed, and generated-game guidance.
- [x] Compose the manifest.
- [x] Run final publication validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
git diff --check
```

Expected: all checks pass, or host-gated blockers are recorded with exact command output. At closeout, return `currentActivePlan` to the master plan or select the next child plan.

Closeout state on 2026-05-29: implementation, docs, manifest composition, focused renderer tests, static checks, full validation, and diff hygiene are ready. Final publication is still blocked by local Git worktree metadata ACL, so this child plan remains `currentActivePlan`.

| command | result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` | Passed after the RED phase for the new renderer profiling/residency evidence rows. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests"` | Passed, 1/1 test. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Passed; wrote `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed after preserving the machine-readable `**Status:** Active.` line. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed after preserving the machine-readable `**Status:** Active.` line. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | Passed. |
| `git diff --check` | Passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-publication-preflight.ps1 -Branch codex/clean-break-broad-production-readiness-selection-v1` | Passed after switching this session to full access: linked-worktree Git admin write ready, remote ready, and `gh auth` ready. Earlier restricted-session failures are retained below as blocker audit evidence. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1 -Jobs 2` | Initially failed with MSVC `C1041`/`C1083` on long runtime package reviewed-eviction test object/PDB paths in the long linked worktree; passed after central CMake MSVC PDB/object-path fixes. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 -StaticOnly -StaticJobs 1 -StaticCheckTimeoutSeconds 120` | Passed after `validate.ps1` bounded child-process exit/stream drains and defaulted `MK_MOBILE_DEVICE_PROBE=0` for the diagnostic mobile check. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed again after publication preflight/tooling updates in the full-access session: static checks ok, build ok, `check-generated-msvc-cxx23-mode.ps1` ok, tidy smoke ok, and CTest passed 76/76. |
| `git add --dry-run -- .` | Blocked: `fatal: Unable to create 'G:/workspace/development/GameEngine/.git/worktrees/clean-break-broad-production-readiness-selection-v1/index.lock': Permission denied`. Normal staging, commit, push, and PR creation are blocked until the local worktree Git ACL is fixed. |
| `git ls-remote --heads origin codex/clean-break-broad-production-readiness-selection-v1` | Blocked in this session: `Failed to connect to github.com port 443`. Push/PR publication also requires remote network access after local Git metadata writes are restored. |

Publication blocker audit:

- `git rev-parse --git-dir --git-common-dir --show-toplevel` resolves this linked worktree to `G:/workspace/development/GameEngine/.git/worktrees/clean-break-broad-production-readiness-selection-v1`, common dir `G:/workspace/development/GameEngine/.git`, and worktree root `G:/workspace/development/GameEngine/.worktrees/clean-break-broad-production-readiness-selection-v1`.
- `Test-Path` confirmed `index.lock` does not already exist; the failure is creation permission, not a stale lock file.
- Current process user is `desktop\codexsandboxoffline` (`S-1-5-21-3638488807-419599994-971196476-1004`) with `Desktop\CodexSandboxUsers`, `BUILTIN\Users`, and `NT AUTHORITY\Authenticated Users` groups.
- `icacls` on the worktree Git admin dir shows inherited deny ACEs for SIDs `S-1-5-21-4057373999-1959094108-4028022297-3471116296` and `S-1-5-21-3922207173-4045917315-1326731370-4175570034`, plus allow ACEs for `Desktop\CodexSandboxUsers` and `Authenticated Users`; direct write tests to both the linked worktree Git admin dir and the common `.git` dir still fail with access denied.
- Do not bypass this with GitHub API object writes or direct default-branch writes. Finish publication in a trusted/full-access local session by staging these task-owned files, committing the validated candidate, pushing the topic branch, and opening a reviewable PR with the validation evidence above.
- Remote preflight also needs GitHub network access; this sandbox currently cannot reach `github.com:443`.
- `tools/check-publication-preflight.ps1` now makes this blocker explicit before future publishable work starts, but it cannot grant OS ACL, network, or host GitHub auth access from inside a restricted session.
- After the session switched to full access, `tools/check-publication-preflight.ps1 -Branch codex/clean-break-broad-production-readiness-selection-v1` passed and publication is unblocked for normal GitHub Flow.

Remaining publication step:

- [x] Publication preflight passed after full-access session switch.
- [ ] Stage, commit, push, and create a reviewable PR.

## Done When

- The renderer has backend-local production quality evidence gates across D3D12, strict Vulkan, and Apple-host-gated Metal.
- Broad renderer readiness remains unclaimed for any missing host/backend lane.
- Profiling and residency rows are package-visible and fail closed on missing evidence.
- Docs, manifest, schemas/static checks, generated-game guidance, and validation recipes match the implemented scope.
- A validated commit and reviewable PR exist for this candidate before moving to the next child plan.
