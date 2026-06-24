# Renderer Backend Parity Apple Memory Profiling Proof Rows v1 Implementation Plan

**Status:** Completed through PR #805 / merge commit `58a2017002895ba4d06ba7bc48675af7af3841f4`.

**Date:** 2026-06-24

## Goal

Add a first-party, backend-neutral renderer policy boundary for the remaining `renderer-backend-parity-v1` Apple/Metal proof rows: `memory_residency` and `profiling_capture`.

This plan does not claim broad backend parity, broad Metal readiness, commercial renderer readiness, public native handles, or broad renderer quality. It adds exact proof row contracts that remain host-gated unless reviewed Apple-host evidence explicitly supplies the required counters.

Follow-up retained artifact contract: [Renderer Metal Memory Profiling Host Evidence v1](2026-06-24-renderer-metal-memory-profiling-host-evidence-v1.md) adds `renderer-metal-memory-profiling-host-evidence-v1`, retained `GameEngine.RendererMetalMemoryProfilingHostEvidence.v1` rows, and `tools/check-renderer-metal-memory-profiling-host-evidence.ps1` so exact Apple-host `MTLHeap`, `MTLResidencySet`, `MTLCaptureManager`, `MTLCaptureScope`, `memory_residency`, `profiling_capture`, and capture artifact evidence can be validated separately. Default validation remains `renderer_metal_memory_profiling_status=host_evidence_required` and `renderer_metal_memory_profiling_ready=0`; broad backend parity, broad Metal readiness, commercial renderer readiness, and broad renderer quality remain unclaimed.

## Official Source Refresh

- Apple Developer Documentation, `MTLHeap`: Metal resources can be suballocated from heaps.
- Apple Developer Documentation, resource fundamentals: resources are allocated from `MTLDevice` or `MTLHeap`, with hazard-tracking behavior.
- Apple Developer Documentation, Metal residency sets: `MTLResidencySet` is the explicit residency mechanism for resource allocations.
- Apple Developer Documentation, `MTLCaptureManager`: programmatic Metal frame capture is driven through the capture manager and capture descriptors/scopes.
- Context7 Metal Shading Language Specification: shader validation evidence remains constrained to Metal shader functions, metallib artifacts, and Metal language restrictions; this slice does not broaden shader claims.

## Architecture

- Keep `MK_renderer` backend-neutral: expose only first-party value rows, booleans, ids, and counters.
- Keep Apple SDK, Objective-C++, `MTL*`, Xcode, and capture implementation details behind `MK_rhi_metal` or reviewed host validation recipes.
- Reuse the reviewed `renderer-metal-apple-host-evidence` recipe id, but do not treat its existing environment counters as memory/profiling evidence.
- Add a separate helper from the existing environment helper so `synchronization`, `shader_validation`, and `package_evidence` cannot imply `memory_residency` or `profiling_capture`.

## Tasks

- [x] Add RED tests for ready, partial host-gated, and native-handle-rejected Apple memory/profiling proof rows.
- [x] Add `BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc` and `make_backend_renderer_parity_apple_metal_memory_profiling_proofs`.
- [x] Update docs, skills, manifest fragments, and static checks to name the new helper and preserve non-claims.
- [x] Run focused renderer validation and agent-surface checks.
- [x] Run the required publication flow for this reviewable slice.

## Implementation Evidence

| Command | Result |
| --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` | PASS: configured the linked worktree `dev` preset. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` before implementation | Failed as expected on missing `BackendRendererParityAppleMetalMemoryProfilingEvidenceDesc` and `make_backend_renderer_parity_apple_metal_memory_profiling_proofs`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tests` after implementation | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_renderer_tests"` | PASS: `MK_renderer_tests` passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | PASS: regenerated `engine/agent/manifest.json`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS: `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS: `first-party-ui-clean-room: ok`; `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | PASS: `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS: `text-format-check: ok`; `format-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1` | PASS: `text-format-check: ok`. |
| `git diff --check` | PASS. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS: `validate: ok`; 157/157 CTest tests passed. Local Windows host lacks Apple Metal tools, and the Apple evidence checks remained diagnostic/host-gated as expected. |
| `gh pr checks 805` | PASS: PR Gate, Windows MSVC, Linux CMake, Linux Vulkan Host Evidence, Linux Coverage, Linux Clang ASan/UBSan, Full Repository Static Analysis shards, macOS Metal CMake, iOS Metal Evidence, iOS Simulator smoke, CodeQL, and selected validation tier passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ready-task-pr.ps1 -PullRequest 805 -ExpectedBase main` | PASS: PR #805 moved from draft to ready at head `7c2f30c544cf66fd2b029bc1102d7d4ca9b8f9cc`. |
| `gh pr merge 805 --auto --merge --match-head-commit 7c2f30c544cf66fd2b029bc1102d7d4ca9b8f9cc` | PASS: PR #805 merged to `origin/main` as merge commit `58a2017002895ba4d06ba7bc48675af7af3841f4`. |
| `git merge-base --is-ancestor 7c2f30c544cf66fd2b029bc1102d7d4ca9b8f9cc origin/main` | PASS: the reviewed head reaches `origin/main`. |

## Done When

- The helper returns exactly two rows: `memory_residency` and `profiling_capture`.
- Each row is independently ready only when its exact Apple-host evidence booleans and reviewed recipe id are present.
- Missing memory or profiling evidence yields host-gated rows, not broad Metal/backend readiness.
- Native-handle requests return no proof rows.
- `unsupportedProductionGaps` remains `[]`, and `renderer-backend-parity-v1` remains host-gated until actual Apple-host proof rows are validated.
