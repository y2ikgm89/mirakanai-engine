# AI-Operable Game Engine Handoff Prompt

Use this prompt when opening a new chat to continue the AI-operable GameEngine production work.

## Prompt

```text
You are Codex working in G:\workspace\development\GameEngine.

Goal: Continue the AI-Operable Game Engine Production Roadmap v1. The engine should become a clean C++23 GameEngine that can create both 2D and 3D games through first-party scene, prefab, asset, package, runtime, renderer, editor-core, and validation contracts. Codex should be able to understand the engine, place assets, update scene/prefab/material/package data, write C++ gameplay, and run validation. Backward compatibility is not required; prefer clean breaking changes when needed.

Context to read first:
- AGENTS.md
- docs/README.md
- docs/architecture.md
- docs/roadmap.md
- docs/ai-integration.md
- docs/ai-game-development.md
- docs/superpowers/plans/README.md
- docs/specs/2026-05-01-ai-operable-game-engine-production-roadmap-v1-design.md
- docs/superpowers/plans/2026-05-01-ai-operable-production-loop-foundation-v1.md
- docs/superpowers/plans/2026-05-01-scene-component-prefab-schema-v2.md
- docs/superpowers/plans/2026-05-01-asset-identity-runtime-resource-v2.md
- docs/superpowers/plans/2026-05-01-renderer-rhi-resource-foundation-v1.md
- docs/superpowers/plans/2026-05-01-frame-graph-upload-staging-foundation-v1.md
- docs/superpowers/plans/2026-05-01-ai-command-surface-foundation-v1.md
- docs/superpowers/plans/2026-05-01-2d-playable-vertical-slice-foundation-v1.md
- docs/superpowers/plans/2026-05-01-2d-desktop-runtime-package-proof-v1.md
- docs/superpowers/plans/2026-05-01-3d-playable-vertical-slice-foundation-v1.md
- docs/superpowers/plans/2026-05-01-native-gpu-ui-overlay-foundation-v1.md
- docs/superpowers/plans/2026-05-01-native-ui-textured-sprite-atlas-foundation-v1.md
- docs/superpowers/plans/2026-05-01-native-ui-atlas-package-metadata-foundation-v1.md
- docs/superpowers/plans/2026-05-01-ui-atlas-metadata-authoring-tooling-v1.md
- docs/superpowers/plans/2026-05-01-ui-atlas-metadata-apply-tooling-v1.md
- docs/superpowers/plans/2026-05-01-source-asset-registration-command-tooling-v1.md
- docs/superpowers/plans/2026-05-01-scene-v2-runtime-package-migration-v1.md
- docs/superpowers/plans/2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md
- docs/superpowers/plans/2026-05-01-runtime-scene-package-validation-command-tooling-v1.md
- engine/agent/manifest.json

Required discovery:
- Run or inspect pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 before changing engine APIs or generating game code.
- Inspect productionLoop / engine/agent/manifest.json.aiOperableProductionLoop before choosing any game workflow.
- Inspect aiCommandSurfaces before changing manifest, package, scene, prefab, material, asset, or validation state. The command surface contract uses schemaVersion, requestModes, requestShape, resultShape, requiredModules, capabilityGates, hostGates, validationRecipes, unsupportedGapIds, and placeholder undoToken fields. Use `validate-runtime-scene-package` for non-mutating runtime scene package validation through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation` when an explicit `.geindex` package and scene key need reviewed load/instantiate diagnostics.
- Use `run-validation-recipe` only for allowlisted validation recipes through `tools/run-validation-recipe.ps1`; it does not evaluate arbitrary shell or raw manifest command strings, and free-form validation commands are unsupported.
- Inspect current game manifests under games/<game_name>/game.agent.json before selecting a game recipe.
- Use docs/specs/generated-game-validation-scenarios.md and docs/specs/game-prompt-pack.md for current supported generated-game constraints.
- Current enforced recipe ids are headless-gameplay, ai-navigation-headless, runtime-ui-headless, 2d-playable-source-tree, 2d-desktop-runtime-package, 3d-playable-desktop-package, native-gpu-runtime-ui-overlay, native-ui-textured-sprite-atlas, native-ui-atlas-package-metadata, desktop-runtime-config-package, desktop-runtime-cooked-scene-package, desktop-runtime-material-shader-package, and future-3d-playable-vertical-slice. The source-tree 2D recipe is ready; `2d-desktop-runtime-package` is host-gated and requires its focused validation recipes; `3d-playable-desktop-package` is host-gated for the `sample_desktop_runtime_game` package foundation only; `native-gpu-runtime-ui-overlay` is host-gated for the same sample package with `--require-native-ui-overlay`; `native-ui-textured-sprite-atlas` is host-gated for the cooked texture/atlas-backed UI image sprite proof with `--require-native-ui-textured-sprite-atlas`; `native-ui-atlas-package-metadata` is host-gated for package-authored UI atlas metadata with `ui_atlas_metadata_status=ready`; `ui-atlas-metadata-authoring-tooling-v1` is the ready cooked-metadata authoring surface for `GameEngine.UiAtlas.v1` via `mirakana::UiAtlasMetadataDocument`, `mirakana::author_cooked_ui_atlas_metadata`, and `mirakana::verify_cooked_ui_atlas_package_metadata` without source image decoding or production atlas packing; `ui-atlas-metadata-apply-tooling-v1`, `material-instance-apply-tooling-v1`, and `scene-package-apply-tooling-v1` are ready reviewed dry-run/apply surfaces for updating `.uiatlas`, `.material`, or first-party `GameEngine.Scene.v1` `.scene` content with matching `.geindex` rows; the future 3D recipe remains planned for broader generated 3D production.

Use subagents:
- This prompt authorizes parallel read-only subagents for discovery and risk review.
- Start with explorer for current AI/game workflow evidence, engine-architect for clean no-backcompat architecture sequencing, and rendering-auditor for renderer/RHI/GPU resource risks.
- Keep reviewer, explorer, architect, and auditor roles read-only.
- Use write-capable builder/fixer roles only for scoped implementation tasks with disjoint file ownership.

Use Context7 and official docs:
- Use Context7 or official documentation before relying on memory for CMake, vcpkg, SDL3, Dear ImGui, Direct3D 12, Vulkan, Metal, C++ tooling, Android, iOS, or platform SDK behavior.
- Keep repository architecture rules stronger than official sample shortcuts when samples expose native handles that GameEngine intentionally hides behind adapters.

Constraints:
- Respond in Japanese to the user.
- Keep code, commands, paths, schemas, and public API names in ASCII.
- Use C++23 and current project CMake/module policy.
- Do not introduce third-party dependencies without updating docs/legal-and-licensing.md, docs/dependencies.md, vcpkg.json, and THIRD_PARTY_NOTICES.md.
- Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, or backend RHI handles through gameplay-facing APIs.
- Keep engine/core independent from OS, GPU, asset format, renderer, platform, editor, SDL3, Dear ImGui, and UI middleware APIs.
- Keep runtime game UI on first-party mirakana_ui contracts.
- Treat D3D12 as the current strongest Windows lane, Vulkan as strict host/toolchain/runtime gated, and Metal/iOS as Apple-host gated until validated.
- Do not append new work to historical MVP plans.

Execution order:
1. Treat docs/superpowers/plans/2026-05-01-ai-operable-production-loop-foundation-v1.md as completed contract groundwork.
2. Treat docs/superpowers/plans/2026-05-01-scene-component-prefab-schema-v2.md as a completed contract-only `mirakana_scene` slice.
3. Treat docs/superpowers/plans/2026-05-01-asset-identity-runtime-resource-v2.md as a completed foundation-only asset/runtime slice.
4. Treat docs/superpowers/plans/2026-05-01-renderer-rhi-resource-foundation-v1.md as a completed foundation-only `mirakana_rhi` slice with `RhiResourceLifetimeRegistry`, resource ids, owner labels, debug names, deferred-release records, frame-indexed retirement, deterministic diagnostics, and marker-style events.
5. Treat docs/superpowers/plans/2026-05-01-frame-graph-upload-staging-foundation-v1.md as a completed foundation-only `mirakana_renderer`/`mirakana_rhi` slice with `FrameGraphV1Desc`, barrier intent rows, `RhiUploadStagingPlan`, buffer/texture copy rows, submitted `FenceValue` tracking, and completed-fence retirement.
6. Treat docs/superpowers/plans/2026-05-01-ai-command-surface-foundation-v1.md as a completed descriptor-only command surface slice. Ready apply remains narrow: register-runtime-package-files for reviewed runtimePackageFiles registration, register-source-asset for `GameEngine.SourceAssetRegistry.v1` rows plus deterministic `GameEngine.AssetIdentity.v2` and import-metadata projections, cook-registered-source-assets for explicitly selected source registry rows through existing import/package helpers, migrate-scene-v2-runtime-package for the reviewed bridge from authored `GameEngine.Scene.v2` plus source-registry rows into the existing `GameEngine.Scene.v1` `.scene` plus `.geindex` update surface, update-ui-atlas-metadata-package for cooked UI atlas metadata package row updates, create-material-instance for first-party material instance `.material` plus `.geindex` `AssetKind::material` / `material_texture` row updates, and update-scene-package for first-party `GameEngine.Scene.v1` `.scene` plus `.geindex` `AssetKind::scene` / `scene_mesh` / `scene_material` / `scene_sprite` row updates. Ready non-mutating execute surfaces include `validate-runtime-scene-package` for explicit package load plus runtime scene instantiation diagnostics.
7. Treat docs/superpowers/plans/2026-05-01-2d-playable-vertical-slice-foundation-v1.md as the completed source-tree 2D recipe slice once its validation evidence is present.
8. Treat docs/superpowers/plans/2026-05-01-2d-desktop-runtime-package-proof-v1.md as the completed host-gated `2d-desktop-runtime-package` recipe slice once its validation evidence is present.
9. Treat docs/superpowers/plans/2026-05-01-3d-playable-vertical-slice-foundation-v1.md as the completed host-gated `3d-playable-desktop-package` recipe slice once its validation evidence is present.
10. Treat docs/superpowers/plans/2026-05-01-native-gpu-ui-overlay-foundation-v1.md as the completed host-gated `native-gpu-runtime-ui-overlay` recipe slice once its validation evidence is present.
11. Treat docs/superpowers/plans/2026-05-01-native-ui-textured-sprite-atlas-foundation-v1.md as the completed host-gated `native-ui-textured-sprite-atlas` recipe slice once its validation evidence is present.
12. Treat docs/superpowers/plans/2026-05-01-ui-atlas-metadata-authoring-tooling-v1.md as the completed ready cooked-metadata authoring surface once its validation evidence is present.
13. Treat docs/superpowers/plans/2026-05-01-registered-source-asset-cook-package-command-tooling-v1.md and docs/superpowers/plans/2026-05-01-scene-v2-registered-asset-runtime-workflow-validation-v1.md as completed explicit source-row cook/package and workflow proof slices once their validation evidence is present.
14. Treat docs/superpowers/plans/2026-05-01-runtime-scene-package-validation-command-tooling-v1.md as the completed reviewed `validate-runtime-scene-package` slice once its validation evidence is present; it is non-mutating runtime scene package validation only and does not make broad package cooking, runtime source parsing, renderer/RHI residency, package streaming, editor productization, native handles, Metal, or general renderer quality ready.
15. Update schemas/static checks, tools/agent-context.ps1, docs/ai-game-development.md, docs/specs/generated-game-validation-scenarios.md, and docs/specs/game-prompt-pack.md whenever the contract changes.
16. Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-json-contracts.ps1, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ai-integration.ps1, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-public-api-boundaries.ps1 for public header changes, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1.

Done when:
- The active plan records validation evidence.
- pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes, or a concrete local environment blocker is recorded.
- engine/agent/manifest.json, docs, schemas, checks, and agent context agree.
- Future or broader 2D package, 3D/editor/GPU/resource work is listed as planned or host-gated unless actually implemented and validated.
```

## Production Loop Foundation Summary

Renderer/RHI Resource Foundation v1 is foundation-only in `mirakana_rhi`: `RhiResourceLifetimeRegistry` covers RHI resource ids, owner labels, debug names, generation-checked handles, deferred-release records, frame-indexed retirement, diagnostics, and marker-style lifetime events. It does not make native backend destruction migration, GPU allocator/residency budgets, package streaming, native upload execution, production render graph scheduling, GPU markers, editor resource panels, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness complete.

Frame Graph and Upload/Staging Foundation v1 is foundation-only in `mirakana_renderer` and `mirakana_rhi`: `FrameGraphV1Desc` covers CPU-side resource access declarations, imported/transient resource policy, pass ordering, and barrier intent rows, while `RhiUploadStagingPlan` covers upload/staging allocation rows, buffer/texture copy rows, submitted `FenceValue` tracking, and completed-fence retirement. It does not migrate postprocess/shadow execution paths from Frame Graph/Postprocess v0, execute native GPU uploads, implement upload rings, staging pools, async copy queues, allocator/residency budgets, package streaming, production renderer readiness, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness.

AI Command Surface Foundation v1 is descriptor-first in `engine/agent/manifest.json.aiOperableProductionLoop.commandSurfaces`: each command surface declares requestModes, requestShape, resultShape, requiredModules, capabilityGates, hostGates, validationRecipes, unsupportedGapIds, and a placeholder undoToken. Ready apply is limited to reviewed runtimePackageFiles registration, source asset registration through `GameEngine.SourceAssetRegistry.v1`, explicit registered source asset cook/package updates through existing import/package helpers, Scene v2 runtime package migration through the existing Scene v1 package update surface, cooked UI atlas metadata package row updates, first-party material instance package row updates, and first-party scene package row updates. `validate-runtime-scene-package` is a ready non-mutating runtime scene package validation surface through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation` for explicit `.geindex` package load plus `mirakana_runtime_scene` instantiation diagnostics. `run-validation-recipe` is a ready non-mutating dry-run/execute surface for the initial allowlisted validation recipes through `tools/run-validation-recipe.ps1`. It does not evaluate arbitrary shell or raw manifest command strings, and free-form validation commands are unsupported. Broad package cooking, broad manifest patching, material graph, shader graph, live shader generation, renderer/RHI residency, and package streaming commands remain planned or blocked until their own slices land.

The production-loop foundation slice completed the machine-readable recipe, command-surface, host-gate, unsupported-gap, validation-map, docs, and `agent-context` contract. Scene/Component/Prefab Schema v2 is a separate contract-only `mirakana_scene` data-spine slice: it covers stable authoring ids, schema-driven component rows, deterministic validation/serialization, and stable prefab property override paths, while the 2D source-tree slice adds `mirakana::validate_playable_2d_scene` for orthographic camera plus visible sprite readiness. The source asset registration slice adds reviewed `mirakana::plan_source_asset_registration` / `mirakana::apply_source_asset_registration` for source registry metadata only; it does not execute importers, write cooked artifacts, update `.geindex`, claim package cooking, renderer/RHI residency, package streaming, material/shader graphs, live shader generation, editor productization, public native/RHI handles, or Metal readiness. The registered source asset cook/package slice adds reviewed `mirakana::plan_registered_source_asset_cook_package` / `mirakana::apply_registered_source_asset_cook_package` for explicitly selected registry rows, deterministic cooked artifacts, and `.geindex` package updates through existing import/package helpers; dependency rows must be selected explicitly. The Scene v2 runtime package migration slice adds reviewed `mirakana::plan_scene_v2_runtime_package_migration` / `mirakana::apply_scene_v2_runtime_package_migration` for converting authored `GameEngine.Scene.v2` plus registered mesh/material/texture keys into the existing Scene v1 package update helper. The validated authored-to-runtime workflow is `register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene`; it is host-independent and ends before renderer-facing, desktop runtime, editor, or native GPU work. These slices do not execute external importers, broaden package cooking, cook dependent assets, parse source assets at runtime, create renderer/RHI residency, stream packages, productize the editor, expose native/RHI handles, make Metal ready, or claim general production renderer quality. The 2D desktop package proof is a separate host-gated recipe and does not claim tilemap/atlas production, 3D material graph, editor UX/productization, broad runtime package migration, nested prefab propagation/merge resolution UX, or Metal readiness. Asset Identity v2 is the closed identity/reference boundary, and Runtime Resource v2 is the closed Engine 1.0 reviewed safe-point/controller surface for generation-checked handles, explicit resident mount/cache operations, reviewed evictions, selected package-streaming safe points, package discovery/candidate load, hot-reload recook replacement, and registered asset watch-tick orchestration. They still do not complete scene/render/UI/gameplay reference cleanup, renderer/RHI residency, broad/background package streaming, native watcher ownership, native upload execution, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness.

2D Playable Vertical Slice Foundation v1 promotes only the source-tree recipe `2d-playable-source-tree`: deterministic runtime input actions, primary orthographic scene camera plus visible sprite validation, scene sprite renderer submission, first-party HUD submission, device-independent audio cue rendering, and `NullRenderer` proof through `games/sample_2d_playable_foundation`. 2D Desktop Runtime Package Proof v1 promotes a separate host-gated recipe `2d-desktop-runtime-package` through `games/sample_2d_desktop_runtime_package`: cooked first-party 2D scene package load, sprite texture/material, cooked audio payload, HUD submission, SDL3 desktop host, manifest-derived package files, installed package smoke, and deterministic `NullRenderer` fallback. It does not claim texture atlas cooking, tilemap editor UX, runtime image decoding, production sprite batching, package streaming, native GPU sprite output, 3D readiness, editor productization, or public native/RHI handle access.

3D Playable Vertical Slice Foundation v1 promotes only the host-gated sample recipe `3d-playable-desktop-package` through `games/sample_desktop_runtime_game`: cooked first-party config, `.geindex`, texture, mesh, material, and Scene.v1 payloads; static mesh scene submission; material instance intent; primary camera/controller status; HUD diagnostics counters; host-owned D3D12 scene GPU binding; strict Vulkan gated scene GPU binding; depth-aware postprocess; and sample fixed 3x3 PCF directional shadow package smoke. Native GPU UI Overlay Foundation v1 adds the separate host-gated `native-gpu-runtime-ui-overlay` proof for first-party `mirakana_ui_renderer` box/image-placeholder sprites through renderer-owned final overlay output, validated on D3D12 and strict Vulkan selected package lanes with `ui_overlay_status=ready`. Native UI Textured Sprite Atlas Foundation v1 adds the separate host-gated `native-ui-textured-sprite-atlas` proof for a cooked texture/atlas-backed UI image sprite with `ui_texture_overlay_status=ready`, `ui_texture_overlay_atlas_ready=1`, and positive texture bind/draw counters. Native UI Atlas Package Metadata Foundation v1 adds the separate host-gated `native-ui-atlas-package-metadata` proof for package-authored UI atlas metadata with `ui_atlas_metadata_status=ready`, `ui_atlas_metadata_pages=1`, and `ui_atlas_metadata_bindings=1`. These recipes do not claim runtime source asset parsing, source image decoding, runtime source PNG/JPEG image decoding, production atlas packing, material graph, shader graph, skeletal animation production path, GPU skinning, package streaming, production text/font/accessibility, Metal readiness, broad generated 3D production readiness, general production renderer quality, editor productization, or public native/RHI handle access. `future-3d-playable-vertical-slice` remains planned for the broader 3D production path.

Validation Recipe Runner Tooling v1 adds a reviewed runner for manifest-declared validation recipes that can dry-run and execute only allowlisted commands with host-gate diagnostics, without evaluating arbitrary shell input or broadening editor productization, package mutation, runtime source import, renderer/RHI residency, package streaming, shader/material graph, live shader generation, public native/RHI handle, Metal readiness, or general production renderer quality claims.
