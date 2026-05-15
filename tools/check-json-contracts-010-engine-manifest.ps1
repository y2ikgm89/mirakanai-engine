#requires -Version 7.0
#requires -PSEdition Core

# Chapter 1 for check-json-contracts.ps1 static contracts.

$engine = Read-Json "engine/agent/manifest.json"
$gameAgentSchema = Read-Json "schemas/game-agent.schema.json"
Assert-GameSourceDirectoryLayout
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("runtimeSceneValidationTargets")) {
    Write-Error "schemas/game-agent.schema.json must define runtimeSceneValidationTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("materialShaderAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define materialShaderAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("packageStreamingResidencyTargets")) {
    Write-Error "schemas/game-agent.schema.json must define packageStreamingResidencyTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("atlasTilemapAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define atlasTilemapAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("prefabScenePackageAuthoringTargets")) {
    Write-Error "schemas/game-agent.schema.json must define prefabScenePackageAuthoringTargets"
}
if (-not $gameAgentSchema.properties.PSObject.Properties.Name.Contains("registeredSourceAssetCookTargets")) {
    Write-Error "schemas/game-agent.schema.json must define registeredSourceAssetCookTargets"
}
Assert-Properties $engine @("schemaVersion", "engine", "commands", "modules", "runtimeBackendReadiness", "importerCapabilities", "packagingTargets", "validationRecipes", "aiOperableProductionLoop", "aiSurfaces", "documentationPolicy", "sourcePolicy", "gameCodeGuidance", "aiDrivenGameWorkflow") "engine manifest"
$geNavigationModule = @($engine.modules | Where-Object { $_.name -eq "MK_navigation" })
if ($geNavigationModule.Count -ne 1) {
    Write-Error "engine manifest must declare exactly one MK_navigation module"
}
if ($geNavigationModule[0].status -ne "implemented-production-path-planner") {
    Write-Error "engine manifest MK_navigation status must be implemented-production-path-planner"
}
foreach ($header in @(
    "engine/navigation/include/mirakana/navigation/navigation_path_planner.hpp",
    "engine/navigation/include/mirakana/navigation/navigation_replan.hpp",
    "engine/navigation/include/mirakana/navigation/path_smoothing.hpp",
    "engine/navigation/include/mirakana/navigation/local_avoidance.hpp"
)) {
    if (@($geNavigationModule[0].publicHeaders) -notcontains $header) {
        Write-Error "engine manifest MK_navigation publicHeaders missing $header"
    }
}
foreach ($needle in @(
    "NavigationGridAgentPathRequest",
    "NavigationGridAgentPathPlan",
    "NavigationGridAgentPathStatus",
    "NavigationGridAgentPathDiagnostic",
    "plan_navigation_grid_agent_path",
    "navmesh",
    "crowd",
    "scene/physics integration",
    "editor visualization"
)) {
    if (-not ([string]$geNavigationModule[0].purpose).Contains($needle)) {
        Write-Error "engine manifest MK_navigation purpose missing $needle"
    }
}
if (-not $engine.gameCodeGuidance.PSObject.Properties.Name.Contains("currentNavigation")) {
    Write-Error "engine manifest gameCodeGuidance must declare currentNavigation"
}
foreach ($needle in @("NavigationGridAgentPathRequest", "NavigationGridAgentPathPlan", "plan_navigation_grid_agent_path", "navmesh", "crowd", "scene/physics integration", "editor visualization")) {
    if (-not ([string]$engine.gameCodeGuidance.currentNavigation).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentNavigation missing $needle"
    }
}
Assert-Properties $engine.gameCodeGuidance @("currentEditorContentBrowserImportDiagnostics") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorPrefabInstanceSourceLinks") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorInProcessRuntimeHost") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorGameModuleDriverLoad") "engine manifest gameCodeGuidance"
Assert-Properties $engine.gameCodeGuidance @("currentEditorRuntimeScenePackageValidationExecution") "engine manifest gameCodeGuidance"
foreach ($needle in @(
    "EditorInProcessRuntimeHostDesc",
    "EditorInProcessRuntimeHostModel",
    "EditorInProcessRuntimeHostBeginResult",
    "make_editor_in_process_runtime_host_model",
    "begin_editor_in_process_runtime_host_session",
    "make_editor_in_process_runtime_host_ui_model",
    "play_in_editor.in_process_runtime_host",
    "Begin In-Process Runtime Host",
    "already-linked caller-supplied IEditorPlaySessionDriver",
    "Dynamic game-module driver loading",
    "currentEditorGameModuleDriverLoad",
    "DesktopGameRunner embedding",
    "renderer/RHI uploads or handles",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorInProcessRuntimeHost).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorInProcessRuntimeHost missing: $needle"
    }
}
foreach ($needle in @(
    "DynamicLibrary",
    "load_dynamic_library",
    "resolve_dynamic_library_symbol",
    "LoadLibraryExW",
    "LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR",
    "LOAD_LIBRARY_SEARCH_SYSTEM32",
    "EditorGameModuleDriverApi",
    "EditorGameModuleDriverContractMetadataModel",
    "EditorGameModuleDriverContractMetadataRow",
    "GameEngine.EditorGameModuleDriver.v1",
    "mirakana_create_editor_game_module_driver_v1",
    "EditorGameModuleDriverLoadDesc",
    "EditorGameModuleDriverReloadDesc",
    "EditorGameModuleDriverReloadModel",
    "EditorGameModuleDriverUnloadDesc",
    "EditorGameModuleDriverUnloadModel",
    "EditorGameModuleDriverCreateResult",
    "make_editor_game_module_driver_contract_metadata_model",
    "make_editor_game_module_driver_contract_metadata_ui_model",
    "make_editor_game_module_driver_ctest_probe_evidence_model",
    "make_editor_game_module_driver_ctest_probe_evidence_ui_model",
    "MK_editor_game_module_driver_probe",
    "MK_editor_game_module_driver_load_tests",
    "make_editor_game_module_driver_load_model",
    "make_editor_game_module_driver_reload_model",
    "make_editor_game_module_driver_from_symbol",
    "make_editor_game_module_driver_load_ui_model",
    "make_editor_game_module_driver_reload_ui_model",
    "make_editor_game_module_driver_unload_model",
    "make_editor_game_module_driver_unload_ui_model",
    "EditorGameModuleDriverHostSessionPhase",
    "EditorGameModuleDriverHostSessionSnapshot",
    "make_editor_game_module_driver_host_session_snapshot",
    "make_editor_game_module_driver_host_session_ui_model",
    "ge.editor.editor_game_module_driver_host_session.v1",
    "play_in_editor.game_module_driver",
    "play_in_editor.game_module_driver.reload",
    "play_in_editor.game_module_driver.contract",
    "play_in_editor.game_module_driver.unload",
    "play_in_editor.game_module_driver.session",
    "policy_dll_mutation_order_guidance",
    "phase_idle_no_driver_order_review_load_then_load_library",
    "play_in_editor.game_module_driver.ctest_probe_evidence",
    "Game Module Driver",
    "Load Game Module Driver",
    "Reload Game Module Driver",
    "Unload Game Module Driver",
    "stopped-state reload",
    "Active-session hot reload",
    "stable third-party ABI",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorGameModuleDriverLoad).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorGameModuleDriverLoad missing: $needle"
    }
}
foreach ($needle in @(
    "EditorRuntimeScenePackageValidationExecutionDesc",
    "EditorRuntimeScenePackageValidationExecutionModel",
    "EditorRuntimeScenePackageValidationExecutionResult",
    "make_editor_runtime_scene_package_validation_execution_model",
    "execute_editor_runtime_scene_package_validation",
    "make_editor_runtime_scene_package_validation_execution_ui_model",
    "playtest_package_review.runtime_scene_validation",
    "Validate Runtime Scene Package",
    "RootedFileSystem",
    "Package cooking",
    "validation recipe execution",
    "package streaming"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorRuntimeScenePackageValidationExecution).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorRuntimeScenePackageValidationExecution missing: $needle"
    }
}
foreach ($needle in @(
    "Editor Scene Prefab Instance Refresh Review v1",
    "Editor Prefab Instance Local Child Refresh Resolution v1",
    "Editor Prefab Instance Stale Node Refresh Resolution v1",
    "Editor Nested Prefab Refresh Resolution v1",
    "ScenePrefabInstanceRefreshPlan",
    "ScenePrefabInstanceRefreshRow",
    "ScenePrefabInstanceRefreshResult",
    "ScenePrefabInstanceRefreshPolicy",
    "plan_scene_prefab_instance_refresh",
    "apply_scene_prefab_instance_refresh",
    "make_scene_prefab_instance_refresh_action",
    "make_scene_prefab_instance_refresh_ui_model",
    "scene_prefab_instance_refresh",
    "keep_local_child",
    "keep_local_children",
    "keep_stale_source_nodes_as_local",
    "keep_stale_source_node_as_local",
    "keep_nested_prefab_instances",
    "keep_nested_prefab_instance",
    "unsupported_nested_prefab_instance",
    "Refresh Prefab Instance",
    "Keep Local Children",
    "Keep Stale Source Nodes",
    "Keep Nested Prefab Instances",
    "preserves existing scene node state",
    "unsupported local children",
    "automatic nested prefab refresh"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorPrefabInstanceSourceLinks).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorPrefabInstanceSourceLinks missing: $needle"
    }
}
foreach ($needle in @(
    "Editor Content Browser Import Codec Adapter Review v1",
    "EditorContentBrowserImportExternalSourceCopyModel",
    "make_content_browser_import_external_source_copy_model",
    "make_content_browser_import_external_source_copy_ui_model",
    "content_browser_import.external_copy",
    "Copy External Sources",
    ".png",
    ".gltf",
    ".glb",
    ".wav",
    ".mp3",
    ".flac",
    "ExternalAssetImportAdapters",
    "arbitrary importer adapters",
    "arbitrary file import",
    "broad editor/importer readiness"
)) {
    if (-not ([string]$engine.gameCodeGuidance.currentEditorContentBrowserImportDiagnostics).Contains($needle)) {
        Write-Error "engine manifest gameCodeGuidance.currentEditorContentBrowserImportDiagnostics missing: $needle"
    }
}
Assert-Properties $engine.commands @("validate", "prepareWorktree", "removeMergedWorktree", "toolchainCheck", "directToolchainCheck", "bootstrapDeps", "build", "buildGui", "buildAssetImporters", "test", "dependencyCheck", "cppStandardCheck", "evaluateCpp23", "shaderToolchainCheck", "agentContext", "agentCheck", "newGame", "ciMatrixCheck", "classifyPrValidationTier") "engine manifest commands"
if ($engine.commands.removeMergedWorktree -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/remove-merged-worktree.ps1 -WorktreePath <path> [-BaseRef origin/main] [-BaseBranch main] [-Remote origin] [-LocalCheckoutPath <path>] [-DeleteLocalBranch]") {
    Write-Error "engine manifest commands.removeMergedWorktree must expose the guarded post-merge worktree cleanup command"
}
if ($engine.commands.ciMatrixCheck -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1") {
    Write-Error "engine manifest commands.ciMatrixCheck must expose pwsh -NoProfile -ExecutionPolicy Bypass -File tools/check-ci-matrix.ps1"
}
if ($engine.commands.classifyPrValidationTier -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/classify-pr-validation-tier.ps1 -ChangedPath <repo-relative-path>") {
    Write-Error "engine manifest commands.classifyPrValidationTier must expose pwsh -NoProfile -ExecutionPolicy Bypass -File tools/classify-pr-validation-tier.ps1 -ChangedPath <repo-relative-path>"
}
if (-not $engine.commands.newGame.Contains("DesktopRuntime2DPackage")) {
    Write-Error "engine manifest commands.newGame must expose DesktopRuntime2DPackage"
}
if (-not $engine.commands.newGame.Contains("DesktopRuntime3DPackage")) {
    Write-Error "engine manifest commands.newGame must expose DesktopRuntime3DPackage"
}
Assert-Properties $engine.documentationPolicy @("preferredMcp", "useFor", "secretStorage", "doNotStoreApiKeysInRepo") "engine manifest documentationPolicy"
Assert-Properties $engine.documentationPolicy @("entrypoints") "engine manifest documentationPolicy"
Assert-Properties $engine.documentationPolicy.entrypoints @("entrypoint", "currentStatus", "currentCapabilities", "workflows", "planRegistry", "specRegistry", "superpowersSpecRegistry", "activeRoadmap") "engine manifest documentationPolicy.entrypoints"
foreach ($docEntrypoint in @(
    $engine.documentationPolicy.entrypoints.entrypoint,
    $engine.documentationPolicy.entrypoints.currentStatus,
    $engine.documentationPolicy.entrypoints.currentCapabilities,
    $engine.documentationPolicy.entrypoints.workflows,
    $engine.documentationPolicy.entrypoints.planRegistry,
    $engine.documentationPolicy.entrypoints.specRegistry,
    $engine.documentationPolicy.entrypoints.superpowersSpecRegistry,
    $engine.documentationPolicy.entrypoints.activeRoadmap
)) {
    if (-not (Test-Path (Join-Path $root $docEntrypoint))) {
        Write-Error "engine manifest documentationPolicy.entrypoints references missing document: $docEntrypoint"
    }
}
Assert-Properties $engine.sourcePolicy @("vcpkgManifest", "vcpkgBaseline", "vcpkgBootstrapCommand", "vcpkgConfigurePolicy", "dependencyPolicy") "engine manifest sourcePolicy"
Assert-Properties $engine.runtimeBackendReadiness @("graphics", "audio", "ui", "physics", "platform") "engine manifest runtimeBackendReadiness"
Assert-Properties $engine.importerCapabilities @("runtimePolicy", "defaultSourceDocuments", "cookedArtifacts", "optionalDependencyFeature", "plannedExternalImporters") "engine manifest importerCapabilities"
if ($engine.commands.bootstrapDeps -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1" -or $engine.sourcePolicy.vcpkgBootstrapCommand -ne "pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1") {
    Write-Error "engine manifest must expose pwsh -NoProfile -ExecutionPolicy Bypass -File tools/bootstrap-deps.ps1 as the vcpkg bootstrap command"
}
if (-not ([string]$engine.sourcePolicy.vcpkgConfigurePolicy).Contains("VCPKG_MANIFEST_INSTALL=OFF")) {
    Write-Error "engine manifest sourcePolicy.vcpkgConfigurePolicy must keep vcpkg install disabled during CMake configure"
}
if ($engine.engine.language -ne "C++23") {
    Write-Error "engine manifest must declare C++23"
}
Assert-Properties $engine.engine @("stageStatus", "stageCompletion", "stageClosurePlan", "stageClosureNotes") "engine manifest engine"
if ($engine.engine.stage -ne "core-first-mvp") {
    Write-Error "engine manifest stage must remain core-first-mvp for the closed MVP foundation"
}
if ($engine.engine.stageStatus -ne "mvp-closed-not-commercial-complete") {
    Write-Error "engine manifest stageStatus must be mvp-closed-not-commercial-complete"
}
if ([string]::IsNullOrWhiteSpace($engine.engine.stageCompletion) -or
    -not ([string]$engine.engine.stageCompletion).Contains("not a commercial-engine completion claim")) {
    Write-Error "engine manifest stageCompletion must distinguish MVP closure from commercial engine completion"
}
if (-not (Test-Path (Join-Path $root $engine.engine.stageClosurePlan))) {
    Write-Error "engine manifest stageClosurePlan references missing document: $($engine.engine.stageClosurePlan)"
}
if ($null -eq $engine.engine.stageClosureNotes -or $engine.engine.stageClosureNotes.Count -lt 1) {
    Write-Error "engine manifest stageClosureNotes must list closure caveats"
}
if (-not ((@($engine.engine.stageClosureNotes) -join " ").Contains("Apple/iOS/Metal"))) {
    Write-Error "engine manifest stageClosureNotes must keep Apple/iOS/Metal host gating explicit"
}
if ($engine.documentationPolicy.preferredMcp -ne "context7") {
    Write-Error "engine manifest documentationPolicy.preferredMcp must be context7"
}
if (-not $engine.documentationPolicy.doNotStoreApiKeysInRepo) {
    Write-Error "engine manifest documentationPolicy must forbid storing API keys in the repository"
}

$vulkanBackendSource = Get-Content -LiteralPath (Join-Path $root "engine/rhi/vulkan/src/vulkan_backend.cpp") -Raw
if (-not $vulkanBackendSource.Contains("refresh_surface_probe_queue_family_snapshots") -or
    -not $vulkanBackendSource.Contains("same native instance handles")) {
    Write-Error "Vulkan surface support probe must refresh queue-family snapshots from the same native instance handles before vkGetPhysicalDeviceSurfaceSupportKHR"
}
$rhiPostprocessSource = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/rhi_postprocess_frame_renderer.cpp") -Raw
$rhiDirectionalShadowSource = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp") -Raw
if ($rhiPostprocessSource.Contains("void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`r`n    require_active_frame();`r`n    commands_->draw(3, 1);") -or
    $rhiPostprocessSource.Contains("void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`n    require_active_frame();`n    commands_->draw(3, 1);") -or
    $rhiDirectionalShadowSource.Contains("pending_sprites_")) {
    Write-Error "3D scene/postprocess renderers must not draw HUD sprites through the scene material pipeline"
}

$moduleNames = @{}
foreach ($module in $engine.modules) {
    Assert-Properties $module @("name", "path", "status", "dependencies", "publicHeaders", "purpose") "engine module"
    $moduleNames[$module.name] = $true
    foreach ($header in $module.publicHeaders) {
        if (-not (Test-Path (Join-Path $root $header))) {
            Write-Error "module '$($module.name)' references missing public header: $header"
        }
    }
}

$geSceneModule = @($engine.modules | Where-Object { $_.name -eq "MK_scene" })
if ($geSceneModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_scene module"
}
$geAssetsModule = @($engine.modules | Where-Object { $_.name -eq "MK_assets" })
if ($geAssetsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_assets module"
}
$geRuntimeModule = @($engine.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_runtime module"
}
$geAudioModule = @($engine.modules | Where-Object { $_.name -eq "MK_audio" })
if ($geAudioModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_audio module"
}
$gePhysicsModule = @($engine.modules | Where-Object { $_.name -eq "MK_physics" })
if ($gePhysicsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_physics module"
}
$geToolsModule = @($engine.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_tools module"
}
if ($geSceneModule[0].status -ne "implemented-scene-schema-v2-contract") {
    Write-Error "engine manifest MK_scene status must advertise the Scene/Component/Prefab Schema v2 contract slice honestly"
}
if (@($geSceneModule[0].publicHeaders) -notcontains "engine/scene/include/mirakana/scene/schema_v2.hpp") {
    Write-Error "engine manifest MK_scene publicHeaders must include schema_v2.hpp"
}
if (-not ([string]$geSceneModule[0].purpose).Contains("contract-only") -or
    -not ([string]$geSceneModule[0].purpose).Contains("GameEngine.Scene.v2") -or
    -not ([string]$geSceneModule[0].purpose).Contains("nested prefab propagation/merge resolution UX")) {
    Write-Error "engine manifest MK_scene purpose must describe Schema v2 as contract-only and keep follow-up limits explicit"
}
if ($geAssetsModule[0].status -ne "implemented-asset-identity-v2-foundation") {
    Write-Error "engine manifest MK_assets status must advertise the Asset Identity v2 foundation slice honestly"
}
if (@($geAssetsModule[0].publicHeaders) -notcontains "engine/assets/include/mirakana/assets/asset_identity.hpp") {
    Write-Error "engine manifest MK_assets publicHeaders must include asset_identity.hpp"
}
if (-not ([string]$geAssetsModule[0].purpose).Contains("Asset Identity v2") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("GameEngine.AssetIdentity.v2") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geAssetsModule[0].purpose).Contains("renderer/RHI residency")) {
    Write-Error "engine manifest MK_assets purpose must describe Asset Identity v2 as foundation-only and keep follow-up limits explicit"
}
if ($geRuntimeModule[0].status -ne "implemented-runtime-resource-v2-foundation") {
    Write-Error "engine manifest MK_runtime status must advertise the Runtime Resource v2 foundation slice honestly"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") {
    Write-Error "engine manifest MK_runtime publicHeaders must include resource_runtime.hpp"
}
if ($geAudioModule[0].status -ne "implemented-device-streaming-baseline") {
    Write-Error "engine manifest MK_audio status must advertise the audio device streaming baseline honestly"
}
if (-not ([string]$geAudioModule[0].purpose).Contains("AudioDeviceStreamRequest") -or
    -not ([string]$geAudioModule[0].purpose).Contains("AudioDeviceStreamPlan") -or
    -not ([string]$geAudioModule[0].purpose).Contains("plan_audio_device_stream") -or
    -not ([string]$geAudioModule[0].purpose).Contains("render_audio_device_stream_interleaved_float") -or
    -not ([string]$geAudioModule[0].purpose).Contains("does not open OS audio devices")) {
    Write-Error "engine manifest MK_audio purpose must describe the audio device stream planning APIs and OS-device boundary"
}
if ($gePhysicsModule[0].status -ne "implemented-physics-1-0-ready-surface") {
    Write-Error "engine manifest MK_physics status must advertise the Physics 1.0 ready surface honestly"
}
if (-not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterController3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("move_physics_character_controller_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsAuthoredCollisionScene3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("build_physics_world_3d_from_authored_collision_scene") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::aabb") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::sphere") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsShape3DDesc::capsule") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsQueryFilter3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::exact_shape_sweep") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsExactSphereCast3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::exact_sphere_cast") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContactPoint3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContactManifold3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::contact_manifolds") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("warm-start-safe") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DConfig") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DRow") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsContinuousStep3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::step_continuous") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DRowKind") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DRow") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsCharacterDynamicPolicy3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("evaluate_physics_character_dynamic_policy_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsJoint3DStatus") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDistanceJoint3DDesc") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsJointSolve3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("solve_physics_joints_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DStatus") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DDiagnostic") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DConfig") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DCounts") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsReplaySignature3D") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsDeterminismGate3DResult") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("make_physics_replay_signature_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("evaluate_physics_determinism_gate_3d") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("validated Physics 1.0 ready surface") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("explicit Jolt/native middleware exclusion") -or
    -not ([string]$gePhysicsModule[0].purpose).Contains("PhysicsWorld3D::step remains discrete")) {
    Write-Error "engine manifest MK_physics purpose must describe exact shape sweeps, contact manifold stability, CCD foundation, character/dynamic policy, joints foundation, and benchmark determinism gates honestly"
}
if (-not ([string]$geRuntimeModule[0].purpose).Contains("Runtime Resource v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("generation-checked") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("commit_runtime_resident_package_replace_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("commit_runtime_resident_package_unmount_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("host-driven reviewed hot-reload replacement safe point") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("plan_runtime_package_hot_reload_candidate_review_v2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("RuntimePackageHotReloadCandidateReviewResultV2") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("file watching/recook execution") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("package streaming") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("renderer/RHI resource ownership")) {
    Write-Error "engine manifest MK_runtime purpose must describe Runtime Resource v2 as foundation-only and keep follow-up limits explicit"
}
if (-not ([string]$geRuntimeModule[0].purpose).Contains("RuntimeInputRebindingPresentationModel") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("make_runtime_input_rebinding_presentation") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("symbolic glyph lookup keys") -or
    -not ([string]$geRuntimeModule[0].purpose).Contains("platform input glyph generation")) {
    Write-Error "engine manifest MK_runtime purpose must describe runtime input rebinding presentation rows and platform glyph follow-up boundary"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PngImageDecodingAdapter") -or
    -not ([string]$geToolsModule[0].purpose).Contains("IImageDecodingAdapter") -or
    -not ([string]$geToolsModule[0].purpose).Contains("decode_audited_png_rgba8")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI PNG image decoding adapter boundary explicitly"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PackedUiAtlasAuthoringDesc") -or
    -not ([string]$geToolsModule[0].purpose).Contains("author_packed_ui_atlas_from_decoded_images") -or
    -not ([string]$geToolsModule[0].purpose).Contains("plan_packed_ui_atlas_package_update") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.UiAtlas.v1")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI decoded image atlas package bridge explicitly"
}
if (-not ([string]$geToolsModule[0].purpose).Contains("PackedUiGlyphAtlasAuthoringDesc") -or
    -not ([string]$geToolsModule[0].purpose).Contains("author_packed_ui_glyph_atlas_from_rasterized_glyphs") -or
    -not ([string]$geToolsModule[0].purpose).Contains("plan_packed_ui_glyph_atlas_package_update") -or
    -not ([string]$geToolsModule[0].purpose).Contains("UiAtlasMetadataGlyph") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.CookedTexture.v1") -or
    -not ([string]$geToolsModule[0].purpose).Contains("GameEngine.UiAtlas.v1")) {
    Write-Error "engine manifest MK_tools purpose must describe Runtime UI glyph atlas package bridge explicitly"
}

$runtimeCategories = @("graphics", "audio", "ui", "physics", "platform")
foreach ($category in $runtimeCategories) {
    foreach ($backend in $engine.runtimeBackendReadiness.$category) {
        Assert-Properties $backend @("name", "module", "status", "hosts", "validation", "notes") "runtime backend readiness '$category'"
        if (-not $moduleNames.ContainsKey($backend.module)) {
            Write-Error "runtime backend readiness '$category/$($backend.name)' references unknown module: $($backend.module)"
        }
        if ($backend.hosts.Count -lt 1) {
            Write-Error "runtime backend readiness '$category/$($backend.name)' must declare at least one host"
        }
    }
}

if ($engine.importerCapabilities.optionalDependencyFeature -ne "asset-importers") {
    Write-Error "engine manifest importerCapabilities.optionalDependencyFeature must be asset-importers"
}
foreach ($importer in $engine.importerCapabilities.plannedExternalImporters) {
    Assert-Properties $importer @("format", "dependency", "status", "ownerModule") "engine manifest plannedExternalImporters"
    if (-not $moduleNames.ContainsKey($importer.ownerModule)) {
        Write-Error "planned external importer '$($importer.format)' references unknown owner module: $($importer.ownerModule)"
    }
}

$packagingTargetNames = @{}
foreach ($target in $engine.packagingTargets) {
    Assert-Properties $target @("name", "platform", "status", "command", "notes") "engine manifest packagingTargets"
    $packagingTargetNames[$target.name] = $true
}

foreach ($recipe in $engine.validationRecipes) {
    Assert-Properties $recipe @("name", "command", "purpose") "engine manifest validationRecipes"
    if ([string]::IsNullOrWhiteSpace($recipe.command)) {
        Write-Error "engine manifest validation recipe '$($recipe.name)' must declare a command"
    }
}
$validationRecipeNames = @{}
foreach ($recipe in $engine.validationRecipes) {
    if ($validationRecipeNames.ContainsKey($recipe.name)) {
        Write-Error "engine manifest validationRecipes has duplicate recipe name: $($recipe.name)"
    }
    $validationRecipeNames[$recipe.name] = $true
}

$productionLoop = $engine.aiOperableProductionLoop
Assert-Properties $productionLoop @("schemaVersion", "design", "foundationPlan", "currentActivePlan", "recommendedNextPlan", "recipeStatusEnum", "recipes", "commandSurfaces", "authoringSurfaces", "packageSurfaces", "physicsBackendAdapterDecisions", "unsupportedProductionGaps", "hostGates", "validationRecipeMap", "reviewLoops", "resourceExecutionLoops", "materialShaderAuthoringLoops", "atlasTilemapAuthoringLoops", "packageStreamingResidencyLoops", "safePointPackageReplacementLoops") "engine manifest aiOperableProductionLoop"
if ($productionLoop.schemaVersion -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop.schemaVersion must be 1"
}
foreach ($productionLoopDoc in @($productionLoop.design, $productionLoop.foundationPlan, $productionLoop.currentActivePlan)) {
    if (-not (Test-Path (Join-Path $root $productionLoopDoc))) {
        Write-Error "engine manifest aiOperableProductionLoop references missing document: $productionLoopDoc"
    }
}
if ($productionLoop.recommendedNextPlan.PSObject.Properties.Name.Contains("path") -and
    -not (Test-Path (Join-Path $root $productionLoop.recommendedNextPlan.path))) {
    Write-Error "engine manifest aiOperableProductionLoop recommendedNextPlan references missing document: $($productionLoop.recommendedNextPlan.path)"
}
Assert-ActiveProductionPlanDrift $productionLoop
$physicsBackendAdapterDecision = @($productionLoop.physicsBackendAdapterDecisions | Where-Object { $_.id -eq "physics-1-0-jolt-native-adapter" })
if ($physicsBackendAdapterDecision.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must record one physics-1-0-jolt-native-adapter decision"
}
Assert-Properties $physicsBackendAdapterDecision[0] @("id", "status", "decision", "futureGate", "unsupportedClaims") "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions"
if ($physicsBackendAdapterDecision[0].status -ne "excluded-from-1-0-ready-surface") {
    Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions status must be excluded-from-1-0-ready-surface"
}
foreach ($needle in @("first-party MK_physics only", "vcpkg manifest feature", "fail-closed capability negotiation")) {
    if (-not ((([string]$physicsBackendAdapterDecision[0].decision), ([string]$physicsBackendAdapterDecision[0].futureGate)) -join " ").Contains($needle)) {
        Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions missing decision/futureGate text: $needle"
    }
}
foreach ($claim in @("Jolt runtime integration", "native physics handles in public gameplay APIs", "middleware type exposure")) {
    if (@($physicsBackendAdapterDecision[0].unsupportedClaims) -notcontains $claim) {
        Write-Error "engine manifest aiOperableProductionLoop physicsBackendAdapterDecisions unsupportedClaims missing: $claim"
    }
}
$allowedProductionStatuses = @("ready", "host-gated", "planned", "blocked")
foreach ($status in @("ready", "host-gated", "planned", "blocked")) {
    if (@($productionLoop.recipeStatusEnum) -notcontains $status) {
        Write-Error "engine manifest aiOperableProductionLoop.recipeStatusEnum missing status: $status"
    }
}
$editorPlaytestReviewLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-playtest-package-review-loop" })
if ($editorPlaytestReviewLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-playtest-package-review-loop review loop"
}
if ($editorPlaytestReviewLoop.Count -eq 1) {
    Assert-Properties $editorPlaytestReviewLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preSmokeGate", "hostGatedSmokeRecipes", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-playtest-package-review-loop"
    if ($editorPlaytestReviewLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-playtest-package-review-loop must be ready inside its narrow review scope"
    }
    $expectedEditorReviewSteps = @(
        "review-editor-package-candidates",
        "apply-reviewed-runtime-package-files",
        "select-runtime-scene-validation-target",
        "validate-runtime-scene-package",
        "run-host-gated-desktop-smoke"
    )
    $actualEditorReviewSteps = @($editorPlaytestReviewLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "notes") "engine manifest editor-playtest-package-review-loop ordered step"
        $_.id
    })
    if (($actualEditorReviewSteps -join "|") -ne ($expectedEditorReviewSteps -join "|")) {
        Write-Error "engine manifest editor-playtest-package-review-loop orderedSteps must be exactly: $($expectedEditorReviewSteps -join ', ')"
    }
    foreach ($step in @($editorPlaytestReviewLoop[0].orderedSteps)) {
        if ($step.status -ne "ready" -and $step.status -ne "host-gated") {
            Write-Error "engine manifest editor-playtest-package-review-loop step '$($step.id)' has invalid status: $($step.status)"
        }
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "validationRecipes")) {
        if (@($editorPlaytestReviewLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-playtest-package-review-loop requiredManifestFields missing: $field"
        }
    }
    if ($editorPlaytestReviewLoop[0].preSmokeGate -ne "validate-runtime-scene-package") {
        Write-Error "engine manifest editor-playtest-package-review-loop preSmokeGate must be validate-runtime-scene-package"
    }
    foreach ($recipe in @("desktop-game-runtime", "desktop-runtime-release-target", "installed-d3d12-3d-package-smoke", "installed-d3d12-3d-directional-shadow-smoke", "installed-d3d12-3d-shadow-morph-composition-smoke", "desktop-runtime-release-target-vulkan-toolchain-gated", "desktop-runtime-release-target-vulkan-directional-shadow-toolchain-gated")) {
        if (@($editorPlaytestReviewLoop[0].hostGatedSmokeRecipes) -notcontains $recipe) {
            Write-Error "engine manifest editor-playtest-package-review-loop hostGatedSmokeRecipes missing: $recipe"
        }
    }
    foreach ($claim in @("broad package cooking", "runtime source parsing", "renderer/RHI residency", "package streaming", "editor productization", "native handles", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorPlaytestReviewLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-playtest-package-review-loop unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPackageDiagnosticsLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-package-authoring-diagnostics" })
if ($editorAiPackageDiagnosticsLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-package-authoring-diagnostics review loop"
}
if ($editorAiPackageDiagnosticsLoop.Count -eq 1) {
    Assert-Properties $editorAiPackageDiagnosticsLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "diagnosticInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-package-authoring-diagnostics"
    if ($editorAiPackageDiagnosticsLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-package-authoring-diagnostics must be ready as diagnostics-only editor-core model"
    }
    $expectedEditorAiDiagnosticsSteps = @(
        "collect-editor-package-candidate-diagnostics",
        "summarize-manifest-descriptor-rows",
        "inspect-runtime-package-payload-diagnostics",
        "summarize-validation-recipe-status",
        "report-host-gated-desktop-smoke-preflight"
    )
    $actualEditorAiDiagnosticsSteps = @($editorAiPackageDiagnosticsLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-package-authoring-diagnostics ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiDiagnosticsSteps -join "|") -ne ($expectedEditorAiDiagnosticsSteps -join "|")) {
        Write-Error "engine manifest editor-ai-package-authoring-diagnostics orderedSteps must be exactly: $($expectedEditorAiDiagnosticsSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPackageDiagnosticsLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("package candidate rows", "descriptor summary rows", "runtime package payload diagnostics", "validation recipe status")) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].diagnosticInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics diagnosticInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "free-form manifest edits", "broad package cooking", "runtime source parsing", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPackageDiagnosticsLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-package-authoring-diagnostics unsupportedClaims missing: $claim"
        }
    }
}
$editorAiValidationRecipePreflightLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-validation-recipe-preflight" })
if ($editorAiValidationRecipePreflightLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-validation-recipe-preflight review loop"
}
if ($editorAiValidationRecipePreflightLoop.Count -eq 1) {
    Assert-Properties $editorAiValidationRecipePreflightLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "preflightInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-validation-recipe-preflight"
    if ($editorAiValidationRecipePreflightLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-validation-recipe-preflight must be ready as preflight-only editor-core model"
    }
    $expectedEditorAiValidationPreflightSteps = @(
        "collect-manifest-validation-recipes",
        "collect-reviewed-dry-run-plans",
        "map-selected-recipes-to-host-gates",
        "report-blocked-or-unsupported-validation-claims"
    )
    $actualEditorAiValidationPreflightSteps = @($editorAiValidationRecipePreflightLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-validation-recipe-preflight ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiValidationPreflightSteps -join "|") -ne ($expectedEditorAiValidationPreflightSteps -join "|")) {
        Write-Error "engine manifest editor-ai-validation-recipe-preflight orderedSteps must be exactly: $($expectedEditorAiValidationPreflightSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiValidationRecipePreflightLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("manifest validationRecipes", "run-validation-recipe dry-run plans", "host gates", "blocked reasons")) {
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].preflightInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight preflightInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "package script execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiValidationRecipePreflightLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-validation-recipe-preflight unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestReadinessReportLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-readiness-report" })
if ($editorAiPlaytestReadinessReportLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-readiness-report review loop"
}
if ($editorAiPlaytestReadinessReportLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestReadinessReportLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "reportInputs", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-readiness-report"
    if ($editorAiPlaytestReadinessReportLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-readiness-report must be ready as read-only editor-core model"
    }
    $expectedEditorAiReadinessSteps = @(
        "collect-package-authoring-diagnostics",
        "collect-validation-recipe-preflight",
        "aggregate-readiness-status",
        "report-readiness-blockers"
    )
    $actualEditorAiReadinessSteps = @($editorAiPlaytestReadinessReportLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-readiness-report ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiReadinessSteps -join "|") -ne ($expectedEditorAiReadinessSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-readiness-report orderedSteps must be exactly: $($expectedEditorAiReadinessSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestReadinessReportLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPackageAuthoringDiagnosticsModel", "EditorAiValidationRecipePreflightModel", "host gates", "blocking diagnostics")) {
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].reportInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report reportInputs missing: $requiredInput"
        }
    }
    foreach ($claim in @("arbitrary shell", "free-form manifest edits", "validation execution", "package script execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestReadinessReportLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-readiness-report unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestOperatorHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-handoff" })
if ($editorAiPlaytestOperatorHandoffLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-handoff review loop"
}
if ($editorAiPlaytestOperatorHandoffLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestOperatorHandoffLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "handoffInputs", "commandFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-operator-handoff"
    if ($editorAiPlaytestOperatorHandoffLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-operator-handoff must be ready as read-only editor-core handoff model"
    }
    $expectedEditorAiOperatorHandoffSteps = @(
        "collect-readiness-report",
        "collect-validation-preflight-commands",
        "assemble-reviewed-operator-command-rows",
        "report-host-gates-blockers-and-unsupported-claims"
    )
    $actualEditorAiOperatorHandoffSteps = @($editorAiPlaytestOperatorHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-operator-handoff ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiOperatorHandoffSteps -join "|") -ne ($expectedEditorAiOperatorHandoffSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-operator-handoff orderedSteps must be exactly: $($expectedEditorAiOperatorHandoffSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestOperatorHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestReadinessReportModel", "EditorAiValidationRecipePreflightModel", "run-validation-recipe dry-run command plan data", "host gates", "blocked reasons")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].handoffInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff handoffInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "reviewed command display", "argv plan data", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].commandFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff commandFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestOperatorHandoffLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-operator-handoff unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestEvidenceSummaryLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-evidence-summary" })
if ($editorAiPlaytestEvidenceSummaryLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-evidence-summary review loop"
}
if ($editorAiPlaytestEvidenceSummaryLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestEvidenceSummaryLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "evidenceInputs", "evidenceFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-evidence-summary"
    if ($editorAiPlaytestEvidenceSummaryLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-evidence-summary must be ready as read-only editor-core evidence summary model"
    }
    $expectedEditorAiEvidenceSummarySteps = @(
        "collect-operator-handoff",
        "collect-external-validation-evidence",
        "summarize-evidence-status",
        "report-evidence-blockers-and-unsupported-claims"
    )
    $actualEditorAiEvidenceSummarySteps = @($editorAiPlaytestEvidenceSummaryLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-evidence-summary ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiEvidenceSummarySteps -join "|") -ne ($expectedEditorAiEvidenceSummarySteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-evidence-summary orderedSteps must be exactly: $($expectedEditorAiEvidenceSummarySteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestEvidenceSummaryLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestOperatorHandoffModel", "externally supplied run-validation-recipe execute results", "recipe status", "exit code or summary text", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary evidenceInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "handoff row", "status passed failed blocked host-gated missing", "exit code", "summary text", "host gates", "blockers", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].evidenceFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary evidenceFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestEvidenceSummaryLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-evidence-summary unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestRemediationQueueLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-remediation-queue" })
if ($editorAiPlaytestRemediationQueueLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-queue review loop"
}
if ($editorAiPlaytestRemediationQueueLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestRemediationQueueLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "queueInputs", "queueFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-remediation-queue"
    if ($editorAiPlaytestRemediationQueueLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-remediation-queue must be ready as read-only editor-core remediation queue model"
    }
    $expectedEditorAiRemediationQueueSteps = @(
        "collect-evidence-summary",
        "classify-remediation-rows",
        "prioritize-remediation-categories",
        "report-remediation-blockers-and-unsupported-claims"
    )
    $actualEditorAiRemediationQueueSteps = @($editorAiPlaytestRemediationQueueLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-remediation-queue ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiRemediationQueueSteps -join "|") -ne ($expectedEditorAiRemediationQueueSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-remediation-queue orderedSteps must be exactly: $($expectedEditorAiRemediationQueueSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestRemediationQueueLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestEvidenceSummaryModel", "failed evidence", "blocked evidence", "missing evidence", "host-gated evidence", "host gates", "blocked reasons", "readiness dependency")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue queueInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "evidence status", "remediation category", "next-action text", "host gates", "blockers", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].queueFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue queueFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestRemediationQueueLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-queue unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestRemediationHandoffLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-remediation-handoff" })
if ($editorAiPlaytestRemediationHandoffLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-remediation-handoff review loop"
}
if ($editorAiPlaytestRemediationHandoffLoop.Count -eq 1) {
    Assert-Properties $editorAiPlaytestRemediationHandoffLoop[0] @("id", "status", "owner", "summary", "orderedSteps", "requiredManifestFields", "handoffInputs", "handoffFields", "blockedExecution", "unsupportedClaims", "notes") "engine manifest aiOperableProductionLoop editor-ai-playtest-remediation-handoff"
    if ($editorAiPlaytestRemediationHandoffLoop[0].status -ne "ready") {
        Write-Error "engine manifest editor-ai-playtest-remediation-handoff must be ready as read-only editor-core remediation handoff model"
    }
    $expectedEditorAiRemediationHandoffSteps = @(
        "collect-remediation-queue",
        "map-remediation-rows-to-external-actions",
        "assemble-external-operator-handoff-rows",
        "report-remediation-handoff-blockers-and-unsupported-claims"
    )
    $actualEditorAiRemediationHandoffSteps = @($editorAiPlaytestRemediationHandoffLoop[0].orderedSteps | ForEach-Object {
        Assert-Properties $_ @("id", "surface", "status", "mutates", "executes", "notes") "engine manifest editor-ai-playtest-remediation-handoff ordered step"
        if ($_.mutates -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff step '$($_.id)' must not mutate"
        }
        if ($_.executes -ne $false) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff step '$($_.id)' must not execute"
        }
        $_.id
    })
    if (($actualEditorAiRemediationHandoffSteps -join "|") -ne ($expectedEditorAiRemediationHandoffSteps -join "|")) {
        Write-Error "engine manifest editor-ai-playtest-remediation-handoff orderedSteps must be exactly: $($expectedEditorAiRemediationHandoffSteps -join ', ')"
    }
    foreach ($field in @("runtimePackageFiles", "runtimeSceneValidationTargets", "prefabScenePackageAuthoringTargets", "registeredSourceAssetCookTargets", "validationRecipes")) {
        if (@($editorAiPlaytestRemediationHandoffLoop[0].requiredManifestFields) -notcontains $field) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff requiredManifestFields missing: $field"
        }
    }
    foreach ($requiredInput in @("EditorAiPlaytestRemediationQueueModel", "recipe id", "evidence status", "remediation category", "host gates", "blocked reasons", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].handoffInputs) -join " ").Contains($requiredInput))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff handoffInputs missing: $requiredInput"
        }
    }
    foreach ($field in @("recipe id", "evidence status", "remediation category", "external-owner", "action kind", "handoff text", "host gates", "blockers", "readiness dependency", "unsupported claims")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].handoffFields) -join " ").Contains($field))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff handoffFields missing: $field"
        }
    }
    foreach ($claim in @("arbitrary shell", "raw manifest command strings", "raw/free-form command evaluation", "validation execution", "package script execution", "free-form manifest edits", "evidence mutation", "remediation mutation", "fix execution", "play-in-editor productization", "renderer/RHI handle exposure", "Metal readiness", "general renderer quality")) {
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].blockedExecution) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff blockedExecution missing: $claim"
        }
        if (-not ((@($editorAiPlaytestRemediationHandoffLoop[0].unsupportedClaims) -join " ").Contains($claim))) {
            Write-Error "engine manifest editor-ai-playtest-remediation-handoff unsupportedClaims missing: $claim"
        }
    }
}
$editorAiPlaytestOperatorWorkflowLoop = @($productionLoop.reviewLoops | Where-Object { $_.id -eq "editor-ai-playtest-operator-workflow" })
if ($editorAiPlaytestOperatorWorkflowLoop.Count -ne 1) {
    Write-Error "engine manifest aiOperableProductionLoop must expose one editor-ai-playtest-operator-workflow review loop"
}
