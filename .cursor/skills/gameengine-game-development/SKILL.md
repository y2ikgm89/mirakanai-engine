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

# GameEngine game development (Cursor)

Full workflow lives in shared skills. Start with the short `SKILL.md` routers, then load the shared detailed references listed below only when exact game API names, manifest fields, package lanes, or mobile/desktop validation recipes are needed.

| Layer | Path |
| --- | --- |
| Claude Code | `.claude/skills/gameengine-game-development/SKILL.md` |
| Claude Code detailed reference | `.claude/skills/gameengine-game-development/references/full-guidance.md` |
| Codex | `.agents/skills/gameengine-game-development/SKILL.md` |
| Codex detailed reference | `.agents/skills/gameengine-game-development/references/full-guidance.md` |
| Baseline | `AGENTS.md` |

Games live under `games/<game_name>/` with `game.agent.json`; see `AGENTS.md` **AI-Driven Game Development**.

Runtime package payloads are byte-hashed. New text cooked/runtime extensions or `runtimePackageFiles` entries need matching `runtime/.gitattributes` `text eol=lf`, scaffold/static-check parity, and the narrowest package smoke before slice completion.

Missing generated-game art/audio should use reviewed engine tooling such as `mirakana::PlaceholderAssetBundleRequest` / `mirakana::plan_placeholder_asset_bundle` to return a `PlaceholderAssetBundlePlan` with deterministic first-party placeholder source documents, changed-file hashes, provenance, and fail-closed diagnostics; package-authoring workflows can use `mirakana::PlaceholderAssetCookPackageRequest` / `mirakana::plan_placeholder_asset_cook_package` to route generated source documents through registered source cook/package planning. Do not download external assets, bypass cook/package validation, or parse source assets at runtime.

Reviewed generated 2D source atlas rows should use `mirakana::SpriteAtlasSourceFrameDesc` / `mirakana::SpriteAtlasSourceAuthoringDesc` / `mirakana::plan_sprite_atlas_source_authoring` to pack already-decoded RGBA8 frames into deterministic `GameEngine.TextureSource.v1` atlas source plus `GameEngine.SourceAssetRegistry.v1` texture rows before registered source cook/package planning. `DesktopRuntime2DPackage` scaffolds expose this as `spriteAtlasSourceAuthoringTargets` while keeping `source/assets/package.geassets` and `source/sprites/player_atlas.texture_source` out of `runtimePackageFiles`. Do not parse source images at runtime, create renderer/RHI residency, infer animation semantics, or use this helper as game-specific art direction.

Generated gameplay diagnostics should use `mirakana::ui::RuntimeGameplayDebugOverlayRowDesc` / `mirakana::ui::RuntimeGameplayDebugOverlayPlan` / `mirakana::ui::plan_runtime_gameplay_debug_overlay` for value-only gameplay debug overlay rows before renderer, telemetry, editor, or native UI presentation. Do not add game-local debug UI frameworks or command dispatchers to bypass `MK_ui`.
