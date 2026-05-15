# Game Prompt Pack

These prompt templates are intentionally constrained to current public GameEngine APIs. They assume the agent first runs or inspects `pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1`, then reads `engine/agent/manifest.json` and the target `game.agent.json`.

For engine-building sessions that continue the AI-operable production roadmap itself, use `docs/specs/2026-05-01-ai-operable-game-engine-handoff-prompt.md` before choosing a generated-game prompt.

## Production Loop Selection

```text
Goal: Select the supported GameEngine production-loop recipe before editing games/<game_name>.
Context: Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1, read productionLoop.recipes, and choose one of headless-gameplay, ai-navigation-headless, runtime-ui-headless, 2d-playable-source-tree, 2d-desktop-runtime-package, 3d-playable-desktop-package, native-gpu-runtime-ui-overlay, native-ui-textured-sprite-atlas, native-ui-atlas-package-metadata, desktop-runtime-config-package, desktop-runtime-cooked-scene-package, or desktop-runtime-material-shader-package. Treat 3d-playable-desktop-package as a host-gated sample package foundation, use native-gpu-runtime-ui-overlay only for the validated sample overlay proof, use native-ui-textured-sprite-atlas only for the validated cooked texture/atlas-backed UI image sprite proof, use native-ui-atlas-package-metadata only when the same proof must build image bindings from package-authored metadata, and treat future-3d-playable-vertical-slice as planned only.
Constraints: Do not treat host-gated recipes as ready until their validationRecipes and hostGates pass. Do not expose SDL3, Win32, D3D12, Vulkan, Metal, Dear ImGui, or RHI backend handles to gameplay code. Do not parse source PNG/glTF/audio/scene files at runtime.
Done when: The selected recipe id is reflected in the game spec and game.agent.json choices, unsupportedClaims are avoided, and the recipe's validation commands are listed before implementation starts.
```

## AI Command Surface Selection

```text
Goal: Choose a supported AI command surface before changing manifest, package, scene, prefab, material, asset, or validation state.
Context: Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 and inspect aiCommandSurfaces. Each descriptor has schemaVersion, requestModes, requestShape, resultShape, requiredModules, capabilityGates, hostGates, validationRecipes, unsupportedGapIds, and placeholder undoToken fields.
Constraints: Use only ready requestModes. `register-runtime-package-files` is ready only for reviewed runtimePackageFiles registration, `register-source-asset` is ready only for `GameEngine.SourceAssetRegistry.v1` rows plus deterministic `GameEngine.AssetIdentity.v2` and import-metadata projections, `cook-registered-source-assets` is ready only for explicitly selected registered source rows through reviewed first-party import/package helpers, `migrate-scene-v2-runtime-package` is ready only for the reviewed bridge from authored `GameEngine.Scene.v2` plus source-registry rows into the existing `GameEngine.Scene.v1` `.scene` plus `.geindex` update surface, `update-ui-atlas-metadata-package` is ready only for cooked `.uiatlas` plus matching `.geindex` row updates, `create-material-instance` is ready only for first-party material instance `.material` plus matching `.geindex` `AssetKind::material` / `material_texture` row updates, and `update-scene-package` is ready only for first-party `GameEngine.Scene.v1` `.scene` plus matching `.geindex` `AssetKind::scene` / `scene_mesh` / `scene_material` / `scene_sprite` row updates. `validate-runtime-scene-package` is ready only for non-mutating runtime scene package validation through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation`; it reads an explicit `.geindex`, loads the cooked package, and reports `mirakana_runtime_scene` instantiation diagnostics without package cooking, runtime source parsing, renderer/RHI residency, package streaming, or free-form edits. `run-validation-recipe` is ready only for allowlisted validation recipes through `tools/run-validation-recipe.ps1`; it does not evaluate arbitrary shell or raw manifest command strings, and free-form validation commands are unsupported. Treat broad package-cook/manifest-patch command surfaces as planned or blocked until their own apply slices land, and do not claim importer execution, unreviewed cooked artifact writes, material graphs, shader graphs, live shader generation, renderer/RHI residency, package streaming, editor productization, or prefab mutation from source asset registration.
Done when: The chosen command id and request mode are supported by aiCommandSurfaces, planned/blocked surfaces are reported as diagnostics with unsupportedGapIds, and follow-up validationRecipes are listed.
```

## Scene v2 Runtime Package Migration

```text
Goal: Convert authored Scene v2 source data into the current runtime-loadable Scene v1 package update surface through reviewed tools only.
Context: Run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/agent-context.ps1 and inspect register-source-asset, cook-registered-source-assets, Scene/Prefab v2 authoring commands, migrate-scene-v2-runtime-package, validate-runtime-scene-package, and run-validation-recipe. Read mirakana/scene/schema_v2.hpp, mirakana/assets/source_asset_registry.hpp, mirakana/tools/registered_source_asset_cook_package_tool.hpp, mirakana/tools/scene_v2_runtime_package_migration_tool.hpp, mirakana/tools/runtime_scene_package_validation_tool.hpp, mirakana/runtime/asset_runtime.hpp, mirakana/runtime_scene/runtime_scene.hpp, and the target game.agent.json.
Constraints: Use the validated authored-to-runtime workflow: register-source-asset -> cook-registered-source-assets -> migrate-scene-v2-runtime-package -> mirakana::runtime::load_runtime_asset_package -> mirakana::runtime_scene::instantiate_runtime_scene. Author Scene/Prefab v2 rows before migration. Do not execute external importers, broaden package cooking, add unselected dependent cooked asset rows outside existing package helpers, parse source assets at runtime, claim renderer/RHI residency, package streaming, material/shader graphs, live shader generation, editor productization, Metal readiness, public native/RHI handles, or general production renderer quality.
Done when: The source registry contains required mesh/material/texture keys, explicitly selected rows are cooked into the package, the authored Scene v2 document uses supported camera/light/mesh_renderer/sprite_renderer properties, `validate-runtime-scene-package` returns successful non-mutating runtime scene package validation through `mirakana::plan_runtime_scene_package_validation` and `mirakana::execute_runtime_scene_package_validation`, and unsupported gaps remain explicit.
```

## Scene/Component/Prefab Schema Contract

```text
Goal: Use the stable-id Scene/Component/Prefab Schema v2 contract while designing future AI scene edits.
Context: Read mirakana/scene/schema_v2.hpp and engine/agent/manifest.json.aiOperableProductionLoop.authoringSurfaces. SceneDocumentV2, SceneComponentDocumentV2, PrefabDocumentV2, and PrefabVariantDocumentV2 are contract-only mirakana_scene value APIs for stable authoring ids, component rows, deterministic validation, GameEngine.Scene.v2 text IO, and stable prefab override paths.
Constraints: Do not claim editor productization, broad package cooking, dependent asset cooking, nested prefab propagation/merge resolution UX, production 2D atlas/tilemap/native GPU readiness, or broad 3D playable vertical-slice readiness from this contract alone.
Done when: The design or code uses stable AuthoringId values and schema-driven component rows, and any unsupported production claims remain listed as follow-up work.
```

## Asset Identity And Runtime Resource Surface

```text
Goal: Use Asset Identity v2 as the closed identity/reference boundary and Runtime Resource v2 as the closed Engine 1.0 reviewed safe-point/controller surface.
Context: Read mirakana/assets/asset_identity.hpp, mirakana/assets/source_asset_registry.hpp, mirakana/runtime/resource_runtime.hpp, and engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps. Asset Identity v2 provides stable AssetKeyV2 rows, deterministic validation, AssetId derivation, GameEngine.AssetIdentity.v2 text IO, and plan_asset_identity_placements_v2 for reviewed placement resolution from keys into AssetId/kind/source rows. Source Asset Registry v1 provides GameEngine.SourceAssetRegistry.v1 rows for first-party source asset identity and deterministic import metadata planning. Runtime Resource v2 provides generation-checked handles, explicit resident mount/cache safe points, reviewed eviction planning/commit, selected package-streaming safe points, reviewed package discovery/candidate load, hot-reload recook replacement, and registered asset watch-tick orchestration.
Constraints: Do not claim renderer/RHI resource ownership, broad/background package streaming, native watcher ownership, automatic target inference, arbitrary/LRU eviction, upload/staging, package scripts, native handles, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness from these surfaces alone.
Done when: Asset references use stable keys, source asset registrations use register-source-asset or the source registry API, runtime references use generation-checked handles, and unsupported production claims remain listed as follow-up work.
```

## Renderer/RHI Resource Foundation

```text
Goal: Use Renderer/RHI Resource Foundation v1 only as a foundation-only backend-neutral lifetime contract.
Context: Read mirakana/rhi/resource_lifetime.hpp and engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps. RhiResourceLifetimeRegistry provides generation-checked RHI resource ids, resource kinds, owner labels, debug names, deferred-release records, frame-indexed retirement, deterministic diagnostics, and marker-style lifetime events.
Constraints: Do not claim native backend destruction migration, GPU allocator/residency budgets, package streaming, native upload execution, production render graph scheduling, GPU markers, editor resource panels, production renderer readiness, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness from this foundation alone.
Done when: Renderer/RHI lifetime references use RhiResourceLifetimeRegistry where this foundation is in scope, and unsupported production claims remain listed as follow-up work.
```

## Frame Graph And Upload/Staging Foundation

```text
Goal: Use Frame Graph and Upload/Staging Foundation v1 only as foundation-only backend-neutral planning contracts.
Context: Read mirakana/renderer/frame_graph.hpp, mirakana/rhi/upload_staging.hpp, and engine/agent/manifest.json.aiOperableProductionLoop.unsupportedProductionGaps. FrameGraphV1Desc provides explicit resource access rows, imported/transient resource policy, deterministic pass ordering, and barrier intent. RhiUploadStagingPlan provides staging allocation rows, buffer/texture copy rows, submitted FenceValue tracking, and completed-fence retirement.
Constraints: Do not claim native GPU uploads, upload rings, staging pools, async copy queues, allocator/residency budgets, package streaming, postprocess/shadow path migration, production renderer readiness, production 2D atlas/batching/native GPU readiness, or 3D playable vertical-slice readiness from these foundations alone.
Done when: Renderer/RHI planning references use FrameGraphV1Desc or RhiUploadStagingPlan where this foundation is in scope, and unsupported production claims remain listed as follow-up work.
```

## Headless Gameplay

```text
Goal: Build a C++23 headless gameplay prototype under games/<game_name>.
Context: Select production recipe headless-gameplay. Use mirakana::GameApp, mirakana::HeadlessRunner, mirakana::Registry, mirakana::ILogger, and public mirakana:: math/scene/assets APIs only.
Constraints: backendReadiness must use platform=headless, graphics=null, audio=device-independent, ui=mirakana_ui-headless. Do not add third-party dependencies or parse external source asset formats in game code.
Done when: The game executable proves one deterministic loop outcome, game.agent.json lists source-tree-default and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## Physics And Animation

```text
Goal: Add deterministic physics and animation behavior to games/<game_name>.
Context: Select production recipe headless-gameplay. Use mirakana::PhysicsWorld2D or mirakana::PhysicsWorld3D plus mirakana::AnimationStateMachine or mirakana::AnimationTimelinePlayback.
Constraints: Keep physics on first-party deterministic APIs. Do not add game-local middleware. Store authored intent in the game README and public source files.
Done when: The executable or tests prove contact resolution and animation state/event behavior, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## AI Navigation Decision

```text
Goal: Add a deterministic tile/grid AI navigation behavior to games/<game_name>.
Context: Select production recipe ai-navigation-headless. Use mirakana::BehaviorTreeDesc, mirakana::BehaviorTreeLeafResult, mirakana::BehaviorTreeBlackboard, mirakana::BehaviorTreeBlackboardCondition, mirakana::BehaviorTreeEvaluationContext, mirakana::evaluate_behavior_tree, mirakana::AiPerceptionAgent2D, mirakana::AiPerceptionTarget2D, mirakana::AiPerceptionSnapshot2D, mirakana::build_ai_perception_snapshot_2d, mirakana::write_ai_perception_blackboard, mirakana::NavigationGrid, mirakana::NavigationGridAgentPathRequest, mirakana::NavigationGridAgentPathPlan, mirakana::plan_navigation_grid_agent_path, mirakana::smooth_navigation_grid_path when manually inspecting planner smoothing behavior, mirakana::validate_navigation_grid_path, mirakana::replan_navigation_grid_path, mirakana::calculate_navigation_local_avoidance, and mirakana::update_navigation_agent. Model condition facts in a caller-owned blackboard snapshot and keep action execution in game code; the engine evaluator does not execute actions or own persistent world state.
Constraints: Keep the game headless unless another validated scenario is selected. Do not claim navmesh assets, polygon navigation, spline/funnel smoothing, full crowd simulation, scene/physics perception integration, async tasks, decorators/services, scene/physics navigation integration, blackboard persistence/services, middleware, or editor graph tooling.
Done when: The executable proves deterministic blackboard-driven behavior tree status/visited-node trace, blackboard condition count, path status, navigation final status, frame count, and final position, game.agent.json lists source-tree-default, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## Runtime UI Model

```text
Goal: Add a first-party runtime HUD or menu model to games/<game_name>.
Context: Select production recipe runtime-ui-headless. Use mirakana::ui::UiDocument, ElementDesc, solve_layout, build_renderer_submission, build_text_adapter_payload, MonospaceTextLayoutPolicy, InteractionState, TransitionState, BindingContext, and CommandRegistry. Use MonospaceTextLayoutPolicy only for dependency-free deterministic line/glyph placeholder layout in tests or simple HUD diagnostics. Use mirakana::UiRendererTheme and submit_ui_renderer_submission only when the game needs renderer-facing UI box submission through mirakana::IRenderer.
Constraints: Do not use Dear ImGui, SDL3, editor APIs, UI middleware, OS handles, fonts, image decoding, or renderer-specific UI code in the game.
Done when: Tests or executable output prove deterministic hierarchy, row/column gap layout, layout-backed renderer submission payloads, optional monospace text layout payloads, focus/navigation, binding, and command behavior, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## 2D Source-Tree Playable

```text
Goal: Build a deterministic source-tree 2D playable C++23 game under games/<game_name>.
Context: Select production recipe 2d-playable-source-tree. Use mirakana::runtime::RuntimeInputActionMap for input actions, mirakana::Scene with a primary orthographic mirakana::CameraComponent and visible mirakana::SpriteRendererComponent rows for world state, mirakana::validate_playable_2d_scene for readiness diagnostics, mirakana::build_scene_render_packet and mirakana::submit_scene_render_packet for sprite renderer intent, mirakana::ui::UiDocument plus mirakana::submit_ui_renderer_submission for HUD/menu submission, mirakana::AudioMixer for device-independent cue rendering, and mirakana::NullRenderer for validation.
Constraints: Select packagingTargets=["source-tree-default"] only. Do not claim SDL3 desktop runtime or desktop package proof from this recipe; select 2d-desktop-runtime-package for the separate host-gated package proof. Do not claim texture atlas cook, tilemap editor UX, runtime image decoding, production sprite batching, native GPU output, or public native/RHI handle access.
Done when: The game executable or tests prove deterministic input-driven movement, orthographic camera plus sprite validation, HUD submission, audio cue scheduling, renderer command counts, game.agent.json.productionRecipe=2d-playable-source-tree, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## 2D Desktop Runtime Package

```text
Goal: Build or validate a host-gated desktop/package 2D C++23 game under games/<game_name>.
Context: Select production recipe 2d-desktop-runtime-package. Use mirakana::runtime::load_runtime_asset_package for manifest-declared cooked files, mirakana::instantiate_runtime_scene_render_data for GameEngine.Scene.v1, mirakana::validate_playable_2d_scene for orthographic sprite readiness, mirakana::SdlDesktopGameHost for the optional desktop host, mirakana::submit_scene_render_packet for sprite renderer intent, mirakana::ui::UiDocument plus mirakana::submit_ui_renderer_submission for HUD, mirakana::AudioMixer for cooked audio payload playback, and mirakana::IRenderer with deterministic NullRenderer fallback.
Constraints: Select packagingTargets=["desktop-game-runtime","desktop-runtime-release"] only for registered desktop-runtime package targets with GAME_MANIFEST, finite smoke args, runtimePackageFiles, PACKAGE_FILES_FROM_MANIFEST, and package validation. Do not parse source PNG/glTF/audio/scene files at runtime. Do not claim texture atlas cook, tilemap editor UX, runtime image decoding, production sprite batching, package streaming, native GPU sprite output, 3D readiness, editor productization, or public native/RHI handle access.
Done when: The executable proves cooked package load, playable 2D scene validation, sprite/HUD/audio submission, deterministic fallback diagnostics, game.agent.json.productionRecipe=2d-desktop-runtime-package, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1 passes, and tools/package-desktop-runtime.ps1 -GameTarget <target> passes or records a concrete host dependency blocker.
```

## Renderer Intent

```text
Goal: Add deterministic render intent to games/<game_name>.
Context: Use mirakana::NullRenderer for headless renderer frame proof or mirakana::SceneRenderPacket with mirakana_scene_renderer helpers.
Constraints: Do not claim visible GPU output unless the selected runtimeBackendReadiness entry is validated for that backend and host.
Done when: Renderer command counts or CPU-readable contract output prove the intended frame behavior, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes.
```

## Desktop Runtime Shell

```text
Goal: Add or adapt games/<game_name> so it can run through the optional desktop game runtime shell.
Context: Select production recipe desktop-runtime-config-package, desktop-runtime-cooked-scene-package, or desktop-runtime-material-shader-package. For a new config-only package-ready desktop runtime game, start with pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimePackage. For a package-ready desktop runtime game that should start with a deterministic cooked scene/material package and --require-scene-package smoke, use pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimeCookedScenePackage. For a scaffold that should include source material/HLSL authoring inputs plus selected-target desktop shader artifacts, use pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimeMaterialShaderPackage. Keep gameplay in mirakana::GameApp. Prefer mirakana::SdlDesktopGameHost in the desktop host executable; use lower-level mirakana::DesktopGameRunner, DesktopHostServices, DesktopRunConfig, SdlRuntime, SdlWindow, SdlDesktopEventPump, SdlDesktopPresentation, VirtualInput, VirtualPointerInput, VirtualGamepadInput, and VirtualLifecycle only for host-adapter work.
Constraints: Select packagingTargets=["desktop-game-runtime"] for the optional desktop runtime lane and add "desktop-runtime-release" only when the game is registered with the desktop runtime package helper, has a matching GAME_MANIFEST, has a finite smoke command, and declares bundled config/assets in runtimePackageFiles with PACKAGE_FILES_FROM_MANIFEST unless literal CMake PACKAGE_FILES intentionally mirror the manifest. Source material and HLSL authoring files must not be listed in runtimePackageFiles. Do not use mirakana_editor, Dear ImGui, SDL headers, native OS handles, GPU handles, or RHI backend handles in gameplay/HUD code. D3D12 rendering is host-owned and requires build-output precompiled DXIL; Vulkan is toolchain/runtime gated; keep NullRenderer fallback diagnostics for missing artifacts or unavailable native presentation.
Done when: The sample or game executable opens a desktop window, exits cleanly on window close or game stop, game.agent.json lists desktop-game-runtime validation, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1 is run or a concrete local dependency blocker is recorded. If "desktop-runtime-release" is selected, run pwsh -NoProfile -ExecutionPolicy Bypass -File tools/package-desktop-runtime.ps1 for the default sample shell or tools/package-desktop-runtime.ps1 -GameTarget <target> for a selected target with registered manifest, package smoke metadata, and package file validation.
```

## Desktop Material/Shader Package

```text
Goal: Create a package-ready SDL3 desktop runtime game under games/<game_name> with first-party source material/HLSL authoring inputs and a cooked scene package.
Context: Select production recipe desktop-runtime-material-shader-package. Start with pwsh -NoProfile -ExecutionPolicy Bypass -File tools/new-game.ps1 -Name <game_name> -Template DesktopRuntimeMaterialShaderPackage. The scaffold provides source/materials/lit.material, shaders/runtime_scene.hlsl, shaders/runtime_postprocess.hlsl, runtime config, .geindex, and cooked texture/mesh/material/scene payloads. CMake owns host-built D3D12 DXIL artifacts and toolchain-gated Vulkan SPIR-V artifacts.
Constraints: Do not runtime-compile shaders, do not create a shader graph/material graph, do not expose native/RHI handles to gameplay, do not ship source material or HLSL files in runtimePackageFiles, and keep Metal/Apple validation host-gated until an Apple toolchain proves it.
Done when: tools/package-desktop-runtime.ps1 -GameTarget <target> validates the selected target package; run the Vulkan -RequireVulkanShaders smoke only on a host with DXC SPIR-V CodeGen, spirv-val, and Vulkan runtime/surface readiness.
```

## 3D Desktop Package Foundation

```text
Goal: Validate the current host-gated 3D desktop package foundation.
Context: Select production recipe 3d-playable-desktop-package, native-gpu-runtime-ui-overlay when the task explicitly requires native GPU UI overlay proof, native-ui-textured-sprite-atlas when the task explicitly requires the cooked texture/atlas-backed UI image sprite proof, or native-ui-atlas-package-metadata when the task explicitly requires package-authored UI atlas metadata to build those bindings. Use games/sample_desktop_runtime_game as the reference proof. The package contains cooked config, .geindex, texture, UI atlas metadata, mesh, material, and Scene.v1 files; gameplay runs as mirakana::GameApp under mirakana::SdlDesktopGameHost, consumes cooked packages through mirakana_runtime/mirakana_scene_renderer, ticks deterministic camera/controller movement through public input, submits HUD diagnostics through mirakana_ui/mirakana_ui_renderer, and leaves D3D12/Vulkan presentation, shader artifacts, scene GPU bindings, postprocess, shadows, native UI overlay, textured UI atlas binding, package atlas metadata validation, and fallback diagnostics inside host/renderer/RHI adapters. When editing cooked UI atlas metadata, use mirakana::UiAtlasMetadataDocument plus mirakana::author_cooked_ui_atlas_metadata or mirakana::verify_cooked_ui_atlas_package_metadata so GameEngine.UiAtlas.v1, AssetKind::ui_atlas, and ui_atlas_texture rows stay synchronized; use mirakana::plan_cooked_ui_atlas_package_update / mirakana::apply_cooked_ui_atlas_package_update when `.uiatlas` and `.geindex` must be updated together. When editing first-party material instances, use mirakana::MaterialInstanceDefinition plus mirakana::plan_material_instance_package_update / mirakana::apply_material_instance_package_update so GameEngine.MaterialInstance.v1, AssetKind::material, and material_texture rows stay synchronized without material graph, shader graph, live shader generation, renderer/RHI residency, or package streaming claims.
Constraints: Select packagingTargets=["desktop-game-runtime","desktop-runtime-release"] only for the registered sample target with GAME_MANIFEST, package smoke metadata, runtimePackageFiles, PACKAGE_FILES_FROM_MANIFEST, and selected package validation. Do not parse source assets at runtime. Do not claim material graph, shader graph, skeletal animation production path, GPU skinning, package streaming, production text/font/image/atlas/accessibility, Metal ready path, broad generated 3D production readiness, general production renderer quality, or public native/RHI handle access.
Done when: game.agent.json.productionRecipe=3d-playable-desktop-package, native-gpu-runtime-ui-overlay, native-ui-textured-sprite-atlas, or native-ui-atlas-package-metadata as appropriate, status output includes scene, camera, HUD, postprocess, shadow, scene GPU, and if selected UI overlay or UI atlas metadata diagnostics, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate-desktop-game-runtime.ps1 passes, pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-shader-toolchain.ps1 has been run, and tools/package-desktop-runtime.ps1 -GameTarget sample_desktop_runtime_game passes or records a concrete host dependency blocker. Run the strict Vulkan package recipe only on a host with DXC SPIR-V CodeGen, spirv-val, and Vulkan runtime/surface readiness; include --require-native-ui-overlay when validating native-gpu-runtime-ui-overlay and add --require-native-ui-textured-sprite-atlas when validating native-ui-textured-sprite-atlas or native-ui-atlas-package-metadata.
```

## Runtime Cooked Scene

```text
Goal: Load a cooked runtime scene/material package in games/<game_name> and submit it through renderer-neutral scene APIs.
Context: Select production recipe desktop-runtime-cooked-scene-package for a packaged generated desktop scaffold, or headless-gameplay for source-tree package-load tests. Use mirakana::runtime::load_runtime_asset_package with IFileSystem, then prefer mirakana::instantiate_runtime_scene_render_data for render flows so the game receives a deserialized mirakana::Scene, SceneMaterialPalette, and SceneRenderPacket with non-throwing diagnostics. Use mirakana::runtime::resolve_runtime_scene_materials only for low-level material-payload diagnostics before custom renderer integration, and use mirakana::runtime::inspect_runtime_asset_package or make_runtime_session_diagnostic_report for deterministic failure rows.
Constraints: Game code must consume cooked package records only. Do not parse source glTF/PNG/audio/material formats, do not reach into RuntimeAssetRecord::content outside typed runtime accessors, and do not expose renderer/RHI/backend handles.
Done when: A headless executable, desktop runtime smoke, or test proves load_runtime_asset_package -> instantiate_runtime_scene_render_data -> submit_scene_render_packet, reports deterministic diagnostics on failure, and pwsh -NoProfile -ExecutionPolicy Bypass -File tools/validate.ps1 passes. For generated desktop package scaffolds, tools/package-desktop-runtime.ps1 -GameTarget <target> must also pass when desktop-runtime-release is selected.
```
