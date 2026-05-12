# Docs Current Truth And Registry Cleanup v1 Implementation Plan (2026-05-04)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Make `docs/` easier to navigate by separating current truth from historical plan evidence without moving historical plan files.

**Architecture:** Keep `docs/README.md`, `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/plans/README.md` as the human entrypoints. Treat `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` and `recommendedNextPlan` as the authoritative narrow-slice pointers, with `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md` as the active production roadmap. Keep dated plan files as immutable evidence records unless a focused follow-up explicitly edits them.

**Tech Stack:** Markdown docs, `engine/agent/manifest.json`, PowerShell validation scripts, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

---

## Context

The docs tree contains many dated implementation plans. Most of them are historical evidence, but the current entrypoints also contain stale "currentActivePlan" wording and long completed-plan lists. This makes it easy for humans or agents to treat historical paragraphs as live task queues.

Official-practice alignment for this cleanup means:

- CMake guidance continues to point at presets, target exports, C++ module file sets, and explicit `import std` gating.
- vcpkg guidance continues to use manifest mode, pinned baselines, manifest features, and repository-owned bootstrap through `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1` instead of configure-time dependency restore.
- Current docs must avoid compatibility-shim language and broad ready claims.

## Constraints

- Do not move or delete historical plan files.
- Do not change C++ code, engine APIs, command surfaces, game manifests, validation recipes, or package semantics.
- Do not treat future-dated local plan records as the current source of truth unless `engine/agent/manifest.json` points at them.
- Preserve required static-check terms in `docs/current-capabilities.md`, `docs/roadmap.md`, and `docs/superpowers/plans/README.md`.

## Done When

- `docs/README.md` states the current-truth vs historical-evidence split.
- `docs/current-capabilities.md` has a concise active-work section based on the manifest active slice.
- `docs/roadmap.md` has a current snapshot and no stale active-slice pointer in the production roadmap summary.
- `docs/superpowers/plans/README.md` keeps the registry focused and calls out date hygiene.
- `engine/agent/manifest.json.documentationPolicy.entrypoints.activeRoadmap` points at the active production completion roadmap.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passes or records a concrete blocker.

## Tasks

- [x] Inventory docs and validation expectations.
- [x] Update docs entrypoints and active-work summaries.
- [x] Update the manifest documentation entrypoint for the active roadmap.
- [x] Run validation and record evidence.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-04:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
  - Local Markdown link check over `docs/**/*.md`: PASS after skipping fenced code blocks.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS. `validate: ok`; CTest 29/29 passed. Diagnostic-only blockers remain unchanged for missing Metal tools and Apple packaging on this Windows host.
