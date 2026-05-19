# Frame Graph v1 1.0 Scope Closeout v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Close `frame-graph-v1` for the Engine 1.0 Windows-default ready surface without claiming a broad production render graph, async overlap/performance, Metal memory alias allocation, public native handles, or actual content preservation.

**Architecture:** `MK_renderer` owns the Frame Graph v1 planning and RHI execution foundation for selected renderer/runtime upload paths. D3D12 and Vulkan now have backend-private transient alias allocation evidence, selected desktop package smokes expose render-pass and multi-queue counters, and the production ownership planner fails closed for broader graph migration claims that remain future or host-gated.

**Tech Stack:** C++23 renderer/RHI/runtime contracts, `engine/agent/manifest.fragments`, `tools/check-ai-integration*.ps1`, `tools/check-json-contracts*.ps1`, package validation scripts, and docs/roadmap/AI game-development guidance.

---

**Plan ID:** `frame-graph-v1-1-0-scope-closeout-v1`

**Status:** Completed.

**Parent:** [../master-plans/2026-05-03-production-completion-master-plan-v1.md](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

**Gap:** `frame-graph-v1`

## Context

- The `frame-graph-v1` burn-down now has selected execution evidence for Frame Graph v1 planning, callback dispatch, texture transitions, pass target-state preparation, final-state restoration, render-pass envelopes, queue dependency waits, multi-queue command submission, automatic aliasing barriers, package-visible render-pass and multi-queue counters, runtime texture/mesh/skinned/morph/material-factor upload command evidence, package-streaming texture binding handoff/transaction bridges, D3D12 placed alias resources, Vulkan alias memory, and Frame Graph Production Ownership Boundary Selection v1 (`FrameGraphProductionOwnershipPlan` / `plan_frame_graph_production_ownership_boundary`) fail-closed production ownership boundary planning.
- The latest `FrameGraphRhiMultiQueuePackageEvidence` package smoke validates `sample_desktop_runtime_game --require-framegraph-multiqueue-evidence` with four submitted command lists, three queue waits, four texture barriers, one aliasing barrier, and four submitted pass fences over transient alias-group lease bindings.
- Remaining broad claims are no longer required blockers for the Engine 1.0 Windows-default ready surface: broad production render graph scheduling, broad/background package streaming, Metal memory alias allocation, async overlap/performance, actual content preservation across aliases, public native handles, and general renderer quality stay explicit future or host-gated work.

## Constraints

- Do not change C++ behavior in this closeout slice.
- Do not claim broad production graph ownership beyond the selected renderer/runtime upload and package evidence already validated.
- Do not claim Metal readiness, data inheritance/content preservation, async overlap/performance, public native/RHI handles, or broad renderer quality.
- Keep `upload-staging-v1` as the next Phase 1 foundation gap and keep package streaming/upload staging limits explicit.
- Keep `engine/agent/manifest.json` composed output only; edit fragments and run `tools/compose-agent-manifest.ps1 -Write`.

## Done When

- `frame-graph-v1` is removed from `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json.unsupportedProductionGaps`, and the composed manifest is regenerated.
- `recommendedNextPlan` records this closeout and points the next Phase 1 foundation work at `upload-staging-v1`.
- Plan registry, master plan, roadmap, current capabilities, architecture, AI game-development guidance, rendering skills, and static guards agree that Frame Graph v1 is closed for Engine 1.0 Windows-default scope.
- Static validation proves the manifest is valid and the production readiness audit lists the remaining unsupported rows.

## Tasks

- [x] **Task 1: Close the machine-readable gap row.**
  Remove `frame-graph-v1` from `unsupportedProductionGaps`, update `recommendedNextPlan` to select `upload-staging-v1`, and regenerate the composed manifest.

- [x] **Task 2: Synchronize current-truth docs and skills.**
  Update this registry, the master plan, roadmap, current capabilities, architecture, AI game-development guidance, and Codex/Claude rendering guidance so the closeout and future/host-gated exclusions are explicit.

- [x] **Task 3: Update static guards.**
  Change manifest/static guard checks so stale `frame-graph-v1` unsupported rows fail validation and `upload-staging-v1` becomes the next selected Phase 1 gap.

- [x] **Task 4: Validate and publish.**
  Run focused manifest/agent/static checks, commit the validated closeout, push the existing PR branch, and re-check PR status.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write` | Pass | Regenerated `engine/agent/manifest.json` from fragments. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | Pass | `agent-manifest-compose: ok`; `json-contract-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1` | Pass | `agent-config-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | Pass | `ai-integration-check: ok`. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-production-readiness-audit.ps1` | Pass | `unsupported_gaps=7`; no `frame-graph-v1` row. |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | Pass | `text-format-check: ok`; `format-check: ok`. |

## Agent Surface Drift Review

- Checked. Durable closeout behavior changed the machine-readable production gap contract, so docs, plan registry, roadmap, current capabilities, AI game-development guidance, Codex/Claude rendering guidance, manifest fragments, composed manifest, and static guards were updated in the same slice.

## Next Candidate After Validation

- Continue the Phase 1 foundation order with `upload-staging-v1`.
