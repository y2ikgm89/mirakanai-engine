# Shader Graph and Generation Pipeline v1 Implementation Plan (2026-05-04)

> **For agentic workers:** Use `superpowers:subagent-driven-development` or `superpowers:executing-plans` for multi-step execution. Use `gameengine-agent-integration` when changing `engine/agent/manifest.json`, schemas, or static agent checks. Shader compiler execution must stay on reviewed `ProcessCommand` / `ShaderToolProcessRunner` paths ([AGENTS.md](../../../AGENTS.md)).

**Goal:** Close the Phase-4 gap between **`GameEngine.MaterialGraph.v1`** and reviewed **DXC** compilation by introducing **`GameEngine.MaterialGraphShaderExport.v0`** (bridge document), deterministic **HLSL stub emission** from validated graphs, and **`plan_material_graph_shader_pipeline`** in `mirakana_tools` that materializes `ShaderCompileExecutionRequest` rows for **D3D12 DXIL** and **Vulkan SPIR-V** (vertex + fragment) without raw shell, new third-party dependencies, or renderer binding changes.

**Architecture:** `mirakana_assets` owns text IO + validation for `MaterialGraphShaderExportDesc` plus `emit_material_graph_reviewed_hlsl_v0`, which lowers a validated graph to `MaterialDefinition` and emits a minimal VS/PS pair (`VSMain` / `PSMain`). `mirakana_tools` exposes `plan_material_graph_shader_pipeline`, which checks filesystem presence of the HLSL path, requires a `ShaderToolKind::dxc` descriptor, optionally skips Vulkan targets when SPIR-V codegen is unavailable (diagnostic), and returns four execution requests when SPIR-V is supported.

**Depends on:** [2026-05-04-material-graph-authoring-v1.md](2026-05-04-material-graph-authoring-v1.md).

---

## Goal

- Operators can store a `.materialgraph_shader_export` (or any repository-relative path) using `GameEngine.MaterialGraphShaderExport.v0`.
- Tooling can emit deterministic HLSL for compile smoke from validated material graphs.
- `plan_material_graph_shader_pipeline` produces reviewed compile requests aligned with `execute_shader_compile_action`.

## Constraints

- No raw shell; only existing shader tool runners / argv patterns.
- No new third-party packages.
- Do not flip `game.agent.json` `shaderGraph` / `materialGraph` readiness enums to `ready` in this slice (descriptor-only authoring bridge).

## Implementation Steps

- [x] Expand `material_graph_shader_export.hpp` / add `material_graph_shader_export.cpp` with serialize / deserialize / validate.
- [x] Add `emit_material_graph_reviewed_hlsl_v0` lowering-driven stub generator.
- [x] Add `material_graph_shader_pipeline.hpp` / `.cpp` with `plan_material_graph_shader_pipeline`.
- [x] Extend `mirakana_tools_tests` with serialization, emit, and planning coverage.
- [x] Register `material_graph_shader_pipeline.hpp` in `engine/agent/manifest.json`; refresh plan registry + `aiOperableProductionLoop` pointers.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tests

- `ctest -C Debug -R mirakana_tools_tests` (new material graph shader pipeline case).

## Validation Evidence

| Command | Result | Notes |
| --- | --- | --- |
| `cmake --build --preset dev --target mirakana_tools_tests` | PASS | |
| `ctest -C Debug -R mirakana_tools_tests` | PASS | |
| `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` | PASS | Windows host 2026-05-04 |

## Done When

- [x] `GameEngine.MaterialGraphShaderExport.v0` round-trips with validation.
- [x] Deterministic HLSL emission compiles structurally (validated via planning + `make_shader_compile_command` in tests).
- [x] Pipeline planner emits DXC requests for D3D12 + Vulkan when SPIR-V codegen is advertised on the tool descriptor.
- [x] Manifest + plan registry updated for the active Phase-4 slice ordering.

## Non-Goals

- Full shader graph IR, node editor UI, renderer PBR upgrades, Metal library packaging automation, pipeline cache, or hot reload integration.
- Automatic promotion of `shaderGraph` manifest readiness.

**Closure:** This slice is complete; `engine/agent/manifest.json` `aiOperableProductionLoop.currentActivePlan` now selects `docs/superpowers/plans/2026-05-04-pbr-material-and-lighting-v1.md`.

---

*Plan authored: 2026-05-04. Slice: shader-graph-and-generation-pipeline-v1 (Phase 4 / child 2).*
