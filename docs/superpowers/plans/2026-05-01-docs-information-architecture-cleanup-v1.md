# Docs Information Architecture Cleanup v1 Implementation Plan (2026-05-01)

> **For agentic workers:** This is a docs-only governance slice. Do not change engine/runtime behavior here, and do not move historical plan files in this slice.

**Status:** Completed on 2026-05-01

**Goal:** Make the documentation easier for humans and AI agents to navigate by separating current truth, capability summaries, specs, active plans, and historical implementation evidence.

**Architecture:** Keep `docs/README.md` as the human documentation entrypoint, `docs/roadmap.md` as current status and priorities, `docs/current-capabilities.md` as a concise human summary of the manifest, `docs/specs/README.md` as the single spec index, and `docs/superpowers/plans/README.md` as a lightweight implementation plan registry.

**Tech Stack:** Markdown docs, `engine/agent/manifest.json` documentation entrypoints, `tools/check-ai-integration.ps1`, and existing validation scripts.

---

## Goal

- Keep current product truth separate from historical implementation evidence.
- Give agents a short capability summary before they read large manifests or historical plans.
- Give specs their own index so design records are not mixed with execution plans.
- Keep the plan registry focused on active/next/recent work and archive navigation.
- Preserve all historical plan files in place.

## Constraints

- Do not delete or move historical plans.
- Do not change engine behavior, public APIs, command surfaces, recipes, host gates, or capability status.
- Do not mark Scene v2 runtime package migration complete.
- Keep `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` on `docs/superpowers/plans/2026-05-01-scene-v2-runtime-package-migration-v1.md`.

## Done When

- `docs/README.md` points readers to current truth, capability summary, spec indexes, and plan registry.
- `docs/current-capabilities.md` exists and summarizes ready, host-gated, and planned capabilities without broad unsupported claims.
- `docs/specs/README.md` classifies design/spec records.
- `docs/superpowers/plans/README.md` is lightweight and preserves archive navigation.
- Agent/static checks require the new docs entrypoints.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass or record concrete blockers.

## Tasks

- [x] Inventory docs entrypoints and static checks.
- [x] Add the current capability summary.
- [x] Add spec indexes.
- [x] Lightweight the plan registry without moving historical files.
- [x] Sync manifest documentation entrypoints and static checks.
- [x] Run validation and record evidence.

## Validation Evidence

Record command results here while implementing this plan.

- 2026-05-01:
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`: PASS. Output includes the updated documentation entrypoints from `engine/agent/manifest.json`.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
  - `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 28/28 CTest tests passed. Existing diagnostic-only gates remain: missing Metal tools on this Windows host, Apple packaging blocked without macOS/Xcode, Android release signing/device smoke not fully configured, and strict tidy compile database availability.
