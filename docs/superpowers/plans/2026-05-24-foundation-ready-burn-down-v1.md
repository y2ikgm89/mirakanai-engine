# Foundation Ready Burn-down v1 (2026-05-24)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Promote the 15 canonical `foundation-ready` rows present at kickoff in the post-1.0 capability backlog to `implemented-1x-foundation` through clean, evidence-backed candidate slices.

**Plan ID:** `foundation-ready-burn-down-v1`

**Status:** Completed.

**Parent index:** [Production Completion Master Plan v1](../master-plans/2026-05-03-production-completion-master-plan-v1.md)

**Architecture:** Treat each backlog row as a reviewable candidate unless the implementation shares one public API family and validation surface with another row. Keep contracts first-party, backend-neutral, and clean-break. Promote a row only after behavior, tests, package/game evidence when required, docs, manifest fragments, schema/static checks, validation evidence, and PR evidence agree.

**Tech Stack:** C++23, CMake/CTest presets through repository PowerShell wrappers, Git worktrees, Context7/official documentation review for SDK/toolchain behavior, composed engine agent manifest fragments, GitHub Flow PR checkpoints.

---

## Classification

Large milestone: this spans runtime, tools, sprite/content, material/shader, physics, navigation, AI, scale, procedural generation, sample packages, docs, manifest fragments, and static checks. The milestone is intentionally wider than a PR. Each candidate closes as a small validated PR when independent; tightly coupled rows can share one PR only when they touch the same public API family and validation surface.

## Official Documentation Review

- Git official `git-worktree` documentation confirms linked worktrees are the native Git mechanism for multiple working trees attached to one repository and created through `git worktree add`.
- Context7 `/kitware/cmake` confirms preset-driven configure/build/test flows use CMake configure presets, `cmake --build` with build presets, and CTest `--preset` test presets. Repository wrappers remain the supported local entrypoints and preserve the same model.
- Additional official documentation or Context7 checks are required inside each candidate before changing CMake, vcpkg, SDL3, D3D12, Vulkan, Metal, Android, Apple, middleware, or toolchain behavior.

## Goal / Context / Constraints / Done When

**Goal:** Burn down every current `foundation-ready` backlog row into a narrow implemented 1.x foundation without reopening Engine 1.0 unsupported production gaps.

**Context:** `unsupportedProductionGaps = []` remains the 1.0 truth. The milestone started with 15 post-1.0 / 1.x developer-owned capability foundations whose narrow basis existed but whose stronger productization evidence was not yet promoted. After the `engine-scene-gameplay-binding-v1`, `engine-input-action-contexts-v1`, `engine-asset-placeholder-generation-v1`, `sprite-atlas-authoring-v1`, `sprite-batching-renderer-v1`, `sprite-animation-flipbook-v1`, `material-shader-graph-production-v1`, `physics-character-dynamics-v1`, `physics-collision-query-v1`, `navigation-navmesh-v1`, `navigation-crowd-local-avoidance-v1`, `ai-behavior-authoring-v1`, `ai-perception-services-v1`, `world-streaming-and-large-scenes-v1`, and `procedural-content-generation-v1` promotions, 0 `foundation-ready` rows remain in the canonical backlog.

**Constraints:**
- No direct `main` commits.
- Use linked worktrees and `tools/prepare-worktree.ps1`.
- Use TDD for C++ behavior changes: red test, green implementation, refactor, focused validation.
- Prefer clean breaking contracts over compatibility shims, aliases, migration layers, duplicate APIs, or broad "ready" wording.
- Keep public APIs first-party and backend-neutral; no public native handles or middleware types.
- Edit `engine/agent/manifest.fragments/*.json` and run `tools/compose-agent-manifest.ps1 -Write`; never hand-edit `engine/agent/manifest.json`.
- Run focused checks while iterating and one fresh `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at each C++/runtime/build/package/public-contract candidate closeout.
- Candidate completion requires commit, push, PR with validation evidence, hosted check review when applicable, merge, main sync, and guarded worktree cleanup.

**Done when:**
- All 15 rows below are `implemented-1x-foundation` in [04-developer-owned-engine-capability-backlog.md](../master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md).
- The composed `engine/agent/manifest.json` and fragment pointers return to the production-completion master plan or the next selected plan with `unsupportedProductionGaps = []`.
- Current capabilities, roadmap, AI/game guidance, skills/static checks, and validation recipes have no contradiction with the promoted rows.
- Every candidate has focused evidence, full validation evidence where required, PR/merge evidence, and no orphan worktrees.

## Candidate Order

| # | Candidate | PR grouping | Primary modules | Promotion evidence |
| --- | --- | --- | --- | --- |
| 1 | `engine-scene-gameplay-binding-v1` | independent | `MK_runtime_scene`, sample 2D/3D packages | Scene validation tests, missing/duplicate binding diagnostics, 2D/3D package counters. |
| 2 | `engine-input-action-contexts-v1` | independent | `MK_runtime`, `MK_ui`, editor/game samples | Package-visible input context stack, profile overlay, action/axis capture, focused capture, and presentation/glyph-key counters. |
| 3 | `engine-asset-placeholder-generation-v1` | independent | `MK_tools`, generated-game manifests | Broader placeholder families, replacement workflow rows, package handoff counters. |
| 4 | `sprite-atlas-authoring-v1` | can pair with #5 only if package evidence is inseparable | `MK_tools`, `MK_assets`, 2D package sample | Decode/import breadth boundary, atlas page policy, pivots/borders diagnostics, editor/package evidence. |
| 5 | `sprite-batching-renderer-v1` | can pair with #4 only if package evidence is inseparable | `MK_scene_renderer`, `MK_renderer`, 2D package sample | High-density world/UI/effects batching budgets, per-backend gated counters. |
| 6 | `sprite-animation-flipbook-v1` | independent | `MK_runtime`, 2D package sample | Direction sets, events, playback modes, gameplay-state integration counters. |
| 7 | `material-shader-graph-production-v1` | independent | `MK_assets`, `MK_tools`, material package sample | Graph lowering, package rows, reviewed HLSL/export text, D3D12/Vulkan compile-request planning, and explicit unsupported boundaries. |
| 8 | `physics-character-dynamics-v1` | can pair with #9 only if controller/query tests share one API boundary | `MK_physics`, 3D package sample | Slope/step/platform polish, push/slide policy diagnostics, tuning evidence. |
| 9 | `physics-collision-query-v1` | can pair with #8 only if query/controller tests share one API boundary | `MK_physics`, 2D/3D package samples | 3D collision-query readiness summaries, package-visible query readiness counters, and explicit oriented-box/mesh/convex/editor/middleware query exclusions. |
| 10 | `navigation-navmesh-v1` | can pair with #11 if validation is one nav milestone | `MK_navigation`, package samples | Navmesh readiness summaries, dynamic obstacle path evidence, selected package counters, and explicit asset-import/bake/streaming boundaries. |
| 11 | `navigation-crowd-local-avoidance-v1` | can pair with #10 if validation is one nav milestone | `MK_navigation`, package samples | Crowd budgets, animation-aware steering boundary, deterministic source-order counters. |
| 12 | `ai-behavior-authoring-v1` | can pair with #13 if AI package evidence is one surface | `MK_ai`, tools/package samples | Behavior validation breadth, deterministic traces, editor/graph non-goals. |
| 13 | `ai-perception-services-v1` | can pair with #12 if AI package evidence is one surface | `MK_ai`, package samples | Memory decay/team/event-stimulus boundaries and performance counters. |
| 14 | `world-streaming-and-large-scenes-v1` | independent | `MK_runtime`, `MK_navigation`, package samples | Background streaming boundary, open-world parity blockers, platform async job gates. |
| 15 | `procedural-content-generation-v1` | independent | `MK_runtime`, package samples | Generator ids, deterministic hashes, generated scene/asset validation, quality-rubric rows. |

## Subagent Dispatch Model

| Stage | Mode | Agents |
| --- | --- | --- |
| Candidate kickoff | parallel read-only | `explorer` for current evidence, `planning-auditor` for plan alignment, domain auditor when renderer/RHI or agent surface risk is high |
| Implementation | sequential write | parent or one `worker`/`gameplay-builder` with disjoint write ownership; do not dispatch parallel writers to the same candidate files |
| Review | parallel read-only | `cpp-reviewer`, `agent-surface-auditor`, plus `rendering-auditor` for renderer/RHI candidates |
| Validation repair | targeted | `build-fixer` only for concrete configure/build/test/validation failures |
| Closeout | parent | compose manifest, run static checks, run full validation, commit, push, PR, monitor/merge/sync, cleanup |

## Task 1: Select And Prove `engine-scene-gameplay-binding-v1`

**Files:**
- Modify: `tests/unit/runtime_scene_tests.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify as needed: `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`
- Modify static checks only if existing checks do not cover the new evidence.

- [x] **Step 1: Record current evidence and final promotion boundary**

Confirm the current runtime-scene public API already includes `resolve_runtime_scene_gameplay_bindings` and `plan_runtime_scene_gameplay_interactions`, then define the closeout as package-visible binding resolution evidence rather than a new scene-reflection system.

- [x] **Step 2: Confirm runtime-scene fail-closed coverage**

Existing `MK_runtime_scene_tests` already cover accepted gameplay binding rows plus fail-closed diagnostics for duplicate binding ids, missing/duplicate node names, missing required components, duplicate action ids, missing source/target bindings, missing objective ids, invalid amounts, and rejected terminal-state transitions. The missing promotion evidence is package-visible 2D/3D counters and installed validation, so the implementation keeps the public API unchanged.

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_runtime_scene_tests
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_runtime_scene_tests
```

Expected before implementation: package and installed validation fail on missing scene gameplay binding counters.

- [x] **Step 3: Add package-visible 2D/3D gameplay binding counters**

Extend the selected `--require-gameplay-systems` paths to report binding readiness, binding row counts, unique gameplay-system counts, missing/duplicate diagnostic counts, and final clean diagnostics without changing scene mutation semantics or adding runtime reflection.

- [x] **Step 4: Verify focused package evidence**

Build the affected package targets and run source-tree package smokes with `--require-gameplay-systems`. The expected output includes:

```text
gameplay_systems_scene_binding_ready=1
gameplay_systems_scene_binding_source_rows=3
gameplay_systems_scene_binding_rows=3
gameplay_systems_scene_binding_systems=3
gameplay_systems_scene_binding_component_rows=3
gameplay_systems_scene_binding_diagnostics=0
gameplay_systems_scene_interaction_rows=3
gameplay_systems_scene_interaction_diagnostics=0
gameplay_systems_scene_interaction_final_session_state=won
```

Focused evidence completed:
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package` passed.
- Direct source-tree smokes for `sample_2d_desktop_runtime_package --require-gameplay-systems --require-procedural-generation` and `sample_generated_desktop_runtime_3d_package --require-gameplay-systems` passed with the exact scene gameplay binding/interaction counters above.
- Direct installed-layout validation through `tools/validate-installed-desktop-runtime.ps1` passed for both package targets with the exact counters above.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-24 with all 78 CTest tests passing; Metal/Apple checks remained diagnostic/host-gated on this Windows host.

The full `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package` path is currently blocked before package validation by unrelated full `desktop-runtime-release` build failures: MSVC `CL.exe` exited with `-1073740791` on runtime package discovery reviewed-eviction tests, and unmodified shader package custom build steps reported that the batch label `VCEnd` was not found.

- [x] **Step 5: Sync docs, manifest, backlog, and static checks**

Promote `engine-scene-gameplay-binding-v1` to `implemented-1x-foundation`, record exact validation evidence, compose the manifest, and add or update static checks when the new counters become AI-operable contract literals.

- [x] **Step 6: Validate, commit, push, and PR**

Run focused checks, then:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Commit only task-owned files, push `codex/foundation-ready-burndown-v1`, open a PR with evidence, wait for applicable hosted checks, merge through the guarded flow, sync `main`, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

Closeout evidence completed through PR #212, merge commit `23bac05348ca32867dfe7980bf265c9e537c5afa`. Hosted checks, including `PR Gate`, completed successfully before merge. `tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/foundation-ready-burndown-v1 -DeleteLocalBranch -DeleteRemoteBranch` completed after merge and main sync.

## Task 2: Select And Prove `engine-input-action-contexts-v1`

**Files:**
- Modify: `games/sample_2d_desktop_runtime_package/main.cpp`
- Modify: `games/sample_generated_desktop_runtime_3d_package/main.cpp`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `tools/validate-installed-desktop-runtime.ps1`
- Modify: `tools/check-json-contracts-050-generated-games.ps1`
- Modify as needed: `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/roadmap.md`, `docs/specs/generated-game-validation-scenarios.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Record current evidence and final promotion boundary**

Existing runtime/editor contracts already cover context stacks, rebinding profiles, action capture, axis capture, focused capture consumption, and presentation rows. The promotion boundary is selected package-visible evidence, not a new input API, real platform glyph rendering, keyboard-layout localization, multiplayer device assignment, native input handles, SDL3 input APIs, or full runtime/game rebinding panels.

- [x] **Step 2: Add package-visible 2D/3D input context and rebinding counters**

Extend selected `--require-gameplay-systems` paths to report `input_context_rebinding_*` and `input_rebinding_*` counters for a 3-layer rebinding/HUD/gameplay stack, profile overlays, action capture, axis capture, focused capture consumption/retention, presentation rows, glyph lookup keys, and clean diagnostics.

- [x] **Step 3: Guard source and installed package evidence**

`tools/check-json-contracts.ps1` now requires the 2D manifest/main/installed-validation literals for `inputContextRebinding` and the new smoke fields. `tools/validate-installed-desktop-runtime.ps1` now requires exact 2D/3D installed package values when `--require-gameplay-systems` is selected.

- [x] **Step 4: Verify focused package evidence**

Focused evidence completed:
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target sample_2d_desktop_runtime_package sample_generated_desktop_runtime_3d_package` passed.
- Direct source-tree smokes for `sample_2d_desktop_runtime_package --require-gameplay-systems --require-procedural-generation` and `sample_generated_desktop_runtime_3d_package --require-gameplay-systems` passed with `input_context_rebinding_ready=1`, 3 requested layers, 1 active modal rebinding context, `input_context_rebinding_capture_active=1`, gameplay input consumed, 2 applied profile overlays, captured action/axis candidates, retained focused waiting capture, 2 presentation rows, 5 glyph lookup keys, and `input_rebinding_diagnostics=0`.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1` passed.

- [x] **Step 5: Sync docs, manifest, backlog, and static checks**

Promote `engine-input-action-contexts-v1` to `implemented-1x-foundation`, keep explicit non-goals for real glyph assets, keyboard-layout localization, device assignment, native handles, SDL3 input APIs, and full runtime/game rebinding panels, compose the manifest, and keep validation recipes/static checks aligned with the package-visible counters.

- [x] **Step 6: Validate, commit, push, and PR**

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Commit only task-owned files, push `codex/foundation-ready-input-contexts-v1`, open a PR with evidence, wait for applicable hosted checks, merge through the guarded flow, sync `main`, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-24 with all 78 CTest tests passing; Metal/Apple checks remained diagnostic/host-gated on this Windows host.

Closeout evidence completed through PR #213, merge commit `e0faf43aa4ccd3bb375044681bcc839bf429b5e8`. Hosted checks, including `PR Gate`, completed successfully before merge. `tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/foundation-ready-input-contexts-v1 -DeleteLocalBranch -DeleteRemoteBranch` completed after merge and main sync.

## Task 3: Select And Prove `engine-asset-placeholder-generation-v1`

**Files:**
- Modify: `tests/unit/tools_tests.cpp`
- Modify: `schemas/game-agent.schema.json`
- Modify: `tools/new-game-templates.ps1`
- Modify: `tools/check-json-contracts-062-placeholder-asset-pipeline.ps1`
- Modify: `games/sample_2d_desktop_runtime_package/game.agent.json`
- Modify: `games/sample_generated_desktop_runtime_3d_package/game.agent.json`
- Modify: `games/sample_desktop_runtime_game/game.agent.json`
- Modify as needed: `docs/current-capabilities.md`, `docs/ai-game-development.md`, `docs/roadmap.md`
- Modify: `docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md`
- Modify: `docs/superpowers/plans/README.md`
- Modify: `engine/agent/manifest.fragments/010-aiOperableProductionLoop.json`
- Generate: `engine/agent/manifest.json`

- [x] **Step 1: Record current evidence and final promotion boundary**

Existing `MK_tools` already exposes placeholder texture, mesh, material, and audio planning plus registered source cook/package handoff. The promotion boundary is selected replacement recook evidence plus manifest/schema/static proof for broader placeholder roles, replacement workflow rows, and package handoff counters, not external asset downloads, arbitrary generation, runtime source parsing, renderer/RHI residency, package streaming, native handles, middleware contracts, or final art direction.

- [x] **Step 2: Add replacement recook and manifest contract evidence**

`MK_tools_tests` now proves `plan_placeholder_asset_cook_package` can recook a replacement through existing source registry and package index content while preserving stable asset keys, provenance/license rows, package mutations, updated content hashes, and source revision handoff. `game.agent.json.aiWorkflow.placeholderAssetPipeline` now requires `replacementWorkflow` rows plus planned/source/provenance/runtime package handoff counters, and scaffold templates emit the same shape for future generated games.

- [x] **Step 3: Guard selected 2D/3D placeholder role and package handoff coverage**

`tools/check-json-contracts-062-placeholder-asset-pipeline.ps1` now requires selected manifests to cover sprite, mesh, material, audio, UI, and scene-prop placeholder roles across committed package samples; validates replacement workflow reviewed tool surfaces, allowed replacement sources, provenance requirements, package handoff requirement, validation recipe ids, and count fields; and keeps unsupported claims explicit.

- [x] **Step 4: Validate, commit, push, and PR**

Focused evidence completed:
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-toolchain.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --preset dev` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-format.ps1` passed.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/cmake.ps1 --build --preset dev --target MK_tools_tests` passed after the expected red compile failure exposed direct `AssetKeyV2` equality in the new test.
- `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/ctest.ps1 --preset dev --output-on-failure -R MK_tools_tests` passed.
- Full `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` passed on 2026-05-24 with all 78 CTest tests passing; Metal/Apple checks remained diagnostic/host-gated on this Windows host. Validation log: `out/validation-logs/validate-20260524-192149-50604`.

Run:

```powershell
pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1
```

Commit only task-owned files, push `codex/foundation-ready-placeholder-generation-v1`, open a PR with evidence, wait for applicable hosted checks, merge through the guarded flow, sync `main`, and remove the merged worktree with `tools/remove-merged-worktree.ps1`.

Closeout evidence completed through PR #214, merge commit `e5ad8f8fe1066c07d6a575b10dd9169e54726afc`. Hosted checks, including `PR Gate`, completed successfully before merge. `tools/remove-merged-worktree.ps1 -WorktreePath .worktrees/foundation-ready-placeholder-generation-v1 -DeleteLocalBranch -DeleteRemoteBranch` completed after merge and main sync.

## Remaining Candidate Loop

For each remaining candidate:

- [ ] Create or reuse this active plan phase with the candidate id, public API boundary, official-doc review, tests-first tasks, package evidence, docs/manifest/static checks, validation commands, and PR evidence.
- [ ] Work in a fresh linked worktree from updated `main` unless the candidate is intentionally coupled to an open milestone branch.
- [ ] Dispatch read-only subagents for current evidence and risk review; dispatch one write-capable worker only when write ownership is disjoint and bounded.
- [ ] Implement with TDD and focused validation before the full slice gate.
- [ ] Promote only the candidate row(s) actually proven by evidence.
- [ ] Publish a PR, merge, sync, and clean the worktree before selecting the next independent candidate.

## Evidence Log

| Candidate | PR | Merge SHA | Full validation | Notes |
| --- | --- | --- | --- | --- |
| `engine-scene-gameplay-binding-v1` | #212 | `23bac05348ca32867dfe7980bf265c9e537c5afa` | pass (`tools/validate.ps1`, 2026-05-24) | Package-visible binding evidence implemented; focused build, runtime-scene CTest, direct smokes, direct installed-layout validation, static checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `engine-input-action-contexts-v1` | #213 | `e0faf43aa4ccd3bb375044681bcc839bf429b5e8` | pass (`tools/validate.ps1`, 2026-05-24) | Package-visible 2D/3D input context/rebinding evidence implemented; focused build, direct smokes, static checks, AI integration checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `engine-asset-placeholder-generation-v1` | #214 | `e5ad8f8fe1066c07d6a575b10dd9169e54726afc` | pass (`tools/validate.ps1`, 2026-05-24) | Replacement recook test, manifest replacement workflow rows, package handoff counters, schema/template/static-check coverage, focused build, focused CTest, json contracts, format checks, AI integration checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `sprite-atlas-authoring-v1` | #215 | `41e88c58571c3ad1161f7cd190ac58637ce00888` | pass (`tools/validate.ps1`, 2026-05-24; `out/validation-logs/validate-20260524-200156-48696`) | Page policy, pivot, and slice-border metadata/diagnostics implemented; focused build, focused CTest, scoped tidy, public API boundary check, schema/template/static-check coverage, AI integration checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `sprite-batching-renderer-v1` | #216 | `e4a3fad31045074a2d592348e0ebdee033d9a6d7` | pass (`tools/validate.ps1`, 2026-05-24; `out/validation-logs/validate-20260524-212552-45648`; package lane passed) | Sprite batch budget profile API and committed 2D package `sprite_batch_budget_*` world/UI/effects counters are implemented; focused renderer tests, sample package build, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed with `sprite_batch_budget_status=ready`, `sprite_batch_budget_profiles_ready=3`, and `sprite_batch_budget_diagnostics=0`. |
| `sprite-animation-flipbook-v1` | #217 | `51c3d115d5b4cb40474544110facbd64cb46d106` | pass (`tools/validate.ps1`, 2026-05-24; `out/validation-logs/validate-20260524-221742-48672`; package lane passed) | `RuntimeSpriteFlipbookDirectionSetRow`, `RuntimeSpriteFlipbookPlaybackRequest`, event rows, loop/clamp playback policy, and committed 2D package `sprite_flipbook_direction_sets`, `sprite_flipbook_event_rows`, `sprite_flipbook_playback_modes`, `sprite_flipbook_gameplay_state_rows`, `sprite_flipbook_events_sampled`, and clean playback diagnostics are implemented. Focused `MK_scene_renderer_tests`, direct `sample_2d_desktop_runtime_package --require-sprite-animation` smoke, full validation, package lane, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `material-shader-graph-production-v1` | #218 | `568a6ff529cd538ff546e2bcb16dfaeda1f8664a` | pass (`tools/validate.ps1`, 2026-05-24; `out/validation-logs/validate-20260524-234155-20768`; package lane passed) | `MaterialGraphProductionAuthoringDesc` / `plan_material_graph_production_authoring`, committed material/shader package graph/export/HLSL source inputs, `material_graph_*` package counters, generated-template coverage, schema/static checks, full validation, package lane, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `physics-character-dynamics-v1` | #219 | `97e2a35d116f187f35e567ff0b6e84d6e247d4f6` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-004421-38000`; package lane passed) | `PhysicsCharacterDynamics3DReadinessConfig` / `PhysicsCharacterDynamics3DReadinessReport` and `evaluate_physics_character_dynamics_readiness_3d` summarize advanced controller evidence for dynamic push, step-up, walkable slope, ground probe, moving platform, constraints, replay changes, and movement-row budgets. Focused RED compile failure, `MK_core_tests`, `sample_generated_desktop_runtime_3d_package` source smokes, `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`, installed validator fields, short `generated_3d_package` shader artifact prefix, scoped tidy, static checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `physics-collision-query-v1` | #220 | `a2c6709e128dc0aa32e0e34ae29b291675589c5b` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-013612-49784`; package lane passed) | `PhysicsCollisionQuery3DReadinessConfig` / `PhysicsCollisionQuery3DReadinessReport` and `evaluate_physics_collision_query_readiness_3d` summarize selected 3D raycast/sweep batch evidence for raycast hits, shape-sweep hits, no-hit rows, invalid-request rows, query-budget rejections, source ordering, and row budgets. Focused RED compile failure, `MK_core_tests`, `sample_generated_desktop_runtime_3d_package` source smoke, `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`, installed validator `collision_query_readiness_*` fields, scaffold/static checks, scoped tidy, public API boundary checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `navigation-navmesh-v1` | #221 | `51c02a0253f23646531f1fe260dc6a9008787ca9` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-021433-50792`; package lane passed) | `NavigationNavmeshReadinessConfig` / `NavigationNavmeshReadinessReport` and `evaluate_navigation_navmesh_readiness` summarize scene-ref route rows, point rows, dynamic-obstacle participation, visited polygon counts, and total-cost budgets. Focused RED compile failure, `MK_navigation_tests`, selected `sample_generated_desktop_runtime_3d_package` source CTest, `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`, installed validator fields, scaffold/static checks, scoped tidy, public API boundary checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `navigation-crowd-local-avoidance-v1` | #222 | `b9817df14881755d45a066b5dfb340e48ddddde5` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-025439-47016`; package lane passed) | `NavigationCrowdReadinessConfig` / `NavigationCrowdReadinessReport` and `evaluate_navigation_crowd_readiness` summarize value-only crowd rows, deterministic source ordering, route/local-avoidance success counts, applied-neighbor counts, dynamic-obstacle propagation, planned-agent minimums, and row budgets. Focused RED compile failure, `MK_navigation_tests`, selected `sample_generated_desktop_runtime_3d_package` source CTest, `tools/package-desktop-runtime.ps1 -GameTarget sample_generated_desktop_runtime_3d_package`, installed validator fields, scaffold/static checks, scoped tidy, public API boundary checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `ai-behavior-authoring-v1` | #223 | `6c2c375f6d877a596f7560a6ddf6868d755c75f4` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-033429-46152`; package lane passed) | `BehaviorAuthoringReadinessConfig` / `BehaviorAuthoringReadinessReport` and `evaluate_behavior_authoring_readiness` summarize validation diagnostics, deterministic trace stability, behavior/action/blackboard-condition counts, and budgets. Focused RED compile failure, `MK_ai_tests`, selected `sample_2d_desktop_runtime_package` source CTest, `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`, installed validator fields, scaffold/static checks, scoped tidy, public API boundary checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `ai-perception-services-v1` | #224 | `14e92e5c1d9404e1b7402eafe071bd151ad80534` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-041415-44244`; package lane passed) | `AiPerceptionReadinessConfig` / `AiPerceptionReadinessReport` and `evaluate_ai_perception_readiness_2d` summarize sight/hearing classification, stable primary-target selection, blackboard projection, visible/audible target counts, and target budgets. Focused RED compile failure, `MK_ai_tests`, selected `sample_2d_desktop_runtime_package` source CTest, `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`, installed validator fields, scaffold/static checks, scoped tidy, public API boundary checks, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `world-streaming-and-large-scenes-v1` | #225 | `d19019cf81adb33dbdd845b2cfd5cb3ee01364b7` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-051926-4312`; package lane passed) | `RuntimeWorldStreamingLargeSceneReadinessConfig` / `RuntimeWorldStreamingLargeSceneReadinessReport` and `evaluate_runtime_world_streaming_large_scene_readiness` summarize reviewed load/keep/unload plans, safe-point results, missing-region diagnostics, package adoption, resident-region/byte budgets, and optional navigation ref/path-cache evidence. Focused RED compile failure, malformed missing-region-probe regression coverage, `MK_runtime_world_region_streaming_tests`, selected `sample_2d_desktop_runtime_package` source CTest, `tools/package-desktop-runtime.ps1 -GameTarget sample_2d_desktop_runtime_package`, installed validator exact fields, scaffold/static checks, scoped tidy, public API boundary checks, agent-surface drift updates, review fixes, full validation, hosted checks, merge, main sync, and guarded worktree cleanup passed. |
| `procedural-content-generation-v1` | #226 | `ccb446f3ef825cb7fdca87b1cb3b7262ed520a62` | pass (`tools/validate.ps1`, 2026-05-25; `out/validation-logs/validate-20260525-060732-49412`; package lane passed) | Existing `RuntimeProceduralGenerationRequest` / `RuntimeProceduralGenerationPlan` / `plan_runtime_procedural_generation` and runtime-scene procedural placement bridge evidence is promoted by tightening installed validation for `gameplay_systems_procedural_generation_*` counters: ready status, clean diagnostics, 3 rows, object/encounter/loot counts, non-zero replay hash, package-visible rows, placement intent rows, and accepted placement intent rows. `tools/check-ai-integration.ps1` now guards the procedural evidence contract across docs, skills, and the installed validator. Hosted checks, merge, main sync, and guarded worktree cleanup passed through PR #226. External services, unbounded asset generation, implicit scene/package mutation, renderer/RHI residency, native handles, and content-quality claims remain separate. |
