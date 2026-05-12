# Authored Scene Cook / Package Workflow v0 Implementation Plan (2026-04-29)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Let first-party authored `GameEngine.Scene.v1` source documents participate in the asset import/cook pipeline so runtime packages can contain deterministic cooked scene payloads with dependency edges instead of relying only on handwritten fixtures.

**Architecture:** Extend `mirakana_assets` metadata and import-plan contracts with scene import records and scene dependency edge kinds. Keep scene source validation/canonicalization in `mirakana_tools` by deserializing and reserializing through `mirakana_scene`, while `mirakana_runtime_scene` remains the renderer-free runtime consumer. This slice does not add editor UI, prefab authoring UX, source format importers, package installer changes, scene/physics/navigation integration, renderer/RHI upload, or native handles.

**Tech Stack:** C++23, `mirakana_assets`, `mirakana_scene`, `mirakana_tools`, CTest, no new third-party dependencies.

---

## Context

- `mirakana_scene` already serializes and deserializes `GameEngine.Scene.v1`.
- `mirakana_runtime_scene` now loads cooked scene payloads from `RuntimeAssetPackage` into gameplay-owned `mirakana::Scene` instances.
- `mirakana_assets` import metadata and import plans currently cover texture, mesh, material, and audio, but not scene source documents.
- `mirakana_tools::execute_asset_import_plan` currently cooks first-party texture/mesh/audio source documents and canonical material definitions, but cannot validate/cook a first-party scene document.

## Constraints

- Keep `mirakana_assets` independent from `mirakana_scene`, renderer, RHI, editor, platform windows, SDL3, Dear ImGui, and native handles.
- `mirakana_tools` may link privately to `mirakana_scene` for scene source validation and canonical serialization.
- Scene import metadata may list mesh, material, and sprite texture dependencies explicitly; this slice does not infer dependencies by parsing arbitrary external scene formats or editor project databases.
- Do not generate package indexes automatically from import execution in this slice. Existing `build_asset_cooked_package_index` remains the package-index builder.
- Do not update desktop package fixtures unless required by tests.

## Done When

- [x] `AssetImportMetadataRegistry` supports validated scene records.
- [x] `AssetImportActionKind::scene` is implemented in import plans, tool execution results, and cooked artifact metadata.
- [x] Scene dependency edges distinguish mesh, material, and sprite texture references.
- [x] `build_asset_import_plan` emits deterministic scene actions and dependency edges, rejects missing referenced mesh/material/sprite texture imports, and keeps recook dependencies intact.
- [x] `execute_asset_import_plan` reads a first-party `GameEngine.Scene.v1` source, validates it with `deserialize_scene`, writes canonical `serialize_scene` output, and leaves no partial artifact on failure.
- [x] Focused core/tools/runtime-scene validation passes.
- [x] Docs, roadmap/gap analysis, manifest, Codex/Claude skills, and gameplay-builder agents describe authored scene cook/package workflow honestly.

---

### Task 1: RED Asset Metadata And Plan Tests

**Files:**
- Modify: `tests/unit/core_tests.cpp`

- [x] Add tests that reference `SceneImportMetadata`, `try_add_scene`, `scene_count`, `find_scene`, `scene_records`, `AssetImportActionKind::scene`, and scene dependency edge kinds before the production API exists.
- [x] Cover valid scene metadata with mesh, material, and sprite texture dependencies.
- [x] Cover invalid scene metadata: empty source path, duplicate mesh/material/sprite dependency within the same list, and duplicate scene id.
- [x] Cover `build_asset_import_plan` emitting a deterministic scene action and dependency edges for mesh/material/sprite texture dependencies.
- [x] Cover missing scene mesh/material/sprite texture dependency rejection.
- [x] Run focused build and confirm RED.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests
```

Expected failure: compile errors for missing scene import metadata/API or `AssetImportActionKind::scene`.

### Task 2: RED Scene Cook Executor Tests

**Files:**
- Modify: `tests/unit/tools_tests.cpp`

- [x] Add tests for `execute_asset_import_plan` with `AssetImportActionKind::scene`.
- [x] Cover canonical scene cooking by writing authored source through `serialize_scene`, executing import, and reading `format=GameEngine.Scene.v1` output.
- [x] Cover malformed scene source failure and verify no output artifact is written.
- [x] Cover mixed texture/material/mesh/scene import execution in one transaction.
- [x] Run focused build and confirm RED after Task 1 public API exists or compile failure before it exists.

Expected RED command:

```powershell
. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_tools_tests
```

Expected failure: scene import action kind is unsupported or `mirakana_tools` lacks scene serialization integration.

### Task 3: Green `mirakana_assets` Scene Import Contracts

**Files:**
- Modify: `engine/assets/include/mirakana/assets/asset_dependency_graph.hpp`
- Modify: `engine/assets/src/asset_dependency_graph.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_import_metadata.hpp`
- Modify: `engine/assets/src/asset_import_metadata.cpp`
- Modify: `engine/assets/include/mirakana/assets/asset_import_pipeline.hpp`
- Modify: `engine/assets/src/asset_import_pipeline.cpp`
- Modify: `engine/assets/src/asset_package.cpp`

- [x] Add `scene_mesh`, `scene_material`, and `scene_sprite` dependency kinds.
- [x] Add `SceneImportMetadata` with explicit mesh, material, and sprite texture dependency vectors.
- [x] Add scene registry methods and deterministic record sorting.
- [x] Add `AssetImportActionKind::scene`.
- [x] Emit scene import actions from `build_asset_import_plan`.
- [x] Validate scene dependencies against known mesh/material/texture imports and emit dependency edges using imported paths.
- [x] Keep combined action dependencies unique and deterministic.

### Task 4: Green `mirakana_tools` Scene Cook Execution

**Files:**
- Modify: `engine/tools/CMakeLists.txt`
- Modify: `engine/tools/src/asset_import_tool.cpp`

- [x] Link `mirakana_tools` privately to `mirakana_scene`.
- [x] Include `mirakana/scene/scene.hpp` only in the `.cpp`.
- [x] Map `AssetImportActionKind::scene` to `scene` and `GameEngine.Scene.v1`.
- [x] Read source text, deserialize with `deserialize_scene`, write canonical `serialize_scene` output.
- [x] Ensure the existing all-or-nothing prepare-then-write transaction remains intact for scene failures.

### Task 5: Docs And Agent Sync

**Files:**
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/testing.md`
- Modify: `docs/specs/2026-04-27-engine-essential-gap-analysis.md`
- Modify: `docs/ai-game-development.md`
- Modify: `engine/agent/manifest.json`
- Modify: `.agents/skills/gameengine-game-development/SKILL.md`
- Modify: `.claude/skills/gameengine-game-development/SKILL.md`
- Modify: `.codex/agents/gameplay-builder.toml`
- Modify: `.claude/agents/gameplay-builder.md`

- [x] Register this plan as active before implementation.
- [x] After validation, move this plan to completed with concise evidence.
- [x] Mark first-party scene source cooking implemented only after validation.
- [x] Keep editor scene UX, prefab authoring UX, source importer adapters, runtime package index generation, scene/physics/navigation integration, and renderer/RHI upload listed as follow-up work.
- [x] Keep `2026-04-29-grid-dynamic-obstacle-replan-v0.md` listed only as an unexecuted host-feasible candidate, not completed.

### Task 6: Verification

- [x] Run focused `mirakana_core_tests`, `mirakana_tools_tests`, `mirakana_runtime_scene_tests`, and `mirakana_scene_renderer_tests`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`.
- [x] Run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`.
- [x] Record exact host/toolchain blockers.

## Validation Evidence

- RED confirmed: `mirakana_core_tests` initially failed to compile before scene import metadata/action APIs existed.
- RED confirmed: `mirakana_tools_tests` initially failed scene cook execution before `mirakana_tools` handled `AssetImportActionKind::scene`.
- Reviewer fix confirmed: `mirakana_core_tests` failed on `cooked package index rejects dependency edges outside package entries` before package-index edge cross-validation was added.
- Focused validation passed:
  - `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CMake --build --preset dev --target mirakana_core_tests mirakana_tools_tests mirakana_runtime_scene_tests mirakana_scene_renderer_tests`
  - `. .\tools\common.ps1; $tools = Assert-CppBuildTools; Invoke-CheckedCommand $tools.CTest --preset dev --output-on-failure -R "^(mirakana_core_tests|mirakana_tools_tests|mirakana_runtime_scene_tests|mirakana_scene_renderer_tests)$"`
  - Result: 4/4 tests passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1`: PASS.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1`: config PASS, diagnostic-only blocker remains `out/build/dev/compile_commands.json` missing for the Visual Studio generator.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1`: PASS, 22/22 tests passed. Diagnostic-only host gates remain Metal `metal`/`metallib` missing, Apple macOS/Xcode unavailable, Android release signing not configured, Android device smoke not connected, and strict clang-tidy compile database unavailable for the current generator.
