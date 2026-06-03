---
name: gameengine-game-development
description: Scaffolds and maintains C++ games, game.agent.json, and desktop or mobile validation lanes. Use when editing games/, new-game scripts, or game manifests.
paths:
  - "games/**"
  - "tools/new-game.ps1"
  - "tools/new-game-helpers.ps1"
  - "tools/new-game-templates.ps1"
  - "docs/ai-game-development.md"
---

# GameEngine Game Development

## Scope

Use this skill for C++ games, game.agent.json, new-game scaffolding, desktop runtime packages, and mobile validation lanes.

## Context Budget Rules

- Start with targeted file reads, targeted manifest fragments, and `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` whenever possible.
- Do not load `references/full-guidance.md` by default. Load it only when the current task needs exact API names, validation counters, retained ids, package lanes, or backend/editor details not present here.
- Keep implementation slices small, clean-break, and evidence-backed. Do not add compatibility shims, stale aliases, broad ready claims, or unsupported host assumptions.
- Prefer focused build/test/static loops while iterating, then run `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1` at the coherent slice gate.

## Required Discipline

- Use `tools/agent-context.ps1 -ContextProfile Minimal` or `Standard` before engine-facing game/API decisions.
- Read `references/full-guidance.md` only for detailed public API lists, package-lane counters, generated-game manifests, or mobile/desktop runtime recipes.
- Keep `game_name` and `new-game -Name` values matching `^[a-z][a-z0-9_]*$` and keep source-tree paths lowercase snake_case.
- Runtime package payloads are byte-hashed. When adding a text cooked/runtime extension or `runtimePackageFiles` entry, update the game/scaffold `runtime/.gitattributes` with `text eol=lf`, keep scaffold/static checks aligned, and run the narrowest package smoke before the slice gate.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.gameDesignSpec` rows before expansion: gameplay family, template, camera, input map, core loop, scene list, asset requests, systems, package targets, validation recipe ids, quality gates, and unsupported claims that reference only same-manifest package targets and validation recipes.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.contentMutationLedger` rows before AI
  content edits: game-local AI-owned roots, generated files, reviewed dry-run/apply or review-only command surfaces, forbidden shared paths,
  and remediation actions. Keep generated-game mutation under `games/<game_name>/` through reviewed surfaces such as
  `tools/create-game-recipe.ps1`, `tools/register-runtime-package-files.ps1`, or `mirakana::review_engine_capability_handoff_request`; stop
  at a developer-owned handoff for `engine/`, `editor/`, shared `tools/`, schemas, CI, agent surfaces, shared docs, or
  `games/CMakeLists.txt`.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.placeholderAssetPipeline` rows before treating placeholders as package-ready: design asset requests map to first-party placeholder assets, package handoff rows, validation recipe ids, and runtime package files. Use reviewed first-party placeholder assets only; no external asset downloads, arbitrary generation, runtime source parsing, renderer/RHI residency, native handles, middleware contracts, or broad quality claims.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.generatedGamePlaytestLoop` rows before playtest evidence can drive remediation: select reviewed validation recipes, evidence roots, failure classification rows, and mutation-ledger remediation actions. Use reviewed recipe execution and package-smoke evidence only; no validation weakening, evidence deletion, host-gate bypass, arbitrary shell, raw manifest command evaluation, cooked-package mutation, engine-internal edits, native handles, or broad quality claims.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.validationRemediationRecipes` rows before
  applying common generation failure remediation: map playtest failure classifications to reviewed mutation-ledger actions and command
  surfaces, record required evidence and stop conditions, then rerun selected validation recipes. Use reviewed game-local surfaces only; no
  validation weakening, evidence deletion, host-gate bypass, arbitrary shell, raw manifest command evaluation, cooked-package mutation,
  engine-internal edits, native handles, or broad quality claims.
- Generated Game Studio v1 is the reviewed editor/agent loop for composing `gameDesignSpec`, generation, mutation ledger, placeholder assets, playtest evidence, validation remediation, quality rubric, and engine capability handoffs into 2D/3D iteration status; use `EditorAiGeneratedGameStudioV1Model` / retained `generated_game_studio` rows as review evidence, not as permission for engine-internal edits, arbitrary shell, native handles, renderer/RHI residency, Metal readiness, or broad editor productization.
- Generated or sample 2D/3D package manifests must carry fail-closed `game.agent.json.aiWorkflow.generatedGameQualityRubric` rows before quality-ready claims: link same-manifest design spec, playtest loop, remediation recipes, validation recipes, and report ids to objective, controls, feedback, fail/restart, deterministic package smoke, and budget evidence gates. Include explicit unsupported rows; do not claim subjective fun, commercial quality, platform parity, unbounded performance, unreviewed content, autonomous balancing, native handles, validation weakening, or broad production readiness.
- Use `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/create-game-recipe.ps1 -Mode DryRun|-Mode Apply -GameName <game_name> -DesignSpecPath <path>` for reviewed AI game generation scaffolds. It supports only `DesktopRuntime2DPackage` and `DesktopRuntime3DPackage`, calls `tools/new-game.ps1` with fixed arguments, reports deterministic `plannedFiles`/`changedFiles`, preserves the reviewed `aiWorkflow.gameDesignSpec`, and does not execute arbitrary shell text, generate external assets, or cook beyond the scaffold.
- Missing generated-game art/audio should use reviewed engine tooling such as `mirakana::PlaceholderAssetBundleRequest` /
  `mirakana::plan_placeholder_asset_bundle` to return a `PlaceholderAssetBundlePlan` with deterministic first-party placeholder source
  documents, changed-file hashes, provenance, and fail-closed diagnostics; package-authoring workflows can use
  `mirakana::PlaceholderAssetCookPackageRequest` / `mirakana::plan_placeholder_asset_cook_package` to route the generated source documents
  through registered source cook/package planning. Do not download external assets, bypass cook/package validation, or parse source assets
  at runtime.
- 2D source atlas rows: `SpriteAtlasSourceFrameDesc`, `SpriteAtlasSourceAuthoringDesc`, `plan_sprite_atlas_source_authoring`, `GameEngine.TextureSource.v1`, `GameEngine.SourceAssetRegistry.v1`, `DesktopRuntime2DPackage`, `spriteAtlasSourceAuthoringTargets`, `source/assets/package.geassets`, and `source/sprites/player_atlas.texture_source`; keep source files out of `runtimePackageFiles` and do not parse images at runtime, create renderer/RHI residency, infer animation semantics, or use this for art direction.
- 2D sandbox authoring: `MK_tools` review/apply APIs, `sandboxWorldAuthoringTargets`, `--require-sandbox-authoring-review`, `sandbox_authoring_review_*`; `.sandbox_authoring` outside `runtimePackageFiles`; no external decode/download/importer, review-time apply, runtime source parsing, renderer/RHI, native handles.
- Modern material package proof uses `plan_modern_material_variants` for value-only factor/texture/shader-evidence rows; do not execute shader graphs, mutate packages, create residency, stream packages, or expose native handles.
- Generated `DesktopRuntime2DPackage` smokes report atlas-backed repeated scene sprite plan counters through `sprite_batch_plan_atlas_backed_batches`, `sprite_batch_plan_repeated_atlas_batches`, and `sprite_batch_plan_repeated_atlas_sprites` over adjacent cooked scene sprite rows. Treat these as deterministic package evidence only, not sprite sorting, production atlas packing, public native/RHI handle access, or broad production sprite batching readiness.
- 2D tile proof: `plan_tile_chunk_renderer`; `--require-production-tile-renderer`; no native texture ownership.
- Generated 2D gameplay can use `mirakana::RuntimeSpriteFlipbookClipDesc`, `mirakana::RuntimeSpriteFlipbookState`, and `mirakana::advance_runtime_sprite_flipbook` over cooked `RuntimeSpriteAnimationFrame` rows for deterministic named clip sampling before applying frames to scene sprite rows. `DesktopRuntime2DPackage` and `sample_2d_desktop_runtime_package` expose `sprite_flipbook_*` package counters through `--require-sprite-animation`; keep game-specific animation graphs/state machines in game code.
- Larger-map gameplay: use `RuntimeWorldRegionStreamingPlanRequest`, `RuntimeWorldRegionPackageDesc`, `plan_runtime_world_region_streaming`,
  `RuntimeWorldRegionStreamingSafePointDesc`, `execute_runtime_world_region_streaming_safe_point`,
  `RuntimeWorldRegionNavigationRefReviewRequest`, `RuntimeWorldRegionNavigationPathCacheReviewRequest`,
  `review_runtime_world_region_navigation_refs`, and `review_runtime_world_region_navigation_path_cache`; summarize with
  `RuntimeWorldStreamingLargeSceneReadinessRequest` / `RuntimeWorldStreamingLargeSceneReadinessReport` /
  `evaluate_runtime_world_streaming_large_scene_readiness`. `--require-world-region-streaming` exposes `world_region_streaming_load_rows`,
  `world_region_streaming_keep_rows`, `world_region_streaming_unload_rows`, `world_region_streaming_large_scene_readiness_status`, and
  `world_region_streaming_navigation_path_cache_ready`; do not claim biome rules, world persistence, automatic eviction policy, background
  streaming, open-world parity, async jobs, renderer/RHI residency, allocator/GPU budgets, package scripts, nav-data baking, or native
  handles.
- Procedural gameplay: `plan_runtime_procedural_generation` plus `plan_runtime_scene_procedural_construction_placement_intents`; `--require-procedural-generation` exposes `gameplay_systems_procedural_generation_rows`, object/encounter/loot rows, replay hash, package-visible rows, placement intent rows, and accepted placement intent rows. Do not claim external services, unbounded generation, package mutation, implicit scene mutation, renderer/RHI residency, native handles, or broad content quality.
- Script/mod metadata: `RuntimeScriptSandboxPolicyDesc`, `RuntimeScriptSandboxModuleDesc`, `RuntimeScriptSandboxEntrypointDesc`,
  `RuntimeScriptSandboxPermissionDesc`, `RuntimeScriptSandboxPlan`, `RuntimeScriptSandboxPermissionKind`, `allowed_host_apis`, and
  `plan_runtime_script_sandbox`; `--require-scripting-sandbox-policy` exposes `scripting_sandbox_entrypoint_rows`,
  `scripting_sandbox_denied_permission_rows`, `scripting_sandbox_rejected_unsafe_capability_rows`, `scripting_sandbox_budget_diagnostics`,
  `scripting_sandbox_replay_seed_rows`, and `scripting_sandbox_diagnostics`. It does not interpret scripts, load code, open files, use
  network/process/native plugins, mutate packages, or expose native handles.
- Multiplayer intent metadata: `RuntimeNetworkFoundationPolicyDesc`, `RuntimeNetworkSessionDesc`, `RuntimeNetworkTransportRequirementDesc`,
  `RuntimeNetworkReplicationChannelDesc`, `RuntimeNetworkReplayPrerequisiteDesc`, `RuntimeNetworkFoundationPlan`, and
  `plan_runtime_network_foundation`; `--require-networking-foundation-policy` exposes `networking_foundation_session_rows`,
  `networking_foundation_transport_rows`, `networking_foundation_channel_rows`, `networking_foundation_rejected_unsafe_transport_rows`,
  `networking_foundation_replay_prerequisite_rows`, `networking_foundation_security_diagnostics`, and `networking_foundation_diagnostics`.
  It does not open sockets, add middleware, perform encryption/authentication, matchmaking, rollback/prediction, NAT traversal, package
  mutation, or expose native handles.
- Gameplay loop planning: `RuntimeSimulationOrchestrationRequest`, `RuntimeSimulationInputCommandDesc`,
  `RuntimeSimulationOrchestrationPlan`, and `plan_runtime_simulation_orchestration`; `--require-simulation-orchestration` exposes
  `simulation_orchestration_planned_steps`, `simulation_orchestration_command_playback_rows`,
  `simulation_orchestration_budget_limited_status`, `simulation_orchestration_invalid_command_diagnostics`, and
  `simulation_orchestration_diagnostics`. It does not run game rules, pace renderer frames, open network transports, implement
  rollback/lockstep multiplayer, create threads, mutate packages, embed editor hosts, call platform timing APIs, or expose native handles.
- Generated or sample games that need unsupported reusable engine capability must stop at AI Engine Capability Handoff v1: use
  `mirakana::EngineCapabilityHandoffRequestRow` and `mirakana::review_engine_capability_handoff_request`, optionally persist descriptor-only
  `game.agent.json.aiWorkflow.engineCapabilityHandoffs` rows, and require a canonical backlog capability id, blocked feature, current
  workaround, affected game files, desired first-party public contract, and required evidence. Do not edit engine internals from the
  game-generation lane or request native, removed SDL3, Dear ImGui, renderer/RHI, backend, or middleware public contracts.
- Generated gameplay diagnostics should use `mirakana::ui::RuntimeGameplayDebugOverlayRowDesc` / `mirakana::ui::RuntimeGameplayDebugOverlayPlan` / `mirakana::ui::plan_runtime_gameplay_debug_overlay` for value-only gameplay debug overlay rows before renderer, telemetry, editor, or native UI presentation. Do not add game-local debug UI frameworks or command dispatchers to bypass `MK_ui`.
- Quest/dialogue: `RuntimeQuestDialogueDocument`, `RuntimeQuestDialogueValidationContext`, `validate_runtime_quest_dialogue_document`, `RuntimeQuestDialogueState`, `validate_runtime_quest_dialogue_state`, `RuntimeQuestDialogueTransitionRequest`, `advance_runtime_quest_dialogue_state`, `set_flag`, `complete_objective`, and `choose_dialogue`; action/reward ids are reviewed intent, while story text, reward/action execution, UI, localization, scripting, save mutation, and editor graph tooling stay game-owned.
- Item/inventory/crafting/placement: `RuntimeItemCatalogDocument`, `RuntimeItemCatalogValidationContext`,
  `RuntimeItemCatalogValidationResult`, `validate_runtime_item_catalog_document`, `RuntimeInventoryState`,
  `validate_runtime_inventory_state`, `RuntimeCraftingRecipeDocument`, `RuntimeInventoryTransitionRequest`,
  `RuntimeInventoryTransitionStatus`, `advance_runtime_inventory_state`, `RuntimeConstructionPlacementSurfaceDesc`,
  `RuntimeConstructionPlacementCandidateDesc`, `RuntimeConstructionPlacementValidationContext`,
  `RuntimeConstructionPlacementValidationResult`, `RuntimeConstructionPlacementDiagnostic`, `RuntimeConstructionPlacementValidationRow`,
  `validate_runtime_construction_placement`, `RuntimeSceneConstructionPlacementIntentDesc`, and
  `plan_runtime_scene_construction_placement_intents`. Preserve finite grid/world positions, candidate row grid/world origins, duplicate
  occupied cells, same-batch occupied-cell collision rejection, and explicit reviewed scene mutation; keep item art, balance, rewards,
  shops, loot, equipment, persistence, implicit scene mutation, and progression game-owned.
- Generated 3D renderer-quality package smokes keep exact frame graph evidence through `mirakana::evaluate_win32_desktop_presentation_quality_gate`: base postprocess expects `renderer_quality_expected_framegraph_passes=2`, `renderer_quality_expected_framegraph_render_passes=4`, `renderer_quality_framegraph_render_passes_ok=1`, and `renderer_quality_expected_framegraph_barrier_steps=4`; depth/shadow variants add `framegraph_render_passes_recorded` and `framegraph_barrier_steps_executed` counts in `references/full-guidance.md`.
- Renderer General Quality Matrix v1: `--require-renderer-quality-matrix`, `renderer_quality_matrix_status=host_evidence_required`, `renderer_quality_matrix_reviewed=1`, `renderer_quality_matrix_ready=0`, `renderer_quality_matrix_dependency_gated_rows=0`, `renderer_quality_matrix_unsupported_rows=0`, `renderer_quality_matrix_general_renderer_quality_ready=0`, and `plan_renderer_quality_matrix`; do not claim Metal readiness, native capture, public native handles, inferred parity, subjective visual-quality approval, or broad renderer quality.
- 2D/3D gameplay package smokes use public `MK_physics`, `MK_navigation`, and `MK_ai`: `NavigationNavmeshPathRequest`, `plan_navigation_navmesh_path`, `NavigationHierarchicalWorldPathRequest`, `plan_navigation_hierarchical_world_path`, `NavigationCrowdPlanRequest`, `plan_navigation_navmesh_crowd`, and `calculate_navigation_local_avoidance`; keep navmesh import, crowd persistence, scene/physics integration, middleware, package streaming, nav-data baking, background jobs, and editor claims out.
- Generated gameplay collision queries should use public `MK_physics` value batches such as `PhysicsWorld2D::raycast_batch`,
  `PhysicsWorld3D::raycast_batch`, `PhysicsWorld2D::shape_sweep_batch`, and `PhysicsWorld3D::shape_sweep_batch`; consume
  `PhysicsCollisionQueryBatchStatus`, `PhysicsCollisionQueryBatchDiagnostic`, `PhysicsCollisionQueryRowStatus`, and
  `PhysicsCollisionQueryRowDiagnostic` diagnostics, rely on default-unbounded query counts unless a positive `max_queries` fail-closed
  budget is needed, expect selected package smokes to expose `collision_query_batch_ready` counters, and keep middleware/native handles or
  scene-owned dispatch out of generated-game code.
- 3D movement: `PhysicsAdvancedController3DDesc`, `PhysicsMovingPlatform3DDesc`, `PhysicsAdvancedController3DResult`,
  `plan_physics_advanced_controller_3d`, `PhysicsConstraintSolve3DResult`, `PhysicsFixedConstraint3DDesc`,
  `PhysicsLinearAxisConstraint3DDesc`, `solve_physics_constraints_3d`, `max_rows`, `row_budget_exceeded`, unsupported rotational rigid-body
  constraints, `PhysicsKinematicMotion3DResult`, `plan_physics_kinematic_motion_3d`, `PhysicsSimpleVehicle3DDesc`,
  `PhysicsSimpleVehicle3DWheelDesc`, `PhysicsSimpleVehicle3DResult`, `plan_physics_simple_vehicle_3d`, and simple vehicle rows. Smokes
  expose `gameplay_systems_advanced_controller_status=moved`, `gameplay_systems_advanced_controller_platform_applied=1`,
  `gameplay_systems_advanced_controller_constraint_rows=1`, `gameplay_systems_advanced_controller_replay_changed=1`,
  `gameplay_systems_physics_constraints_status=solved`, `gameplay_systems_physics_constraints_diagnostic=none`,
  `gameplay_systems_physics_constraints_rows=2`, `gameplay_systems_physics_constraints_fixed_rows=1`,
  `gameplay_systems_physics_constraints_linear_axis_rows=1`, `gameplay_systems_physics_constraints_axis_limit_clamped=1`,
  `gameplay_systems_kinematic_motion_status=constrained`, `gameplay_systems_kinematic_motion_rows=2`,
  `gameplay_systems_vehicle_status=grounded`, `gameplay_systems_vehicle_diagnostic=none`, `gameplay_systems_vehicle_wheel_rows=4`,
  `gameplay_systems_vehicle_grounded_wheels=4`, and `gameplay_systems_vehicle_wheel_probe_hits=4`.
- Generated AI behavior should validate reusable value documents with `mirakana::BehaviorAuthoringDocument`, `mirakana::BehaviorAuthoringValidationContext`, and `mirakana::validate_behavior_authoring_document` before runtime evaluation. Keep action execution game-owned and avoid scripting, persistent blackboard services, middleware, or editor graph claims.
- Validate with the smallest relevant package/game lane first, then `tools/validate.ps1` at the slice gate.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
