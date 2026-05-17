# Frame Graph Package Streaming Texture Binding Handoff v1 Implementation Plan (2026-05-17)

**Plan ID:** `frame-graph-package-streaming-texture-binding-handoff-v1`
**Status:** Completed.
**Owner:** Runtime RHI / Frame Graph
**Parent:** `frame-graph-v1` foundation follow-up in `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`

## Goal

Add a narrow public `MK_runtime_rhi` bridge that turns caller-owned, already-uploaded runtime texture rows from a committed package streaming safe point into `FrameGraphTextureBinding` rows for imported Frame Graph resources.

## Context

- Package streaming already commits resident package state and refreshes a live `RuntimeResourceCatalogV2`.
- `upload_runtime_texture` already returns shader-readable `RuntimeTextureUploadResult` rows for host-owned uploads.
- `execute_frame_graph_rhi_texture_schedule` already accepts imported `FrameGraphTextureBinding` rows.
- This slice connects those surfaces without adding background streaming, upload staging rings, GPU residency budgets, renderer ownership, native handles, multi-queue scheduling, Vulkan/Metal memory alias allocation, public wildcard/null barriers, or data inheritance/content preservation.

## Constraints

- Use TDD: add focused failing tests before production code.
- Keep the bridge backend-neutral and value-based; do not expose `IRhiDevice`, native handles, or package internals to game code.
- Validate only committed package streaming results, live resident-catalog texture assets, non-empty frame-graph resource names, unique resource names, and successful non-null texture uploads.
- Keep docs, manifest fragments, plan registry, and affected agent surfaces synchronized.

## Done When

- `MK_runtime_rhi` exposes a focused package streaming texture handoff API.
- Unit tests cover success into an imported Frame Graph execution and rejection of non-committed results, missing/stale catalog assets, failed or empty uploads, and duplicate frame-graph resource names.
- Master plan, registry, manifest fragments plus composed manifest, and relevant docs record the new narrow ready surface and remaining unsupported claims.
- Focused checks and the slice-closing validation evidence are recorded here before the plan is marked complete.

## Execution Checklist

- [x] Add RED `MK_runtime_rhi_tests` coverage for package streaming texture handoff.
- [x] Implement the minimal public header/source and CMake target update.
- [x] Run focused runtime/renderer test loops.
- [x] Synchronize docs, manifest fragments, composed manifest, and static agent checks.
- [x] Run slice-closing validation and record evidence.
- [x] Mark this plan complete and restore `currentActivePlan` to the master plan with the next selection pointer.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Failed as expected (RED) | New test include failed before `mirakana/runtime_rhi/package_streaming_frame_graph.hpp` existed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_rhi_tests` | Passed | Focused runtime_rhi test target built after implementation. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_rhi_tests` | Passed | `MK_runtime_rhi_tests` passed, including imported Frame Graph execution and rejection diagnostics. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` | Passed | C++ and tracked text formatting normalized. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1` | Passed | Public header boundary remains valid for the new `MK_runtime_rhi` API. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Passed | Codex/Claude rendering guidance and auditor surface updates remain within agent budgets. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Passed | Agent/manifest guidance includes the narrow package streaming texture-binding handoff and unsupported broad claims. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Passed | Manifest compose and plan pointer contracts pass after closeout. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | Passed | Full slice gate passed; host-gated Apple/Metal diagnostics remain non-fatal, and CTest reported 65/65 tests passed. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/build.ps1` | Passed | Standalone build gate passed before publishing the branch. |
