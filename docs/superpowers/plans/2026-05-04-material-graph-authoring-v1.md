# Material Graph Authoring v1 Implementation Plan (2026-05-04)

> **For agentic workers:** REQUIRED SUB-SKILL: Use `superpowers:subagent-driven-development` (recommended) or `superpowers:executing-plans` to implement this plan task-by-task. Use `editor-change` for `mirakana_editor_core` / editor wiring; use `rendering-change` only when touching renderer/RHI/shaders beyond lowering contracts; use `gameengine-agent-integration` when changing `engine/agent/manifest.json`, schemas, or static agent checks. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Introduce **first-party `GameEngine.MaterialGraph.v1`** as the canonical authoring document for material graphs (format **A** from the Phase 4 plan), with deterministic text IO, validation, **lowering to existing runtime `MaterialDefinition`**, and a **GUI-independent** `MaterialGraphAuthoringDocument` in `mirakana_editor_core` mirroring `MaterialAuthoringDocument` patterns.

**Architecture:** Graph IR lives in `mirakana_assets` next to `MaterialDefinition` (`mirakana/assets/material_graph.hpp`). Nodes are value types (`graph_output`, `constant_vec4`, `constant_vec3`, `constant_scalar`, `texture`). Edges connect producer `out` sockets to the root output node inputs named `factor.*` and `texture.*`. Lowering walks only edges whose `to_node` is the declared `graph.output_node`; unsupported topology or unknown sockets produce diagnostics (no silent defaults beyond missing factors, which use the same numeric defaults as empty `MaterialFactors`).

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_editor_core`, `ITextStore`, `UndoStack`, `AssetRegistry`, existing `MaterialDefinition` serialization style (line-oriented `key=value` with unique keys).

---

## Goal

- Authors can store a `.materialgraph` (or repository-relative path) containing `GameEngine.MaterialGraph.v1`.
- `validate_material_graph` rejects cycles/duplicates/unknown kinds with structured diagnostics.
- `lower_material_graph_to_definition` produces a `MaterialDefinition` suitable for existing cook/runtime paths **without** claiming shader graphs, live generation, or PBR upgrades.

## Context

- Master plan Phase 4 entry: [2026-05-03-production-completion-master-plan-v1.md](2026-05-03-production-completion-master-plan-v1.md) Phase 4.
- Prior factor/texture authoring remains on `GameEngine.Material.v1` / `MaterialAuthoringDocument`; graphs are **additive** until cook tooling consumes `.materialgraph` directly (Non-Goals).

## Constraints

- No public native handles; no raw shell for shader work in this slice.
- No new third-party dependencies (no `license-audit` package changes in this slice).
- Do not broaden manifest `ready` recipes; update `unsupportedProductionGaps` / purpose text only to describe the new **authoring** surface accurately.

## Implementation Steps

- [x] Add `material_graph.hpp` / `material_graph.cpp` to `mirakana_assets` with serialize/deserialize/validate/lower.
- [x] Add `material_graph_shader_export.hpp` placeholder for the next slice (`shader-graph-and-generation-pipeline-v1`) documenting the future hook from graph outputs to shader sources (no runtime consumption yet).
- [x] Add `material_graph_authoring.hpp` / `.cpp` to `mirakana_editor_core` with save/load/stage/undo-friendly replace.
- [x] Extend `mirakana_editor_core_tests` with round-trip, lowering, and validation failure coverage.
- [x] Register new public header paths in `engine/agent/manifest.json` for `mirakana_assets` and set `aiOperableProductionLoop.currentActivePlan` to this file while the slice is active; point `recommendedNextPlan` at `shader-graph-and-generation-pipeline-v1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` / `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`, and `cmake --build --preset dev` with `ctest -R mirakana_editor_core_tests` (full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` when host time allows).

## Tests

- `ctest -C Debug -R mirakana_editor_core_tests` (new cases).
- Optional: add `mirakana_assets`-only test target if future slices need faster loops; not required for v1.

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_editor_core_tests` | PASS | Windows MSVC `dev` preset |
| `ctest -C Debug -R mirakana_editor_core_tests` | PASS | Includes new material graph cases |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` | PASS | |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` | PASS | `tools/check-json-contracts.ps1` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` | PASS | `tools/check-ai-integration.ps1` |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows host 2026-05-04; Metal/Apple packaging reported diagnostic-only blockers as expected |

## Done When

- [x] `GameEngine.MaterialGraph.v1` text round-trip stable for the supported node/edge subset.
- [x] Lowering matches hand-authored `MaterialDefinition` for the same numeric/texture data in tests.
- [x] Editor document save rejects non-empty diagnostics.
- [x] Manifest and agent checks remain green.

## Non-Goals

- Shader graph IR, HLSL/MSL generation, `ProcessCommand` compile loops, renderer PBR changes, cascaded shadows, postprocess stacks, pipeline cache (later Phase 4 children).
- Cook/package pipeline automatically consuming `.materialgraph` without an explicit future command surface.

---

*Plan authored: 2026-05-04. Slice: material-graph-authoring-v1 (Phase 4 / child 1).*
