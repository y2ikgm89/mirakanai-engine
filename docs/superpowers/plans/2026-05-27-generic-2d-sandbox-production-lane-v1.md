# 2026-05-27 Generic 2D Sandbox Production Lane v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the existing 2D tilemap, sandbox-world, streaming, persistence, renderer, authoring, networking, and package foundations into a generic production lane for large mutable 2D sandbox games without adopting one game's vocabulary or claiming broad readiness early.

**Architecture:** Keep the engine-owned surface first-party, data-driven, backend-neutral, and package-proven. Native Windows window, input, presentation, audio, and OS handles stay behind `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi`; public gameplay/runtime APIs stay value-only and backend-neutral. Game-specific rules such as biome names, enemy catalogs, crafting balance, item stats, boss behavior, and story remain in `games/<game_name>/`; reusable engine primitives cover chunked world state, tile mutation execution, persistence, streaming, renderer budgets, authoring review, validation recipes, and optional host-gated adapters.

**Tech Stack:** C++23, `MK_runtime`, `MK_assets`, `MK_tools`, `MK_renderer`, `MK_rhi`, `MK_scene_renderer`, `MK_ui`, `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, `MK_audio_wasapi`, `MK_runtime_network_enet` optional adapter, Windows SDK Win32, Raw Input, XInput/GameInput-class controller gates, WASAPI shared-mode playback, DXGI/D3D12 presentation, CMake/CTest/CPack, PowerShell 7 repository tools, vcpkg manifest features, D3D12 primary Windows lane, strict Vulkan host-gated lane, Metal Apple-host gate, generated `DesktopRuntime2DPackage` package proof.

---

**Plan ID:** `generic-2d-sandbox-production-lane-v1`

**Status:** Phased milestone implementation in progress. Phases 1-9 have completed as reviewable slices, and Phase 10 is selected through the focused child plan `Sandbox World Package Validation And Performance Budgets v1`. This file does not replace `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan`, does not reopen `unsupportedProductionGaps`, and does not mark later planned capabilities as ready.

## Current Evidence And Gap Summary

Existing ready foundations:

- `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp` plans sandbox chunk/cell placement, destruction, construction cost, mutation, persistence-review, and replay-hash rows, but it does not mutate a live world or write persistence.
- `engine/runtime/include/mirakana/runtime/asset_runtime.hpp` exposes runtime tilemap payload sampling, but current tilemap support is metadata and visible-cell sampling rather than mutable chunk execution.
- `engine/tools/include/mirakana/tools/tilemap_tool.hpp` authors cooked tilemap metadata and package index rows, and `engine/tools/include/mirakana/tools/sandbox_world_authoring.hpp` reviews deterministic sandbox tile/palette/chunk/seed changed-file rows before safe package-relative apply. They explicitly reject production atlas/source decode/native GPU claims, external downloads, arbitrary importer plugins, runtime source parsing, and package apply during review unless later adapters implement them.
- `engine/runtime/include/mirakana/runtime/world_region_streaming.hpp`, addressable content rows, runtime package resident mounts, and resource catalogs provide reviewed load/release planning, but broad background streaming and renderer-owned residency remain outside the current claim.
- `engine/renderer/include/mirakana/renderer/sprite_batch.hpp` and the selected 2D package have sprite batching, sprite animation, sorting, 9-slice/tiled, particle, and native sprite evidence, but broad high-density production tile rendering is not yet proven.
- `games/sample_2d_desktop_runtime_package/README.md` intentionally excludes runtime source image decoding, production atlas packing, full tilemap editor UX, broad package streaming, socket IO, rollback/lockstep, broad renderer quality, and broad multiplayer readiness.
- The completed first-party desktop platform replacement milestone is the prerequisite desktop host evidence: selected Windows desktop proof must route through `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi`, not through a legacy desktop middleware compatibility layer.

This milestone closes those gaps by implementing reusable engine primitives. It must not implement a Terraria clone. The proof game should be a small generic sandbox probe with neutral names such as `generic_2d_sandbox_probe`.

## Research And Official Practice Gate

Before any phase starts, refresh and record the exact official sources used by that phase. The planning pass used these anchors:

- Context7 `/websites/learn_microsoft_en-us_windows_win32_api`: Win32 windows, Raw Input, WASAPI, and related COM/native handles are official Windows host APIs; keep them inside platform/runtime-host/audio adapters and expose only first-party value contracts to runtime/game code.
- Context7 `/microsoft/vcpkg`: optional dependencies must stay in manifest mode through `vcpkg.json`, `builtin-baseline`, and additive feature rows. Dependency installation remains owned by `tools/bootstrap-deps.ps1`.
- Context7 `/kitware/cmake`: install/export, C++ module file sets, target usage requirements, CTest, and CPack must stay target-based and wrapper-driven.
- CMake presets and install/export docs: <https://cmake.org/cmake/help/latest/manual/cmake-presets.7.html> and <https://cmake.org/cmake/help/latest/command/install.html>.
- Microsoft vcpkg manifest mode: <https://learn.microsoft.com/en-us/vcpkg/concepts/manifest-mode>.
- Microsoft Win32 window creation: <https://learn.microsoft.com/en-us/windows/win32/learnwin32/creating-a-window>.
- Microsoft Win32 messages and message queues: <https://learn.microsoft.com/en-us/windows/win32/winmsg/about-messages-and-message-queues>.
- Microsoft Raw Input overview: <https://learn.microsoft.com/en-us/windows/win32/inputdev/about-raw-input>.
- Microsoft `RegisterRawInputDevices`: <https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-registerrawinputdevices>.
- Microsoft WASAPI `IAudioClient`: <https://learn.microsoft.com/en-us/windows/win32/api/audioclient/nn-audioclient-iaudioclient>.
- Microsoft XInput getting started: <https://learn.microsoft.com/en-us/windows/win32/xinput/getting-started-with-xinput>.
- Unreal World Partition: <https://dev.epicgames.com/documentation/en-us/unreal-engine/world-partition-in-unreal-engine>. Use it only as comparison evidence for grid cells, streaming sources, editor-loaded regions, cooked world data, and debug overlays.
- Unity Tilemaps and Addressables: <https://docs.unity3d.com/Manual/tilemaps/tilemaps-landing.html> and <https://docs.unity.cn/Packages/com.unity.addressables%401.16/manual/AddressableAssetsOverview.html>. Use them only as comparison evidence for tile palettes, brushes, collision, asset addresses, dependency catalogs, and asynchronous loading.
- D3D12 resource barriers and multi-engine synchronization: <https://learn.microsoft.com/en-us/windows/win32/direct3d12/using-resource-barriers-to-synchronize-resource-states-in-direct3d-12> and <https://learn.microsoft.com/en-us/windows/win32/direct3d12/user-mode-heap-synchronization>.
- Vulkan synchronization guide: <https://docs.vulkan.org/guide/latest/synchronization.html>.
- WAI-ARIA Authoring Practices Guide: <https://www.w3.org/WAI/ARIA/apg/> for runtime UI semantics when editor or game menus expose sandbox authoring and inventory surfaces.

Every implementation phase must add a short `Official Evidence` subsection before its first checked item. Record the Context7 library id or URL, the date, and the exact decision affected by that source.

## Non-Goals

- No game-specific biome, item, enemy, NPC, boss, crafting, economy, story, or balance rules in engine modules.
- No public `HWND`, `HINSTANCE`, `HANDLE`, COM interface, D3D12, Vulkan, Metal, ENet, Dear ImGui, OS, editor, or RHI handles in gameplay APIs.
- No legacy desktop middleware compatibility shim, alias target, migration layer, or adapter facade. This milestone targets a clean breaking first-party host surface.
- No broad renderer, multiplayer, modding, cloud save, source asset, accessibility, mobile, or editor productization claim unless a later focused plan validates it.
- No background threads, filesystem mutation, package mutation, network IO, or renderer/RHI uploads from value-only review APIs.
- No new third-party dependency without `vcpkg.json`, `docs/dependencies.md`, `docs/legal-and-licensing.md`, `THIRD_PARTY_NOTICES.md`, dependency bootstrap, and license validation in the same phase.

## Proposed File Map

Expected new or expanded implementation surfaces:

- Modify: `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp`
- Modify: `engine/runtime/src/genre_sandbox_world.cpp`
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp`
- Create: `engine/runtime/src/sandbox_world_runtime.cpp`
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_persistence.hpp`
- Create: `engine/runtime/src/sandbox_world_persistence.cpp`
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_streaming.hpp`
- Create: `engine/runtime/src/sandbox_world_streaming.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/world_region_streaming.hpp`
- Modify: `engine/runtime/src/world_region_streaming.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/asset_runtime.hpp`
- Modify: `engine/runtime/src/asset_runtime.cpp`
- Create: `engine/renderer/include/mirakana/renderer/tile_chunk_renderer.hpp`
- Create: `engine/renderer/src/tile_chunk_renderer.cpp`
- Modify: `engine/renderer/include/mirakana/renderer/sprite_batch.hpp`
- Modify: `engine/renderer/src/sprite_batch.cpp`
- Create: `engine/tools/include/mirakana/tools/sandbox_world_authoring.hpp`
- Create: `engine/tools/asset/sandbox_world_authoring.cpp`
- Modify: `engine/tools/include/mirakana/tools/tilemap_tool.hpp`
- Modify: `engine/tools/asset/tilemap_tool.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Create optional proof game only when needed: `games/generic_2d_sandbox_probe/`
- Create tests: `tests/unit/runtime_sandbox_world_runtime_tests.cpp`
- Create tests: `tests/unit/runtime_sandbox_world_persistence_tests.cpp`
- Create tests: `tests/unit/runtime_sandbox_world_streaming_tests.cpp`
- Create tests: `tests/unit/renderer_tile_chunk_renderer_tests.cpp`
- Create tests: `tests/unit/tools_sandbox_world_authoring_tests.cpp`
- Modify tests: `tests/unit/runtime_genre_sandbox_world_tests.cpp`
- Modify tests: `tests/unit/runtime_world_region_streaming_tests.cpp`
- Modify tests: `tests/unit/tools_tests.cpp`
- Modify docs: `docs/current-capabilities.md`, `docs/roadmap.md`, `docs/ai-game-development.md`, `docs/testing.md`
- Modify manifest fragments only after code proves a surface: `engine/agent/manifest.fragments/*.json`
- Generate only through compose: `engine/agent/manifest.json`

## Phase 0 - Selection, Reconciliation, And Branch Hygiene

**Goal:** Start from current truth and prevent this proposed milestone from conflicting with the active production-completion selection state.

- [x] Check current branch and worktree:

```powershell
git status --short --branch
```

Expected: no unrelated task-owned changes are staged for this milestone.

- [x] Refresh minimal engine context:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 -ContextProfile Minimal
```

Expected: record the current `currentActivePlan`, `recommendedNextPlan`, and `unsupportedProductionGaps` values from the composed manifest. Do not assume an older active-plan filename; this milestone stays proposed unless a selection PR explicitly updates the manifest fragments.

- [x] Re-read the active plan, current capabilities, and plan registry before editing runtime code.

Files:
- `docs/current-capabilities.md`
- `docs/superpowers/plans/README.md`
- `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- the current first-party desktop platform replacement plan, if present in the registry, as prerequisite/historical evidence for the first-party host lane

- [x] If this milestone is selected for implementation, first verify that `MK_platform_win32`, `MK_runtime_host_win32`, `MK_runtime_host_win32_presentation`, and `MK_audio_wasapi` are implemented and validated or select that prerequisite as a separate plan. Then update `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json` only in the selection PR, run compose, and keep `unsupportedProductionGaps` honest.

## Phase 1 - Sandbox World Runtime Data Model

**Goal:** Convert value-only sandbox intent rows into a reusable live in-memory chunked world model with deterministic validation and no filesystem, renderer, or package side effects.

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp`
- Create: `engine/runtime/src/sandbox_world_runtime.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Create: `tests/unit/runtime_sandbox_world_runtime_tests.cpp`

- [x] Add RED tests for chunk descriptor validation: world id, chunk coord, chunk size, layer count, cell count, stable row sorting, duplicate chunk rejection, invalid bounds rejection, and budget diagnostics.
- [x] Add RED tests for live cell lookup and immutable snapshots: empty cell, existing cell, out-of-bounds cell, missing chunk, and deterministic snapshot hash.
- [x] Add RED tests that prove no package IO, persistence IO, renderer upload, threading, native handle, or platform calls occur.
- [x] Implement `RuntimeSandboxWorldDesc`, `RuntimeSandboxChunkCoord`, `RuntimeSandboxLayerId`, `RuntimeSandboxTileId`, `RuntimeSandboxCell`, `RuntimeSandboxChunk`, `RuntimeSandboxWorld`, and `build_runtime_sandbox_world`.
- [x] Implement deterministic lookup and snapshot helpers:

```cpp
mirakana::runtime::RuntimeSandboxCellSample sample_runtime_sandbox_cell(
    const RuntimeSandboxWorld& world,
    RuntimeSandboxCellCoord coord);

mirakana::runtime::RuntimeSandboxWorldSnapshot snapshot_runtime_sandbox_world(
    const RuntimeSandboxWorld& world);
```

- [x] Run focused validation:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_sandbox_world_runtime"
```

Expected: tests pass and runtime module still has no renderer/RHI/platform/editor dependency.

Evidence: Phase 1 landed as `Generic 2D Sandbox Runtime Foundation v1` with `RuntimeSandboxWorldDesc`, `RuntimeSandboxWorldBuildResult`, `RuntimeSandboxWorld`, `RuntimeSandboxWorldSnapshot`, `RuntimeSandboxCellSample`, `build_runtime_sandbox_world`, `sample_runtime_sandbox_cell`, and `snapshot_runtime_sandbox_world`, covered by `MK_runtime_sandbox_world_runtime_tests` and documented without renderer/RHI/platform/editor/package side effects.

## Phase 2 - Mutation Execution And Dirty Region Events

**Goal:** Execute reviewed placement/destruction intents against the live world with deterministic mutation rows, rejected-intent diagnostics, dirty chunk/cell regions, and replay signatures.

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp`
- Modify: `engine/runtime/src/sandbox_world_runtime.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp`
- Modify: `engine/runtime/src/genre_sandbox_world.cpp`
- Modify: `tests/unit/runtime_sandbox_world_runtime_tests.cpp`
- Modify: `tests/unit/runtime_genre_sandbox_world_tests.cpp`

- [x] Add RED tests for placement execution into empty cells, destruction execution from occupied cells, replace-denied diagnostics, missing-cost diagnostics, unsupported layer diagnostics, and deterministic mutation order.
- [x] Add RED tests for dirty region output: chunk dirty flag, rectangular cell bounds, layer mask, previous tile id, new tile id, and event replay hash.
- [x] Add RED tests for safe failure: rejected requests leave the original world snapshot unchanged.
- [x] Implement `apply_runtime_sandbox_world_mutations` as an explicit mutation API. Keep `plan_runtime_sandbox_world_mutation` value-only and side-effect-free.
- [x] Add package counter names only after the API is green: `sandbox_world_runtime_mutations_applied`, `sandbox_world_runtime_dirty_chunks`, `sandbox_world_runtime_replay_hash`, `sandbox_world_runtime_side_effect_diagnostics`.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests MK_runtime_genre_sandbox_world_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_sandbox_world_runtime|runtime_genre_sandbox_world"
```

Evidence: Phase 2 landed as `Generic 2D Sandbox Mutation Execution v1` with `RuntimeSandboxWorldMutationExecutionStatus`, `RuntimeSandboxWorldDirtyRegion`, `RuntimeSandboxWorldMutationExecutionResult`, and `apply_runtime_sandbox_world_mutations`, covered by focused runtime sandbox and genre sandbox tests while preserving input snapshots on invalid plans and keeping persistence IO, package IO, renderer/platform/editor APIs, threads, native handles, and game-specific rules out of scope.

## Phase 3 - Tile Simulation Primitives

**Goal:** Add generic tile simulation primitives for collision, light, liquid, triggers, and scheduled tile updates without hardcoding one game.

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/sandbox_world_runtime.hpp`
- Modify: `engine/runtime/src/sandbox_world_runtime.cpp`
- Create tests or extend: `tests/unit/runtime_sandbox_world_runtime_tests.cpp`

**Official Evidence:** 2026-05-30 implementation reused the plan's Microsoft/Context7 clean-break boundary: Win32/platform/render/audio handles remain behind first-party adapters, while Phase 3 adds only backend-neutral `MK_runtime` value rows. No new external SDK or dependency was introduced for this phase.

- [x] Add RED tests for tile material metadata rows: solid, platform, liquid, light emitter, replaceable, trigger, update cadence, and render layer.
- [x] Add RED tests for collision extraction: occupied solid cells become deterministic collision spans; platform/liquid/non-solid rows are separated.
- [x] Add RED tests for light propagation planning: seeded light emitters, bounded propagation radius, blocked-by-solid policy, light row output, and deterministic budget limits.
- [x] Add RED tests for liquid planning only as bounded simulation rows: source cells, flow candidates, blocked flow, update budget, and replay hash. Do not claim full fluid simulation quality.
- [x] Add RED tests for scheduled tile update rows driven by tile material update cadence.
- [x] Implement first-party value/execution APIs behind `MK_runtime` only. Physics-specific collision shape upload and renderer lightmap upload remain later phases.
- [x] Run focused tests:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_runtime_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_sandbox_world_runtime_tests
```

Evidence: `MK_runtime_sandbox_world_runtime_tests` passed after `RuntimeSandboxTileMaterialRow`, `RuntimeSandboxTileSimulationDesc`, `RuntimeSandboxTileSimulationPlan`, `RuntimeSandboxTileCollisionSpanRow`, `RuntimeSandboxTileCellRow`, `RuntimeSandboxLightPropagationRow`, `RuntimeSandboxLiquidFlowRow`, `RuntimeSandboxScheduledTileUpdateRow`, and `plan_runtime_sandbox_tile_simulation` landed. Outputs stay value-only with explicit zero physics/renderer/platform/thread side-effect flags.

## Phase 4 - Persistence, Snapshots, Migration, And Corruption Recovery

**Goal:** Persist large mutable sandbox worlds through chunk snapshots and deterministic migration review before any filesystem write occurs.

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_persistence.hpp`
- Create: `engine/runtime/src/sandbox_world_persistence.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Create: `tests/unit/runtime_sandbox_world_persistence_tests.cpp`
- Modify: `docs/testing.md`

**Official Evidence:** 2026-05-30 implementation used Context7 `/websites/cmake_cmake_help` for the official `add_test(NAME ... COMMAND ...)` and target-based CTest registration pattern, and Microsoft Learn Win32 documentation for future platform execution boundaries: `ReplaceFileW` replaces a file with an optional backup, and `FlushFileBuffers` flushes buffered data for a file handle. Phase 4 therefore implements only first-party `MK_runtime` value planning for canonical snapshot rows, diff rows, migration/recovery review, and atomic-save operation intent; it does not call Win32, `IFileSystem`, platform APIs, filesystem IO, package IO, renderer APIs, or threads. Future apply/execution APIs must stay in rooted project-relative platform/filesystem adapters.

- [x] Add RED tests for canonical text or binary-neutral document rows: world id, schema version, chunk coords, layer rows, changed cells, source package id, seed, tick, and content hash.
- [x] Add RED tests for snapshot diff planning: unchanged chunks omitted, dirty chunks included, deterministic chunk ordering, size budget diagnostics.
- [x] Add RED tests for migration review: exact schema chain, unsupported future schema, missing migration, repairable corruption, unrecoverable corruption, and no filesystem side effects.
- [x] Add RED tests for atomic-save planning: temp path, target path, backup path, write order, fsync/flush intent, rollback diagnostics. The first API only plans operations; platform/filesystem execution is separate.
- [x] Implement persistence review and serialization helpers using `IFileSystem` only when the phase explicitly adds apply APIs. Use rooted project-relative paths and reject absolute or parent-relative paths.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_persistence_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_sandbox_world_persistence"
```

Evidence: RED compile proof first failed with unresolved `RuntimeSandboxWorldPersistenceDocumentPlan::succeeded`, `RuntimeSandboxWorldMigrationReviewPlan::succeeded`, `RuntimeSandboxWorldAtomicSavePlan::succeeded`, `plan_runtime_sandbox_world_persistence_document`, `plan_runtime_sandbox_world_snapshot_diff`, `review_runtime_sandbox_world_migration`, and `plan_runtime_sandbox_world_atomic_save`. GREEN focused validation then passed `MK_runtime_sandbox_world_persistence_tests` build and `ctest -R "runtime_sandbox_world_persistence"` with `1/1` tests passing. Outputs stay value-only with explicit zero filesystem/platform/thread side-effect flags.

## Phase 5 - Chunk Streaming, Addressable Content, And Residency

**Goal:** Connect mutable sandbox chunks to existing world-region streaming, runtime package mounts, addressable content rows, and residency budgets.

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/sandbox_world_streaming.hpp`
- Create: `engine/runtime/src/sandbox_world_streaming.cpp`
- Create: `tests/unit/runtime_sandbox_world_streaming_tests.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Modify: `CMakeLists.txt`

- [x] Add RED tests for streaming-source rows: player/source position, rectangular or radius load range, chunk priority, target state `loaded` versus `active`, and unload protection for dirty chunks.
- [x] Add RED tests for addressable chunk dependencies: tile atlas, biome material rows, audio rows, prefab/object rows, and missing dependency diagnostics.
- [x] Add RED tests for resident budget execution: chunk count cap, payload byte cap, dirty chunk pinning, explicit eviction review, and no automatic LRU claim until implemented.
- [x] Implement safe-point adoption APIs that consume reviewed package candidates and existing resident catalog rows. Do not create background threads in this phase.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_sandbox_world_streaming_tests MK_runtime_world_region_streaming_tests MK_runtime_addressable_content_streaming_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "sandbox_world_streaming|world_region_streaming|addressable_content_streaming"
```

Evidence: RED compile proof first failed with unresolved `mirakana/runtime/sandbox_world_streaming.hpp` after registering `MK_runtime_sandbox_world_streaming_tests`. GREEN focused validation then passed `MK_runtime_sandbox_world_streaming_tests`, `MK_runtime_world_region_streaming_tests`, and `MK_runtime_addressable_content_streaming_tests` build plus `ctest -R "sandbox_world_streaming|world_region_streaming|addressable_content_streaming"` with `3/3` tests passing. Phase 5 adds `RuntimeSandboxWorldStreamingSourceRow`, `RuntimeSandboxWorldAddressableDependencyRow`, `RuntimeSandboxWorldStreamingPlan`, `RuntimeSandboxWorldStreamingSafePointDesc`, `RuntimeSandboxWorldStreamingSafePointResult`, `plan_runtime_sandbox_world_streaming`, and `execute_runtime_sandbox_world_streaming_safe_point`. Outputs cover source-range chunk selection, dirty resident chunk pinning, addressable dependency rows, resident budget diagnostics, and reviewed world-region safe-point adoption. Background streaming, async addressable execution, automatic LRU eviction, renderer/RHI residency, native handles, and game-specific biome/block-art ownership remain outside this phase.

## Phase 6 - Production Tile Renderer And 2D Lighting Evidence

**Goal:** Render large tile chunks with deterministic batching, dirty-region rebuilds, layer ordering, lightmap submission, and backend-local package evidence.

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/tile_chunk_renderer.hpp`
- Create: `engine/renderer/src/tile_chunk_renderer.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Create: `tests/unit/renderer_tile_chunk_renderer_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`

- [x] Re-check D3D12 and Vulkan synchronization docs before GPU-visible tile upload or lightmap work.
- [x] Add RED tests for chunk mesh generation: visible cells only, atlas UV rows, layer sorting, material grouping, transparent/opaque split, and deterministic draw rows.
- [x] Add RED tests for dirty-region rebuild budgets: partial chunk rebuild, full chunk rebuild, no-op clean chunk, and over-budget diagnostics.
- [x] Add RED tests for lightmap rows: light cell grid, changed light region, atlas/light texture dependency, and renderer-neutral submission. Do not implement backend-native texture ownership in `MK_runtime`.
- [x] Add package smoke counters under the selected 2D sample only after focused tests pass.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_renderer_tile_chunk_renderer_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "renderer_tile_chunk_renderer"
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package"
```

Evidence: RED compile proof first failed with unresolved `mirakana/renderer/tile_chunk_renderer.hpp` after registering `MK_renderer_tile_chunk_renderer_tests`. Context7 official CMake, Direct3D 12 barrier/state-transition, and Vulkan synchronization2 barrier/layout/access-mask references were rechecked before choosing a renderer-neutral value planner rather than backend upload ownership. GREEN focused validation then passed `MK_renderer_tile_chunk_renderer_tests` and `ctest -R "renderer_tile_chunk_renderer"` under `dev`, and `sample_2d_desktop_runtime_package` build plus `ctest -R "sample_2d_desktop_runtime_package"` under `desktop-runtime`. Manual package-smoke proof with `--require-production-tile-renderer` reported `tile_chunk_renderer_status=ready`, visible/opaque/transparent cells `3/2/1`, sprite rows `3`, draw rows `2`, texture binds `1`, dirty rebuild rows/cells `2/5`, light rows `3`, changed light cells `1`, diagnostics `0`, and zero backend-specific submission or native texture ownership invocation. Phase 6 adds `TileChunkCellRow`, `TileChunkDirtyRegion`, `TileChunkLightCellRow`, `TileChunkRendererDesc`, `TileChunkRendererPlan`, `TileChunkSpriteRow`, `TileChunkDrawRow`, `TileChunkDirtyRebuildRow`, `TileChunkLightmapRow`, and `plan_tile_chunk_renderer` in `MK_renderer`. Backend-native texture ownership, backend-specific submission, runtime renderer/RHI residency, public native handles, SDL3, and broad renderer quality remain outside this phase.

## Phase 7 - Sandbox Authoring And Cook Pipeline

**Goal:** Provide reviewed authoring and cook surfaces for tiles, palettes, chunk templates, procedural seeds, object placement, and package registration.

**Files:**
- Create: `engine/tools/include/mirakana/tools/sandbox_world_authoring.hpp`
- Create: `engine/tools/asset/sandbox_world_authoring.cpp`
- Modify: `engine/tools/include/mirakana/tools/tilemap_tool.hpp`
- Modify: `engine/tools/asset/tilemap_tool.cpp`
- Modify: `engine/tools/asset/CMakeLists.txt`
- Create: `tests/unit/tools_sandbox_world_authoring_tests.cpp`
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`

**Official Evidence:** 2026-05-30 implementation refreshed Context7 `/websites/unity3d_manual` for Unity Tilemap/Tile Palette/Tile Asset authoring concepts and kept this phase as first-party rows rather than an editor-specific clone; Unity Addressables official manual (<https://docs.unity3d.com/ja/6000.0/Manual/com.unity.addressables.html>) for address/dependency package intent without adopting Unity runtime APIs; Context7 `/websites/cmake_cmake_help` for target/test registration through `target_sources`, `target_link_libraries`, and `add_test(NAME ... COMMAND ...)`; and Context7 `/microsoft/vcpkg` for preserving manifest-feature/dependency ownership without adding new dependencies. Phase 7 therefore adds `MK_tools` value review/apply APIs and selected package smoke counters only; it rejects external image decoding, external downloads, arbitrary importer plugins, runtime source parsing, renderer/RHI residency, native handles, and package apply during review.

- [x] Re-check Unity Tilemaps, Unity Addressables, CMake, vcpkg, and asset/import official practice anchors before changing authoring or package metadata.
- [x] Add RED tests for tile definition documents: tile id, atlas frame, collision kind, material tags, light/liquid/update policy, localization key, accessibility label key for UI-facing tiles, and provenance/license rows.
- [x] Add RED tests for palette/brush rows: brush id, shape, layer mask, replacement policy, symmetry, fill policy, and invalid path diagnostics.
- [x] Add RED tests for chunk template and procedural seed rows: stable generator id, seed, dimensions, allowed tile sets, object placement rules, and deterministic preview hash.
- [x] Add RED tests for package update dry-run/apply rows using `IFileSystem`, safe paths, deterministic changed files, and package index dependency edges.
- [x] Implement authoring APIs. Keep actual external image decoding, external downloads, and arbitrary importer plugins rejected unless a separate dependency phase implements them.
- [x] Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_sandbox_world_authoring_tests MK_tools_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "tools_sandbox_world_authoring|tools"
```

Evidence: RED compile proof first failed on missing `mirakana/tools/sandbox_world_authoring.hpp` after registering `MK_tools_sandbox_world_authoring_tests`. GREEN focused validation then passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_sandbox_world_authoring_tests MK_tools_tests` and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "tools_sandbox_world_authoring|MK_tools_tests"`. Desktop-runtime focused validation passed `tools/prepare-worktree.ps1`, `tools/cmake.ps1 --preset desktop-runtime`, `tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package`, `tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package"`, and a manual package smoke with `--require-sandbox-authoring-review`. The smoke reported `sandbox_authoring_review_status=ready`, tile definition/palette brush/chunk template/procedural seed rows `2/1/1/1`, changed files `3`, tilemap package changed files `2`, dependency edges `1`, positive preview hash, zero external image decoding/download/importer plugin/package apply during review invocation, and zero diagnostics.

## Phase 8 - Generic Gameplay Integration Rows

**Goal:** Connect mutable worlds to existing generic inventory, crafting, quest/dialogue, AI, perception, spawning, and interaction rows without moving game balance into the engine.

**Files:**
- Modify: `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp`
- Modify: `engine/runtime/src/genre_sandbox_world.cpp`
- Modify: `engine/runtime/include/mirakana/runtime/gameplay_interaction.hpp` if existing extension points require it.
- Modify: `tests/unit/runtime_genre_sandbox_world_tests.cpp`
- Modify selected runtime gameplay tests that already own inventory/crafting/spawn rows.

**Official Evidence:** 2026-05-30 implementation refreshed Context7 `/websites/cmake_cmake_help` for official CMake/CTest test registration practice (`add_test(NAME ... COMMAND ...)`) and kept the phase on existing first-party `MK_runtime`/sample package targets rather than adding dependencies, SDK adapters, or middleware. No new third-party library, package manager feature, renderer/RHI API, platform SDK surface, or SDL3 adapter was introduced; the phase extends value-only runtime rows and selected package counters.

- [x] Add RED tests for engine-owned generic hooks: tile drops as data rows, construction cost consumption, tool effectiveness category, spawn-region rows, day/night event rows, and trigger rows.
- [x] Add RED tests that reject game-specific content names, hardcoded damage formulas, boss/NPC catalogs, and economy balance inside engine modules.
- [x] Implement generic row production only. Actual game rules consume rows in `games/<game_name>/`.
- [x] Add package counters to `sample_2d_desktop_runtime_package` only for generic hooks.

Evidence: RED compile proof first failed on missing `RuntimeSandboxDayNightPhase`, `RuntimeSandboxTriggerKind`, `RuntimeSandboxTileDropRow`, `RuntimeSandboxToolEffectivenessRow`, `RuntimeSandboxSpawnRegionRow`, `RuntimeSandboxDayNightEventRow`, `RuntimeSandboxTriggerRow`, and new `RuntimeSandboxWorldMutationRequest` / `RuntimeSandboxWorldMutationPlan` members. GREEN focused validation passed `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_genre_sandbox_world_tests`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "runtime_genre_sandbox_world"`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset desktop-runtime`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset desktop-runtime --target sample_2d_desktop_runtime_package`, `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset desktop-runtime --output-on-failure -R "sample_2d_desktop_runtime_package"`, and `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`. A direct source-tree smoke with the registered 2D package args reported `sandbox_world_cost_consumption_rows=1`, `sandbox_world_tile_drop_rows=1`, `sandbox_world_tool_effectiveness_rows=1`, `sandbox_world_spawn_region_rows=1`, `sandbox_world_day_night_event_rows=1`, `sandbox_world_trigger_rows=1`, positive `sandbox_world_replay_hash`, zero world/persistence/package side-effect counters, and `sandbox_world_diagnostics=0`; installed package validation also passed for `sample_2d_desktop_runtime_package`. The negative runtime test rejects boss/NPC/damage-formula/economy hook ids before output rows, and the engine still does not execute inventory/crafting/spawn/dialogue/AI rules, mutate worlds from review, write persistence, call renderer/platform/editor APIs, expose native handles, or reintroduce SDL3.

## Phase 9 - Optional Multiplayer And Modding Gate

**Goal:** Define the host-gated path for replicating world mutations and optional scripting/modding without making broad online or modding claims.

**Files:**
- Modify only after threat model selection: `engine/runtime/include/mirakana/runtime/production_network_replication.hpp`
- Modify only after threat model selection: `engine/runtime/src/production_network_replication.cpp`
- Modify optional: `engine/runtime/network/enet/`
- Modify optional: `engine/runtime/include/mirakana/runtime/scripting_sandbox.hpp`
- Modify tests: `tests/unit/runtime_production_network_replication_tests.cpp`, `tests/unit/runtime_network_*`, `tests/unit/runtime_scripting_sandbox_tests.cpp`
- Create docs only when selected: `docs/specs/2026-05-27-sandbox-world-network-security-threat-model.md`

- [x] Write the threat model before any network execution code. Cover client authority, tile mutation tampering, replay attacks, rollback windows, bandwidth abuse, chunk streaming abuse, save corruption, authentication exclusions, encryption exclusions, NAT/matchmaking exclusions, and server authority assumptions.
- [x] Add RED tests for serialized mutation command validation, sequence/replay rejection, snapshot delta rows, rollback-window diagnostics, authority diagnostics, and transport host evidence.
- [x] Keep ENet optional behind the existing `network-enet` feature and validation wrapper.
- [x] Add scripting/modding only as reviewed policy and deterministic adapter rows. Filesystem, network, process, native plugin, and package script access remain denied by default.

Evidence: Phase 9 selected and implemented through `docs/superpowers/plans/2026-05-30-sandbox-world-network-modding-gate-v1.md`. It adds `RuntimeNetworkSandboxMutationCommandRow`, `RuntimeNetworkSandboxSnapshotDeltaRow`, `RuntimeScriptModdingAdapterPolicyRow`, `RuntimeScriptModdingDeniedCapabilityRow`, and `plan_runtime_script_modding_policy`, with sandbox mutation authority, unique command ids, sequence uniqueness, monotonic tick, snapshot delta reference/hash/shared-byte-budget, reviewed deterministic adapter, replay-seed, and denied capability tests. Focused build/CTest and full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed locally; PR #312 merged at `0fa26b1823d6bb4b68837f19b4040ede781e09b9` after hosted `PR Gate`, `Windows MSVC`, Linux, macOS Metal CMake, iOS smoke, static analysis, and CodeQL checks succeeded. ENet remains optional/host-gated, SDL3 remains absent, and broad online multiplayer/modding readiness remains unclaimed.

## Phase 10 - Sample Package, Validation Recipes, And Performance Budgets

**Goal:** Prove the lane through package-visible counters, smoke commands, and deterministic budgets.

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Optionally create: `games/generic_2d_sandbox_probe/`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`

- [x] Select Phase 10 through child plan `docs/superpowers/plans/2026-05-30-sandbox-world-package-validation-performance-budgets-v1.md`.
- [x] Add smoke flags only after each underlying phase is green. Candidate flags: `--require-win32-runtime-host`, `--require-win32-d3d12-presentation`, `--require-wasapi-audio`, `--require-sandbox-world-runtime`, `--require-sandbox-world-persistence`, `--require-sandbox-world-streaming`, `--require-production-tile-renderer`, `--require-sandbox-authoring-review`, and `--require-sandbox-package-budgets`.
- [x] Add package-visible counters for rows, diagnostics, dirty chunks, chunk bytes, renderer draw rows, tile draw calls, light rows, persisted chunks, streaming loads/unloads, replay hashes, and unsupported side-effect counters.
- [x] Add over-budget negative probes that fail closed when row counts or byte counts exceed declared budgets.
- [x] Run selected package proof:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
out\install\desktop-runtime-release\bin\sample_2d_desktop_runtime_package.exe --smoke --require-config runtime/sample_2d_desktop_runtime_package.config --require-scene-package runtime/sample_2d_desktop_runtime_package.geindex --require-d3d12-shaders --require-win32-runtime-host --require-win32-d3d12-presentation --require-wasapi-audio --require-sandbox-world-runtime --require-sandbox-world-persistence --require-sandbox-world-streaming --require-production-tile-renderer --require-sandbox-authoring-review --require-sandbox-package-budgets
```

Expected: package stdout includes positive ready counters for implemented phases and explicit zero counters for forbidden package IO, native handle exposure, arbitrary shell execution, external downloads, broad multiplayer, and broad renderer quality.

## Phase 11 - Docs, Manifest, Static Checks, And Closeout

**Goal:** Publish only the evidence that exists, keep AI-operable contracts honest, and preserve host gates.

**Files:**
- Modify: `docs/current-capabilities.md`
- Modify: `docs/roadmap.md`
- Modify: `docs/ai-game-development.md`
- Modify: `docs/testing.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/004-modules.json`
- Modify: `engine/agent/manifest.fragments/007-importerCapabilities.json`
- Modify: `engine/agent/manifest.fragments/009-validationRecipes.json`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Modify: `engine/agent/manifest.fragments/014-gameCodeGuidance.json`
- Generate: `engine/agent/manifest.json`
- Modify static checks only for new machine-readable literals.

- [x] Update current capabilities with exact supported boundaries. Do not write that the engine supports Terraria-level games broadly; write the exact generic systems proven by validation.
- [x] Update manifest fragments and compose output:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
```

- [x] Run targeted drift checks:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-agents.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1
git diff --check
```

- [x] Run full validation for runtime/public-contract closeout:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

- [ ] Publish each coherent implementation phase through GitHub Flow with one reviewable PR per phase or tightly related phase group. Do not push directly to `main`, force-push, or mark host-gated lanes ready without hosted or local host evidence.

## Done When

- `MK_runtime` owns a tested chunked mutable 2D sandbox world model, mutation executor, dirty-region stream, simulation primitive rows, persistence review/apply path, and streaming safe-point integration.
- `MK_renderer` owns package-proven tile chunk draw planning, dirty rebuild evidence, tile/light rows, and high-density budget counters without exposing RHI/backend handles.
- `MK_tools` owns reviewed tile/world authoring and package update surfaces with safe paths, deterministic changed files, provenance/license rows, and no arbitrary importer execution.
- The selected 2D package proves the lane with smoke flags, stdout counters, and installed validation fields.
- Docs, manifest fragments, validation recipes, and static checks match exactly what the code proves.
- Unsupported claims remain explicit: broad multiplayer, cloud save, mobile parity, production source image import, broad renderer quality, full editor productization, and game-specific content production stay out of the ready claim until separate plans prove them.
- The production lane remains clean-break and first-party: no legacy desktop middleware dependency, public native handle exposure, or compatibility alias is introduced to make old runtime-host behavior appear supported.
