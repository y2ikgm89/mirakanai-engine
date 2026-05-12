# Shader Hot Reload And Pipeline Cache v1 Implementation Plan (2026-05-06)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add a deterministic `mirakana_tools` batch plan that turns shader artifact provenance, persistent cache-index state, and pipeline recreation decisions into package/operator-visible hot-reload and cache reconciliation rows.

**Status:** Completed

**Architecture:** Reuse the existing shell-free `ShaderCompileExecutionRequest`, artifact provenance, and `ShaderArtifactCacheIndex` contracts. Add a narrow host-independent planner that groups selected shader compile requests, reports compile/recreate/cache-index work, and can repair missing or stale first-party cache-index rows from current provenance without invoking a compiler or exposing native D3D12/Vulkan/Metal pipeline-cache blobs. Native PSO/VkPipelineCache blob ownership remains backend-private because official APIs describe those caches as device/driver-specific data.

**Tech Stack:** C++23, `mirakana_tools` shader compile action/toolchain contracts, `mirakana_platform::IFileSystem`, focused `mirakana_tools_tests`, docs/manifest/static AI guidance sync.

---

## Context

- Existing `execute_shader_compile_action` writes artifacts, `.geprovenance`, and a persistent `ShaderArtifactCacheIndex`.
- Existing `build_shader_hot_reload_plan` handles one compile request, and `build_shader_pipeline_recreation_plan` handles one compile result.
- The master plan still lists `shader-hot-reload-and-pipeline-cache-v1` as a renderer/material quality follow-up after material graph package binding.
- D3D12 cached PSO blobs and Vulkan pipeline caches are backend/device-specific; this slice records only first-party metadata needed to decide when a host should compile, recreate, or repair cache-index state.

## Constraints

- Do not execute shader compilers, validators, process runners, package scripts, or arbitrary shell from the new planner.
- Do not add native PSO blob, `VkPipelineCache`, `ID3D12PipelineState::GetCachedBlob`, Metal library-cache, or public native-handle APIs.
- Do not claim renderer/RHI residency, broader package streaming, live shader generation, shader graph IR, or broad renderer quality.
- Keep cache reconciliation deterministic, package-relative, and derived only from current artifacts plus provenance.
- Public header changes require `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- End with focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.

## Tasks

### Task 1: RED Shader Pipeline Cache Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`

- [x] Add a test named `shader pipeline cache plan batches hot reload and repairs missing cache index`.
- [x] Use two reviewed `ShaderCompileExecutionRequest` rows that share one safe cache-index path.
- [x] Compile once through `execute_shader_compile_action`, remove the cache-index file, and call the new batch planner.
- [x] Assert no compile or pipeline recreation is required, both artifact rows require cache-index repair, and reconciliation writes one deterministic cache index with both entries.
- [x] Add stale-source coverage proving changed input fingerprints still require compile and pipeline recreation, not cache-index repair.
- [x] Add malformed cache-index coverage proving current provenance can rebuild the first-party index without touching artifacts.
- [x] Run `cmake --build --preset dev --target mirakana_tools_tests` and confirm RED because the new API does not exist.

### Task 2: Shader Pipeline Cache API

**Files:**
- Modify: `engine/tools/include/mirakana/tools/shader_compile_action.hpp`
- Modify: `engine/tools/src/shader_compile_action.cpp`

- [x] Add `ShaderPipelineCachePlanEntry`, `ShaderPipelineCachePlan`, and `ShaderPipelineCacheReconcileResult` value types.
- [x] Add public functions:
  - `build_shader_pipeline_cache_plan(const IFileSystem& filesystem, const std::vector<ShaderCompileExecutionRequest>& requests)`
  - `reconcile_shader_pipeline_cache_index(IFileSystem& filesystem, const std::vector<ShaderCompileExecutionRequest>& requests)`
- [x] Reuse `build_shader_hot_reload_plan` per request and sort output rows by artifact path.
- [x] Mark cache-index repair only when artifact and provenance inputs are current but the cache index is missing, malformed, missing the artifact row, or has a stale fingerprint/path.
- [x] Rebuild or update only the selected safe cache-index files from current provenance; do not compile or write artifacts.

### Task 3: GREEN Tools Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `engine/tools/include/mirakana/tools/shader_compile_action.hpp`
- Modify: `engine/tools/src/shader_compile_action.cpp`

- [x] Run the focused `mirakana_tools_tests` build until the new tests pass.
- [x] Run `ctest --preset dev -R "mirakana_tools_tests" --output-on-failure`.
- [x] Refactor only while the focused tests stay green.

### Task 4: Docs, Manifest, Skills, And Static Guidance

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/architecture.md`
- Modify: `docs/rhi.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.agents/skills/gameengine-agent-integration/SKILL.md`
- Modify: `.claude/skills/gameengine-agent-integration/SKILL.md`

- [x] Document the new batch planner and cache-index reconciliation boundary.
- [x] Keep ready claims narrow: no native PSO/Vulkan/Metal cache blob API, no shader compiler execution from the planner, no live shader generation, no public native handles, no renderer/RHI residency, and no broad renderer quality.
- [x] Update manifest `mirakana_tools`, tool-execution, live-iteration, and recommended-next-plan text.
- [x] Return `currentActivePlan` to the master plan after validation passes.

### Task 5: Validation

- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run focused `mirakana_tools_tests` build/CTest commands from Task 3.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record validation evidence here and move this plan to Recent Completed in the registry.

## Done When

- Batch planning reports per-artifact hot-reload, compile-required, pipeline-recreation, cache-entry-current, and cache-index-repair rows deterministically.
- Cache-index reconciliation can rebuild missing or malformed selected first-party cache indexes from current artifact provenance without invoking compilers or writing artifacts.
- Stale shader inputs continue to require compile plus pipeline recreation instead of masking the change as a cache-index repair.
- Docs, manifest, Codex/Claude skills, and static checks state the new boundary honestly.
- Focused tests and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` pass, or a concrete local-tool blocker is recorded.

## Validation Results

- RED: `cmd.exe /D /S /C "set Path=%PATH%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests"` failed before implementation because `mirakana::build_shader_pipeline_cache_plan` and `mirakana::reconcile_shader_pipeline_cache_index` did not exist.
- GREEN focused build: `cmd.exe /D /S /C "set Path=%PATH%&& set PATH=&& cmake --build --preset dev --target mirakana_tools_tests"` passed.
- GREEN focused CTest: `ctest --preset dev -R "mirakana_tools_tests" --output-on-failure` passed, 1/1 selected test target.
- Formatting/static checks: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.
- Full validation: `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed, including license/config/schema/dependency/toolchain checks, public API boundary check, dev configure/build, and 29/29 CTest targets. Diagnostic-only host gates remain honest: Metal shader/library tools are missing on this host, and Apple packaging is blocked without macOS/Xcode.
