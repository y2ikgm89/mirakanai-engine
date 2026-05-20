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
- Missing generated-game art/audio should use reviewed engine tooling such as `mirakana::PlaceholderAssetBundleRequest` / `mirakana::plan_placeholder_asset_bundle` to return a `PlaceholderAssetBundlePlan` with deterministic first-party placeholder source documents, changed-file hashes, provenance, and fail-closed diagnostics; package-authoring workflows can use `mirakana::PlaceholderAssetCookPackageRequest` / `mirakana::plan_placeholder_asset_cook_package` to route the generated source documents through registered source cook/package planning. Do not download external assets, bypass cook/package validation, or parse source assets at runtime.
- Reviewed generated 2D source atlas rows should use `mirakana::SpriteAtlasSourceFrameDesc` / `mirakana::SpriteAtlasSourceAuthoringDesc` / `mirakana::plan_sprite_atlas_source_authoring` to pack already-decoded RGBA8 frames into deterministic `GameEngine.TextureSource.v1` atlas source plus `GameEngine.SourceAssetRegistry.v1` texture rows before registered source cook/package planning. `DesktopRuntime2DPackage` scaffolds expose this as `spriteAtlasSourceAuthoringTargets` while keeping `source/assets/package.geassets` and `source/sprites/player_atlas.texture_source` out of `runtimePackageFiles`. Do not parse source images at runtime, create renderer/RHI residency, infer animation semantics, or use this helper as game-specific art direction.
- Generated `DesktopRuntime2DPackage` smokes report atlas-backed repeated scene sprite plan counters through `sprite_batch_plan_atlas_backed_batches`, `sprite_batch_plan_repeated_atlas_batches`, and `sprite_batch_plan_repeated_atlas_sprites` over adjacent cooked scene sprite rows. Treat these as deterministic package evidence only, not sprite sorting, production atlas packing, public native/RHI handle access, or broad production sprite batching readiness.
- Generated 2D gameplay can use `mirakana::RuntimeSpriteFlipbookClipDesc`, `mirakana::RuntimeSpriteFlipbookState`, and `mirakana::advance_runtime_sprite_flipbook` over cooked `RuntimeSpriteAnimationFrame` rows for deterministic named clip sampling before applying frames to scene sprite rows. `DesktopRuntime2DPackage` and `sample_2d_desktop_runtime_package` expose `sprite_flipbook_*` package counters through `--require-sprite-animation`; keep game-specific animation graphs/state machines in game code.
- Generated gameplay diagnostics should use `mirakana::ui::RuntimeGameplayDebugOverlayRowDesc` / `mirakana::ui::RuntimeGameplayDebugOverlayPlan` / `mirakana::ui::plan_runtime_gameplay_debug_overlay` for value-only gameplay debug overlay rows before renderer, telemetry, editor, or native UI presentation. Do not add game-local debug UI frameworks or command dispatchers to bypass `MK_ui`.
- Generated 3D renderer-quality package smokes keep exact frame graph evidence: base postprocess expects `renderer_quality_expected_framegraph_passes=2`, `renderer_quality_expected_framegraph_render_passes=4`, `renderer_quality_framegraph_render_passes_ok=1`, and `renderer_quality_expected_framegraph_barrier_steps=4`; depth/shadow variants add `framegraph_render_passes_recorded` and `framegraph_barrier_steps_executed` counts in `references/full-guidance.md`.
- Generated 2D/3D gameplay package smokes use public `MK_physics`, `MK_navigation`, and `MK_ai` only. For scene-ref routes use `NavigationNavmeshPathRequest` / `plan_navigation_navmesh_path`; for value-only crowd rows use `NavigationCrowdPlanRequest` / `plan_navigation_navmesh_crowd` alongside grid replanning and `calculate_navigation_local_avoidance`; keep navmesh asset import, spline/funnel smoothing, persistent/full crowd simulation beyond value-only batch planning, scene/physics-owned integration, middleware, and background navigation jobs out of ready claims.
- Generated gameplay collision queries should use public `MK_physics` value batches such as `PhysicsWorld2D::raycast_batch`, `PhysicsWorld3D::raycast_batch`, `PhysicsWorld2D::shape_sweep_batch`, and `PhysicsWorld3D::shape_sweep_batch`; consume `PhysicsCollisionQueryBatchStatus`, `PhysicsCollisionQueryBatchDiagnostic`, `PhysicsCollisionQueryRowStatus`, and `PhysicsCollisionQueryRowDiagnostic` diagnostics, rely on default-unbounded query counts unless a positive `max_queries` fail-closed budget is needed, expect selected package smokes to expose `collision_query_batch_ready` counters, and keep middleware/native handles or scene-owned dispatch out of generated-game code.
- Generated AI behavior should validate reusable value documents with `mirakana::BehaviorAuthoringDocument`, `mirakana::BehaviorAuthoringValidationContext`, and `mirakana::validate_behavior_authoring_document` before runtime evaluation. Keep action execution game-owned and avoid scripting, persistent blackboard services, middleware, or editor graph claims.
- Validate with the smallest relevant package/game lane first, then `tools/validate.ps1` at the slice gate.

## Detailed Reference

- `references/full-guidance.md`: detailed procedures, API inventory, retained row ids, package/backend/editor lanes, and detailed validation evidence. Load only the sections needed for the current task.
