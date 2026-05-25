# General Purpose Game Production v1 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Plan ID:** `general-purpose-game-production-v1`

**Status:** Active.

**Goal:** Add a post-foundation production track that turns the completed 1.x foundations into reusable, production-oriented game creation surfaces for RPG, sandbox, simulation, and broad 2D/3D games.

**Architecture:** This is a clean-breaking production milestone layered over the completed `implemented-1x-foundation` rows. It does not reopen `foundation-ready` or historical `candidate` work; it adds new canonical production rows, then promotes one focused production surface at a time through first-party public contracts, package-visible evidence, manifests, docs, and static checks.

**Tech Stack:** C++23, PowerShell 7 repository tools, target-based CMake install/export with `FILE_SET CXX_MODULES` where applicable, vcpkg manifest features with a pinned `builtin-baseline`, SDL3 only behind platform/runtime-host adapters, backend-private D3D12/Vulkan/Metal ownership, JSON schemas, `game.agent.json`, and `engine/agent/manifest.fragments`.

---

## Decision

Do not add new work back into `foundation-ready`. That status described rows whose narrow foundations were not yet promoted; all such rows are now complete. Do not reopen completed candidate burn-down evidence either. The correct post-foundation shape is a new production track with explicit production statuses:

- `production-candidate`: proposed reusable production surface.
- `selected-production-slice`: active production phase selected by a dated plan.
- `implemented-production-surface`: production surface with tests, package evidence, docs, manifest rows, and static checks.
- `host-gated-production`: production surface that needs backend, platform, SDK, device, or toolchain evidence before ready claims.

This plan selects the new `general-purpose-game-production-v1` milestone. Each phase may land through a separate PR, but the dated plan stays the milestone record while related phases share one architecture decision: clean, first-party, backend-neutral production surfaces for general game creation.

## Official Practice Baseline

Every implementation phase starts by recording current official guidance for the touched surface. The phase evidence must cite the exact official page or Context7 result used.

- CMake: use target-based usage requirements, install/export packages, package config/version files, CTest/CPack wrappers, and explicit C++ module file-set behavior. Source anchors include CMake Presets, packages, install, and `cmake-cxxmodules(7)`.
- vcpkg: use manifest mode, pinned `builtin-baseline`, additive features, explicit bootstrap through `tools/bootstrap-deps.ps1`, and no CMake configure-time package install.
- SDL3: keep windows, input, clipboard, text input, audio, dialogs, and GPU/window ownership inside platform/runtime-host adapters. Public runtime/gameplay/UI APIs must not expose SDL headers or SDL handles.
- D3D12/Vulkan/Metal: keep native resources and synchronization backend-private. D3D12 proof does not imply Vulkan or Metal readiness; Vulkan requires validation/SPIR-V evidence and Metal requires Apple host evidence.
- Large worlds: follow the architecture pattern used by Unreal World Partition and data layers, but implement first-party region/chunk ownership rather than Unreal-specific concepts.
- Addressable assets: follow the production shape used by Unity Addressables, but expose first-party package/address/dependency/refcount/release contracts rather than Unity-specific APIs.
- Data-oriented scale: follow the data-oriented separation used by Unity Entities where useful, but keep public contracts in `mirakana::` value types and avoid ECS vocabulary unless the implementation actually adopts an ECS.
- UI tooling: follow the retained UI direction already selected in the project docs; Dear ImGui remains editor/debug shell only, not runtime game UI.
- C++ ownership: use RAII, value types, `std::unique_ptr` for ownership, and no compatibility shims for old public APIs unless a later release policy explicitly requires them.

## Production Capability Rows

Add the following canonical rows to the backlog under a new `General-Purpose Game Production` group. These are production candidates until their phase lands:

| Capability id | Initial status | Purpose |
| --- | --- | --- |
| `general-purpose-game-production-v1` | `selected-production-slice` | Milestone row selecting this post-foundation production track. |
| `gameplay-runtime-scheduler-production-v1` | `implemented-production-surface` | Authoritative fixed-tick gameplay scheduler with explicit system order, command playback, replay diagnostics, budget rows, pause/step policies, and rollback/network extension points. |
| `world-entity-model-production-v1` | `production-candidate` | Stable entity/component/region ownership model with lifecycle, identity, serialization boundaries, and scene/runtime bridge diagnostics. |
| `addressable-content-streaming-production-v1` | `production-candidate` | Addressable package/content handles with dependency tracking, explicit load/release/refcount plans, resident budget diagnostics, and package evidence. |
| `production-authoring-workflows-v1` | `production-candidate` | Reviewed authoring flows for scene, placement, quest/dialogue, item/economy, AI behavior, world regions, and validation repair without free-form engine mutation. |
| `production-runtime-ui-workbench-v1` | `production-candidate` | Dense runtime UI primitives for menus, inventory/equipment/shop, simulation dashboards, tables, graphs, focus navigation, text input, localization, and accessibility boundaries. |
| `genre-rpg-systems-pack-v1` | `production-candidate` | Reusable RPG systems for stats, progression, skills, equipment, party/enemy combat loops, rewards, and save validation. |
| `genre-sandbox-world-pack-v1` | `production-candidate` | Reusable sandbox systems for block/voxel-like world chunks, placement/destruction rules, construction costs, world mutation validation, and persistence. |
| `genre-simulation-management-pack-v1` | `production-candidate` | Reusable simulation systems for economy, logistics, jobs, population/needs, production chains, schedules, and deterministic long-run validation. |
| `production-network-replication-v1` | `production-candidate` | Authoritative session, replication, rollback/lockstep hooks, security/threat model, and real transport host evidence. |
| `production-rendering-vfx-profiling-v1` | `host-gated-production` | Broader renderer/VFX/profile production evidence, including GPU particles, richer postprocess, backend-specific timing, crash/telemetry handoff, and strict backend parity gates. |

## Long-Running Execution Strategy

Execute this milestone as ten reviewable candidate slices after Phase 0 selection:

1. `gameplay-runtime-scheduler-production-v1`
2. `world-entity-model-production-v1`
3. `addressable-content-streaming-production-v1`
4. `production-authoring-workflows-v1`
5. `production-runtime-ui-workbench-v1`
6. `genre-rpg-systems-pack-v1`
7. `genre-sandbox-world-pack-v1`
8. `genre-simulation-management-pack-v1`
9. `production-network-replication-v1`
10. `production-rendering-vfx-profiling-v1`

Each candidate gets its own validated commit. Push, PR, hosted validation, merge, and main-sync happen per candidate when the candidate is independently reviewable. If two adjacent candidates become tightly coupled during implementation, keep multiple validated commits but publish them in one reviewable PR with the coupling reason recorded in the PR body and plan evidence.

Use a linked worktree branch (`codex/general-purpose-production-v1` or a candidate-specific `codex/<candidate-id>` branch) rather than working directly on `main`. Use subagents for bounded read-only research, spec/agent-surface audit, implementation sidecars with disjoint write scopes, and C++/rendering review; close each subagent after its result is consumed. The controller owns sequencing, integration, final validation, commit, push, and PR state.

Every candidate starts with current official practice evidence. Prefer Context7 for CMake, vcpkg, SDL3, Direct3D 12, Vulkan, Metal, and C++ tooling documentation where available, official vendor documentation otherwise, and repository skills for project-local rules. Do not use compatibility shims or deprecated aliases when a clean breaking value contract is better for the greenfield production model.

## File Structure

The milestone itself changes documentation and manifest pointers first. Later phases add C++ contracts in the owning module:

- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
  - Add production status vocabulary and the `General-Purpose Game Production` canonical row group.
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
  - Add a `General-Purpose Game Production Track` projection and evidence rules.
- Create: `docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md`
  - This milestone plan.
- Modify: `docs/superpowers/plans/README.md`
  - Select this milestone as the active plan and keep the production master plan as parent roadmap.
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
  - Update current verdict text to point at this selected production slice.
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
  - Set `currentActivePlan` and `recommendedNextPlan` to this milestone, then compose `engine/agent/manifest.json`.
- Later phase files:
  - `engine/runtime/include/mirakana/runtime/gameplay_runtime_scheduler.hpp`
  - `engine/runtime/src/gameplay_runtime_scheduler.cpp`
  - `engine/runtime/include/mirakana/runtime/world_entity_model.hpp`
  - `engine/runtime/src/world_entity_model.cpp`
  - `engine/runtime/include/mirakana/runtime/addressable_content_streaming.hpp`
  - `engine/runtime/src/addressable_content_streaming.cpp`
  - `engine/ui/include/mirakana/ui/runtime_ui_workbench.hpp`
  - `engine/ui/src/runtime_ui_workbench.cpp`
  - `engine/tools/include/mirakana/tools/production_authoring_workflows.hpp`
  - `engine/tools/asset/production_authoring_workflows.cpp`
  - `engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp`
  - `engine/runtime/src/genre_rpg_systems.cpp`
  - `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp`
  - `engine/runtime/src/genre_sandbox_world.cpp`
  - `engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp`
  - `engine/runtime/src/genre_simulation_management.cpp`
  - `engine/runtime/include/mirakana/runtime/production_network_replication.hpp`
  - `engine/runtime/src/production_network_replication.cpp`
  - `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
  - `engine/renderer/src/production_vfx_profiling.cpp`
  - Focused tests under `tests/unit/*_tests.cpp`.

## Phase 0: Select The Production Track

**Files:**
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md`
- Create: `docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`

- [ ] **Step 1: Record official practice anchors**

  Add or preserve official anchors for CMake, vcpkg, SDL3, D3D12, Vulkan, Metal, Android GameActivity, Unreal World Partition, Unity Addressables, Unity Entities, and C++ ownership in the plan or projection chapter.

- [ ] **Step 2: Add production status vocabulary**

  Add these rows to the backlog status table:

  ```markdown
  | `production-candidate` | A post-foundation reusable production surface proposed by a dated plan; it is not implemented until a selected phase lands. |
  | `selected-production-slice` | The active production surface or milestone selected by `currentActivePlan`. |
  | `implemented-production-surface` | A reusable production surface with tests, package evidence, docs, manifest rows, and static checks. |
  | `host-gated-production` | Production readiness depends on backend, platform, SDK, device, or toolchain evidence. |
  ```

- [ ] **Step 3: Add canonical production rows**

  Add the rows listed in **Production Capability Rows** to the backlog under `### General-Purpose Game Production`.

- [ ] **Step 4: Add the projection track**

  Add `## General-Purpose Game Production Track` to the projection chapter. The track must state that the row group is the fifth production track and that it composes existing implemented foundations without reopening them.

- [ ] **Step 5: Select this plan in the registry and manifest**

  Update the plan registry and manifest fragment so `currentActivePlan` points to:

  ```text
  docs/superpowers/plans/2026-05-25-general-purpose-game-production-v1.md
  ```

- [ ] **Step 6: Compose and verify agent surfaces**

  Run:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
  ```

  Expected: all commands pass. If a static guard expects the master plan to remain the active pointer, update the guard to accept this new selected production milestone without weakening validation.

## Phase 1: Gameplay Runtime Scheduler Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/gameplay_runtime_scheduler.hpp`
- Create: `engine/runtime/src/gameplay_runtime_scheduler.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_gameplay_scheduler_tests.cpp`
- Update: manifest/docs/validation recipes when evidence lands.

- [x] **Step 1: Write failing scheduler tests**

  Add tests that construct three systems, two input commands, and a fixed tick budget. Assert deterministic system order, command playback before system ticks, clamped step count, and replay hash stability.

  ```cpp
  TEST(RuntimeGameplaySchedulerTests, PlansFixedTicksInStableSystemOrder) {
      mirakana::runtime::RuntimeGameplaySchedulerRequest request{};
      request.tick_delta_seconds = 1.0 / 60.0;
      request.accumulated_seconds = 3.0 / 60.0;
      request.max_steps = 2;
      request.systems = {
          {.id = "physics", .order = 20, .enabled = true, .budget_us = 400},
          {.id = "input", .order = 10, .enabled = true, .budget_us = 100},
          {.id = "ai", .order = 30, .enabled = true, .budget_us = 300},
      };
      request.commands = {
          {.id = "move", .target_tick = 42, .payload_hash = 1001},
          {.id = "interact", .target_tick = 43, .payload_hash = 1002},
      };

      const auto plan = mirakana::runtime::plan_runtime_gameplay_schedule(request);

      ASSERT_EQ(plan.status, mirakana::runtime::RuntimeGameplaySchedulerStatus::ready);
      EXPECT_EQ(plan.steps.size(), 2u);
      EXPECT_EQ(plan.steps[0].system_rows[0].system_id, "input");
      EXPECT_EQ(plan.steps[0].system_rows[1].system_id, "physics");
      EXPECT_EQ(plan.steps[0].system_rows[2].system_id, "ai");
      EXPECT_EQ(plan.steps[0].command_rows.size(), 1u);
      EXPECT_EQ(plan.remaining_seconds, 1.0 / 60.0);
      EXPECT_NE(plan.replay_hash, 0u);
      EXPECT_TRUE(plan.diagnostics.empty());
  }
  ```

- [x] **Step 2: Run focused tests and verify failure**

  Run:

  ```powershell
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev
  pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_scheduler_tests
  ```

  Expected: compile failure for missing scheduler types or a failing runtime scheduler test.

- [x] **Step 3: Implement the public scheduler contract**

  Add value types:

  ```cpp
  namespace mirakana::runtime {
  enum class RuntimeGameplaySchedulerStatus { ready, invalid_request, budget_limited };
  struct RuntimeGameplaySystemDesc { std::string id; int order{}; bool enabled{true}; std::uint32_t budget_us{}; };
  struct RuntimeGameplayInputCommandDesc { std::string id; std::uint64_t target_tick{}; std::uint64_t payload_hash{}; };
  struct RuntimeGameplaySchedulerRequest {
      double tick_delta_seconds{};
      double accumulated_seconds{};
      std::uint32_t max_steps{};
      std::vector<RuntimeGameplaySystemDesc> systems;
      std::vector<RuntimeGameplayInputCommandDesc> commands;
  };
  struct RuntimeGameplaySchedulerSystemRow { std::string system_id; int order{}; std::uint32_t budget_us{}; };
  struct RuntimeGameplaySchedulerStepRow {
      std::uint64_t tick{};
      std::vector<RuntimeGameplayInputCommandDesc> command_rows;
      std::vector<RuntimeGameplaySchedulerSystemRow> system_rows;
  };
  struct RuntimeGameplaySchedulerDiagnostic { std::string code; std::string message; };
  struct RuntimeGameplaySchedulerPlan {
      RuntimeGameplaySchedulerStatus status{RuntimeGameplaySchedulerStatus::invalid_request};
      std::vector<RuntimeGameplaySchedulerStepRow> steps;
      std::vector<RuntimeGameplaySchedulerDiagnostic> diagnostics;
      double consumed_seconds{};
      double remaining_seconds{};
      std::uint64_t replay_hash{};
  };
  [[nodiscard]] RuntimeGameplaySchedulerPlan plan_runtime_gameplay_schedule(const RuntimeGameplaySchedulerRequest& request);
  }
  ```

- [x] **Step 4: Pass focused tests**

  Run the same focused build and CTest commands. Expected: runtime tests pass.

- [x] **Step 5: Add package-visible scheduler evidence**

  Extend the selected 2D and 3D package smoke paths with counters:

  ```text
  gameplay_runtime_scheduler_ready=1
  gameplay_runtime_scheduler_steps=2
  gameplay_runtime_scheduler_system_rows=6
  gameplay_runtime_scheduler_command_rows=2
  gameplay_runtime_scheduler_budget_limited=1
  gameplay_runtime_scheduler_diagnostics=0
  ```

- [x] **Step 6: Close Phase 1**

  Update docs, manifest fragments, generated-game guidance, and static checks for the scheduler counters. Run focused package validation and then `tools/validate.ps1` because this phase changes runtime C++ and package contracts.

  Evidence captured for the closeout gate:

  ```text
  RED: MK_runtime_gameplay_scheduler_tests failed to build before mirakana/runtime/gameplay_runtime_scheduler.hpp existed.
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_scheduler_tests
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_gameplay_scheduler_tests
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_gameplay_scheduler_tests sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R "MK_runtime_gameplay_scheduler_tests|sample_2d_desktop_runtime_package_smoke|sample_generated_desktop_runtime_3d_package_smoke"
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files engine/runtime/src/gameplay_runtime_scheduler.cpp,games/sample_2d_desktop_runtime_package/main.cpp,games/sample_generated_desktop_runtime_3d_package/main.cpp,tests/unit/runtime_gameplay_scheduler_tests.cpp
  GREEN: pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
  DIRECT: sample_2d_desktop_runtime_package and sample_generated_desktop_runtime_3d_package direct package-smoke invocations emitted gameplay_runtime_scheduler_status=budget_limited, gameplay_runtime_scheduler_ready=1, gameplay_runtime_scheduler_steps=2, gameplay_runtime_scheduler_system_rows=6, gameplay_runtime_scheduler_command_rows=2, positive gameplay_runtime_scheduler_replay_hash, and gameplay_runtime_scheduler_diagnostics=0.
  ```

## Phase 2: World Entity Model Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/world_entity_model.hpp`
- Create: `engine/runtime/src/world_entity_model.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_tests.cpp`

- [ ] **Step 1: Write failing identity/lifecycle tests**

  Test stable entity ids, component schema ids, region ownership, spawn/despawn rows, and duplicate id diagnostics. The smallest durable guarantee is that the model can validate a world snapshot without scene mutation.

- [ ] **Step 2: Implement first-party world/entity value rows**

  Add `RuntimeWorldEntityId`, `RuntimeWorldRegionId`, `RuntimeWorldComponentSchemaId`, `RuntimeWorldEntityRow`, `RuntimeWorldComponentRow`, `RuntimeWorldEntityLifecycleRequest`, `RuntimeWorldEntityLifecyclePlan`, and `plan_runtime_world_entity_lifecycle`.

- [ ] **Step 3: Bridge to persistence and streaming**

  Add review-only adapters from existing simulation persistence and world-region streaming rows. The bridge must not load packages, mutate scenes, upload renderer resources, or expose native handles.

- [ ] **Step 4: Add package evidence**

  Add counters for validated entity rows, component rows, region ownership rows, lifecycle rows, rejected duplicate ids, and clean diagnostics.

## Phase 3: Addressable Content Streaming Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/addressable_content_streaming.hpp`
- Create: `engine/runtime/src/addressable_content_streaming.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_tests.cpp`

- [ ] **Step 1: Write failing address/dependency tests**

  Test deterministic address lookup, dependency rows, explicit load handles, explicit release rows, reference counts, budget rejection, and missing dependency diagnostics.

- [ ] **Step 2: Implement addressable value contracts**

  Add `RuntimeAddressableAssetId`, `RuntimeAddressableLoadRequest`, `RuntimeAddressableLoadPlan`, `RuntimeAddressableReleaseRequest`, `RuntimeAddressableReleasePlan`, and dependency/refcount diagnostics over existing runtime package/catalog rows.

- [ ] **Step 3: Keep async execution host-owned**

  The production contract may plan asynchronous work, but actual threads, file watching, package reads, renderer residency, and RHI uploads remain behind reviewed host/runtime safe points.

- [ ] **Step 4: Prove package counters**

  Add selected package counters for address rows, dependency rows, planned loads, releases, resident byte estimates, and clean diagnostics.

## Phase 4: Production Authoring Workflows

**Files:**
- Create: `engine/tools/include/mirakana/tools/production_authoring_workflows.hpp`
- Create: `engine/tools/asset/production_authoring_workflows.cpp`
- Modify: `engine/tools/asset/CMakeLists.txt`
- Test: `tests/unit/tools_tests.cpp`

- [ ] **Step 1: Write failing workflow review tests**

  Test reviewed authoring requests for scene placement, quest/dialogue, item/economy, AI behavior, world region, and validation repair rows. Reject shared-surface mutation, arbitrary shell commands, cooked package mutation, and native/backend terms.

- [ ] **Step 2: Implement workflow review contracts**

  Add `ProductionAuthoringWorkflowRequest`, `ProductionAuthoringWorkflowRow`, `ProductionAuthoringWorkflowReviewResult`, and `review_production_authoring_workflow`.

- [ ] **Step 3: Connect to editor without engine mutation**

  Add retained editor rows only after the tool review contract lands. Editor UI remains a caller of reviewed rows; it must not become an execution backdoor.

## Phase 5: Production Runtime UI Workbench

**Files:**
- Create: `engine/ui/include/mirakana/ui/runtime_ui_workbench.hpp`
- Create: `engine/ui/src/runtime_ui_workbench.cpp`
- Modify: `engine/ui/CMakeLists.txt`
- Test: `tests/unit/ui_tests.cpp`

- [ ] **Step 1: Write failing dense UI model tests**

  Test table rows, graph series, inventory/equipment/shop rows, focus navigation, text input request rows, localization keys, and accessibility payload references.

- [ ] **Step 2: Implement retained UI workbench value types**

  Add `RuntimeUiWorkbenchDocument`, `RuntimeUiWorkbenchPanelRow`, `RuntimeUiWorkbenchTableRow`, `RuntimeUiWorkbenchGraphSeries`, `RuntimeUiWorkbenchFocusPlan`, and `plan_runtime_ui_workbench`.

- [ ] **Step 3: Keep rendering/adapters separate**

  UI workbench rows describe validated runtime UI intent. Text shaping, font rasterization, IME, OS accessibility, image decoding, and renderer texture upload stay behind the existing adapter boundaries until selected production phases promote them.

## Phase 6: RPG Systems Pack Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/genre_rpg_systems.hpp`
- Create: `engine/runtime/src/genre_rpg_systems.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_genre_rpg_systems_tests.cpp`
- Package evidence: selected 2D and 3D package counters.

- [ ] **Step 1: Write failing RPG system tests**

  Test stat rows, progression rows, skill unlock prerequisites, equipment slot validation, party/enemy combat-loop intents, reward rows, and save-validation summaries. The tests must use generic engine vocabulary and must not encode story, balance, enemy names, or genre-specific content values.

- [ ] **Step 2: Implement first-party RPG value contracts**

  Add `RuntimeRpgStatRow`, `RuntimeRpgProgressionRow`, `RuntimeRpgSkillRow`, `RuntimeRpgEquipmentRow`, `RuntimeRpgCombatLoopRequest`, `RuntimeRpgRewardRow`, `RuntimeRpgSystemsPlan`, and `plan_runtime_rpg_systems`.

- [ ] **Step 3: Add package evidence and manifest rows**

  Add selected package counters for validated stats, progression rows, skills, equipment rows, combat-loop rows, rewards, save-validation rows, and clean diagnostics.

## Phase 7: Sandbox World Pack Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/genre_sandbox_world.hpp`
- Create: `engine/runtime/src/genre_sandbox_world.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_genre_sandbox_world_tests.cpp`
- Package evidence: selected package validation recipes.

- [ ] **Step 1: Write failing sandbox world tests**

  Test chunk/region identity, placement intent validation, destruction intent validation, construction cost rows, world mutation review rows, persistence bridge rows, and rejection of game-owned biome/content rules inside engine contracts.

- [ ] **Step 2: Implement sandbox world value contracts**

  Add `RuntimeSandboxChunkRow`, `RuntimeSandboxPlacementIntent`, `RuntimeSandboxDestructionIntent`, `RuntimeSandboxConstructionCostRow`, `RuntimeSandboxWorldMutationPlan`, and `plan_runtime_sandbox_world_mutation`.

- [ ] **Step 3: Add package evidence and static checks**

  Add selected package counters for chunks, placement/destruction intents, construction costs, persistence rows, rejected unsafe mutation rows, and clean diagnostics.

## Phase 8: Simulation Management Pack Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/genre_simulation_management.hpp`
- Create: `engine/runtime/src/genre_simulation_management.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_genre_simulation_management_tests.cpp`
- Package evidence: selected long-run validation recipe.

- [ ] **Step 1: Write failing simulation management tests**

  Test resources, jobs, logistics links, economy summaries, population/needs rows, schedules, deterministic long-run replay hashes, save/load review rows, and runtime UI dashboard counter handoff.

- [ ] **Step 2: Implement simulation management value contracts**

  Add `RuntimeSimulationResourceRow`, `RuntimeSimulationJobRow`, `RuntimeSimulationLogisticsLink`, `RuntimeSimulationEconomySummary`, `RuntimeSimulationPopulationNeedRow`, `RuntimeSimulationScheduleRow`, `RuntimeSimulationManagementPlan`, and `plan_runtime_simulation_management`.

- [ ] **Step 3: Add long-run package evidence**

  Add deterministic long-run counters for tick count, resource balance rows, job assignments, logistics transfers, need deficits, dashboard rows, replay hash, and clean diagnostics.

## Phase 9: Network Replication Production Surface

**Files:**
- Create: `engine/runtime/include/mirakana/runtime/production_network_replication.hpp`
- Create: `engine/runtime/src/production_network_replication.cpp`
- Modify: `engine/runtime/CMakeLists.txt`
- Test: `tests/unit/runtime_production_network_replication_tests.cpp`
- Optional host evidence: `tools/validate-network-enet.ps1` when the `network-enet` feature is available.

- [ ] **Step 1: Write failing replication plan tests**

  Test authoritative session rows, replicated object rows, input-command sequencing, snapshot/rollback rows, lockstep policy diagnostics, transport capability review, and threat-model/security diagnostics. Loopback-only proof must not create a broad multiplayer ready claim.

- [ ] **Step 2: Implement replication value contracts**

  Add `RuntimeNetworkReplicationSessionDesc`, `RuntimeReplicatedObjectRow`, `RuntimeReplicationInputCommandRow`, `RuntimeReplicationSnapshotRow`, `RuntimeRollbackPolicyRow`, `RuntimeNetworkReplicationPlan`, and `plan_runtime_network_replication`.

- [ ] **Step 3: Add package and host-gate evidence**

  Add selected package counters for replicated objects, input rows, snapshot rows, rollback rows, rejected unsafe rows, and clean diagnostics. Record real-transport evidence only through the optional network validation wrapper and keep unsupported rows explicit when host evidence is absent.

## Phase 10: Rendering VFX Profiling Production Surface

**Files:**
- Create: `engine/renderer/include/mirakana/renderer/production_vfx_profiling.hpp`
- Create: `engine/renderer/src/production_vfx_profiling.cpp`
- Modify: `engine/renderer/CMakeLists.txt`
- Test: `tests/unit/renderer_production_vfx_profiling_tests.cpp`
- Host evidence: D3D12/Vulkan/Metal-specific validation where available.

- [ ] **Step 1: Write failing renderer/VFX/profile tests**

  Test GPU particle budget rows, richer postprocess feature rows, backend timing/profile rows, crash/telemetry handoff rows, backend parity requirements, and host-gated Metal evidence diagnostics. D3D12 evidence must not imply Vulkan or Metal readiness.

- [ ] **Step 2: Implement backend-neutral renderer production evidence contracts**

  Add `RendererProductionVfxFeatureRow`, `RendererProductionGpuParticleBudgetRow`, `RendererProductionPostprocessRow`, `RendererProductionBackendTimingRow`, `RendererProductionCrashTelemetryHandoffRow`, `RendererProductionVfxProfilingPlan`, and `plan_renderer_production_vfx_profiling`.

- [ ] **Step 3: Add package and host-gated validation**

  Add selected package counters for VFX/profile rows and backend evidence. Mark backend-specific readiness as `host-gated-production` until the matching D3D12, Vulkan, or Metal host proof exists.

## Validation

For docs/manifest-only Phase 0, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/compose-agent-manifest.ps1 -Write
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-text-format.ps1
```

For any C++ runtime, UI, tools, package, or public contract phase, run the focused target tests first, then:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-tidy.ps1 -Files <changed-cpp-or-header-files>
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

For dependency-changing phases, run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-dependency-policy.ps1
```

## Done When

- The backlog has a new `General-Purpose Game Production` group and production status vocabulary.
- The projection chapter has a fifth production track for general-purpose production surfaces.
- `engine/agent/manifest.json.aiOperableProductionLoop.currentActivePlan` points at this milestone.
- Each implemented phase promotes exactly one reusable production surface with tests, package evidence, docs, manifest fragments, and static checks.
- Existing completed `foundation-ready` and candidate burn-down rows remain closed and are not reopened.
- No new public API leaks SDL3, Dear ImGui, renderer internals, RHI handles, backend types, platform native handles, or middleware types.
- Clean breaking changes are used when old foundation contracts block the production data model.
