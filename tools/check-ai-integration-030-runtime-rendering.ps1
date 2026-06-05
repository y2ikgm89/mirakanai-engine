#requires -Version 7.0
#requires -PSEdition Core
# Chapter 3 for check-ai-integration.ps1 static contracts.
$historicalVerdictArchiveText = Get-AgentSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$editorProductizationGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "editor-productization" })
if ($editorProductizationGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop editor-productization gap must leave unsupportedProductionGaps after 1.0 host-gated exclusion closeout"
}
$editorProductizationCloseoutText = $historicalVerdictArchiveText
foreach ($needle in @(
    "Editor Productization 1.0 Host-Gated Exclusion Closeout",
    "reviewed editor authoring/playtest/AI command/resource/input/prefab/material-preview evidence",
    "Vulkan/Metal material-preview display parity",
    "explicit 1.0 host-gated exclusion",
    "EditorPlaySession",
    "EditorAiReviewedValidationExecutionModel",
    "EditorMaterialGpuPreviewExecutionSnapshot",
    "ScenePrefabInstanceRefreshPlan",
    "PrefabVariantBaseRefreshPlan",
    "production-ui-importer-platform-adapters",
    "full-repository-quality-gate"
)) {
    Assert-ContainsText $editorProductizationCloseoutText $needle "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md editor-productization closeout evidence"
}
$productionUiImporterPlatformGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "production-ui-importer-platform-adapters" })
if ($productionUiImporterPlatformGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop production-ui-importer-platform-adapters gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$productionUiCloseoutText = $historicalVerdictArchiveText
foreach ($needle in @(
    "Production UI Importer Platform Adapters 1.0 Closeout",
    "reviewed adapter-boundary and package evidence",
    "AccessibilityPublishPlan",
    "ImeCompositionPublishPlan",
    "PlatformTextInputSessionPlan",
    "TextShapingRequestPlan",
    "FontRasterizationRequestPlan",
    "ImageDecodeRequestPlan",
    "PngImageDecodingAdapter",
    "author_packed_ui_atlas_from_decoded_images",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "selected SDL3 platform bridges",
    "package-visible native UI overlay/atlas smokes",
    "production text shaping implementation",
    "real font loading/rasterization",
    "OS accessibility publication",
    "broad native IME/text services",
    "broader source codecs",
    "SVG/vector parsing",
    "renderer texture-upload APIs",
    "arbitrary importer adapters",
    "UI middleware",
    "full-repository-quality-gate"
)) {
    Assert-ContainsText $productionUiCloseoutText $needle "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md production-ui closeout evidence"
}
$fullRepoQualityGap = @($productionLoop.unsupportedProductionGaps | Where-Object { $_.id -eq "full-repository-quality-gate" })
if ($fullRepoQualityGap.Count -ne 0) {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop full-repository-quality-gate gap must leave unsupportedProductionGaps after 1.0 closeout"
}
$fullRepoQualityCloseoutText = $historicalVerdictArchiveText
foreach ($needle in @(
    "Full Repository Quality Gate 1.0 Closeout",
    "local full validate",
    "CI Matrix Contract Check v1",
    "Full Repository Static Analysis CI Contract v1",
    "Linux coverage threshold policy",
    "sanitizer lane documentation",
    "Windows release package artifact evidence",
    "broader analyzer profile expansion",
    "full cross-platform package execution evidence",
    "signing",
    "notarization",
    "release distribution",
    "unsupported_gaps=0"
)) {
    Assert-ContainsText $fullRepoQualityCloseoutText $needle "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md full repository quality closeout evidence"
}
$vulkanGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict" })
if ($vulkanGate.Count -ne 1 -or $vulkanGate[0].status -ne "host-gated") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must keep vulkan-strict host-gated"
}
$metalGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalGate.Count -ne 1 -or $metalGate[0].status -ne "host-gated") {
    Write-Error "engine/agent/manifest.json aiOperableProductionLoop must keep metal-apple host-gated"
}

foreach ($surface in @("codex", "claudeCode", "cursor")) {
    if (-not $manifest.aiSurfaces.PSObject.Properties.Name.Contains($surface)) {
        Write-Error "engine/agent/manifest.json aiSurfaces missing required field: $surface"
    }
    foreach ($field in @("requiredSkills", "requiredAgents", "readOnlyAgents")) {
        if (-not $manifest.aiSurfaces.$surface.PSObject.Properties.Name.Contains($field)) {
            Write-Error "engine/agent/manifest.json aiSurfaces.$surface missing required field: $field"
        }
    }
}
$requiredSurfaceSkills = @{
    codex      = @($manifest.aiSurfaces.codex.requiredSkills)
    claudeCode = @($manifest.aiSurfaces.claudeCode.requiredSkills)
    cursor     = @($manifest.aiSurfaces.cursor.requiredSkills)
}
foreach ($surfaceSkills in $requiredSurfaceSkills.GetEnumerator()) {
    $surfaceName = $surfaceSkills.Key
    foreach ($skillName in $surfaceSkills.Value) {
        if (@($manifest.aiSurfaces.$surfaceName.requiredSkills) -notcontains $skillName) {
            Write-Error "engine/agent/manifest.json aiSurfaces.$surfaceName.requiredSkills missing $skillName"
        }
    }
}
foreach ($agentName in @("agent-surface-auditor", "build-fixer", "cpp-reviewer", "engine-architect", "explorer", "gameplay-builder", "planning-auditor", "rendering-auditor")) {
    foreach ($surface in @("codex", "claudeCode", "cursor")) {
        if (@($manifest.aiSurfaces.$surface.requiredAgents) -notcontains $agentName) {
            Write-Error "engine/agent/manifest.json aiSurfaces.$surface.requiredAgents missing $agentName"
        }
    }
}
foreach ($agentName in @("agent-surface-auditor", "cpp-reviewer", "engine-architect", "explorer", "planning-auditor", "rendering-auditor")) {
    foreach ($surface in @("codex", "claudeCode", "cursor")) {
        if (@($manifest.aiSurfaces.$surface.readOnlyAgents) -notcontains $agentName) {
            Write-Error "engine/agent/manifest.json aiSurfaces.$surface.readOnlyAgents missing $agentName"
        }
    }
}

if (-not $manifest.PSObject.Properties.Name.Contains("documentationPolicy")) {
    Write-Error "engine/agent/manifest.json missing required field: documentationPolicy"
}
if (-not $manifest.documentationPolicy.PSObject.Properties.Name.Contains("entrypoints")) {
    Write-Error "engine/agent/manifest.json documentationPolicy missing required field: entrypoints"
}
foreach ($field in @("entrypoint", "currentStatus", "currentCapabilities", "workflows", "planRegistry", "specRegistry", "superpowersSpecRegistry", "activeRoadmap")) {
    if (-not $manifest.documentationPolicy.entrypoints.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json documentationPolicy.entrypoints missing required field: $field"
    }
    Resolve-RequiredAgentPath $manifest.documentationPolicy.entrypoints.$field | Out-Null
}
if ($manifest.documentationPolicy.preferredMcp -ne "context7") {
    Write-Error "engine/agent/manifest.json documentationPolicy.preferredMcp must be context7"
}
if (-not $manifest.documentationPolicy.doNotStoreApiKeysInRepo) {
    Write-Error "engine/agent/manifest.json documentationPolicy must forbid storing API keys in the repository"
}

if (-not $manifest.commands.PSObject.Properties.Name.Contains("newGame")) {
    Write-Error "engine/agent/manifest.json must expose commands.newGame"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntimeMaterialShaderPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntimeMaterialShaderPackage"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntime2DPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntime2DPackage"
}
if (-not $manifest.commands.newGame.Contains("DesktopRuntime3DPackage")) {
    Write-Error "engine/agent/manifest.json commands.newGame must expose DesktopRuntime3DPackage"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("buildAssetImporters")) {
    Write-Error "engine/agent/manifest.json must expose commands.buildAssetImporters"
}
if (-not $manifest.commands.PSObject.Properties.Name.Contains("registerRuntimePackageFiles")) {
    Write-Error "engine/agent/manifest.json must expose commands.registerRuntimePackageFiles"
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("gameRoot")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.gameRoot"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentRuntimeUi")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentRuntimeUi" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentRuntimeUiWorkbench")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentRuntimeUiWorkbench" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorAiReviewedValidationExecution")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorAiReviewedValidationExecution" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorProfilerTraceExport")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorProfilerTraceExport" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentMemoryDiagnostics")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentMemoryDiagnostics" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorPrefabVariantFileDialogs")) { Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorPrefabVariantFileDialogs" }
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentEditorPrefabInstanceSourceLinks")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentEditorPrefabInstanceSourceLinks"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAssetPlaceholderGeneration")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAssetPlaceholderGeneration"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentSpriteAnimationFlipbook")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentSpriteAnimationFlipbook"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentGameplayDebugOverlay")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentGameplayDebugOverlay"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentQuestDialogueState")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentQuestDialogueState"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentInventoryItemCatalog")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentInventoryItemCatalog"
}
$geUiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ui" })
if ($geUiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ui module"
}
$geUiRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ui_renderer" })
if ($geUiRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ui_renderer module"
}
$geSceneModule = @($manifest.modules | Where-Object { $_.name -eq "MK_scene" })
if ($geSceneModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_scene module"
}
$geAssetsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_assets" })
if ($geAssetsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_assets module"
}
$geRuntimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module"
}
$geAudioModule = @($manifest.modules | Where-Object { $_.name -eq "MK_audio" })
if ($geAudioModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_audio module"
}
$gePhysicsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_physics" })
if ($gePhysicsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_physics module"
}
$geToolsModule = @($manifest.modules | Where-Object { $_.name -eq "MK_tools" })
if ($geToolsModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_tools module"
}
$geEditorCoreModule = @($manifest.modules | Where-Object { $_.name -eq "MK_editor_core" })
if ($geEditorCoreModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_editor_core module"
}
if ($geUiModule[0].status -ne "implemented-production-runtime-ui-workbench") { Write-Error "engine/agent/manifest.json MK_ui status must advertise the production runtime UI workbench slice honestly" }
if ($geUiRendererModule[0].status -ne "implemented-runtime-ui-font-image-adapter") {
    Write-Error "engine/agent/manifest.json MK_ui_renderer status must advertise the runtime UI font image adapter slice honestly"
}
if ($geSceneModule[0].status -ne "implemented-scene-schema-v2-contract") {
    Write-Error "engine/agent/manifest.json MK_scene status must advertise the Scene/Component/Prefab Schema v2 contract slice honestly"
}
if (@($geSceneModule[0].publicHeaders) -notcontains "engine/scene/include/mirakana/scene/schema_v2.hpp") {
    Write-Error "engine/agent/manifest.json MK_scene publicHeaders must include schema_v2.hpp"
}
if ($geAssetsModule[0].status -ne "implemented-asset-identity-v2-foundation") {
    Write-Error "engine/agent/manifest.json MK_assets status must advertise the Asset Identity v2 foundation slice honestly"
}
if (@($geAssetsModule[0].publicHeaders) -notcontains "engine/assets/include/mirakana/assets/asset_identity.hpp") {
    Write-Error "engine/agent/manifest.json MK_assets publicHeaders must include asset_identity.hpp"
}
if ($geRuntimeModule[0].status -ne "ready-runtime-resource-v2-gameplay-interaction-framework") {
    Write-Error "engine/agent/manifest.json MK_runtime status must advertise the closed Runtime Resource v2 plus gameplay interaction framework surface honestly"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/resource_runtime.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders must include resource_runtime.hpp"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/quest_dialogue.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders must include quest_dialogue.hpp"
}
if (@($geRuntimeModule[0].publicHeaders) -notcontains "engine/runtime/include/mirakana/runtime/inventory_items.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime publicHeaders must include inventory_items.hpp"
}
if ($geAudioModule[0].status -ne "implemented-gameplay-audio-mix-planner") {
    Write-Error "engine/agent/manifest.json MK_audio status must advertise the gameplay audio mix planner honestly"
}
if (@($geAudioModule[0].publicHeaders) -notcontains "engine/audio/include/mirakana/audio/audio_mixer.hpp") {
    Write-Error "engine/agent/manifest.json MK_audio publicHeaders must include audio_mixer.hpp"
}
if ($gePhysicsModule[0].status -ne "implemented-physics-1-0-ready-surface") {
    Write-Error "engine/agent/manifest.json MK_physics status must advertise the Physics 1.0 ready surface honestly"
}
if (@($gePhysicsModule[0].publicHeaders) -notcontains "engine/physics/include/mirakana/physics/physics3d.hpp") {
    Write-Error "engine/agent/manifest.json MK_physics publicHeaders must include physics3d.hpp"
}
if (@($gePhysicsModule[0].publicHeaders) -notcontains "engine/physics/include/mirakana/physics/collision_query.hpp") {
    Write-Error "engine/agent/manifest.json MK_physics publicHeaders must include collision_query.hpp"
}
if (@($geToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") {
    Write-Error "engine/agent/manifest.json MK_tools publicHeaders must include gltf_node_animation_import.hpp"
}
if (@($geToolsModule[0].publicHeaders) -notcontains "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") {
    Write-Error "engine/agent/manifest.json MK_tools publicHeaders must include placeholder_asset_tool.hpp"
}
if (@($geEditorCoreModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/play_in_editor.hpp") {
    Write-Error "engine/agent/manifest.json MK_editor_core publicHeaders must include play_in_editor.hpp"
}
if (@($geEditorCoreModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/ai_operation_surface.hpp") {
    Write-Error "engine/agent/manifest.json MK_editor_core publicHeaders must include ai_operation_surface.hpp"
}
if (@($geEditorCoreModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/editor_dock_layout.hpp") {
    Write-Error "engine/agent/manifest.json MK_editor_core publicHeaders must include editor_dock_layout.hpp"
}
if (@($geEditorCoreModule[0].publicHeaders) -notcontains "editor/core/include/mirakana/editor/editor_panel.hpp") {
    Write-Error "engine/agent/manifest.json MK_editor_core publicHeaders must include editor_panel.hpp"
}
if (@($geUiModule[0].publicHeaders) -notcontains "engine/ui/include/mirakana/ui/runtime_ui_workbench.hpp") { Write-Error "engine/agent/manifest.json MK_ui publicHeaders must include runtime_ui_workbench.hpp" }
foreach ($runtimeUiWorkbenchModuleNeedle in @("Runtime UI Workbench Production v1", "RuntimeUiWorkbenchDocument", "RuntimeUiWorkbenchPlan", "plan_runtime_ui_workbench", "backend/native/editor/RHI/UI-middleware", "zero renderer/text-shaping/font-rasterization/IME/accessibility-bridge/image-decoding/native-platform adapter invocation")) { Assert-ContainsText ([string]$geUiModule[0].purpose) $runtimeUiWorkbenchModuleNeedle "MK_ui module purpose" }
Assert-ContainsText ([string]$geUiModule[0].purpose) "MonospaceTextLayoutPolicy" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "stable glyph ids" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "AccessibilityPublishPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "AccessibilityPublishResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_accessibility_publish" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "publish_accessibility_payload" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IAccessibilityAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "OS accessibility bridge publication" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImeCompositionPublishPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImeCompositionPublishResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_ime_composition_update" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "publish_ime_composition" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IImeAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "native IME/text-input session integration" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "PlatformTextInputSessionPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "PlatformTextInputEndResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "begin_platform_text_input" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "end_platform_text_input" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IPlatformIntegrationAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "TextShapingRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "TextShapingResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_text_shaping_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "shape_text_run" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ITextShapingAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "FontRasterizationRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "FontRasterizationResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_font_rasterization_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "rasterize_font_glyph" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IFontRasterizerAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImageDecodeRequestPlan" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "ImageDecodeDispatchResult" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "plan_image_decode_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "decode_image_request" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiModule[0].purpose) "IImageDecodingAdapter" "MK_ui module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "UiRendererImagePalette" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "UiRendererGlyphAtlasPalette" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "text glyph sprite submissions" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geUiRendererModule[0].purpose) "image sprite submission" "MK_ui_renderer module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "contract-only" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "GameEngine.Scene.v2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "ScenePrefabInstanceRefreshPlanV2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "ScenePrefabInstanceRefreshRowV2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "plan_scene_prefab_instance_refresh_v2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "ScenePrefabInstanceRefreshResultV2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "apply_scene_prefab_instance_refresh_v2" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "duplicate_prefab_source_identity" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "unsupported_nested_prefab_instance" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "unsupported_local_prefab_child" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "unsupported_local_prefab_component" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "source_node_id" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "source_component_id" "MK_scene module purpose"
Assert-ContainsText ([string]$geSceneModule[0].purpose) "nested prefab propagation/merge resolution UX" "MK_scene module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "Asset Identity v2" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.AssetIdentity.v2" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "foundation-only" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "renderer/RHI residency" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.MorphMeshCpuSource.v1" "MK_assets module purpose"
Assert-ContainsText ([string]$geAssetsModule[0].purpose) "GameEngine.AnimationFloatClipSource.v1" "MK_assets module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterController3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "move_physics_character_controller_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsAuthoredCollisionScene3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "build_physics_world_3d_from_authored_collision_scene" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "native backend requests failing closed" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCollisionQueryBatchStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCollisionQueryBatchDiagnostic" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCollisionQueryRowStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCollisionQueryRowDiagnostic" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsRaycastBatch2DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld2D::raycast_batch" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::shape_sweep_batch" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "source-indexed value-only query rows" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "default-unbounded query counts" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "explicit positive max_queries budgets" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::aabb" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::sphere" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsShape3DDesc::capsule" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsQueryFilter3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::exact_shape_sweep" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsExactSphereCast3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsExactSphereCast3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::exact_sphere_cast" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContactPoint3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContactManifold3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::contact_manifolds" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "stable feature ids" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "warm-start-safe" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DConfig" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DRow" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsContinuousStep3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::step_continuous" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsWorld3D::step remains discrete" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DRowKind" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DRow" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsCharacterDynamicPolicy3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "evaluate_physics_character_dynamic_policy_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsAdvancedController3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsMovingPlatform3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsAdvancedController3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "plan_physics_advanced_controller_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsJoint3DStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDistanceJoint3DDesc" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsJointSolve3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "solve_physics_joints_3d" "MK_physics module purpose"
foreach ($needle in @("PhysicsConstraint3DStatus", "PhysicsConstraint3DDiagnostic", "PhysicsFixedConstraint3DDesc", "PhysicsLinearAxisConstraint3DDesc", "PhysicsConstraintSolve3DResult", "solve_physics_constraints_3d", "max_rows", "row_budget_exceeded", "rotational rigid-body constraints")) { Assert-ContainsText ([string]$gePhysicsModule[0].purpose) $needle "MK_physics module purpose" }
foreach ($needle in @("PhysicsKinematicMotion3DResult", "plan_physics_kinematic_motion_3d", "PhysicsSimpleVehicle3DDesc", "PhysicsSimpleVehicle3DWheelDesc", "PhysicsSimpleVehicle3DResult", "plan_physics_simple_vehicle_3d", "simple vehicle")) { Assert-ContainsText ([string]$gePhysicsModule[0].purpose) $needle "MK_physics module purpose" }
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DStatus" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DDiagnostic" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DConfig" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DCounts" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsReplaySignature3D" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "PhysicsDeterminismGate3DResult" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "make_physics_replay_signature_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "evaluate_physics_determinism_gate_3d" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "validated Physics 1.0 ready surface" "MK_physics module purpose"
Assert-ContainsText ([string]$gePhysicsModule[0].purpose) "explicit Jolt/native middleware exclusion" "MK_physics module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_tracks" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_tracks_3d" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_float_clip" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "import_gltf_node_transform_animation_binding_source" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "refresh-prefab-instance" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "apply_scene_prefab_instance_refresh_v2" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PngImageDecodingAdapter" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "IImageDecodingAdapter" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "decode_audited_png_rgba8" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PackedUiAtlasAuthoringDesc" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "author_packed_ui_atlas_from_decoded_images" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "plan_packed_ui_atlas_package_update" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "GameEngine.CookedTexture.v1" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "PackedUiGlyphAtlasAuthoringDesc" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "author_packed_ui_glyph_atlas_from_rasterized_glyphs" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "plan_packed_ui_glyph_atlas_package_update" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "UiAtlasMetadataGlyph" "MK_tools module purpose"
Assert-ContainsText ([string]$geToolsModule[0].purpose) "GameEngine.UiAtlas.v1" "MK_tools module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "Runtime Resource v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "generation-checked" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_resident_package_replace_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_resident_package_unmount_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_package_discovery_resident_replace_with_reviewed_evictions_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "host-driven reviewed hot-reload replacement safe point" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_candidate_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadCandidateReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_recook_change_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadRecookChangeReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "plan_runtime_package_hot_reload_replacement_intent_review_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadReplacementIntentReviewResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "commit_runtime_package_hot_reload_recook_replacement_v2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimePackageHotReloadRecookReplacementResultV2" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "candidate/discovery root coherence" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "defined overlay" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "file watching/recook execution" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "closed for the Engine 1.0 reviewed safe-point/controller surface" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "package streaming" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "native watcher ownership" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "renderer/RHI resource ownership" "MK_runtime module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioDeviceStreamRequest" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioDeviceStreamPlan" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "plan_audio_device_stream" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "render_audio_device_stream_interleaved_float" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioGameplayMixRequest" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "AudioGameplayMixPlan" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "plan_gameplay_audio_mix" "MK_audio module purpose"
Assert-ContainsText ([string]$geAudioModule[0].purpose) "does not open OS audio devices" "MK_audio module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "runtime_morph_mesh_cpu_payload" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "runtime_animation_float_clip_payload" "MK_runtime module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "EditorPlaySession" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "IEditorPlaySessionDriver" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "EditorPlaySessionTickContext" "MK_editor_core module purpose"
Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) "isolated simulation scene" "MK_editor_core module purpose"
foreach ($needle in @("EditorAiOperationSnapshot", "EditorAiCommandCatalog", "EditorAiCommandRequest", "EditorAiCommandDryRunResult", "EditorAiCommandApplyResult", "dry-run-before-apply AI panel visibility command rows")) {
    Assert-ContainsText ([string]$geEditorCoreModule[0].purpose) $needle "MK_editor_core module purpose"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "EditorPlaySession" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "IEditorPlaySessionDriver" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "isolated simulation scene" "editor game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditor) "scene_prefab_source_links" "editor prefab source-link guidance"
foreach ($memoryDiagnosticsGuidanceNeedle in @(
        "Memory Diagnostics v1", "ScratchArena", "ScratchLease", "ScratchLeaseStatus", "reset_at_safe_point", "memory_counter_row",
        "MemoryCounterRow",
        "MemoryLifetimeClass",
        "MemoryDiagnosticsSummary",
        "summarize_memory_diagnostics",
        "reuse counts",
        "reset counts",
        "high-water bytes",
        "budget pressure",
        "stale-generation diagnostics",
        "use-after-safe-point diagnostics",
        "cross-thread-free diagnostics",
        "false-sharing diagnostics",
        "package-visible memory_diagnostics_* counters",
        "sample_desktop_runtime_game --require-memory-diagnostics",
        "memory_diagnostics_status=ready",
        "memory_diagnostics_resident_gpu_pressure=nominal",
        "broad memory optimization"
    )) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentMemoryDiagnostics) $memoryDiagnosticsGuidanceNeedle "memory diagnostics game guidance"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceExportModel" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_export_model" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_export" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "mirakana::export_diagnostics_trace_json" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileSaveRequest" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "save_editor_profiler_trace_json" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_file_save" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceSaveDialogModel" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_save_dialog_request" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_save_dialog_model" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_save_dialog" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTelemetryHandoffModel" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_telemetry_handoff_model" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.telemetry" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "mirakana::build_diagnostics_ops_plan" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "DiagnosticsTraceImportReview" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "DiagnosticsTraceImportResult" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "review_diagnostics_trace_json" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "import_diagnostics_trace_json" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceImportReviewModel" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_import_review_model" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_import" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_import.reconstructed_*" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileImportRequest" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceFileImportResult" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "import_editor_profiler_trace_json" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_file_import" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "EditorProfilerTraceOpenDialogModel" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_open_dialog_request" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "make_editor_profiler_trace_open_dialog_model" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "profiler.trace_open_dialog" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Copy Trace JSON" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Save Trace JSON" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Browse Save Trace JSON" "editor profiler native trace save dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Profiler Telemetry Handoff" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Review Trace JSON" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Trace Import Path" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Import Trace JSON" "editor profiler trace file import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "Browse Trace JSON" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "native shell file-dialog service" "editor profiler native trace open dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "broader editor native save/open dialogs outside Profiler" "editor profiler native trace dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "arbitrary JSON conversion" "editor profiler trace import guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "first-party exported Trace Event JSON subset" "editor profiler trace import reconstruction guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProfilerTraceExport) "telemetry SDK/upload execution" "editor profiler trace export guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "EditorAiReviewedValidationExecutionModel" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "EditorAiReviewedValidationExecutionBatchModel" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_editor_ai_reviewed_validation_execution_plan" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_editor_ai_reviewed_validation_execution_batch" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "make_ai_reviewed_validation_execution_batch_ui_model" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "mirakana::ProcessCommand" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "mirakana::Win32ProcessRunner" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Host-Gated Validation Execution Ack v1" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Reviewed Validation Batch Execution v1" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "-HostGateAcknowledgements" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "ai_commands.execution.batch" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "Execute Ready" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorAiReviewedValidationExecution) "automatic host-gated recipe execution" "editor AI reviewed validation execution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "resolve_prefab_variant_conflict" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "PrefabVariantConflictBatchResolutionPlan" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "resolve_prefab_variant_conflicts" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "make_prefab_variant_conflict_resolution_action" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "make_prefab_variant_conflict_batch_resolution_action" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "prefab_variant_conflicts" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "prefab_variant_conflicts.batch_resolution" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Apply All Reviewed" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "PrefabNodeOverride::source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "override.N.source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "source_node_mismatch" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "accept_current_node" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Accept current node N" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "updates only source_node_name" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Strict MK_scene composition remains index-based" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "deserialize_prefab_variant_definition_for_review" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "missing-node stale override rows" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "retargets a missing-node stale override" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "retargets an existing-node source_node_mismatch row" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "accepts the current indexed node" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "without creating duplicate node/kind overrides" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "Nested prefab propagation" "editor prefab variant reviewed resolution guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantReviewedResolution) "automatic merge/rebase/resolution UX" "editor prefab variant reviewed resolution guidance"
foreach ($sourceLinkNeedle in @(
    "Editor Prefab Instance Source-Link Review v1",
    "ScenePrefabSourceLink",
    "PrefabInstantiateDesc",
    "SceneNode::prefab_source",
    "ScenePrefabInstanceSourceLinkModel",
    "make_scene_prefab_instance_source_link_model",
    "make_scene_prefab_instance_source_link_ui_model",
    "scene_prefab_source_links",
    "Editor Scene Prefab Instance Refresh Review v1",
    "Editor Prefab Instance Local Child Refresh Resolution v1",
    "Editor Prefab Instance Stale Node Refresh Resolution v1",
    "Editor Nested Prefab Refresh Resolution v1",
    "ScenePrefabInstanceRefreshPlan",
    "ScenePrefabInstanceRefreshRow",
    "ScenePrefabInstanceRefreshResult",
    "ScenePrefabInstanceRefreshPolicy",
    "plan_scene_prefab_instance_refresh",
    "plan_scene_prefab_instance_refresh_batch",
    "apply_scene_prefab_instance_refresh",
    "apply_scene_prefab_instance_refresh_batch",
    "make_scene_prefab_instance_refresh_action",
    "make_scene_prefab_instance_refresh_batch_action",
    "make_scene_prefab_instance_refresh_ui_model",
    "make_scene_prefab_instance_refresh_batch_ui_model",
    "scene_prefab_instance_refresh",
    "scene_prefab_instance_refresh_batch",
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
    "path-aware make_scene_authoring_instantiate_prefab_action",
    "full nested prefab propagation",
    "automatic merge/rebase/resolution UX"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabInstanceSourceLinks) $sourceLinkNeedle "editor prefab source-link guidance"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "EditorPrefabVariantFileDialogModel" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_open_dialog_request" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_save_dialog_request" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_open_dialog_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_save_dialog_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "make_prefab_variant_file_dialog_ui_model" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "prefab_variant_file_dialog.open" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "prefab_variant_file_dialog.save" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Browse Load Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Browse Save Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "native shell file-dialog service" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) ".prefabvariant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "first non-Profiler native document dialog" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorPrefabVariantFileDialogs) "native handles" "editor prefab variant native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Editor Scene Native Dialog v1" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "EditorSceneFileDialogModel" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "EditorSceneFileDialogMode" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_open_dialog_request" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_save_dialog_request" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_open_dialog_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_save_dialog_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "make_scene_file_dialog_ui_model" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "SceneAuthoringDocument::set_scene_path" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "scene_file_dialog.open" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "scene_file_dialog.save" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Open Scene..." "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Save Scene As..." "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Browse Open Scene" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Browse Save Scene As" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "native shell file-dialog service" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) ".scene" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, and Prefab Variant" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorSceneFileDialogs) "native handles" "editor scene native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Editor Project Native Dialog v1" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "EditorProjectFileDialogModel" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "EditorProjectFileDialogMode" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_open_dialog_request" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_save_dialog_request" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_open_dialog_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_save_dialog_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "make_project_file_dialog_ui_model" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "ProjectBundlePaths" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "project_file_dialog.open" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "project_file_dialog.save" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Open Project..." "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Save Project As..." "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "native shell file-dialog service" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) ".geproject" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "load_project_bundle" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "save_project_bundle" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "Broader editor native save/open dialogs outside Profiler, Scene, Prefab Variant, and Project" "editor project native dialog guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEditorProjectFileDialogs) "native handles" "editor project native dialog guidance"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "author_cooked_ui_atlas_metadata" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "verify_cooked_ui_atlas_package_metadata" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "plan_cooked_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "apply_cooked_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "PackedUiAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "author_packed_ui_atlas_from_decoded_images" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "plan_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp") "apply_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_morph_animation_import.hpp") "import_gltf_morph_mesh_cpu_primitive" "MK_tools gltf morph import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_morph_animation_import.hpp") "import_gltf_morph_weights_animation_float_clip" "MK_tools gltf morph import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_tracks" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "GltfNodeTransformAnimationTrack3d" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_tracks_3d" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_float_clip" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/gltf_node_animation_import.hpp") "import_gltf_node_transform_animation_binding_source" "MK_tools gltf node animation import public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/gltf/CMakeLists.txt") "gltf_node_animation_import.cpp" "MK_tools gltf CMake source list"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/morph_mesh_cpu_source_bridge.hpp") "morph_mesh_cpu_source_document_from_animation_desc" "MK_tools morph mesh CPU source bridge public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "plan_material_instance_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "apply_material_instance_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "plan_material_graph_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/material_tool.hpp") "apply_material_graph_package_update" "MK_tools material tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetBundleRequest" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetBundlePlan" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetChangedFile" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetProvenanceRow" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetDiagnostic" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "plan_placeholder_asset_bundle" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetCookPackageRequest" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "PlaceholderAssetCookPackagePlan" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/placeholder_asset_tool.hpp") "plan_placeholder_asset_cook_package" "MK_tools placeholder asset tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/sprite_atlas_tool.hpp") "SpriteAtlasSourceFrameDesc" "MK_tools sprite atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/sprite_atlas_tool.hpp") "SpriteAtlasSourceAuthoringDesc" "MK_tools sprite atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/sprite_atlas_tool.hpp") "SpriteAtlasSourceAuthoringPlan" "MK_tools sprite atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/sprite_atlas_tool.hpp") "plan_sprite_atlas_source_authoring" "MK_tools sprite atlas tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "source_asset_registry_format_v1" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "serialize_source_asset_registry_document" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "plan_registered_source_asset_cook_package" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "mirakana-placeholder-asset-tool-v1" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "LicenseRef-Proprietary" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/placeholder_asset_tool.cpp") "invalid_texture_dimensions" "MK_tools placeholder asset tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/sprite_atlas_tool.cpp") "pack_sprite_atlas_rgba8_max_side" "MK_tools sprite atlas tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/sprite_atlas_tool.cpp") "serialize_source_asset_registry_document" "MK_tools sprite atlas tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/sprite_atlas_tool.cpp") "runtime_source_image_decoding" "MK_tools sprite atlas tool source"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt") "placeholder_asset_tool.cpp" "MK_tools asset CMake source list"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/asset/CMakeLists.txt") "sprite_atlas_tool.cpp" "MK_tools asset CMake source list"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "placeholder asset bundle plans deterministic legal source documents and provenance rows" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "placeholder asset bundle fails closed on unsafe duplicate and unsupported rows" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "placeholder asset cook package routes generated source documents through registered package planning" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "unsupported_asset_kind" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "sprite atlas source authoring packs deterministic texture source and registry rows" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "sprite atlas source authoring rejects duplicate and invalid frame ids" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/tools_tests.cpp") "sprite atlas source authoring rejects unsupported frame format and dimensions" "MK_tools tests"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/scene_tool.hpp") "plan_scene_package_update" "MK_tools scene tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/tools/include/mirakana/tools/scene_tool.hpp") "apply_scene_package_update" "MK_tools scene tool public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/assets/include/mirakana/assets/ui_atlas_metadata.hpp") "GameEngine.UiAtlas.v1" "MK_assets ui atlas metadata public header"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "plan_placeholder_asset_bundle" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "PlaceholderAssetBundleRequest" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "PlaceholderAssetBundlePlan" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "PlaceholderAssetChangedFile" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "plan_placeholder_asset_cook_package" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "PlaceholderAssetCookPackageRequest" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "GameEngine.SourceAssetRegistry.v1" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "PlaceholderAssetProvenanceRow" "asset placeholder game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAssetPlaceholderGeneration) "must not download external assets" "asset placeholder game guidance"
foreach ($spriteAtlasSourceAuthoringNeedle in @(
    "SpriteAtlasSourceFrameDesc",
    "SpriteAtlasSourceAuthoringDesc",
    "SpriteAtlasSourceAuthoringPlan",
    "plan_sprite_atlas_source_authoring",
    "GameEngine.TextureSource.v1",
    "GameEngine.SourceAssetRegistry.v1",
    "SpriteAtlasSourceChangedFile",
    "SpriteAtlasSourceFrameRow",
    "registered source cook/package validation",
    "must not parse PNG/source images",
    "renderer/RHI residency",
    "animation semantics"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAtlasSourceAuthoring) $spriteAtlasSourceAuthoringNeedle "sprite atlas source authoring game guidance"
}
foreach ($gameplayDebugOverlayGuidanceNeedle in @(
    "RuntimeGameplayDebugOverlayRowDesc",
    "RuntimeGameplayDebugOverlayCategory",
    "RuntimeGameplayDebugOverlayRowKind",
    "RuntimeGameplayDebugOverlayDiagnosticCode",
    "RuntimeGameplayDebugOverlayRow",
    "RuntimeGameplayDebugOverlayDiagnostic",
    "RuntimeGameplayDebugOverlayPlan",
    "plan_runtime_gameplay_debug_overlay",
    "value-only",
    "does not render UI"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentGameplayDebugOverlay) $gameplayDebugOverlayGuidanceNeedle "gameplay debug overlay game guidance"
}
foreach ($questDialogueGuidanceNeedle in @(
    "RuntimeQuestDialogueDocument",
    "RuntimeQuestDesc",
    "RuntimeQuestObjectiveDesc",
    "RuntimeDialogueGraphDesc",
    "RuntimeDialogueNodeDesc",
    "RuntimeDialogueChoiceDesc",
    "RuntimeQuestDialogueValidationContext",
    "validate_runtime_quest_dialogue_document",
    "RuntimeQuestDialogueState",
    "RuntimeQuestDialogueTransitionRequest",
    "RuntimeQuestDialogueTransitionRow",
    "RuntimeQuestDialogueTransitionResult",
    "RuntimeQuestDialogueTransitionStatus",
    "RuntimeQuestDialogueStateValidationResult",
    "validate_runtime_quest_dialogue_state",
    "advance_runtime_quest_dialogue_state",
    "accepted",
    "ignored",
    "blocked",
    "completed",
    "invalid",
    "gameplay_systems_quest_dialogue_transition_rows",
    "gameplay_systems_quest_dialogue_action_ids",
    "gameplay_systems_quest_dialogue_reward_ids",
    "gameplay_systems_quest_dialogue_state_rows",
    "RuntimeQuestDialogueDiagnostic",
    "RuntimeQuestDialogueValidationRow",
    "game-owned",
    "unsupported reward ids",
    "unsupported action ids",
    "unsafe localization keys"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentQuestDialogueState) $questDialogueGuidanceNeedle "quest dialogue game guidance"
}
foreach ($questDialogueSurface in @(
    "docs/current-capabilities.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $questDialogueSurfaceText = Get-AgentSurfaceText $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "RuntimeQuestDialogueDocument" $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "validate_runtime_quest_dialogue_document" $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "RuntimeQuestDialogueState" $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "validate_runtime_quest_dialogue_state" $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "advance_runtime_quest_dialogue_state" $questDialogueSurface
    Assert-ContainsText $questDialogueSurfaceText "game-owned" $questDialogueSurface
}
foreach ($inventoryItemGuidanceNeedle in @(
    "RuntimeItemCatalogDocument",
    "RuntimeItemDesc",
    "RuntimeItemCostDesc",
    "RuntimeItemCatalogValidationContext",
    "RuntimeItemCatalogValidationResult",
    "RuntimeItemCatalogDiagnostic",
    "RuntimeItemCatalogValidationRow",
    "validate_runtime_item_catalog_document",
    "RuntimeInventoryState",
    "RuntimeInventoryStateValidationResult",
    "validate_runtime_inventory_state",
    "RuntimeCraftingRecipeDocument",
    "RuntimeInventoryTransitionRequest",
    "RuntimeInventoryTransitionStatus",
    "advance_runtime_inventory_state",
    "RuntimeConstructionPlacementSurfaceDesc",
    "RuntimeConstructionPlacementCandidateDesc",
    "RuntimeConstructionPlacementValidationContext",
    "RuntimeConstructionPlacementValidationResult",
    "RuntimeConstructionPlacementDiagnostic",
    "RuntimeConstructionPlacementValidationRow",
    "validate_runtime_construction_placement",
    "RuntimeSceneConstructionPlacementIntentDesc",
    "RuntimeSceneConstructionPlacementIntentContext",
    "RuntimeSceneConstructionPlacementIntentPlan",
    "RuntimeSceneConstructionPlacementIntentStatus",
    "plan_runtime_scene_construction_placement_intents",
    "already_occupied",
    "accepted",
    "ignored",
    "blocked",
    "completed",
    "invalid",
    "gameplay_systems_inventory_items_transition_rows",
    "gameplay_systems_construction_placement_validation_rows",
    "gameplay_systems_construction_placement_intent_accepted_rows",
    "duplicate item ids",
    "invalid stack limits",
    "unsupported category ids",
    "unsupported tag ids",
    "unsafe localization keys",
    "missing item references",
    "invalid cost quantities",
    "finite grid/world positions",
    "candidate row grid/world origins",
    "same-batch occupied cells",
    "duplicate occupied cells",
    "missing costs",
    "unsupported placement surfaces",
    "game-owned"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInventoryItemCatalog) $inventoryItemGuidanceNeedle "inventory item catalog game guidance"
}
foreach ($inventoryItemSurface in @(
    "docs/current-capabilities.md",
    "docs/ai-game-development.md",
    "docs/roadmap.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $inventoryItemSurfaceText = Get-AgentSurfaceText $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeItemCatalogDocument" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeItemCatalogValidationContext" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeItemCatalogValidationResult" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "validate_runtime_item_catalog_document" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeInventoryState" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "validate_runtime_inventory_state" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeCraftingRecipeDocument" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeInventoryTransitionRequest" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "advance_runtime_inventory_state" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeConstructionPlacementValidationContext" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "validate_runtime_construction_placement" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "RuntimeSceneConstructionPlacementIntentDesc" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "plan_runtime_scene_construction_placement_intents" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "candidate row grid/world origins" $inventoryItemSurface; Assert-ContainsText $inventoryItemSurfaceText "same-batch occupied-cell" $inventoryItemSurface
    Assert-ContainsText $inventoryItemSurfaceText "game-owned" $inventoryItemSurface
}
foreach ($inventoryItemPackageSurface in @(
    "games/sample_2d_desktop_runtime_package/README.md",
    "games/sample_2d_desktop_runtime_package/game.agent.json"
)) {
    $inventoryItemPackageText = Get-AgentSurfaceText $inventoryItemPackageSurface
    Assert-ContainsText $inventoryItemPackageText "gameplay_systems_inventory_items_transition_rows" $inventoryItemPackageSurface; Assert-ContainsText $inventoryItemPackageText "gameplay_systems_inventory_items_final_workbench_quantity" $inventoryItemPackageSurface
    Assert-ContainsText $inventoryItemPackageText "gameplay_systems_construction_placement_validation_rows" $inventoryItemPackageSurface
    Assert-ContainsText $inventoryItemPackageText "gameplay_systems_construction_placement_intent_rows" $inventoryItemPackageSurface
    Assert-ContainsText $inventoryItemPackageText "gameplay_systems_construction_placement_intent_accepted_rows" $inventoryItemPackageSurface
    Assert-ContainsText $inventoryItemPackageText "gameplay_systems_construction_placement_intent_occupied_cells" $inventoryItemPackageSurface
}
foreach ($runtimeUiWorkbenchGuidanceNeedle in @("RuntimeUiWorkbenchDocument", "RuntimeUiWorkbenchPanelRow", "RuntimeUiWorkbenchTableRow", "RuntimeUiWorkbenchGraphSeries", "RuntimeUiWorkbenchItemRow", "RuntimeUiWorkbenchTextInputFieldRow", "RuntimeUiWorkbenchFocusPlan", "RuntimeUiWorkbenchLocalizationRef", "RuntimeUiWorkbenchAccessibilityRef", "RuntimeUiWorkbenchPlan", "plan_runtime_ui_workbench", "--require-runtime-ui-workbench", "runtime_ui_workbench_status=ready", "runtime_ui_workbench_panels=5", "runtime_ui_workbench_table_columns=3", "runtime_ui_workbench_table_rows=2", "runtime_ui_workbench_graph_series=2", "runtime_ui_workbench_item_rows=3", "runtime_ui_workbench_inventory_rows=1", "runtime_ui_workbench_equipment_rows=1", "runtime_ui_workbench_shop_rows=1", "runtime_ui_workbench_text_inputs=1", "runtime_ui_workbench_platform_text_input_requests=1", "runtime_ui_workbench_focus_edges=4", "runtime_ui_workbench_localization_refs=17", "runtime_ui_workbench_localization_identity_ready=1", "runtime_ui_workbench_accessibility_refs=11", "runtime_ui_workbench_accessibility_identity_ready=1", "runtime_ui_workbench_diagnostics=0", "zero renderer/text-shaping/font-rasterization/IME/accessibility-bridge/image-decoding/native-platform adapter invocation", "Dear ImGui/SDL3/UI middleware")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUiWorkbench) $runtimeUiWorkbenchGuidanceNeedle "runtime UI workbench game guidance" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "MonospaceTextLayoutPolicy" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_accessibility_publish" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "publish_accessibility_payload" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IAccessibilityAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_ime_composition_update" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "publish_ime_composition" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IImeAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_platform_text_input_session" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "begin_platform_text_input" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_platform_text_input_end" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "end_platform_text_input" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IPlatformIntegrationAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_text_shaping_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "shape_text_run" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "ITextShapingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "text shaping request validation" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_font_rasterization_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "rasterize_font_glyph" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IFontRasterizerAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "adapter allocation diagnostics" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_image_decode_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "decode_image_request" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "IImageDecodingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "adapter output diagnostics" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "PngImageDecodingAdapter" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "decode_audited_png_rgba8" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "author_packed_ui_atlas_from_decoded_images" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "plan_packed_ui_atlas_package_update" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "GameEngine.CookedTexture.v1" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "instead of parsing source PNG files" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererGlyphAtlasPalette" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererGlyphAtlasBinding" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "text glyph availability/resolution/missing/submission" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "UiRendererImagePalette" "runtime UI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntimeUi) "image sprite submission" "runtime UI game guidance"
foreach ($runtimeUiGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $runtimeUiText = Get-AgentSurfaceText $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "MonospaceTextLayoutPolicy" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "UiRendererGlyphAtlasPalette" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "UiRendererImagePalette" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "AccessibilityPublishPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "ImeCompositionPublishPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "PlatformTextInputSessionPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "FontRasterizationRequestPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "ImageDecodeRequestPlan" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "plan_image_decode_request" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "glyph atlas generation" $runtimeUiGuidance
    Assert-ContainsText $runtimeUiText "image decoding" $runtimeUiGuidance
}
foreach ($gameplayDebugOverlayGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    "docs/specs/generated-game-validation-scenarios.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $gameplayDebugOverlayText = Get-AgentSurfaceText $gameplayDebugOverlayGuidance
    Assert-ContainsText $gameplayDebugOverlayText "debug overlay" $gameplayDebugOverlayGuidance
    Assert-ContainsText $gameplayDebugOverlayText "RuntimeGameplayDebugOverlayPlan" $gameplayDebugOverlayGuidance
    Assert-ContainsText $gameplayDebugOverlayText "plan_runtime_gameplay_debug_overlay" $gameplayDebugOverlayGuidance
}
foreach ($runtimeUiPngGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    "docs/dependencies.md",
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md",
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $runtimeUiPngText = Get-AgentSurfaceText $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText "PngImageDecodingAdapter" $runtimeUiPngGuidance
    Assert-ContainsText $runtimeUiPngText "decode_audited_png_rgba8" $runtimeUiPngGuidance
}
foreach ($runtimeUiDecodedAtlasGuidance in @(
    "docs/ui.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/current-capabilities.md",
    "docs/dependencies.md",
    "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md",
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
)) {
    $runtimeUiDecodedAtlasText = Get-AgentSurfaceText $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "author_packed_ui_atlas_from_decoded_images" $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "plan_packed_ui_atlas_package_update" $runtimeUiDecodedAtlasGuidance
    Assert-ContainsText $runtimeUiDecodedAtlasText "GameEngine.CookedTexture.v1" $runtimeUiDecodedAtlasGuidance
}
$geUiHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$geUiSourceText = Get-AgentSurfaceText "engine/ui/src/ui.cpp"
$runtimeUiWorkbenchHeaderText = Get-AgentSurfaceText "engine/ui/include/mirakana/ui/runtime_ui_workbench.hpp"; $runtimeUiWorkbenchSourceText = Get-AgentSurfaceText "engine/ui/src/runtime_ui_workbench.cpp"; $runtimeUiWorkbenchTestsText = Get-AgentSurfaceText "tests/unit/runtime_ui_workbench_tests.cpp"
$sourceImageDecodeHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/source_image_decode.hpp"
$sourceImageDecodeSourceText = Get-AgentSurfaceText "engine/tools/asset/source_image_decode.cpp"
$uiAtlasToolHeaderText = Get-AgentSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp"
$uiAtlasToolSourceText = Get-AgentSurfaceText "engine/tools/asset/ui_atlas_tool.cpp"
$toolsTestsText = Get-AgentSurfaceText "tests/unit/tools_tests.cpp"
$uiRendererHeaderText = Get-AgentSurfaceText "engine/ui_renderer/include/mirakana/ui_renderer/ui_renderer.hpp"
$uiRendererSourceText = Get-AgentSurfaceText "engine/ui_renderer/src/ui_renderer.cpp"
$uiRendererTestsText = Get-AgentSurfaceText "tests/unit/ui_renderer_tests.cpp"
Assert-ContainsText $geUiHeaderText "TextAdapterGlyphPlaceholder" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "std::uint32_t glyph" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "AccessibilityPublishPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "AccessibilityPublishResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_accessibility_publish" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "publish_accessibility_payload" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IAccessibilityAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImeCompositionPublishPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImeCompositionPublishResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_ime_composition_update" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "publish_ime_composition" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IImeAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputSessionPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputSessionResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputEndPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "PlatformTextInputEndResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "begin_platform_text_input" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "end_platform_text_input" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IPlatformIntegrationAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "TextShapingRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "TextShapingResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_text_shaping_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "shape_text_run" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ITextShapingAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_text_shaping_result" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "FontRasterizationRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "FontRasterizationResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_font_rasterization_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "rasterize_font_glyph" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IFontRasterizerAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_font_allocation" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodeRequestPlan" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodeDispatchResult" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "ImageDecodePixelFormat" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "plan_image_decode_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "decode_image_request" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "IImageDecodingAdapter" "MK_ui public header"
Assert-ContainsText $geUiHeaderText "invalid_image_decode_result" "MK_ui public header"
foreach ($gameplayDebugOverlayHeaderNeedle in @(
    "RuntimeGameplayDebugOverlayCategory",
    "RuntimeGameplayDebugOverlayRowKind",
    "RuntimeGameplayDebugOverlayDiagnosticCode",
    "RuntimeGameplayDebugOverlayRowDesc",
    "RuntimeGameplayDebugOverlayRow",
    "RuntimeGameplayDebugOverlayDiagnostic",
    "RuntimeGameplayDebugOverlayPlan",
    "plan_runtime_gameplay_debug_overlay"
)) {
    Assert-ContainsText $geUiHeaderText $gameplayDebugOverlayHeaderNeedle "MK_ui public header"
}
Assert-ContainsText $geUiSourceText "utf8_scalar_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "span.glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "AccessibilityPublishPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "AccessibilityPublishResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.publish_nodes" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_accessibility_bounds" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImeCompositionPublishPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImeCompositionPublishResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.update_composition" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_ime_target" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_ime_cursor" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputSessionPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputSessionResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputEndPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "PlatformTextInputEndResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.begin_text_input" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.end_text_input" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_platform_text_input_target" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_platform_text_input_bounds" "MK_ui source"
Assert-ContainsText $geUiSourceText "TextShapingRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "TextShapingResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.shape_text" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_text" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_font_family" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_max_width" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_text_shaping_result" "MK_ui source"
Assert-ContainsText $geUiSourceText "FontRasterizationRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "FontRasterizationResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.rasterize_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_family" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_glyph" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_pixel_size" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_font_allocation" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImageDecodeRequestPlan::ready" "MK_ui source"
Assert-ContainsText $geUiSourceText "ImageDecodeDispatchResult::succeeded" "MK_ui source"
Assert-ContainsText $geUiSourceText "adapter.decode_image" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_image_decode_uri" "MK_ui source"
Assert-ContainsText $geUiSourceText "empty_image_decode_bytes" "MK_ui source"
Assert-ContainsText $geUiSourceText "invalid_image_decode_result" "MK_ui source"
foreach ($gameplayDebugOverlaySourceNeedle in @(
    "RuntimeGameplayDebugOverlayPlan::succeeded",
    "is_valid_runtime_gameplay_debug_overlay_category",
    "is_valid_runtime_gameplay_debug_overlay_row_kind",
    "append_runtime_gameplay_debug_overlay_diagnostic",
    "RuntimeGameplayDebugOverlayDiagnosticCode::duplicate_row_id",
    "RuntimeGameplayDebugOverlayDiagnosticCode::missing_label",
    "RuntimeGameplayDebugOverlayDiagnosticCode::unsupported_category",
    "RuntimeGameplayDebugOverlayDiagnosticCode::unsupported_row_kind"
)) {
    Assert-ContainsText $geUiSourceText $gameplayDebugOverlaySourceNeedle "MK_ui source"
}
foreach ($runtimeUiWorkbenchHeaderNeedle in @("RuntimeUiWorkbenchStatus", "RuntimeUiWorkbenchPanelKind", "RuntimeUiWorkbenchItemRowKind", "RuntimeUiWorkbenchDiagnosticCode", "RuntimeUiWorkbenchPanelRow", "RuntimeUiWorkbenchTableColumn", "RuntimeUiWorkbenchTableRow", "RuntimeUiWorkbenchGraphSeries", "RuntimeUiWorkbenchItemRow", "RuntimeUiWorkbenchTextInputFieldRow", "RuntimeUiWorkbenchFocusPlan", "RuntimeUiWorkbenchLocalizationRef", "RuntimeUiWorkbenchAccessibilityRef", "RuntimeUiWorkbenchDocument", "RuntimeUiWorkbenchPlan", "PlatformTextInputRequest", "plan_runtime_ui_workbench")) { Assert-ContainsText $runtimeUiWorkbenchHeaderText $runtimeUiWorkbenchHeaderNeedle "runtime UI workbench public header" }
foreach ($runtimeUiWorkbenchSourceNeedle in @("RuntimeUiWorkbenchPlan::succeeded", "is_valid_panel_kind", "is_valid_item_kind", "has_backend_reference", "is_forbidden_backend_token", "table_column_owner_id", "unsupported_backend_reference", "invalid_graph_point", "invalid_shop_price", "invalid_text_input_bounds", "missing_focus_target_id", "unknown_focus_target", "RuntimeUiWorkbenchStatus::ready")) { Assert-ContainsText $runtimeUiWorkbenchSourceText $runtimeUiWorkbenchSourceNeedle "runtime UI workbench source" }
foreach ($runtimeUiWorkbenchTestNeedle in @("runtime ui workbench plans dense runtime rows without adapter invocation", "runtime ui workbench fails closed for invalid dense ui and backend references", "runtime ui workbench accepts bounded natural language and localization tokens", "runtime ui workbench rejects empty focus target ids", "runtime ui workbench rejects colliding focus target ids", "RuntimeUiWorkbenchDiagnosticCode::unsupported_backend_reference", "RuntimeUiWorkbenchDiagnosticCode::unknown_focus_target", "invoked_renderer_submission", "platform_text_input_requests.size() == 1U")) { Assert-ContainsText $runtimeUiWorkbenchTestsText $runtimeUiWorkbenchTestNeedle "runtime UI workbench tests" }
Assert-ContainsText (Get-AgentSurfaceText "engine/ui/CMakeLists.txt") "runtime_ui_workbench.cpp" "MK_ui source list"
Assert-ContainsText (Get-AgentSurfaceText "CMakeLists.txt") "MK_runtime_ui_workbench_tests" "runtime UI workbench test target"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/core_tests.cpp") "runtime gameplay debug overlay plan produces deterministic display rows" "tests/unit/core_tests.cpp"
Assert-ContainsText (Get-AgentSurfaceText "tests/unit/core_tests.cpp") "runtime gameplay debug overlay plan rejects duplicate and invalid rows" "tests/unit/core_tests.cpp"
Assert-ContainsText $sourceImageDecodeHeaderText "PngImageDecodingAdapter" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeHeaderText "ui::IImageDecodingAdapter" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeHeaderText "decode_image" "MK_tools source image decode public header"
Assert-ContainsText $sourceImageDecodeSourceText "PngImageDecodingAdapter::decode_image" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "decode_audited_png_rgba8" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "ImageDecodePixelFormat::rgba8_unorm" "MK_tools source image decode source"
Assert-ContainsText $sourceImageDecodeSourceText "catch (...)" "MK_tools source image decode source"
Assert-ContainsText $toolsTestsText "runtime UI PNG image decoding adapter fails closed when importers are disabled" "MK_tools tests"
Assert-ContainsText $toolsTestsText "runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled" "MK_tools tests"
Assert-ContainsText $toolsTestsText "invalid_image_decode_result" "MK_tools tests"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiAtlasPackageUpdateDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "author_packed_ui_atlas_from_decoded_images" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "plan_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "apply_packed_ui_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiGlyphAtlasAuthoringDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "PackedUiGlyphAtlasPackageUpdateDesc" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "author_packed_ui_glyph_atlas_from_rasterized_glyphs" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "plan_packed_ui_glyph_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "apply_packed_ui_glyph_atlas_package_update" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "rasterized-glyph-adapter" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolHeaderText "deterministic-glyph-atlas-rgba8-max-side" "MK_tools ui atlas tool public header"
Assert-ContainsText $uiAtlasToolSourceText "pack_sprite_atlas_rgba8_max_side" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "GameEngine.CookedTexture.v1" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "decoded image must be RGBA8" "MK_tools ui atlas tool source"
Assert-ContainsText $uiAtlasToolSourceText "rasterized glyph must be RGBA8" "MK_tools ui atlas tool source"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas authoring maps decoded images into texture page and metadata" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas package update writes texture page metadata and package index" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas rejects invalid decoded images and package path collisions" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI atlas apply leaves existing files unchanged when validation fails" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas package update writes texture page metadata and package index" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions" "MK_tools tests"
Assert-ContainsText $toolsTestsText "packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails" "MK_tools tests"
Assert-ContainsText $uiRendererHeaderText "UiRendererGlyphAtlasBinding" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "UiRendererGlyphAtlasPalette" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "text_glyph_sprites_submitted" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "make_ui_text_glyph_sprite_command" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererHeaderText "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "MK_ui_renderer public header"
Assert-ContainsText $uiRendererSourceText "resolve_ui_text_glyph_binding" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "make_ui_text_glyph_sprite_command" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas" "MK_ui_renderer source"
Assert-ContainsText $uiRendererSourceText "text_glyphs_missing" "MK_ui_renderer source"
Assert-ContainsText $uiRendererTestsText "ui renderer submits monospace text glyphs through glyph atlas palette" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui renderer reports missing glyph atlas bindings without fake sprites" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingAccessibilityAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui accessibility publish plan dispatches validated nodes to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui accessibility publish plan blocks invalid nodes before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingImeAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui ime composition publish plan dispatches valid composition to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui ime composition publish plan blocks invalid composition before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingFontRasterizerAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "InvalidFontRasterizerAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui font rasterization result reports invalid adapter allocation" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingTextShapingAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui text shaping result reports invalid adapter runs" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "CapturingImageDecodingAdapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode request plan dispatches valid request to adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode request plan blocks invalid request before adapter" "MK_ui_renderer tests"
Assert-ContainsText $uiRendererTestsText "ui image decode result reports missing or invalid adapter output" "MK_ui_renderer tests"

$geRuntimeModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime" })
if ($geRuntimeModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime module"
}
if ($geRuntimeModule[0].status -ne "ready-runtime-resource-v2-gameplay-interaction-framework") {
    Write-Error "engine/agent/manifest.json MK_runtime status must advertise the closed Runtime Resource v2 plus gameplay interaction framework surface honestly"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentInput")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentInput"
}
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputStateView" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputContextStack" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "GameEngine.RuntimeInputActions.v4" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_gamepad_axis" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_key_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_pointer_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_gamepad_button_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "bind_key_axis_in_context" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "RuntimeInputRebindingPresentationModel" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "make_runtime_input_rebinding_presentation" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "capture_runtime_input_rebinding_axis" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "symbolic glyph lookup keys" "MK_runtime module purpose"
Assert-ContainsText ([string]$geRuntimeModule[0].purpose) "platform input glyph generation" "MK_runtime module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputStateView" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputContextStack" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_button" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_axis" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_key_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_pointer_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_gamepad_button_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "bind_key_axis_in_context" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "axis_value" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "UI focus integration" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "global input consumption" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "radial stick deadzones" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "per-device profiles" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_action" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingFocusCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingFocusCaptureResult" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_action_with_focus" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "gameplay_input_consumed" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingPresentationModel" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "make_runtime_input_rebinding_presentation" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "RuntimeInputRebindingAxisCaptureRequest" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "capture_runtime_input_rebinding_axis" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInput) "symbolic glyph lookup keys" "input game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "GameEngine.RuntimeInputRebindingProfile.v1" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "apply_runtime_input_rebinding_profile" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingCaptureResult" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_action" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingFocusCaptureRequest" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingFocusCaptureResult" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_action_with_focus" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingAxisCaptureRequest" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "capture_runtime_input_rebinding_axis" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "gameplay_input_consumed" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "RuntimeInputRebindingPresentationToken" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "present_runtime_input_axis_source" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "platform input glyph generation" "input rebinding profile guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentInputRebindingProfiles) "interactive runtime/game rebinding panels" "input rebinding profile guidance"
foreach ($inputGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $inputText = Get-AgentSurfaceText $inputGuidance
    Assert-ContainsText $inputText "RuntimeInputStateView" $inputGuidance
    Assert-ContainsText $inputText "RuntimeInputContextStack" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_button" $inputGuidance
    Assert-ContainsText $inputText "GameEngine.RuntimeInputActions.v4" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_axis" $inputGuidance
    Assert-ContainsText $inputText "bind_key_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_pointer_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_gamepad_button_in_context" $inputGuidance
    Assert-ContainsText $inputText "bind_key_axis_in_context" $inputGuidance
    Assert-ContainsText $inputText "axis_value" $inputGuidance
    Assert-ContainsText $inputText "UI focus integration" $inputGuidance
    Assert-ContainsText $inputText "global input consumption" $inputGuidance
    Assert-ContainsText $inputText "radial stick deadzones" $inputGuidance
    Assert-ContainsText $inputText "per-device profiles" $inputGuidance
}
foreach ($inputRebindingGuidance in @(
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $inputRebindingText = Get-AgentSurfaceText $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "GameEngine.RuntimeInputRebindingProfile.v1" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingProfile" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "apply_runtime_input_rebinding_profile" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_action" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingFocusCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingFocusCaptureResult" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_action_with_focus" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingAxisCaptureRequest" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "capture_runtime_input_rebinding_axis" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "gameplay_input_consumed" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "RuntimeInputRebindingPresentationModel" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "make_runtime_input_rebinding_presentation" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "symbolic glyph lookup keys" $inputRebindingGuidance
    Assert-ContainsText $inputRebindingText "interactive runtime/game rebinding" $inputRebindingGuidance
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAudio")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAudio"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioDeviceStreamRequest" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioDeviceStreamPlan" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "plan_audio_device_stream" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "render_audio_device_stream_interleaved_float" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioGameplayMixRequest" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "AudioGameplayMixPlan" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "plan_gameplay_audio_mix" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "device_frame + queued_frames" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "device hotplug/selection" "audio game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAudio) "mixer authoring" "audio game guidance"
foreach ($audioGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $audioText = Get-AgentSurfaceText $audioGuidance
    Assert-ContainsText $audioText "AudioDeviceStreamRequest" $audioGuidance
    Assert-ContainsText $audioText "AudioDeviceStreamPlan" $audioGuidance
    Assert-ContainsText $audioText "plan_audio_device_stream" $audioGuidance
    Assert-ContainsText $audioText "render_audio_device_stream_interleaved_float" $audioGuidance
    Assert-ContainsText $audioText "AudioGameplayMixRequest" $audioGuidance
    Assert-ContainsText $audioText "plan_gameplay_audio_mix" $audioGuidance
    Assert-ContainsText $audioText "device hotplug/selection" $audioGuidance
    Assert-ContainsText $audioText "mixer authoring" $audioGuidance
}
$geNavigationModule = @($manifest.modules | Where-Object { $_.name -eq "MK_navigation" })
if ($geNavigationModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_navigation module"
}
if ($geNavigationModule[0].status -ne "implemented-production-path-planner") {
    Write-Error "engine/agent/manifest.json MK_navigation status must advertise the production path planner slice honestly"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_replan.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_replan.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/local_avoidance.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include local_avoidance.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/path_smoothing.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include path_smoothing.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_path_planner.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_path_planner.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_navmesh.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_navmesh.hpp"
}
if ($geNavigationModule[0].publicHeaders -notcontains "engine/navigation/include/mirakana/navigation/navigation_hierarchical_world.hpp") {
    Write-Error "engine/agent/manifest.json MK_navigation publicHeaders must include navigation_hierarchical_world.hpp"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentNavigation")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentNavigation"
}
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "validate_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "replan_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "calculate_navigation_local_avoidance" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "smooth_navigation_grid_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationGridAgentPathRequest" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationGridAgentPathPlan" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "plan_navigation_grid_agent_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationNavmeshPathRequest" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "plan_navigation_navmesh_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "NavigationHierarchicalWorldPathRequest" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "plan_navigation_hierarchical_world_path" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "automatic nav baking" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "navmesh" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "crowd" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "scene/physics integration" "MK_navigation module purpose"
Assert-ContainsText ([string]$geNavigationModule[0].purpose) "editor visualization" "MK_navigation module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "validate_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "replan_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "calculate_navigation_local_avoidance" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "smooth_navigation_grid_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationGridAgentPathRequest" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationGridAgentPathPlan" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "plan_navigation_grid_agent_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationNavmeshPathRequest" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "plan_navigation_navmesh_path" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "NavigationCrowdPlanRequest" "navigation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentNavigation) "plan_navigation_navmesh_crowd" "navigation game guidance"
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentHierarchicalWorldNavigation")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentHierarchicalWorldNavigation"
}
foreach ($navigationWorldGuidanceNeedle in @(
    "NavigationHierarchicalWorldPathRequest",
    "NavigationHierarchicalWorldPathResult",
    "NavigationHierarchicalWorldPortalPathRow",
    "plan_navigation_hierarchical_world_path",
    "world-region refs",
    "nav-data refs",
    "renderer/RHI/native handles"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentHierarchicalWorldNavigation) $navigationWorldGuidanceNeedle "hierarchical world navigation game guidance"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentWorldRegionNavigationRefs")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentWorldRegionNavigationRefs"
}
foreach ($worldRegionNavigationGuidanceNeedle in @(
    "RuntimeWorldRegionNavigationRefReviewRequest",
    "RuntimeWorldRegionNavigationRefReviewResult",
    "RuntimeWorldRegionNavigationPathCacheReviewRequest",
    "RuntimeWorldRegionNavigationPathCacheReviewResult",
    "review_runtime_world_region_navigation_refs",
    "review_runtime_world_region_navigation_path_cache",
    "RuntimeResidentCatalogCacheV2",
    "unrefreshed",
    "value-only"
)) {
    Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentWorldRegionNavigationRefs) $worldRegionNavigationGuidanceNeedle "world-region navigation game guidance"
}
foreach ($navigationGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "docs/ai-game-development.md",
    "docs/specs/generated-game-validation-scenarios.md",
    "docs/specs/game-prompt-pack.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $navigationText = Get-AgentSurfaceText $navigationGuidance
    Assert-ContainsText $navigationText "validate_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "replan_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "calculate_navigation_local_avoidance" $navigationGuidance
    Assert-ContainsText $navigationText "smooth_navigation_grid_path" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationGridAgentPathRequest" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationGridAgentPathPlan" $navigationGuidance
    Assert-ContainsText $navigationText "plan_navigation_grid_agent_path" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationNavmeshPathRequest" $navigationGuidance
    Assert-ContainsText $navigationText "plan_navigation_navmesh_path" $navigationGuidance
    Assert-ContainsText $navigationText "NavigationCrowdPlanRequest" $navigationGuidance
    Assert-ContainsText $navigationText "plan_navigation_navmesh_crowd" $navigationGuidance
    Assert-ContainsText $navigationText "navmesh" $navigationGuidance
    Assert-ContainsText $navigationText "crowd" $navigationGuidance
    Assert-ContainsText $navigationText "scene/physics" $navigationGuidance
    Assert-ContainsText $navigationText "editor" $navigationGuidance
}
foreach ($hierarchicalNavigationGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    "docs/specs/generated-game-validation-scenarios.md",
    "docs/specs/game-prompt-pack.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $hierarchicalNavigationText = Get-AgentSurfaceText $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "NavigationHierarchicalWorldPathRequest" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "plan_navigation_hierarchical_world_path" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "RuntimeWorldRegionNavigationRefReviewRequest" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "review_runtime_world_region_navigation_refs" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "RuntimeWorldRegionNavigationPathCacheReviewRequest" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "review_runtime_world_region_navigation_path_cache" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "nav-data" $hierarchicalNavigationGuidance
    Assert-ContainsText $hierarchicalNavigationText "native handles" $hierarchicalNavigationGuidance
}
$navigationHierarchicalWorldHeader = Get-AgentSurfaceText "engine/navigation/include/mirakana/navigation/navigation_hierarchical_world.hpp"
foreach ($navigationHierarchicalWorldHeaderNeedle in @(
    "NavigationHierarchicalWorldPathRequest",
    "NavigationHierarchicalWorldPathResult",
    "NavigationHierarchicalWorldPortalPathRow",
    "plan_navigation_hierarchical_world_path"
)) {
    Assert-ContainsText $navigationHierarchicalWorldHeader $navigationHierarchicalWorldHeaderNeedle "navigation_hierarchical_world.hpp"
}
$worldRegionStreamingHeader = Get-AgentSurfaceText "engine/runtime/include/mirakana/runtime/world_region_streaming.hpp"
foreach ($worldRegionNavigationHeaderNeedle in @(
    "RuntimeWorldRegionNavigationRefReviewRequest",
    "RuntimeWorldRegionNavigationRefReviewResult",
    "RuntimeWorldRegionNavigationPathCacheEntry",
    "RuntimeWorldRegionNavigationPathCacheReviewRequest",
    "RuntimeWorldRegionNavigationPathCacheReviewResult",
    "review_runtime_world_region_navigation_refs",
    "review_runtime_world_region_navigation_path_cache", "catalog_cache_not_ready",
    "RuntimeWorldStreamingLargeSceneReadinessRequest", "RuntimeWorldStreamingLargeSceneReadinessReport", "evaluate_runtime_world_streaming_large_scene_readiness"
)) {
    Assert-ContainsText $worldRegionStreamingHeader $worldRegionNavigationHeaderNeedle "world_region_streaming.hpp"
}

if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentPhysics")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentPhysics"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterController3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterController3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "move_physics_character_controller_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAuthoredCollisionScene3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAuthoredCollisionScene3DBuildResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "build_physics_world_3d_from_authored_collision_scene" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCollisionQueryBatchStatus" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCollisionQueryBatchDiagnostic" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCollisionQueryRowStatus" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCollisionQueryRowDiagnostic" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsRaycastBatch2DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld2D::raycast_batch" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::shape_sweep_batch" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "source-indexed value-only query rows" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "default-unbounded query counts" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "explicit positive max_queries budgets" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "collision_query_batch_ready" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::aabb" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::sphere" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsShape3DDesc::capsule" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsQueryFilter3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::exact_shape_sweep" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsExactSphereCast3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsExactSphereCast3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::exact_sphere_cast" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContactPoint3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContactManifold3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::contact_manifolds" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "warm-start-safe" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DConfig" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DRow" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsContinuousStep3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsWorld3D::step_continuous" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DRowKind" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DRow" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsCharacterDynamicPolicy3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "evaluate_physics_character_dynamic_policy_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAdvancedController3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsMovingPlatform3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsAdvancedController3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "plan_physics_advanced_controller_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsJoint3DStatus" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsDistanceJoint3DDesc" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsJointSolve3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "solve_physics_joints_3d" "physics game guidance"
foreach ($needle in @("PhysicsConstraint3DStatus", "PhysicsConstraint3DDiagnostic", "PhysicsFixedConstraint3DDesc", "PhysicsLinearAxisConstraint3DDesc", "PhysicsConstraintSolve3DResult", "solve_physics_constraints_3d", "max_rows", "row_budget_exceeded", "rotational rigid-body constraints")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) $needle "physics game guidance" }
foreach ($needle in @("PhysicsKinematicMotion3DResult", "plan_physics_kinematic_motion_3d", "PhysicsSimpleVehicle3DDesc", "PhysicsSimpleVehicle3DWheelDesc", "PhysicsSimpleVehicle3DResult", "plan_physics_simple_vehicle_3d", "simple vehicle")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) $needle "physics game guidance" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsReplaySignature3D" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "PhysicsDeterminismGate3DResult" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "make_physics_replay_signature_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "evaluate_physics_determinism_gate_3d" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "first-party Physics 1.0 ready surface" "physics game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentPhysics) "Jolt/native backends" "physics game guidance"
foreach ($physicsGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $physicsText = Get-AgentSurfaceText $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCollisionQueryBatchStatus" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCollisionQueryBatchDiagnostic" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCollisionQueryRowStatus" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCollisionQueryRowDiagnostic" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld2D::raycast_batch" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::shape_sweep_batch" $physicsGuidance
    Assert-ContainsText $physicsText "default-unbounded" $physicsGuidance
    Assert-ContainsText $physicsText "collision_query_batch_ready" $physicsGuidance
    Assert-ContainsText $physicsText "move_physics_character_controller_3d" $physicsGuidance
    Assert-ContainsText $physicsText "build_physics_world_3d_from_authored_collision_scene" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::exact_shape_sweep" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::aabb" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::sphere" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsShape3DDesc::capsule" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsQueryFilter3D" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::exact_sphere_cast" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::contact_manifolds" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsWorld3D::step_continuous" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsCharacterDynamicPolicy3DDesc" $physicsGuidance
    Assert-ContainsText $physicsText "evaluate_physics_character_dynamic_policy_3d" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsAdvancedController3DDesc" $physicsGuidance
    Assert-ContainsText $physicsText "plan_physics_advanced_controller_3d" $physicsGuidance
    Assert-ContainsText $physicsText "PhysicsJointSolve3DResult" $physicsGuidance
    Assert-ContainsText $physicsText "solve_physics_joints_3d" $physicsGuidance
    foreach ($needle in @("PhysicsConstraintSolve3DResult", "solve_physics_constraints_3d", "max_rows", "row_budget_exceeded", "rotational rigid-body constraints")) { Assert-ContainsText $physicsText $needle $physicsGuidance }
    foreach ($needle in @("PhysicsKinematicMotion3DResult", "plan_physics_kinematic_motion_3d", "PhysicsSimpleVehicle3DDesc", "plan_physics_simple_vehicle_3d", "simple vehicle")) { Assert-ContainsText $physicsText $needle $physicsGuidance }
    Assert-ContainsText $physicsText "PhysicsReplaySignature3D" $physicsGuidance
    Assert-ContainsText $physicsText "evaluate_physics_determinism_gate_3d" $physicsGuidance
}
foreach ($advancedControllerPackageGuidance in @(
    "docs/current-capabilities.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    "engine/agent/manifest.json",
    "games/sample_generated_desktop_runtime_3d_package/game.agent.json",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".agents/skills/gameengine-game-development/references/full-guidance.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/references/full-guidance.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $advancedControllerPackageText = Get-AgentSurfaceText $advancedControllerPackageGuidance
    foreach ($needle in @("gameplay_systems_advanced_controller_status=moved", "gameplay_systems_advanced_controller_platform_applied=1", "gameplay_systems_advanced_controller_constraint_rows=1", "gameplay_systems_advanced_controller_replay_changed=1", "gameplay_systems_physics_constraints_status=solved", "gameplay_systems_physics_constraints_diagnostic=none", "gameplay_systems_physics_constraints_rows=2", "gameplay_systems_physics_constraints_fixed_rows=1", "gameplay_systems_physics_constraints_linear_axis_rows=1", "gameplay_systems_physics_constraints_axis_limit_clamped=1", "gameplay_systems_kinematic_motion_status=constrained", "gameplay_systems_kinematic_motion_rows=2", "gameplay_systems_vehicle_status=grounded", "gameplay_systems_vehicle_diagnostic=none", "gameplay_systems_vehicle_wheel_rows=4", "gameplay_systems_vehicle_grounded_wheels=4", "gameplay_systems_vehicle_wheel_probe_hits=4")) { Assert-ContainsText $advancedControllerPackageText $needle $advancedControllerPackageGuidance }
}
foreach ($physicsUnsupportedGuidance in @(
    "docs/architecture.md",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $physicsText = Get-AgentSurfaceText $physicsUnsupportedGuidance
    foreach ($needle in @("dynamic-vs-dynamic TOI", "rotational CCD", "2D CCD", "Jolt")) { Assert-ContainsText $physicsText $needle $physicsUnsupportedGuidance }
}

$geAiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_ai" })
if ($geAiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_ai module"
}
if ($geAiModule[0].status -ne "implemented-behavior-tree-blackboard-perception-authoring") {
    Write-Error "engine/agent/manifest.json MK_ai status must advertise the behavior tree blackboard, perception, and behavior authoring slice honestly"
}
if ($geAiModule[0].publicHeaders -notcontains "engine/ai/include/mirakana/ai/behavior_tree.hpp") {
    Write-Error "engine/agent/manifest.json MK_ai publicHeaders must include behavior_tree.hpp"
}
if ($geAiModule[0].publicHeaders -notcontains "engine/ai/include/mirakana/ai/perception.hpp") {
    Write-Error "engine/agent/manifest.json MK_ai publicHeaders must include perception.hpp"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAi")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAi"
}
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeBlackboard" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeBlackboardCondition" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorTreeEvaluationContext" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "AiPerceptionAgent2D" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "AiPerceptionTarget2D" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "build_ai_perception_snapshot_2d" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "write_ai_perception_blackboard" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorAuthoringDocument" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "BehaviorAuthoringValidationContext" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "validate_behavior_authoring_document" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "duplicate-blackboard-key" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "blackboard-type-mismatch" "MK_ai module purpose"
Assert-ContainsText ([string]$geAiModule[0].purpose) "persistent blackboard services" "MK_ai module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeBlackboard" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeBlackboardCondition" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorTreeEvaluationContext" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "AiPerceptionAgent2D" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "AiPerceptionTarget2D" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "build_ai_perception_snapshot_2d" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "write_ai_perception_blackboard" "AI game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "evaluate_ai_perception_readiness_2d" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorAuthoringDocument" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "BehaviorAuthoringValidationContext" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "validate_behavior_authoring_document" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "blackboard-type-mismatch" "AI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAi) "persistent blackboard services" "AI game guidance"
foreach ($aiApiGuidance in @(
    "docs/architecture.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    "docs/specs/generated-game-validation-scenarios.md",
    "docs/specs/game-prompt-pack.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $aiApiText = Get-AgentSurfaceText $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorTreeBlackboard" $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorTreeEvaluationContext" $aiApiGuidance
    Assert-ContainsText $aiApiText "AiPerceptionAgent2D" $aiApiGuidance
    Assert-ContainsText $aiApiText "AiPerceptionTarget2D" $aiApiGuidance
    Assert-ContainsText $aiApiText "build_ai_perception_snapshot_2d" $aiApiGuidance
    Assert-ContainsText $aiApiText "write_ai_perception_blackboard" $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorAuthoringDocument" $aiApiGuidance
    Assert-ContainsText $aiApiText "BehaviorAuthoringValidationContext" $aiApiGuidance
    Assert-ContainsText $aiApiText "validate_behavior_authoring_document" $aiApiGuidance
    Assert-ContainsText $aiApiText "blackboard" $aiApiGuidance
}
foreach ($aiStatusGuidance in @(
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json"
)) {
    $aiStatusText = Get-AgentSurfaceText $aiStatusGuidance
    Assert-ContainsText $aiStatusText "Behavior Tree Blackboard Conditions v0" $aiStatusGuidance
    Assert-ContainsText $aiStatusText "AI Perception Services v1" $aiStatusGuidance
    Assert-ContainsText $aiStatusText "AI Behavior Authoring Foundation v1" $aiStatusGuidance
    Assert-ContainsText $aiStatusText "persistent blackboard" $aiStatusGuidance
}
foreach ($sampleAiGuidance in @(
    "games/sample_ai_navigation/main.cpp",
    "games/sample_ai_navigation/README.md",
    "games/sample_ai_navigation/game.agent.json"
)) {
    $sampleAiText = Get-AgentSurfaceText $sampleAiGuidance
    Assert-ContainsText $sampleAiText "blackboard" $sampleAiGuidance
}
Assert-ContainsText (Get-AgentSurfaceText "games/sample_ai_navigation/game.agent.json") "behavior-tree-blackboard-perception-services-v1" "sample_ai_navigation manifest"

foreach ($sampleGameplayGuidance in @(
    "games/sample_gameplay_foundation/main.cpp",
    "games/sample_gameplay_foundation/README.md",
    "games/sample_gameplay_foundation/game.agent.json",
    "docs/current-capabilities.md",
    "docs/roadmap.md",
    "docs/ai-game-development.md",
    "engine/agent/manifest.json"
)) {
    $sampleGameplayText = Get-AgentSurfaceText $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "sample_gameplay_foundation" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "build_physics_world_3d_from_authored_collision_scene" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "move_physics_character_controller_3d" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "plan_navigation_grid_agent_path" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "build_ai_perception_snapshot_2d" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "write_ai_perception_blackboard" $sampleGameplayGuidance
    Assert-ContainsText $sampleGameplayText "render_audio_device_stream_interleaved_float" $sampleGameplayGuidance
}
Assert-ContainsText (Get-AgentSurfaceText "games/sample_gameplay_foundation/game.agent.json") "headless runtime systems composition proof" "sample_gameplay_foundation manifest"
Assert-ContainsText (Get-AgentSurfaceText "games/sample_gameplay_foundation/README.md") "source-tree headless composition evidence only" "sample_gameplay_foundation README"

$geAnimationModule = @($manifest.modules | Where-Object { $_.name -eq "MK_animation" })
if ($geAnimationModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_animation module"
}
if (-not $manifest.gameCodeGuidance.PSObject.Properties.Name.Contains("currentAnimation")) {
    Write-Error "engine/agent/manifest.json must expose gameCodeGuidance.currentAnimation"
}
$geMathModule = @($manifest.modules | Where-Object { $_.name -eq "MK_math" })
if ($geMathModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_math module"
}
$geMathPublicHeaders = @($geMathModule[0].publicHeaders)
if ($geMathPublicHeaders -notcontains "engine/math/include/mirakana/math/quat.hpp") {
    Write-Error "engine/agent/manifest.json MK_math publicHeaders must include quat.hpp"
}
Assert-ContainsText ([string]$geMathModule[0].purpose) "unit quaternions" "MK_math module purpose"
Assert-ContainsText ([string]$geMathModule[0].purpose) "Mat4::rotation_quat" "MK_math module purpose"
Assert-ContainsText (Get-AgentSurfaceText "engine/math/include/mirakana/math/quat.hpp") "struct Quat" "MK_math quaternion public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/math/include/mirakana/math/mat4.hpp") "rotation_quat" "MK_math Mat4 public header"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationCpuSkinningDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationMorphMeshCpuDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationMorphTargetCpuDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_morph_targets_normals_cpu" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_morph_targets_tangents_cpu" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_animation_morph_weights_at_time" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_two_bone_ik_xy_plane" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_two_bone_ik_3d_orientation" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_fabrik_ik_xy_chain" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "solve_animation_fabrik_ik_3d_chain" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationSkeleton3dDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "build_animation_model_pose_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_fabrik_ik_3d_solution_to_pose" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_quat_keyframes" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationJointTrack3dDesc" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "sample_animation_local_pose_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationIkLocalRotationLimit" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "AnimationIkLocalRotationLimit3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_local_rotation_limits_3d" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_animation_fabrik_ik_xy_solution_to_pose" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "make_float_animation_tracks_from_f32_bytes" "MK_animation module purpose"
Assert-ContainsText ([string]$geAnimationModule[0].purpose) "apply_float_animation_samples_to_transform3d" "MK_animation module purpose"
$geAnimationPublicHeaders = @($geAnimationModule[0].publicHeaders)
if ($geAnimationPublicHeaders -notcontains "engine/animation/include/mirakana/animation/chain_ik.hpp") {
    Write-Error "engine/agent/manifest.json MK_animation publicHeaders must include chain_ik.hpp"
}
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "apply_float_animation_samples_to_transform3d" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "QuatKeyframe" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/keyframe_animation.hpp") "sample_quat_keyframes" "MK_animation keyframe animation public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "AnimationFabrikIk3dDesc" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "solve_animation_fabrik_ik_3d_chain" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "apply_animation_fabrik_ik_3d_solution_to_pose" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "AnimationIkLocalRotationLimit3d" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/chain_ik.hpp") "apply_animation_local_rotation_limits_3d" "MK_animation chain IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/two_bone_ik.hpp") "AnimationTwoBoneIk3dDesc" "MK_animation two bone IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/two_bone_ik.hpp") "solve_animation_two_bone_ik_3d_orientation" "MK_animation two bone IK public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "AnimationSkeleton3dDesc" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "build_animation_model_pose_3d" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "AnimationJointTrack3dDesc" "MK_animation skeleton public header"
Assert-ContainsText (Get-AgentSurfaceText "engine/animation/include/mirakana/animation/skeleton.hpp") "sample_animation_local_pose_3d" "MK_animation skeleton public header"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "skin_animation_vertices_cpu" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "make_float_animation_tracks_from_f32_bytes" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_float_animation_samples_to_transform3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "import_gltf_node_transform_animation_binding_source" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "import_gltf_node_transform_animation_tracks_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_and_apply_runtime_scene_render_animation_float_clip" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_runtime_morph_mesh_cpu_animation_float_clip" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_two_bone_ik_3d_orientation" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_fabrik_ik_xy_chain" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "solve_animation_fabrik_ik_3d_chain" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationSkeleton3dDesc" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "build_animation_model_pose_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_fabrik_ik_3d_solution_to_pose" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_quat_keyframes" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationJointTrack3dDesc" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "sample_animation_local_pose_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationIkLocalRotationLimit" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "AnimationIkLocalRotationLimit3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_local_rotation_limits_3d" "animation game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentAnimation) "apply_animation_fabrik_ik_xy_solution_to_pose" "animation game guidance"
foreach ($animationGuidance in @(
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/testing.md",
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $animationText = Get-AgentSurfaceText $animationGuidance
    Assert-ContainsText $animationText "Animation CPU Skinning" $animationGuidance
}
foreach ($animationApiGuidance in @(
    "docs/ai-game-development.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $animationApiText = Get-AgentSurfaceText $animationApiGuidance
    Assert-ContainsText $animationApiText "skin_animation_vertices_cpu" $animationApiGuidance
    Assert-ContainsText $animationApiText "solve_animation_two_bone_ik_3d_orientation" $animationApiGuidance
    Assert-ContainsText $animationApiText "solve_animation_fabrik_ik_3d_chain" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationSkeleton3dDesc" $animationApiGuidance
    Assert-ContainsText $animationApiText "build_animation_model_pose_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "apply_animation_fabrik_ik_3d_solution_to_pose" $animationApiGuidance
    Assert-ContainsText $animationApiText "sample_quat_keyframes" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationJointTrack3dDesc" $animationApiGuidance
    Assert-ContainsText $animationApiText "sample_animation_local_pose_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "import_gltf_node_transform_animation_tracks_3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "AnimationIkLocalRotationLimit3d" $animationApiGuidance
    Assert-ContainsText $animationApiText "apply_animation_local_rotation_limits_3d" $animationApiGuidance
}
$geRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($geRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_rhi module"
}
$geRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_renderer" })
if ($geRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_renderer module"
}
$geRuntimeRhiModule = @($manifest.modules | Where-Object { $_.name -eq "MK_runtime_rhi" })
if ($geRuntimeRhiModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_runtime_rhi module"
}
$geSceneRendererModule = @($manifest.modules | Where-Object { $_.name -eq "MK_scene_renderer" })
if ($geSceneRendererModule.Count -ne 1) {
    Write-Error "engine/agent/manifest.json must expose exactly one MK_scene_renderer module"
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RenderPassDepthAttachment" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "DepthStencilStateDesc" "MK_rhi module purpose"
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp") {
    Write-Error "engine/agent/manifest.json MK_rhi publicHeaders must include resource_lifetime.hpp"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/upload_staging.hpp") {
    Write-Error "engine/agent/manifest.json MK_rhi publicHeaders must include upload_staging.hpp"
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiResourceLifetimeRegistry" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiUploadStagingPlan" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiStagingBufferLease" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiUploadRingDesc::buffer" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "FenceValue" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "foundation-only" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "GPU allocator" "MK_rhi module purpose"
$rhiResourceLifetimeHeaderText = Get-AgentSurfaceText "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp"
$rhiResourceLifetimeTestsText = Get-AgentSurfaceText "tests/unit/rhi_resource_lifetime_tests.cpp"
$d3d12BackendHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "queue-qualified fence retirement" "MK_rhi module purpose"
Assert-ContainsText $rhiResourceLifetimeHeaderText "RhiResourceLifetimeFence" "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp"
Assert-ContainsText $rhiResourceLifetimeHeaderText "RhiResourceLifetimeQueue" "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp"
Assert-ContainsText $rhiResourceLifetimeTestsText "keeps queue fence identities separate" "tests/unit/rhi_resource_lifetime_tests.cpp"
Assert-ContainsText $d3d12BackendHeaderText "completed_queue_fences" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12BackendHeaderText "last_submitted_queue_fences" "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
$geRhiRecentEvidenceText = @($geRhiModule[0].recentEvidence | ForEach-Object { [string]$_ }) -join " "
Assert-ContainsText $geRhiRecentEvidenceText "RHI Native Async Upload Execution v1" "MK_rhi module recentEvidence"
Assert-ContainsText $geRhiRecentEvidenceText "execute_upload_gpu_batch_async" "MK_rhi module recentEvidence"
Assert-ContainsText $geRhiRecentEvidenceText "RhiUploadGpuBatchExecutionResult" "MK_rhi module recentEvidence"
Assert-ContainsText $geRhiRecentEvidenceText "Staging Pool Lease Adoption v1" "MK_rhi module recentEvidence"
Assert-ContainsText $geRhiRecentEvidenceText "RhiStagingBufferLease" "MK_rhi module recentEvidence"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph v1 foundation-only" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphV1Desc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "barrier intent" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Pass Callback Execution v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "execute_frame_graph_v1_schedule" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph RHI Texture Schedule Execution v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Render Pass Envelope v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiTextureExecutionDesc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTexturePassTargetAccess" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "build_frame_graph_texture_pass_target_accesses" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTexturePassTargetState" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "pass_target_state_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTextureFinalState" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "final_state_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "aliasing_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiRenderPassDesc" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "render_passes_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RendererStats::framegraph_render_passes_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "automatic aliasing-barrier insertion" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "execute_frame_graph_rhi_texture_schedule" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Transient Texture Alias Planning v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTransientTextureAliasPlan" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "plan_frame_graph_transient_texture_aliases" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Backend-Neutral Distinct Alias-Group Lease Binding v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "IRhiDevice::acquire_transient_texture_alias_group" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "distinct resource-name FrameGraphTextureBinding rows" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "acquire_frame_graph_transient_texture_lease_bindings" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Texture Aliasing Barrier Command v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphTextureAliasingBarrier" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "record_frame_graph_texture_aliasing_barriers" "MK_renderer module purpose"
if (@($geRuntimeRhiModule[0].publicHeaders) -notcontains "engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp") {
    Write-Error "engine/agent/manifest.json MK_runtime_rhi publicHeaders must include package_streaming_frame_graph.hpp"
}
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "make_runtime_package_streaming_frame_graph_texture_bindings" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimePackageStreamingFrameGraphTextureBindingSource" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "upload_runtime_package_streaming_frame_graph_texture_bindings" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimePackageStreamingFrameGraphTextureUploadSource" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "upload_runtime_package_streaming_mesh_gpu_bindings" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimePackageStreamingMeshUploadSource" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimePackageStreamingMeshUploadBindingResult" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeUploadQueueWaitResult" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "wait_for_runtime_uploads_on_queue" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "upload_queue_waits_recorded" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "MeshGpuBinding rows" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeTextureUploadOptions::upload_ring" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeMeshUploadOptions::upload_ring" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeSkinnedMeshUploadOptions::upload_ring" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeMorphMeshUploadOptions::upload_ring" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RhiUploadRing/RhiUploadStagingPlan" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeTextureUploadResult command-list/queue-wait/pass-target/final-state/barrier/callback counters" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "RuntimeMaterialGpuBinding command-list/queue-wait/barrier/callback counters" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "material-factor uploads" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$geRuntimeRhiModule[0].purpose) "broad/background package streaming" "MK_runtime_rhi module purpose"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "make_runtime_package_streaming_frame_graph_texture_bindings" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "upload_runtime_package_streaming_frame_graph_texture_bindings" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "upload_runtime_package_streaming_mesh_gpu_bindings" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "wait_for_runtime_uploads_on_queue" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "upload_queue_waits_recorded" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "RuntimeMeshUploadOptions::upload_ring" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "RhiStagingBufferLease" "desktop runtime 3d package streaming guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dPackageStreamingSafePointSmoke) "RhiUploadRingDesc::buffer" "desktop runtime 3d package streaming guidance"
foreach ($packageStreamingFrameGraphGuidance in @(
        "docs/rhi.md",
        "docs/current-capabilities.md",
        "docs/ai-game-development.md",
        "docs/architecture.md",
        ".agents/skills/rendering-change/references/full-guidance.md",
        ".claude/skills/gameengine-rendering/references/full-guidance.md"
    )) {
    $packageStreamingFrameGraphText = Get-AgentSurfaceText $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "make_runtime_package_streaming_frame_graph_texture_bindings" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "upload_runtime_package_streaming_frame_graph_texture_bindings" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "upload_runtime_package_streaming_mesh_gpu_bindings" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "wait_for_runtime_uploads_on_queue" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "upload_queue_waits_recorded" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "RuntimeTextureUploadOptions::upload_ring" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "RuntimeMeshUploadOptions::upload_ring" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "RhiStagingBufferLease" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "RhiUploadRingDesc::buffer" $packageStreamingFrameGraphGuidance
    Assert-ContainsText $packageStreamingFrameGraphText "broad" $packageStreamingFrameGraphGuidance
}
Assert-ContainsText ([string]$geRhiModule[0].purpose) "IRhiCommandList::texture_aliasing_barrier" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRhiModule[0].purpose) "RhiStats::texture_aliasing_barriers" "MK_rhi module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiFrameRenderer executor-owned primary_color pass timing" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "primary_color pass timing and render pass envelope" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiFrameRenderer primary_color texture binding and target-state evidence" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "optional primary_depth depth target-state" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiPostprocessFrameRenderer executor-owned scene-color/scene-depth target preparation plus executor-owned scene/postprocess-chain render pass envelopes, pass-body callback recording, and post-chain/final-state transition adoption" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiDirectionalShadowSmokeFrameRenderer scheduled shadow-depth/scene-color/scene-depth inter-pass plus shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state, executor-owned shadow-depth/scene-receiver/postprocess render pass envelope, and scene-depth/shadow-depth final-state transition adoption" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "RhiViewportSurface viewport_color render_target/copy_source/shader_read color-state transitions through execute_frame_graph_rhi_texture_schedule plus executor-owned viewport.clear render pass envelope" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph RHI Queue Dependency Plan v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "plan_frame_graph_rhi_queue_waits" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "IRhiDevice::wait_for_queue" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph RHI Multi-Queue Executor v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "execute_frame_graph_rhi_multi_queue_schedule" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiMultiQueueExecutionDesc::texture_bindings" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiMultiQueueExecutionDesc::final_states" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiMultiQueueExecutionResult::final_state_barriers_recorded" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "consumer-pass texture barriers recorded before callbacks" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "alias-induced cross-queue waits" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphRhiMultiQueuePackageEvidence" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "aliasing-barrier, submitted-fence" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "final-state transitions recorded on the last scheduled resource pass command list" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "Frame Graph Production Ownership Boundary Selection v1" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "FrameGraphProductionOwnershipPlan" "MK_renderer module purpose"
Assert-ContainsText ([string]$geRendererModule[0].purpose) "plan_frame_graph_production_ownership_boundary" "MK_renderer module purpose"
if ([string]$productionLoop.recommendedNextPlan.id -notin @("general-purpose-game-production-v1", "generated-game-studio-v1", "engine-1-0-gap-matrix-v1", "next-production-gap-selection", "native-win32-editor-shell-v1", "first-party-editor-shell-v1", "first-party-ui-editor-production-stack-v1", "physics-navigation-commercial-coverage-v1", "renderer-backend-parity-metal-apple-evidence-v1", "renderer-postprocess-tone-mapping-evidence-v1", "sandbox-world-network-modding-gate-v1", "sandbox-world-package-validation-performance-budgets-v1", "ai-operable-performance-budget-and-evidence-v1", "performance-baseline-v1", "long-running-performance-readiness-v1-phase-1", "long-running-performance-readiness-v1-phase-2", "long-running-performance-readiness-v1-phase-7", "memory-lifetime-taxonomy-v1", "memory-diagnostics-v1", "frame-thread-scratch-v1", "job-scheduling-evidence-v1", "job-execution-worker-pool-v1", "job-execution-topology-policy-v1", "job-execution-work-stealing-v1", "job-execution-placement-policy-v1", "windows-cpu-set-worker-placement-v1", "windows-cpu-set-smt-worker-placement-v1", "simd-dispatch-policy-and-evidence-v1", "avx2-reviewed-target-execution-v1", "environment-system-v1", "mavg-research-legal-benchmark-baseline-v1", "mavg-runtime-lod-milestone-v1")) { foreach ($needle in @("Frame Graph Remaining Render Pass Envelopes v1", "Frame Graph Primary Pass Target-State Evidence v1", "Frame Graph RHI Queue Dependency Plan v1", "record_frame_graph_rhi_queue_waits", "Frame Graph RHI Multi-Queue Executor v1", "execute_frame_graph_rhi_multi_queue_schedule", "Frame Graph RHI Multi-Queue Texture Barrier Execution v1", "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded", "Runtime Material Factor Frame Graph Command Evidence v1", "RuntimeMaterialGpuBinding", "Frame Graph v1 1.0 Scope Closeout v1 closes frame-graph-v1")) { Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.completedContext) $needle "recommended next plan completed context" } }
if ([string]$productionLoop.recommendedNextPlan.id -eq "general-purpose-game-production-v1") {
    foreach ($needle in @("General Purpose Game Production v1", "addressable-content-streaming-production-v1", "production-authoring-workflows-v1", "production-runtime-ui-workbench-v1")) {
        Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) $needle "recommended next plan production milestone reason"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "generated-game-studio-v1") { Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "EditorAiGeneratedGameStudioV1Model" "recommended next plan generated game studio reason"
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "engine-1-0-gap-matrix-v1") { foreach ($needle in @("Engine 1.0 Gap Matrix v1", "renderer-backend-parity-v1", "strict Vulkan evidence", "Metal remains Apple-host-gated", "unsupportedProductionGaps empty")) { Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) $needle "recommended next plan engine gap matrix reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "next-production-gap-selection") {
    $selectionGateRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
    foreach ($needle in @("First-Party Desktop Platform And SDL3 Removal v1", "MK_platform_win32", "MK_runtime_host_win32_presentation", "MK_audio_wasapi", "Job Execution Placement Policy v1", "job_execution_placement_policy_status=ready", "host-independent CPU placement policy evidence", "Windows CPU Set Worker Placement v1", "windows_cpu_set_worker_placement_status=ready", "windows_cpu_set_worker_placement_native_thread_handles_exposed=0", "selection gate")) { Assert-ContainsText $selectionGateRecommendedText $needle "recommended next plan selection gate reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-research-legal-benchmark-baseline-v1") { $mavgRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("MAVG Phase 0", "research/specification", "official-source checks", "clean-room/legal guardrails", "benchmark methodology", "stale-doc cleanup", "MAVG not implemented", "no SDL3/Dear ImGui", "no public native handles", "no Nanite/UE compatibility", "unsupportedProductionGaps = []")) { Assert-ContainsText $mavgRecommendedText $needle "recommended next plan MAVG Phase 0 selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "physics-navigation-commercial-coverage-v1") {
    $physicsNavigationRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
    foreach ($needle in @("Physics Navigation Commercial Coverage v1", "Jolt/Recast/Detour-class", "adapter_boundary_id", "host_validation_recipe_id", "adapter_lifecycle_reviewed", "unsupportedProductionGaps = []", "native handles hidden", "broad middleware parity fail-closed")) {
        Assert-ContainsText $physicsNavigationRecommendedText $needle "recommended next plan physics/navigation selection reason"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-backend-parity-metal-apple-evidence-v1") {
    $rendererMetalRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
    foreach ($needle in @("Renderer Backend Parity Metal Apple Evidence v1", "renderer-backend-parity-v1", "metal-apple remains host-gated", "shader-toolchain", "mobile-packaging", "ios-simulator-smoke", "Apple/Metal host evidence", "Windows/Vulkan proof must not promote Metal readiness", "no SDL3", "native handles remain hidden", "unsupportedProductionGaps = []")) {
        Assert-ContainsText $rendererMetalRecommendedText $needle "recommended next plan renderer Metal Apple selection reason"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "renderer-postprocess-tone-mapping-evidence-v1") { $toneMappingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Renderer Postprocess Tone Mapping Evidence v1", "renderer-postprocess-v1", "PostprocessToneMappingEvidencePlan", "plan_postprocess_tone_mapping_evidence", "D3D12/Vulkan", "Metal host-gated", "no SDL3", "native handles", "subjective visual quality", "unsupportedProductionGaps = []")) { Assert-ContainsText $toneMappingRecommendedText $needle "recommended next plan renderer postprocess tone mapping selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-network-modding-gate-v1") { $sandboxNetworkModdingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Selected focused child plan", "sandbox-world-specific mutation replication", "reviewed modding policy gates", "unsupportedProductionGaps = []", "Broad online multiplayer", "SDL3", "native handle exposure")) { Assert-ContainsText $sandboxNetworkModdingRecommendedText $needle "recommended next plan sandbox world network/modding selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "sandbox-world-package-validation-performance-budgets-v1") { $sandboxPackageBudgetsRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Selected focused child plan", "sample package smoke flags", "installed validation", "package-visible counters", "--require-sandbox-package-budgets", "sandbox_package_budget_*", "unsupportedProductionGaps = []", "broad renderer quality", "package mutation", "SDL3", "native handle exposure")) { Assert-ContainsText $sandboxPackageBudgetsRecommendedText $needle "recommended next plan sandbox world package validation and performance budget selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "ai-operable-performance-budget-and-evidence-v1") { $performanceBudgetEvidenceRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("performanceBudgets", "budgetRows", "evidenceRows", "validation recipe", "unsupported broad optimization claims", "CPU/GPU/memory optimization")) { Assert-ContainsText $performanceBudgetEvidenceRecommendedText $needle "recommended next plan performance budget evidence selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "performance-baseline-v1") { $performanceBaselineRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("reproducible benchmark scenes/packages", "trace export recipes", "subsystem counters", "p95/p99 frame budget reporting", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $performanceBaselineRecommendedText $needle "recommended next plan performance baseline selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-1") { $longRunRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Long-Running Performance Readiness v1", "sample_2d_desktop_runtime_package", "--require-long-run-performance-readiness", "long_run_readiness_status=ready", "host-2d-long-run-readiness-soak", "Linux affinity", "NUMA", "broad SIMD", "GPU async overlap", "CUDA", "HIP", "SYCL", "unsupportedProductionGaps = []")) { Assert-ContainsText $longRunRecommendedText $needle "recommended next plan long-running performance readiness selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-2") { $cpuProfilingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Intel/AMD CPU Profiling Matrix v1", "cpuProfilingMatrix", "host-cpu-profiling-matrix", "representative Intel/AMD host classes", "before/after trace", "regression budgets", "Linux affinity", "NUMA", "broader SIMD", "PGO/LTO", "data-layout", "host-gated", "unsupportedProductionGaps = []")) { Assert-ContainsText $cpuProfilingRecommendedText $needle "recommended next plan CPU profiling matrix selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "long-running-performance-readiness-v1-phase-7") { $optionalGpuRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Optional GPU Compute Review v1", "optionalGpuComputeReview", "host-optional-gpu-compute-review", "rhi_compute", "offline_tool_acceleration", "cuda_hip_private_adapter_candidate", "sycl_private_adapter_candidate", "non_goal", "data transfer cost", "memory residency", "synchronization", "stream/event usage", "queue/profiler visibility", "dependency burden", "scalar or RHI fallback", "CUDA/HIP/SYCL runtime dependency", "vcpkg.json", "CMake", "default validation", "broad GPU compute", "async overlap", "cross-vendor", "cross-backend", "broad CPU/GPU/memory optimization", "unsupportedProductionGaps = []")) { Assert-ContainsText $optionalGpuRecommendedText $needle "recommended next plan optional GPU compute review selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-lifetime-taxonomy-v1") { $memoryLifetimeRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("memory lifetime taxonomy", "ownership semantics", "raw pointers are non-owning", "std::unique_ptr", "std::span", "allocator/job/NUMA/GPU memory", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $memoryLifetimeRecommendedText $needle "recommended next plan memory lifetime taxonomy selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "memory-diagnostics-v1") { $memoryDiagnosticsRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("memory diagnostics", "memory class counters", "high-water marks", "budget pressure", "stale-generation", "use-after-safe-point", "allocator replacement", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $memoryDiagnosticsRecommendedText $needle "recommended next plan memory diagnostics selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "frame-thread-scratch-v1") { $frameThreadScratchRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("frame temporary", "worker scratch", "first-party frame arenas", "per-worker scratch arenas", "explicit ownership APIs", "high-water marks", "false-sharing diagnostics", "allocator replacement", "all-core CPU scheduling", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $frameThreadScratchRecommendedText $needle "recommended next plan frame/thread scratch selection reason" } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-scheduling-evidence-v1") { $jobSchedulingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("job scheduling", "worker topology", "bounded job queues", "deterministic job/scratch evidence", "queue/steal/wait/merge diagnostics", "work stealing", "processor groups", "package-visible job_scheduling_evidence_* counters", "all-core CPU scheduling", "affinity pinning", "NUMA placement", "SIMD dispatch", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $jobSchedulingRecommendedText $needle "recommended next plan job scheduling evidence selection reason" } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-worker-pool-v1") { $jobExecutionRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Job Execution Worker Pool v1", "persistent worker-thread pool", "explicit worker_count", "bounded worker queues", "std::thread", "JobExecutionStopToken", "execute(batch)", "worker-local ScratchArena", "deterministic publish order", "JobSchedulingExecutionEvidence", "--require-job-execution-foundation", "job_execution_foundation_status=ready", "job_execution_foundation_worker_threads_started=2", "unsupportedProductionGaps = []", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $jobExecutionRecommendedText $needle "recommended next plan job execution worker pool selection reason" } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-topology-policy-v1") { $jobTopologyRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Job Execution Topology Policy v1", "portable MK_core worker-count selection", "JobExecutionTopologyPolicyDesc", "select_job_execution_topology_policy", "observe_job_execution_logical_processor_count", "derived JobExecutionPoolDesc", "fallback/cap/reserve rules", "processor-group and NUMA host-evidence diagnostics", "--require-job-execution-topology-policy", "job_execution_topology_policy_status=ready", "job_execution_topology_policy_ready=1", "selected worker count = 2", "unsupportedProductionGaps = []", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $jobTopologyRecommendedText $needle "recommended next plan job execution topology policy selection reason" } } elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-work-stealing-v1") { $jobWorkStealingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Job Execution Work Stealing v1", "opt-in JobExecutionPool", "work_stealing_enabled", "JobExecutionTopologyPolicyDesc.enable_work_stealing", "steal attempt/success/wait counters", "--require-job-execution-work-stealing", "job_execution_work_stealing_status=ready", "job_execution_work_stealing_applied=1", "deterministic publish order", "unsupportedProductionGaps = []", "affinity", "NUMA", "SMT/hybrid", "SIMD", "GPU async overlap", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $jobWorkStealingRecommendedText $needle "recommended next plan job execution work stealing selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "job-execution-placement-policy-v1") { $jobPlacementRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Job Execution Placement Policy v1", "host-independent CPU placement policy evidence", "topology", "worker-pool", "work-stealing", "affinity", "NUMA", "SMT", "hybrid-core", "Windows CPU Set", "Linux affinity", "SIMD", "GPU async", "CUDA/HIP/SYCL", "broad all-core CPU/GPU/memory optimization")) { Assert-ContainsText $jobPlacementRecommendedText $needle "recommended next plan job execution placement policy selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "windows-cpu-set-worker-placement-v1") { $windowsCpuSetRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Windows CPU Set Worker Placement v1", "Windows CPU Sets", "worker-start placement", "value-only MK_core callback", "MK_platform_win32 adapter", "sample_desktop_runtime_game", "unsupportedProductionGaps = []", "native handles", "Linux affinity", "NUMA allocation execution", "hybrid P-core/E-core", "SMT scheduling", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $windowsCpuSetRecommendedText $needle "recommended next plan Windows CPU Set worker placement selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "windows-cpu-set-smt-worker-placement-v1") { $windowsCpuSetSmtRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Windows CPU Set SMT Worker Placement v1", "avoid_smt_siblings", "CoreIndex", "distinct cores", "SMT sibling", "--require-windows-cpu-set-smt-worker-placement", "windows_cpu_set_smt_worker_placement_status=ready", "smt_policy_applied=1", "unsupportedProductionGaps = []", "native handles", "hybrid P-core/E-core", "Linux affinity", "NUMA allocation execution", "SIMD dispatch", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $windowsCpuSetSmtRecommendedText $needle "recommended next plan Windows CPU Set SMT worker placement selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "avx2-reviewed-target-execution-v1") { $avx2RecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("AVX2 Reviewed Target Execution v1", "target-local AVX2 OBJECT", "mirakana_core_avx2", "simd_dispatch_avx2.cpp", "CpuSimdFeatureSet", "avx2_compile_supported", "avx2_runtime_supported", "auto_select", "sample_desktop_runtime_game --require-simd-dispatch-policy", "unsupportedProductionGaps = []", "global /arch:AVX2", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $avx2RecommendedText $needle "recommended next plan AVX2 reviewed target execution selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "simd-dispatch-policy-and-evidence-v1") { $simdDispatchRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("SIMD Dispatch Policy And Evidence v1", "scalar/SSE2", "CPU SIMD dispatch", "simd_dispatch_policy_*", "Intel and AMD x86/x64", "AVX2 behind compile/runtime gates", "span-based inputs", "raw pointers non-owning", "unsupportedProductionGaps = []", "ARM NEON", "NUMA allocation execution", "GPU async overlap", "CUDA/HIP/SYCL", "broad CPU/GPU/memory optimization")) { Assert-ContainsText $simdDispatchRecommendedText $needle "recommended next plan SIMD dispatch policy selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "environment-system-v1") {
    $environmentRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " ";
    foreach ($needle in @("Environment System v1", "MK_environment", "EnvironmentProfileDesc", "validate_environment_profile", "official docs/Context7", "sky", "sun/moon", "fog", "clouds", "rain/snow/storm", "time-of-day", "quality tiers", "D3D12", "strict Vulkan", "Metal host-gated", "unsupportedProductionGaps = []", "broad environment_ready", "native handles", "Dear ImGui", "SDL3", "OpenEXR/KTX")) { Assert-ContainsText $environmentRecommendedText $needle "recommended next plan environment system selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-ui-editor-production-stack-v1") {
    $firstPartyUiEditorRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "
    foreach ($needle in @("First-Party UI Editor Production Stack v1", "MK_editor", "MK_editor_core", "desktop-editor", "mirakana::ui", "MK_ui_renderer", "dock graph", "rich text", "DirectWrite", "selected text atlas handoff evidence", "Text Services Framework", "UI Automation", "D3D12 viewport/material texture display", "AI-operable", "compatibility shims", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) {
        Assert-ContainsText $firstPartyUiEditorRecommendedText $needle "recommended next plan first-party UI editor production selection reason"
    }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-editor-shell-v1") { $firstPartyEditorRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("First-Party Editor Shell v1", "MK_editor", "MK_editor_core", "first-party retained", "desktop-editor", "mirakana::ui", "MK_ui_renderer", "EditorAiOperationSnapshot", "EditorAiCommandCatalog", "EditorAiCommandRequest", "EditorAiCommandDryRunResult", "EditorAiCommandApplyResult", "dock graph", "rich-text", "DirectWrite", "Text Services Framework", "UI Automation", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) { Assert-ContainsText $firstPartyEditorRecommendedText $needle "recommended next plan first-party editor shell selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "native-win32-editor-shell-v1") { $nativeEditorRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("Native Win32 Editor Shell v1", "MK_editor", "Dear ImGui", "Direct3D 12", "desktop-gui", "PR #316", "PR #318", "PR #320", "editor_shell_panels=11", "editor_shell_viewport_status=diagnostic_only", "unsupportedProductionGaps = []", "SDL3", "native handles", "editor/developer-shell")) { Assert-ContainsText $nativeEditorRecommendedText $needle "recommended next plan native editor shell selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "first-party-editor-shell-v1") { $firstPartyEditorRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("First-Party Editor Shell v1", "first-party retained MK_editor shell", "desktop-editor", "dock graph", "rich-text rows", "EditorAiOperationSnapshot", "EditorAiCommandCatalog", "EditorAiCommandRequest", "EditorAiCommandDryRunResult", "EditorAiCommandApplyResult", "adapter-boundary diagnostics", "DirectWrite", "Text Services Framework", "UI Automation", "unsupportedProductionGaps = []", "SDL3", "native handles", "Dear ImGui")) { Assert-ContainsText $firstPartyEditorRecommendedText $needle "recommended next plan first-party editor shell selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-asset-graph-v1") { $mavgRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("MAVG Asset Graph v1", "deterministic MK_assets MAVG cluster graph validation", "MK_tools cook/package planning", "renderer execution", "streaming/residency", "Nanite compatibility/equivalence/superiority", "unsupportedProductionGaps = []")) { Assert-ContainsText $mavgRecommendedText $needle "recommended next plan MAVG asset graph selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-runtime-lod-milestone-v1") { $mavgRuntimeRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("MAVG Runtime LOD Milestone v1", "MAVG Asset Graph v1", "MAVG Phase 0 completed", "hierarchy/error/fallback", "MavgClusterGraphCluster", "resident_fallback_cluster_index", "geometric_error", "first_index", "index_count", "vertex_base", "GameEngine.MavgClusterGraph.v1", "MavgClusterCookVertex", "GameEngine.MavgClusterPayload.v1", "vertex.data_hex", "index.data_hex", "per-material root/leaf", "mavg_lod_selection.hpp", "MavgLodViewDesc", "MavgLodResidentPageSet", "MavgLodSelectionResult", "CPU LOD selection", "select_mavg_lod_clusters", "MK_mavg_lod_selection_tests", "mavg_lod_residency.hpp", "RuntimeMavgLodResidencyDesc", "RuntimeMavgLodResidencyResult", "build_runtime_mavg_lod_residency", "MK_runtime_mavg_lod_residency_tests", "range-aware conventional indexed draws", "mavg_scene_lod.hpp", "MavgSceneLodSubmitDesc", "MavgSceneLodSubmitResult", "plan_mavg_scene_lod_mesh_commands", "MK_scene_renderer_mavg_lod_tests", "GPU culling", "indirect draw execution", "mesh shaders", "Nanite compatibility/equivalence/superiority", "unsupportedProductionGaps = []")) { Assert-ContainsText $mavgRuntimeRecommendedText $needle "recommended next plan MAVG runtime LOD selection reason" }
} elseif ([string]$productionLoop.recommendedNextPlan.id -eq "mavg-gpu-culling-indirect-v1") { $mavgGpuCullingRecommendedText = (([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence), ([string]$productionLoop.recommendedNextPlan.completedContext), ([string]$productionLoop.recommendedNextPlan.reason)) -join " "; foreach ($needle in @("MAVG GPU Culling Indirect", "MAVG Runtime LOD Milestone v1", "mavg_gpu_culling.hpp", "MavgGpuCullingClusterBoundsRow", "MavgGpuCullingIndirectCommand", "MavgGpuCullingIndirectCommandLayout", "MavgGpuCullingSyncRequirement", "MavgGpuCullingIndirectPlan", "plan_mavg_gpu_culling_indirect_commands", "packed indexed indirect command", "visibility-cull counters", "20-byte five-field argument layout", "4-byte count-buffer", "D3D12/Vulkan compute-write-to-indirect-read synchronization", "actual GPU dispatch", "ExecuteIndirect", "Vulkan indirect execution", "mesh shaders", "Nanite equivalence/superiority", "unsupportedProductionGaps = []")) { Assert-ContainsText $mavgGpuCullingRecommendedText $needle "recommended next plan MAVG GPU culling indirect selection reason" }
} else {
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "Frame Graph v1" "recommended next plan reason"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "upload-staging-v1" "recommended next plan reason"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "scene-component-prefab-schema-v2" "recommended next plan reason"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "2d-playable-vertical-slice" "recommended next plan reason"
    Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.reason) "3d-playable-vertical-slice" "recommended next plan reason"
}
foreach ($needle in @("RHI Depth Attachment Contract v0", "GPU Memory Policy v1", "GpuMemoryPolicyPlan", "GpuMemoryResidencyClass", "plan_gpu_memory_policy", "Debug Profiling Policy v1", "DebugProfilingPolicyPlan", "plan_debug_profiling_policy", "Scene Scale Policy v1", "SceneScalePolicyPlan", "SceneScaleBatchingMode", "plan_scene_scale_policy", "Postprocess Chain Policy v1", "PostprocessChainPolicyPlan", "plan_postprocess_chain_policy", "Postprocess Tone Mapping Evidence v1", "PostprocessToneMappingEvidencePlan", "plan_postprocess_tone_mapping_evidence", "Lighting Shadow Policy v1", "LightingShadowPolicyPlan", "plan_lighting_shadow_policy", "Stable Directional Light-Space Policy v0", "DirectionalShadowLightSpacePlan")) {
    Assert-ContainsText ([string]$geRendererModule[0].purpose) $needle "MK_renderer module purpose"
}
foreach ($needle in @("environment-gated height-fog runtime readback proof", "MK_VULKAN_TEST_HEIGHT_FOG", "VK_LAYER_KHRONOS_validation")) { Assert-ContainsText ([string]$geRhiModule[0].purpose) $needle "MK_rhi module purpose" }
$geRendererRecentEvidenceText = @($geRendererModule[0].recentEvidence | ForEach-Object { [string]$_ }) -join " "
foreach ($needle in @("Environment Volumetric Fog Policy v1", "volumetric_fog_constants_byte_size", "pack_volumetric_fog_constants", "froxel output storage-buffer binding 13", "RWByteAddressBuffer froxel proof output", "selected D3D12 WARP-safe compute readback proof", "Vulkan runtime/package proof", "Metal readiness")) { Assert-ContainsText $geRendererRecentEvidenceText $needle "MK_renderer recentEvidence volumetric fog evidence" }
if (@($geRendererModule[0].publicHeaders) -notcontains "engine/renderer/include/mirakana/renderer/volumetric_fog_policy.hpp") { Write-Error "engine/agent/manifest.json MK_renderer publicHeaders must include volumetric_fog_policy.hpp" }
foreach ($volumetricFogCheck in @(
        @("engine/renderer/include/mirakana/renderer/volumetric_fog_policy.hpp", @("volumetric_fog_froxel_output_buffer_binding", "return 13", "volumetric_fog_constants_byte_size", "return 256", "pack_volumetric_fog_constants")),
        @("shaders/environment/volumetric_fog.hlsl", @("RWByteAddressBuffer volumetric_fog_output_buffer : register(u13)", "SampleLevel(scene_depth_sampler", "volumetric_fog_output_buffer.Store", "SV_DispatchThreadID")),
        @("tests/unit/d3d12_rhi_tests.cpp", @("dispatches volumetric fog compute from scene depth readback", "compile_volumetric_fog_compute_shader", "volumetric_fog_froxel_output_buffer_binding", "left_far > left_near", "right_far > right_near"))
    )) {
    $volumetricFogCheckText = Get-AgentSurfaceText ([string]$volumetricFogCheck[0])
    foreach ($needle in @($volumetricFogCheck[1])) { Assert-ContainsText $volumetricFogCheckText $needle ([string]$volumetricFogCheck[0]) }
}
if ([string]$productionLoop.recommendedNextPlan.id -eq "environment-system-v1") {
    foreach ($needle in @("PR19 adds selected D3D12 volumetric-fog compute execution/readback proof", "volumetric_fog_constants_byte_size", "pack_volumetric_fog_constants", "froxel output storage-buffer binding 13", "volumetric-fog Vulkan/Metal execution proof", "volumetric-fog package readiness", "unsupportedProductionGaps = []")) { Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "environment-system-v1 latest closeout evidence" }
}
foreach ($rendererPolicyHeader in @("postprocess_policy.hpp", "gpu_memory_policy.hpp", "debug_profiling_policy.hpp", "scene_scale_policy.hpp", "tile_chunk_renderer.hpp")) { if (@($geRendererModule[0].publicHeaders) -notcontains "engine/renderer/include/mirakana/renderer/$rendererPolicyHeader") { Write-Error "engine/agent/manifest.json MK_renderer publicHeaders must include $rendererPolicyHeader" } }
foreach ($tileChunkRendererCheck in @(
        @("engine/renderer/include/mirakana/renderer/tile_chunk_renderer.hpp", @("TileChunkCellRow", "TileChunkDirtyRegion", "TileChunkLightCellRow", "TileChunkRendererDesc", "TileChunkRendererPlan", "TileChunkSpriteRow", "TileChunkDrawRow", "TileChunkDirtyRebuildRow", "TileChunkLightmapRow", "plan_tile_chunk_renderer")),
        @("engine/renderer/src/tile_chunk_renderer.cpp", @("unsupported_native_texture_ownership", "unsupported_backend_specific_submission", "plan_sprite_batches", "dirty_rebuild_rows", "lightmap_rows")),
        @("tests/unit/renderer_tile_chunk_renderer_tests.cpp", @("deterministic opaque transparent material batches", "budgets dirty region rebuild rows", "renderer neutral lightmap rows", "backend ownership claims"))
    )) {
    $tileChunkRendererCheckText = Get-AgentSurfaceText ([string]$tileChunkRendererCheck[0])
    foreach ($needle in @($tileChunkRendererCheck[1])) { Assert-ContainsText $tileChunkRendererCheckText $needle ([string]$tileChunkRendererCheck[0]) }
}
foreach ($needle in @("Generic 2D Sandbox Production Tile Renderer v1", "plan_tile_chunk_renderer", "--require-production-tile-renderer", "zero backend submission/native texture ownership invocation")) { Assert-ContainsText $geRendererRecentEvidenceText $needle "MK_renderer module recentEvidence"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentProductionTileRenderer) $needle "production tile renderer game guidance" }
foreach ($productionTileRendererGuidance in @("docs/current-capabilities.md", "docs/ai-game-development.md", "docs/roadmap.md", "docs/testing.md", "docs/superpowers/plans/README.md", "docs/superpowers/plans/2026-05-27-generic-2d-sandbox-production-lane-v1.md", ".agents/skills/gameengine-game-development/SKILL.md", ".claude/skills/gameengine-game-development/SKILL.md", "games/sample_2d_desktop_runtime_package/README.md", "games/sample_2d_desktop_runtime_package/game.agent.json")) { foreach ($needle in @("plan_tile_chunk_renderer", "--require-production-tile-renderer", "native texture ownership")) { Assert-ContainsText (Get-AgentSurfaceText $productionTileRendererGuidance) $needle $productionTileRendererGuidance } }
foreach ($needle in @("plan_scene_lighting_shadow_policy", "build_scene_directional_shadow_light_space_plan", "sample_and_apply_runtime_scene_render_animation_float_clip", "advance_runtime_sprite_flipbook", "sample_runtime_morph_mesh_cpu_animation_float_clip")) {
    Assert-ContainsText ([string]$geSceneRendererModule[0].purpose) $needle "MK_scene_renderer module purpose"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRhi) "RHI Depth Attachment Contract v0" "RHI game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRhi) "RenderPassDepthAttachment" "RHI game guidance"
foreach ($depthGuidance in @(
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md"
)) {
    $depthText = Get-AgentSurfaceText $depthGuidance
    Assert-ContainsText $depthText "RHI Depth Attachment Contract v0" $depthGuidance
}
foreach ($sampledDepthGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md"
)) {
    $sampledDepthText = Get-AgentSurfaceText $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_VERTEX_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV" $sampledDepthGuidance
    Assert-ContainsText $sampledDepthText "sampled-depth readback" $sampledDepthGuidance
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_SAMPLE_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_DEPTH_SAMPLE_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "sampled-depth readback" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_COMPUTE_MORPH_RENDER_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "create_runtime_compute_pipeline" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "record_runtime_compute_dispatch" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkanRuntimeOwners) "VulkanRuntimeComputePipeline" "Vulkan runtime owner guidance"
$vulkanBackendSource = Get-AgentSurfaceText "engine/rhi/vulkan/src/vulkan_backend.cpp"
Assert-ContainsText $vulkanBackendSource "refresh_surface_probe_queue_family_snapshots" "Vulkan surface support implementation"
Assert-ContainsText $vulkanBackendSource "same native instance handles" "Vulkan surface support implementation"
$frameGraphHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/frame_graph.hpp"
$frameGraphSource = Get-AgentSurfaceText "engine/renderer/src/frame_graph.cpp"
$frameGraphRhiHeader = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp"
$frameGraphRhiSource = Get-AgentSurfaceText "engine/renderer/src/frame_graph_rhi.cpp"
$rhiFrameRendererSource = Get-AgentSurfaceText "engine/renderer/src/rhi_frame_renderer.cpp"
$rhiPostprocessSource = Get-AgentSurfaceText "engine/renderer/src/rhi_postprocess_frame_renderer.cpp"
$rhiDirectionalShadowSource = Get-AgentSurfaceText "engine/renderer/src/rhi_directional_shadow_smoke_frame_renderer.cpp"
$rendererHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/renderer.hpp"
$spriteBatchHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/sprite_batch.hpp"
$rhiViewportSurfaceSource = Get-AgentSurfaceText "engine/renderer/src/rhi_viewport_surface.cpp"
Assert-ContainsText $frameGraphHeader "FrameGraphProductionOwnershipCapability" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphHeader "vulkan_memory_aliasing" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphHeader "metal_memory_aliasing" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphHeader "FrameGraphProductionOwnershipPlan" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphHeader "plan_frame_graph_production_ownership_boundary" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphSource "frame graph production ownership boundary request disagrees with supported boundary" "Frame graph production ownership boundary fail-closed diagnostics"
Assert-DoesNotContainText $frameGraphHeader "vulkan_metal_memory_aliasing" "Frame graph production ownership boundary public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTexturePassTargetAccess" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "build_frame_graph_texture_pass_target_accesses" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "std::span<const FrameGraphTexturePassTargetAccess> pass_target_accesses" "Frame graph RHI pass target access public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTransientTextureLeaseBindingResult" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "acquire_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "release_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "rhi::TransientTextureAliasGroup transient_alias_group" "Frame graph transient texture lease binding public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTextureAliasingBarrier" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphTextureAliasingBarrierRecordResult" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $frameGraphRhiHeader "record_frame_graph_texture_aliasing_barriers" "Frame graph texture aliasing barrier command public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphRhiPassCommandBinding" "Frame graph RHI multi-queue executor public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphRhiMultiQueueExecutionDesc" "Frame graph RHI multi-queue executor public API"
Assert-ContainsText $frameGraphRhiHeader "FrameGraphRhiMultiQueueExecutionResult" "Frame graph RHI multi-queue executor public API"
Assert-ContainsText $frameGraphRhiHeader "std::span<FrameGraphTextureBinding> texture_bindings" "Frame graph RHI multi-queue texture barrier public API"
Assert-ContainsText $frameGraphRhiHeader "std::span<const FrameGraphTextureFinalState> final_states" "Frame graph RHI multi-queue final-state public API"
Assert-ContainsText $frameGraphRhiHeader "std::span<const FrameGraphTransientTextureLifetime> transient_texture_lifetimes" "Frame graph RHI multi-queue automatic aliasing barrier public API"
Assert-ContainsText $frameGraphRhiHeader "std::size_t barriers_recorded" "Frame graph RHI multi-queue texture barrier public API"
Assert-ContainsText $frameGraphRhiHeader "std::size_t aliasing_barriers_recorded" "Frame graph RHI multi-queue automatic aliasing barrier public API"
Assert-ContainsText $frameGraphRhiHeader "std::size_t final_state_barriers_recorded" "Frame graph RHI multi-queue final-state public API"
Assert-ContainsText $frameGraphRhiHeader "execute_frame_graph_rhi_multi_queue_schedule" "Frame graph RHI multi-queue executor public API"
Assert-ContainsText $rhiPublicHeaderText "texture_aliasing_barriers" "RHI texture aliasing barrier command public API"
Assert-ContainsText $rhiPublicHeaderText "texture_aliasing_barrier(TextureHandle before, TextureHandle after)" "RHI texture aliasing barrier command public API"
Assert-ContainsText $rhiPublicHeaderText "TransientTextureAliasGroup" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "std::vector<TextureHandle> textures" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "acquire_transient_texture_alias_group(const TextureDesc& desc" "RHI transient texture alias-group public API"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_heap_allocations" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_placed_allocations" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $rhiPublicHeaderText "transient_texture_placed_resources_alive" "RHI D3D12 placed transient texture lease evidence"
Assert-ContainsText $nullRhiSourceText "void texture_aliasing_barrier(TextureHandle before, TextureHandle after) override" "NullRHI texture aliasing barrier command"
Assert-ContainsText $nullRhiSourceText "NullRhiDevice::acquire_transient_texture_alias_group" "NullRHI transient texture alias-group lease"
$d3d12RhiHeaderText = Get-AgentSurfaceText "engine/rhi/d3d12/include/mirakana/rhi/d3d12/d3d12_backend.hpp"
Assert-ContainsText $d3d12RhiHeaderText "null_resource_aliasing_barriers" "D3D12 texture aliasing barrier command evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_texture_heaps_created" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_texture_alias_groups_created" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_textures_created" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resources_alive" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resource_activation_barriers" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "placed_resource_aliasing_barriers" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "create_placed_texture" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiHeaderText "create_placed_texture_alias_group" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiHeaderText "activate_placed_texture" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "backend-private" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "null-resource aliasing barrier" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "D3D12_RESOURCE_BARRIER_TYPE_ALIASING" "D3D12 texture aliasing barrier command"
Assert-ContainsText $d3d12RhiSourceText "CreateHeap" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "CreatePlacedResource" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "GetResourceAllocationInfo" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "D3D12_HEAP_FLAG_ALLOW_ONLY_RT_DS_TEXTURES" "D3D12 placed transient texture lease evidence"
Assert-ContainsText $d3d12RhiSourceText "pResourceAfter = texture_resource" "D3D12 placed transient texture activation evidence"
Assert-ContainsText $d3d12RhiSourceText "resource_alias_group_ids" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiSourceText "resources_share_placed_alias_group" "D3D12 placed alias group evidence"
Assert-ContainsText $d3d12RhiSourceText "create_placed_texture_alias_group(desc, texture_count)" "D3D12 transient texture alias-group lease"
Assert-ContainsText $d3d12RhiSourceText "pResourceBefore = before_resource" "D3D12 placed alias group non-null barrier evidence"
Assert-ContainsText $d3d12RhiSourceText "pResourceAfter = after_resource" "D3D12 placed alias group non-null barrier evidence"
Assert-ContainsText $d3d12RhiSourceText "placed_resource_state_updates" "D3D12 placed alias group submit-time state evidence"
Assert-ContainsText $d3d12RhiSourceText "buffer_last_used_fences_" "D3D12 resource-backed transient queue fence release"
Assert-ContainsText $d3d12RhiSourceText "texture_last_used_fences_" "D3D12 resource-backed transient queue fence release"
Assert-ContainsText $d3d12RhiSourceText "mark_submitted_resource_usage" "D3D12 resource-backed transient queue fence release"
Assert-ContainsText $d3d12RhiSourceText "observed_buffers" "D3D12 resource-backed transient queue fence release"
Assert-ContainsText $d3d12RhiSourceText "descriptor_set_resource_refs_" "D3D12 descriptor-set resource queue fence release"
Assert-DoesNotContainText $d3d12RhiSourceText "const auto release_fence = lifetime_fence(context_->last_submitted_fence())" "D3D12 transient releases must not use global last-submitted fence"
Assert-ContainsText $vulkanRhiSourceText "record_runtime_texture_aliasing_barrier" "Vulkan texture aliasing barrier command"
Assert-ContainsText $vulkanRhiSourceText "vulkan_image_create_alias_bit" "Vulkan transient texture alias memory allocation"
Assert-ContainsText $vulkanRhiSourceText "create_transient_texture_alias_images" "Vulkan transient texture alias memory allocation"
Assert-ContainsText $vulkanRhiSourceText "VulkanRuntimeTexture::Impl::MemoryAllocation" "Vulkan transient texture alias memory allocation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group acquisition failed" "Frame graph transient texture lease binding failure cleanup"
Assert-ContainsText $frameGraphRhiSource "device.acquire_transient_texture_alias_group(group.desc, group.resources.size())" "Frame graph transient texture distinct alias-group lease binding"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group has no resources" "Frame graph transient texture lease binding malformed-plan validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group resource name is empty" "Frame graph transient texture lease binding malformed-plan validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group returned an invalid texture count" "Frame graph transient texture lease binding backend validation"
Assert-ContainsText $frameGraphRhiSource "frame graph transient texture alias group returned duplicate texture handles" "Frame graph transient texture lease binding backend validation"
Assert-ContainsText $frameGraphRhiSource "release_frame_graph_transient_texture_lease_bindings" "Frame graph transient texture lease binding failure cleanup"
Assert-ContainsText $frameGraphRhiSource "propagate_shared_simulated_texture_state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "propagate_shared_texture_binding_state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "frame graph texture bindings sharing a handle disagree on current state" "Frame graph shared TextureHandle state handoff"
Assert-ContainsText $frameGraphRhiSource "record_frame_graph_texture_aliasing_barriers" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "texture_aliasing_barrier" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "frame graph texture aliasing barrier requires distinct texture handles" "Frame graph texture aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "or wildcard endpoints" "Frame graph public null aliasing barrier command"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target state has no declared writer access" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target state disagrees with writer access" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "frame graph texture pass target access is declared more than once" "Frame graph RHI pass target access validation"
Assert-ContainsText $frameGraphRhiSource "FrameGraphRhiRenderPassDesc" "Frame graph render pass envelope"
Assert-ContainsText $frameGraphRhiSource "begin_planned_render_pass" "Frame graph render pass envelope"
Assert-ContainsText $frameGraphRhiSource "attachment references an unknown resource" "Frame graph render pass envelope validation"
Assert-ContainsText $frameGraphRhiSource "execute_frame_graph_rhi_multi_queue_schedule" "Frame graph RHI multi-queue executor"
Assert-ContainsText $frameGraphRhiSource "record_frame_graph_rhi_queue_waits(*desc.device, waits->second, result.submitted_pass_fences)" "Frame graph RHI multi-queue executor"
Assert-ContainsText $frameGraphRhiSource "desc.texture_bindings, desc.transient_texture_lifetimes" "Frame graph RHI multi-queue automatic aliasing barrier executor"
Assert-ContainsText $frameGraphRhiSource "validate_transient_render_pass_content_initialization(result, scheduled_pass_order, desc.render_passes" "Frame graph RHI multi-queue transient content initialization validation"
Assert-ContainsText $frameGraphRhiSource "record_planned_texture_aliasing_barrier(result, *commands, desc.texture_bindings" "Frame graph RHI multi-queue automatic aliasing barrier executor"
Assert-ContainsText $frameGraphRhiSource "texture_barriers_by_pass" "Frame graph RHI multi-queue texture barrier executor"
Assert-ContainsText $frameGraphRhiSource "record_planned_texture_barrier(result, *commands, desc.texture_bindings" "Frame graph RHI multi-queue texture barrier executor"
Assert-ContainsText $frameGraphRhiSource "frame graph rhi pass command callback failed" "Frame graph RHI multi-queue executor diagnostics"
Assert-ContainsText $rhiFrameRendererSource "PrimaryColorFrameGraphExecutionPlan" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource "build_frame_graph_texture_pass_target_accesses" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource ".pass_target_states = pass_target_states" "RhiFrameRenderer primary target-state evidence"
Assert-ContainsText $rhiFrameRendererSource "primary_depth" "RhiFrameRenderer primary depth target-state evidence"
$rendererRhiTests = Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp"
Assert-ContainsText $rendererRhiTests "frame graph production ownership boundary selects reviewed executor rows" "Frame graph production ownership boundary tests"
Assert-ContainsText $rendererRhiTests "frame graph production ownership boundary rejects broadened ownership claims" "Frame graph production ownership boundary tests"
Assert-ContainsText $rendererRhiTests "frame graph production ownership boundary rejects invalid candidate rows" "Frame graph production ownership boundary tests"
Assert-ContainsText $rendererRhiTests "frame graph v1 texture barrier recording propagates shared texture handle state" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph v1 texture barrier recording rejects conflicting shared texture handle states" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution hands off shared texture handle state between aliases" "Frame graph shared TextureHandle state handoff tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture aliasing barrier recording maps resource names to texture handles" "Frame graph texture aliasing barrier command tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture aliasing barrier recording maps empty resource names to wildcards" "Frame graph public null aliasing barrier tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture aliasing barrier recording rejects missing resources and shared handles" "Frame graph texture aliasing barrier command tests"
Assert-ContainsText $rendererRhiTests "rhi frame renderer carries primary target state across texture frames" "RhiFrameRenderer primary target-state evidence tests"
Assert-ContainsText $rendererRhiTests "rhi frame renderer reports primary target state failures before pass body" "RhiFrameRenderer primary target-state evidence tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi transient texture lease binding acquires distinct handles per alias group resource" "Frame graph transient texture distinct alias-group lease tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi transient texture lease binding rejects duplicate backend alias handles" "Frame graph transient texture distinct alias-group lease tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution wraps render pass envelopes around callbacks" "Frame graph render pass envelope tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi texture schedule execution rejects invalid render pass envelopes before callbacks" "Frame graph render pass envelope tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor submits declared pass queues and waits for producer fences" "Frame graph RHI multi-queue executor tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor records texture barriers before consumer callbacks" "Frame graph RHI multi-queue texture barrier tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor inserts aliasing barriers before later alias callbacks" "Frame graph RHI multi-queue automatic aliasing barrier tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor rejects transient alias first render pass load" "Frame graph RHI multi-queue transient content initialization tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor validates texture barriers before command recording" "Frame graph RHI multi-queue texture barrier tests"
Assert-ContainsText $rendererRhiTests "frame graph rhi multi queue executor preserves submitted producer evidence on callback failure" "Frame graph RHI multi-queue executor tests"
Assert-ContainsText $rendererRhiTests "framegraph_render_passes_recorded == 1" "Frame graph render pass stats evidence tests"
Assert-ContainsText $rendererRhiTests "framegraph_render_passes_recorded == 2" "Frame graph render pass stats evidence tests"
Assert-ContainsText $rendererRhiTests "framegraph_render_passes_recorded == 3" "Frame graph render pass stats evidence tests"
Assert-ContainsText $rhiTestsText "null rhi records texture aliasing barriers without changing texture state" "NullRHI texture aliasing barrier command tests"
Assert-ContainsText $rhiTestsText "null rhi records wildcard texture aliasing barriers" "NullRHI public null aliasing barrier tests"
Assert-ContainsText $rhiTestsText "null rhi transient texture alias group returns distinct handles under one lease" "NullRHI transient texture alias-group lease tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device records texture aliasing barrier commands" "D3D12 texture aliasing barrier command tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device records public wildcard texture aliasing barrier commands" "D3D12 public null aliasing barrier tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context applies public null placed texture aliasing state updates" "D3D12 public null placed aliasing state tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device transient texture alias group returns distinct placed handles" "D3D12 transient texture alias-group lease tests"
Assert-ContainsText $d3d12RhiTestsText "null_resource_aliasing_barriers" "D3D12 texture aliasing barrier command evidence tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context creates placed transient texture resources" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context keeps unrelated placed texture aliasing barriers conservative" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 device context records non null placed resource aliasing barriers" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "placed_resource_activation_barriers" "D3D12 placed transient texture activation tests"
Assert-ContainsText $d3d12RhiTestsText "placed_texture_alias_groups_created" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "placed_resource_aliasing_barriers" "D3D12 placed alias group tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_heap_allocations" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_placed_allocations" "D3D12 placed transient texture lease tests"
Assert-ContainsText $d3d12RhiTestsText "transient_texture_placed_resources_alive" "D3D12 placed transient texture release tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device reports completed telemetry for waited queue fence" "D3D12 per-queue completed telemetry tests"
Assert-ContainsText $d3d12RhiTestsText "d3d12 rhi device releases transient resources at their last used queue fences" "D3D12 resource-backed transient queue fence release tests"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device bridge records texture aliasing barrier" "Vulkan texture aliasing barrier command tests"
Assert-ContainsText $backendScaffoldTestsText "vulkan rhi device transient texture alias group shares one memory allocation" "Vulkan transient texture alias memory allocation tests"
Assert-ContainsText $rhiFrameRendererSource "execute_frame_graph_rhi_texture_schedule" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "primary_color" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "framegraph_passes_executed" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rendererHeaderText "framegraph_render_passes_recorded" "RendererStats render pass envelope evidence"
foreach ($needle in @(
    "SpriteBatchPlanDesc",
    "SpriteBatchPlanOptions",
    "atlas_backed_batch_count",
    "repeated_atlas_batch_count",
    "repeated_atlas_sprite_count",
    "unsupported_reordering_policy",
    "untextured_sprite_disallowed"
)) {
    Assert-ContainsText $spriteBatchHeaderText $needle "Sprite Batching Renderer v1 Phase 1 public API"
}
Assert-ContainsText $rhiFrameRendererSource "framegraph_render_passes_recorded += frame_graph_execution.render_passes_recorded" "RHI frame renderer render pass stats evidence"
Assert-ContainsText $rhiFrameRendererSource "record_queued_mesh_command(draw.mesh, recorded_primary_stats)" "RHI frame renderer primary pass ownership"
Assert-ContainsText $rhiFrameRendererSource "FrameGraphRhiRenderPassDesc" "RHI frame renderer primary render pass envelope"
Assert-ContainsText $rhiFrameRendererSource ".render_passes = render_passes" "RHI frame renderer primary render pass envelope"
Assert-DoesNotContainText $rhiFrameRendererSource "commands_->begin_render_pass" "RHI frame renderer primary render pass envelope"
Assert-DoesNotContainText $rhiFrameRendererSource "commands_->end_render_pass" "RHI frame renderer primary render pass envelope"
$beginFrameFunctionMatch = [regex]::Match(
    $rhiFrameRendererSource,
    '(?s)void RhiFrameRenderer::begin_frame\(\)(?<body>.*?)\r?\nvoid RhiFrameRenderer::draw_sprite'
)
if (-not $beginFrameFunctionMatch.Success) {
    Write-Error "RhiFrameRenderer::begin_frame body could not be located for primary pass ownership checks"
}
$beginFrameFunctionBody = $beginFrameFunctionMatch.Groups["body"].Value
Assert-DoesNotContainText $beginFrameFunctionBody "begin_render_pass" "RHI frame renderer primary pass ownership"
Assert-DoesNotContainText $rhiPostprocessSource "void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {
    require_active_frame();
    commands_->draw(3, 1);" "RHI postprocess sprite submission"
Assert-DoesNotContainText $rhiPostprocessSource "void RhiPostprocessFrameRenderer::draw_sprite(const SpriteCommand&) {`n    require_active_frame();`n    commands_->draw(3, 1);" "RHI postprocess sprite submission"
Assert-ContainsText $rhiPostprocessSource "execute_frame_graph_rhi_texture_schedule" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiPostprocessSource "build_frame_graph_texture_pass_target_accesses" "RHI postprocess frame graph pass target access execution"
Assert-ContainsText $rhiPostprocessSource ".pass_target_accesses = postprocess_frame_graph_target_accesses_" "RHI postprocess frame graph pass target access execution"
Assert-ContainsText $rhiPostprocessSource "pending_meshes_" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource "record_scene_pass_body" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource ".pass_name = `"scene_color`"" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource ".resource = `"scene_depth`"" "RHI postprocess scene pass ownership"
Assert-ContainsText $rhiPostprocessSource "FrameGraphRhiRenderPassDesc" "RHI postprocess frame graph render pass envelope"
Assert-ContainsText $rhiPostprocessSource ".render_passes = render_passes" "RHI postprocess frame graph render pass envelope"
Assert-DoesNotContainText $rhiPostprocessSource "commands_->begin_render_pass" "RHI postprocess frame graph render pass envelope"
Assert-DoesNotContainText $rhiPostprocessSource "commands_->end_render_pass" "RHI postprocess frame graph render pass envelope"
Assert-ContainsText $rhiPostprocessSource "FrameGraphTexturePassTargetState" "RHI postprocess frame graph pass target-state execution"
Assert-ContainsText $rhiPostprocessSource ".pass_target_states = pass_target_states" "RHI postprocess frame graph pass target-state execution"
Assert-ContainsText $rhiPostprocessSource "FrameGraphTextureFinalState" "RHI postprocess frame graph final-state execution"
Assert-ContainsText $rhiPostprocessSource ".final_states = final_states" "RHI postprocess frame graph final-state execution"
Assert-ContainsText $rhiPostprocessSource "frame_graph_execution.barriers_recorded" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiPostprocessSource "frame_graph_execution.pass_callbacks_invoked" "RHI postprocess frame graph RHI execution"
Assert-ContainsText $rhiPostprocessSource "framegraph_render_passes_recorded += frame_graph_execution.render_passes_recorded" "RHI postprocess render pass stats evidence"
Assert-ContainsText $rhiDirectionalShadowSource "execute_frame_graph_rhi_texture_schedule" "RHI directional shadow frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "build_frame_graph_texture_pass_target_accesses" "RHI directional shadow frame graph pass target access execution"
Assert-ContainsText $rhiDirectionalShadowSource ".pass_target_accesses = shadow_smoke_frame_graph_target_accesses_" "RHI directional shadow frame graph pass target access execution"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphRhiRenderPassDesc" "RHI directional shadow frame graph render pass envelope"
Assert-ContainsText $rhiDirectionalShadowSource ".render_passes = render_passes" "RHI directional shadow frame graph render pass envelope"
Assert-DoesNotContainText $rhiDirectionalShadowSource "commands_->begin_render_pass" "RHI directional shadow frame graph render pass envelope"
Assert-DoesNotContainText $rhiDirectionalShadowSource "commands_->end_render_pass" "RHI directional shadow frame graph render pass envelope"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphTexturePassTargetState" "RHI directional shadow frame graph pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".pass_target_states = pass_target_states" "RHI directional shadow frame graph pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".resource = `"shadow_color`"" "RHI directional shadow shadow_color pass target-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".access = FrameGraphAccess::color_attachment_write" "RHI directional shadow shadow_color writer access"
if ($rhiDirectionalShadowSource -notmatch '(?s)FrameGraphResourceAccess\{\s*\.resource\s*=\s*"shadow_color",\s*\.access\s*=\s*FrameGraphAccess::color_attachment_write\s*\}') {
    Write-Error "RHI directional shadow shadow_color writer access must be a declared color_attachment_write row"
}
if ($rhiDirectionalShadowSource -notmatch '(?s)FrameGraphTexturePassTargetState\{\s*\.pass_name\s*=\s*"shadow\.directional\.depth",\s*\.resource\s*=\s*"shadow_color",\s*\.state\s*=\s*rhi::ResourceState::render_target,?\s*\}') {
    Write-Error "RHI directional shadow shadow_color target-state row must prepare shadow.directional.depth as render_target"
}
Assert-DoesNotContainText $rhiDirectionalShadowSource "transition_texture(shadow_color_texture_" "RHI directional shadow shadow_color target-state ownership"
Assert-ContainsText $rhiDirectionalShadowSource "FrameGraphTextureFinalState" "RHI directional shadow frame graph final-state execution"
Assert-ContainsText $rhiDirectionalShadowSource ".final_states = final_states" "RHI directional shadow frame graph final-state execution"
Assert-ContainsText $rhiDirectionalShadowSource "frame_graph_execution.barriers_recorded" "RHI directional shadow frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "frame_graph_execution.pass_callbacks_invoked" "RHI directional shadow frame graph RHI execution"
Assert-ContainsText $rhiDirectionalShadowSource "framegraph_render_passes_recorded += frame_graph_execution.render_passes_recorded" "RHI directional shadow render pass stats evidence"
Assert-DoesNotContainText $rhiDirectionalShadowSource "pending_sprites_" "RHI directional shadow sprite submission"
Assert-ContainsText $rhiViewportSurfaceSource "execute_frame_graph_rhi_texture_schedule" "RHI viewport surface frame graph color state execution"
Assert-ContainsText $rhiViewportSurfaceSource "FrameGraphRhiRenderPassDesc" "RHI viewport surface frame graph render pass envelope"
Assert-ContainsText $rhiViewportSurfaceSource ".render_passes = std::span<const FrameGraphRhiRenderPassDesc>{render_passes}" "RHI viewport surface frame graph render pass envelope"
Assert-DoesNotContainText $rhiViewportSurfaceSource "commands->begin_render_pass" "RHI viewport surface frame graph render pass envelope"
Assert-DoesNotContainText $rhiViewportSurfaceSource "commands->end_render_pass" "RHI viewport surface frame graph render pass envelope"
Assert-ContainsText $rhiViewportSurfaceSource "FrameGraphTextureFinalState" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource ".resource = `"viewport_color`"" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource ".final_states = std::span<const FrameGraphTextureFinalState>{final_states}" "RHI viewport surface frame graph color final state execution"
Assert-ContainsText $rhiViewportSurfaceSource "color_state_ = texture_bindings.front().current_state" "RHI viewport surface recorded color state adoption"
Assert-DoesNotContainText $rhiViewportSurfaceSource "transition_texture(" "RHI viewport surface high-level color transition ownership"
foreach ($renderingGuidancePath in @(
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md"
)) {
    $renderingGuidanceText = Get-AgentSurfaceText $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "declared shadow-color/shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "viewport color-state executor slices" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphTransientTextureLeaseBindingResult" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "one backend-neutral ``IRhiDevice::acquire_transient_texture_alias_group`` lease per alias group" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "distinct texture handles in group resource order" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "shared-handle state-handoff aware" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "conflicting initial shared-handle states" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphTextureAliasingBarrier" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "record_frame_graph_texture_aliasing_barriers" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "same alias-group placed pairs" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "same-offset placed textures" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "after ``ExecuteCommandLists`` submits work rather than after fence completion" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "automatic aliasing barrier before the first pass" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "aliasing_barriers_recorded" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText 'resource-backed transient first render-pass `LoadAction::load` rows' $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "Frame Graph RHI queue dependency and multi-queue pass-command work" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "plan_frame_graph_rhi_queue_waits" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "IRhiDevice::wait_for_queue" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "execute_frame_graph_rhi_multi_queue_schedule" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphRhiMultiQueueExecutionDesc::texture_bindings" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphRhiMultiQueueExecutionResult::barriers_recorded" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "after producer waits and before the callback" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "wildcard/null barrier support" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "data inheritance/content preservation" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "FrameGraphRhiRenderPassDesc`` envelope around the ``primary_color`` callback" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "viewport.clear`` render pass envelopes" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText "direct render pass begin/end APIs" $renderingGuidancePath
    Assert-ContainsText $renderingGuidanceText 'engine/renderer/src/rhi_viewport_surface.cpp` must not call `transition_texture(' $renderingGuidancePath
    Assert-DoesNotContainText $renderingGuidanceText "declared shadow-depth/scene-color/scene-depth writer-access-backed target-state preparation" $renderingGuidancePath
    Assert-DoesNotContainText $renderingGuidanceText "Treat those bindings as acquisition output only until a separate alias-aware executor state handoff/barrier slice exists" $renderingGuidancePath
}
foreach ($postprocessDepthGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md"
)) {
    $postprocessDepthText = Get-AgentSurfaceText $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "Postprocess Depth Input Readback Foundation v0" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV" $postprocessDepthGuidance
    Assert-ContainsText $postprocessDepthText "package-visible" $postprocessDepthGuidance
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Postprocess Depth Input Readback Foundation v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Postprocess Chain Policy v1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "PostprocessChainPolicyPlan" "rendering game guidance"
foreach ($postprocessToneMappingGuidanceNeedle in @("Postprocess Tone Mapping Evidence v1", "PostprocessToneMappingEvidencePlan", "plan_postprocess_tone_mapping_evidence", "Metal host-gated", "subjective-quality")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $postprocessToneMappingGuidanceNeedle "rendering game guidance" }
$postprocessToneMappingHeaderText = Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/postprocess_policy.hpp"; foreach ($postprocessToneMappingHeaderNeedle in @("PostprocessToneMappingEvidenceRow", "PostprocessToneMappingEvidenceRequest", "PostprocessToneMappingEvidencePlan", "PostprocessToneMappingEvidenceDiagnosticCode", "plan_postprocess_tone_mapping_evidence", "has_postprocess_tone_mapping_evidence_diagnostic")) { Assert-ContainsText $postprocessToneMappingHeaderText $postprocessToneMappingHeaderNeedle "postprocess tone mapping evidence public API" }
$postprocessToneMappingSourceText = Get-AgentSurfaceText "engine/renderer/src/postprocess_policy.cpp"; foreach ($postprocessToneMappingSourceNeedle in @("missing_color_space_evidence", "missing_resource_synchronization_evidence", "missing_shader_validation_evidence", "missing_backend_validation_evidence", "missing_host_validation_evidence", "unsupported_native_handle_claim", "unsupported_subjective_quality_claim")) { Assert-ContainsText $postprocessToneMappingSourceText $postprocessToneMappingSourceNeedle "postprocess tone mapping evidence fail-closed implementation" }
$postprocessToneMappingTestText = Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp"; foreach ($postprocessToneMappingTestNeedle in @("postprocess tone mapping evidence keeps Metal host gated", "postprocess tone mapping evidence is ready with all backend host evidence", "postprocess tone mapping evidence rejects missing strict backend proof", "postprocess tone mapping evidence rejects unsafe claims and invalid tone rows", "postprocess tone mapping evidence replay hash includes accepted row details", "postprocess tone mapping evidence reports no rows without backend claims")) { Assert-ContainsText $postprocessToneMappingTestText $postprocessToneMappingTestNeedle "postprocess tone mapping evidence tests" }
foreach ($postprocessToneMappingGuidance in @("engine/agent/manifest.json", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $postprocessToneMappingGuidanceText = Get-AgentSurfaceText $postprocessToneMappingGuidance; foreach ($postprocessToneMappingNeedle in @("Postprocess Tone Mapping Evidence v1", "PostprocessToneMappingEvidencePlan", "plan_postprocess_tone_mapping_evidence", "Metal host-gated")) { Assert-ContainsText $postprocessToneMappingGuidanceText $postprocessToneMappingNeedle $postprocessToneMappingGuidance } }
$postprocessPolicyNeedles = @("postprocess_policy_status=ready", "postprocess_policy_ready=1", "postprocess_policy_diagnostics=0", "postprocess_policy_effects=1", "postprocess_policy_framegraph_passes=2", "postprocess_policy_scene_depth_required=1")
foreach ($needle in $postprocessPolicyNeedles) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) $needle "runtime game guidance" }
$d3d12PostprocessExecutionNeedles = @("postprocess_d3d12_execution_status=ready", "postprocess_d3d12_execution_ready=1", "postprocess_d3d12_execution_selected=1", "postprocess_d3d12_execution_shader_evidence_ready=1", "postprocess_d3d12_execution_passes_ok=1")
$d3d12PostprocessExecutionGuidanceNeedles = $d3d12PostprocessExecutionNeedles + @("--require-d3d12-postprocess-evidence")
foreach ($needle in $d3d12PostprocessExecutionGuidanceNeedles) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) $needle "runtime game guidance" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Sprite Batching Renderer v1 Phase 1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "SpriteBatchPlanDesc" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "atlas_backed_batch_count" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "unsupported_reordering_policy" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteFlipbookClipDesc" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteFlipbookState" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteFlipbookSampleResult" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteFlipbookDirectionSetRow" "sprite animation flipbook game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteFlipbookPlaybackRequest" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "advance_runtime_sprite_flipbook" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "advance_runtime_sprite_flipbook_playback" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "RuntimeSpriteAnimationFrame" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "sprite_flipbook_frames_sampled" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "sprite_flipbook_frames_applied" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "sprite_flipbook_direction_sets" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "sprite_flipbook_events_sampled" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentSpriteAnimationFlipbook) "package-visible flipbook counters" "sprite animation flipbook game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "stable scene color/depth/fog-constant bindings 0/1, 2/3, and 4" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "selected D3D12 height-fog constants/depth readback proof" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "postprocess_depth_input_ready=1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-postprocess-depth-input" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "generated color-postprocess scaffold" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_POSTPROCESS_DEPTH_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_POSTPROCESS_DEPTH_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "postprocess-depth readback" "Vulkan game guidance"
$vulkanHeightFogArtifactNeedles = @("MK_VULKAN_TEST_HEIGHT_FOG_DEPTH_VERTEX_SPV", "MK_VULKAN_TEST_HEIGHT_FOG_DEPTH_FRAGMENT_SPV", "MK_VULKAN_TEST_HEIGHT_FOG_VERTEX_SPV", "MK_VULKAN_TEST_HEIGHT_FOG_FRAGMENT_SPV", "VK_LAYER_KHRONOS_validation")
foreach ($needle in $vulkanHeightFogArtifactNeedles + @("height-fog runtime readback")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) $needle "Vulkan game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkanRuntimeOwners) $needle "Vulkan runtime owner guidance" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Vulkan env-gated height-fog runtime readback proof" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEnvironmentSystem) "strict env-gated Vulkan height-fog runtime readback evidence" "environment system game guidance"
$environmentPrecipitationPackageNeedles = @("--require-environment-precipitation-package-evidence", "environment_precipitation_status=ready", "stable particle/depth/sampler/constants binding slots", "8/9/8/7"); foreach ($needle in $environmentPrecipitationPackageNeedles) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentEnvironmentSystem) $needle "environment system game guidance" }; foreach ($environmentPrecipitationCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $environmentPrecipitationCodeText = Get-AgentSurfaceText $environmentPrecipitationCodeGuidance; foreach ($needle in @("Win32DesktopPresentationEnvironmentPrecipitationReport", "evaluate_win32_desktop_presentation_environment_precipitation", "Win32DesktopPresentationEnvironmentPrecipitationStatus", "constants_binding")) { Assert-ContainsText $environmentPrecipitationCodeText $needle $environmentPrecipitationCodeGuidance } }; foreach ($needle in @("--require-environment-precipitation-package-evidence", "environment_precipitation_status", "environment_precipitation_particle_buffer_uploads", "environment_precipitation_audio_playback")) { Assert-ContainsText (Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp") $needle "games/sample_desktop_runtime_game/main.cpp" }; foreach ($environmentPrecipitationPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", "docs/roadmap.md")) { $environmentPrecipitationPackageText = Get-AgentSurfaceText $environmentPrecipitationPackageGuidance; foreach ($needle in $environmentPrecipitationPackageNeedles) { Assert-ContainsText $environmentPrecipitationPackageText $needle $environmentPrecipitationPackageGuidance } }; foreach ($environmentPrecipitationValidateNeedle in @('$requiresEnvironmentPrecipitationPackageEvidence', '"environment_precipitation_status" = "ready"', '"environment_precipitation_ready" = "1"', '"environment_precipitation_selected_backend" = "d3d12"', '"environment_precipitation_weather" = "storm"', '"environment_precipitation_kind" = "rain"', '"environment_precipitation_particle_texture_binding" = "8"', '"environment_precipitation_scene_depth_texture_binding" = "9"', '"environment_precipitation_sampler_binding" = "8"', '"environment_precipitation_constants_binding" = "7"', '"environment_precipitation_audio_handoff_rows" = "4"', '"environment_precipitation_particle_buffer_uploads" = "0"', '"environment_precipitation_material_mutations" = "0"', '"environment_precipitation_audio_playback" = "0"')) { Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $environmentPrecipitationValidateNeedle "tools/validate-installed-desktop-runtime.ps1" }
foreach ($vulkanHeightFogArtifactGuidance in @("docs/testing.md", "docs/rhi.md", "docs/current-capabilities.md", "docs/superpowers/plans/2026-05-26-environment-system-v1.md", "engine/agent/manifest.json")) { $vulkanHeightFogArtifactText = Get-AgentSurfaceText $vulkanHeightFogArtifactGuidance; foreach ($needle in $vulkanHeightFogArtifactNeedles) { Assert-ContainsText $vulkanHeightFogArtifactText $needle $vulkanHeightFogArtifactGuidance } }
foreach ($vulkanHeightFogClaimGuidance in @("docs/current-capabilities.md", "docs/roadmap.md", "docs/architecture.md", "docs/superpowers/plans/2026-05-26-environment-system-v1.md")) { $vulkanHeightFogClaimText = Get-AgentSurfaceText $vulkanHeightFogClaimGuidance; foreach ($needle in @("height-fog Vulkan package readiness", "broad")) { Assert-ContainsText $vulkanHeightFogClaimText $needle $vulkanHeightFogClaimGuidance } }
Assert-ContainsText (Get-AgentSurfaceText "docs/superpowers/plans/2026-05-26-environment-system-v1.md") "MK_backend_scaffold_tests" "environment system plan"
foreach ($postprocessDepthReadyGuidance in @("docs/testing.md", "docs/rhi.md", "docs/architecture.md", "docs/roadmap.md", "docs/specs/2026-04-27-engine-essential-gap-analysis.md", ".agents/skills/rendering-change/SKILL.md", ".claude/skills/gameengine-rendering/SKILL.md", "games/sample_desktop_runtime_game/README.md")) { Assert-ContainsText (Get-AgentSurfaceText $postprocessDepthReadyGuidance) "postprocess_depth_input_ready" $postprocessDepthReadyGuidance }
foreach ($postprocessDepthPackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { Assert-ContainsText (Get-AgentSurfaceText $postprocessDepthPackageCommandGuidance) "--require-postprocess-depth-input" $postprocessDepthPackageCommandGuidance }
foreach ($postprocessPolicyGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $postprocessPolicyText = Get-AgentSurfaceText $postprocessPolicyGuidance; foreach ($postprocessPolicyNeedle in $postprocessPolicyNeedles) { Assert-ContainsText $postprocessPolicyText $postprocessPolicyNeedle $postprocessPolicyGuidance } }
$postprocessPolicyValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
foreach ($postprocessPolicyExpectedField in @('"postprocess_policy_status" = "ready"', '"postprocess_policy_ready" = "1"', '"postprocess_policy_diagnostics" = "0"', '"postprocess_policy_effects" = "1"', '"postprocess_policy_framegraph_passes" = "2"', '"postprocess_policy_scene_depth_required" = $expectedPostprocessPolicySceneDepthRequired', '"postprocess_policy_backend_shader_evidence_ready" = "1"')) { Assert-ContainsText $postprocessPolicyValidationText $postprocessPolicyExpectedField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($d3d12PostprocessExecutionGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md")) { $d3d12PostprocessExecutionText = Get-AgentSurfaceText $d3d12PostprocessExecutionGuidance; foreach ($d3d12PostprocessExecutionNeedle in $d3d12PostprocessExecutionGuidanceNeedles) { Assert-ContainsText $d3d12PostprocessExecutionText $d3d12PostprocessExecutionNeedle $d3d12PostprocessExecutionGuidance } }
foreach ($d3d12PostprocessExecutionExpectedField in @('$requiresD3d12PostprocessEvidence', '"postprocess_d3d12_execution_status" = "ready"', '"postprocess_d3d12_execution_ready" = "1"', '"postprocess_d3d12_execution_selected" = "1"', '"postprocess_d3d12_execution_shader_evidence_ready" = "1"', '"postprocess_d3d12_execution_expected_passes" = [string]$expectedSmokeFrames', '"postprocess_d3d12_execution_passes" = [string]$expectedSmokeFrames', '"postprocess_d3d12_execution_passes_ok" = "1"')) { Assert-ContainsText $postprocessPolicyValidationText $d3d12PostprocessExecutionExpectedField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($postprocessPolicyCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $postprocessPolicyCodeText = Get-AgentSurfaceText $postprocessPolicyCodeGuidance; foreach ($needle in @("Win32DesktopPresentationPostprocessPolicyReport", "evaluate_win32_desktop_presentation_postprocess_policy")) { Assert-ContainsText $postprocessPolicyCodeText $needle $postprocessPolicyCodeGuidance } }
$sampleDesktopRuntimePostprocessPolicyText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
foreach ($needle in @("evaluate_win32_desktop_presentation_postprocess_policy", "postprocess_policy_status")) { Assert-ContainsText $sampleDesktopRuntimePostprocessPolicyText $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($d3d12PostprocessExecutionCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $d3d12PostprocessExecutionCodeText = Get-AgentSurfaceText $d3d12PostprocessExecutionCodeGuidance; foreach ($needle in @("Win32DesktopPresentationD3d12PostprocessExecutionReport", "evaluate_win32_desktop_presentation_d3d12_postprocess_execution")) { Assert-ContainsText $d3d12PostprocessExecutionCodeText $needle $d3d12PostprocessExecutionCodeGuidance } }
$vulkanPostprocessExecutionNeedles = @("vulkan_postprocess_execution_status=ready", "vulkan_postprocess_execution_ready=1", "vulkan_postprocess_execution_selected=1", "vulkan_postprocess_execution_shader_evidence_ready=1", "vulkan_postprocess_execution_passes_ok=1")
foreach ($needle in $vulkanPostprocessExecutionNeedles) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) $needle "runtime game guidance" }
foreach ($vulkanPostprocessExecutionGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $vulkanPostprocessExecutionText = Get-AgentSurfaceText $vulkanPostprocessExecutionGuidance; foreach ($vulkanPostprocessExecutionNeedle in $vulkanPostprocessExecutionNeedles) { Assert-ContainsText $vulkanPostprocessExecutionText $vulkanPostprocessExecutionNeedle $vulkanPostprocessExecutionGuidance } }
foreach ($vulkanPostprocessExecutionExpectedField in @('"vulkan_postprocess_execution_status" = "ready"', '"vulkan_postprocess_execution_ready" = "1"', '"vulkan_postprocess_execution_selected" = "1"', '"vulkan_postprocess_execution_shader_evidence_ready" = "1"', '"vulkan_postprocess_execution_expected_passes" = [string]$expectedSmokeFrames', '"vulkan_postprocess_execution_passes" = [string]$expectedSmokeFrames', '"vulkan_postprocess_execution_passes_ok" = "1"')) { Assert-ContainsText $postprocessPolicyValidationText $vulkanPostprocessExecutionExpectedField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($vulkanPostprocessExecutionCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $vulkanPostprocessExecutionCodeText = Get-AgentSurfaceText $vulkanPostprocessExecutionCodeGuidance; foreach ($needle in @("Win32DesktopPresentationVulkanPostprocessExecutionReport", "evaluate_win32_desktop_presentation_vulkan_postprocess_execution")) { Assert-ContainsText $vulkanPostprocessExecutionCodeText $needle $vulkanPostprocessExecutionCodeGuidance } }
foreach ($needle in @("evaluate_win32_desktop_presentation_d3d12_postprocess_execution", "evaluate_win32_desktop_presentation_vulkan_postprocess_execution", "postprocess_d3d12_execution_status", "vulkan_postprocess_execution_status", "--require-d3d12-postprocess-evidence", "--require-vulkan-postprocess-evidence")) { Assert-ContainsText $sampleDesktopRuntimePostprocessPolicyText $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($postprocessDepthPackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $postprocessDepthPackageCommandText = Get-AgentSurfaceText $postprocessDepthPackageCommandGuidance; Assert-ContainsText $postprocessDepthPackageCommandText "--require-d3d12-postprocess-evidence" $postprocessDepthPackageCommandGuidance; Assert-ContainsText $postprocessDepthPackageCommandText "--require-vulkan-postprocess-evidence" $postprocessDepthPackageCommandGuidance }
foreach ($gameDevelopmentDepthGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $gameDevelopmentDepthText = Get-AgentSurfaceText $gameDevelopmentDepthGuidance
    Assert-ContainsText $gameDevelopmentDepthText "generated color-postprocess scaffold" $gameDevelopmentDepthGuidance
    Assert-ContainsText $gameDevelopmentDepthText "postprocess_depth_input_ready" $gameDevelopmentDepthGuidance
}
foreach ($shadowReceiverGuidance in @("docs/testing.md", "docs/rhi.md", "docs/architecture.md", "docs/roadmap.md", "docs/specs/2026-04-27-engine-essential-gap-analysis.md", ".agents/skills/rendering-change/SKILL.md", ".claude/skills/gameengine-rendering/SKILL.md")) { $shadowReceiverText = Get-AgentSurfaceText $shadowReceiverGuidance; foreach ($needle in @("MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV", "MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV", "shadow receiver readback", "package-visible")) { Assert-ContainsText $shadowReceiverText $needle $shadowReceiverGuidance } }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_SHADOW_RECEIVER_VERTEX_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "MK_VULKAN_TEST_SHADOW_RECEIVER_FRAGMENT_SPV" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentVulkan) "shadow receiver readback" "Vulkan game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Directional Shadow Receiver Readback Proof v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Package-Visible Directional Shadow Filtering v0" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_status=ready" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_ready=1" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_mode=fixed_pcf_3x3" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_taps=9" "rendering game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "directional_shadow_filter_radius_texels=1" "rendering game guidance"
foreach ($needle in @("directional_shadow_cascade_count=4", "directional_shadow_cascade_tile_width=225", "directional_shadow_atlas_width=900", "directional_shadow_atlas_height=225", "directional_shadow_light_space_cascades=4", "directional_shadow_cascade_splits=5", "framegraph_passes=3")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "Stable Directional Light-Space Policy v0" "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) "DirectionalShadowLightSpacePlan" "rendering game guidance"
foreach ($sceneScaleNeedle in @("Scene Scale Policy v1", "SceneScalePolicyDesc", "SceneScalePolicyPlan", "SceneScaleBatchingMode", "plan_scene_scale_policy", "scene_scale_policy_backend_instancing_evidence_ready", "backend instancing-evidence", "performance-measurement", "MeshCommand::instance_count", "SceneRenderSubmitDesc::mesh_instance_count", "RhiStats", "instanced_draw_calls", "instanced_indexed_draw_calls", "instanced_instances_submitted")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $sceneScaleNeedle "rendering game guidance" }
foreach ($sceneScaleGuidance in @("engine/agent/manifest.json", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $sceneScaleText = Get-AgentSurfaceText $sceneScaleGuidance; foreach ($sceneScaleNeedle in @("Scene Scale Policy v1", "SceneScalePolicyDesc", "SceneScaleBatchingMode", "plan_scene_scale_policy")) { Assert-ContainsText $sceneScaleText $sceneScaleNeedle $sceneScaleGuidance } }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dSceneScalePolicyPackageSmoke) "--require-scene-scale-policy" "scene scale policy package guidance"
foreach ($sceneScalePackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $sceneScalePackageCommandText = Get-AgentSurfaceText $sceneScalePackageCommandGuidance; foreach ($sceneScalePackageCommandNeedle in @("--require-scene-scale-policy", "--require-d3d12-instanced-draw-evidence", "--require-vulkan-instanced-draw-evidence")) { Assert-ContainsText $sceneScalePackageCommandText $sceneScalePackageCommandNeedle $sceneScalePackageCommandGuidance } }
foreach ($sceneScalePackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $sceneScalePackageText = Get-AgentSurfaceText $sceneScalePackageGuidance; foreach ($sceneScalePackageNeedle in @("scene_scale_policy_status=ready", "scene_scale_policy_ready=1", "scene_scale_policy_diagnostics=0", "scene_scale_policy_draw_groups", "scene_scale_policy_requested_instances", "scene_scale_policy_visible_instances", "scene_scale_policy_draw_calls", "scene_scale_policy_backend_instancing_evidence_required=1", "scene_scale_policy_backend_instancing_evidence_ready=1", "d3d12_instanced_draw_execution_status=ready", "d3d12_instanced_draw_execution_ready=1", "d3d12_instanced_draw_execution_selected=1", "d3d12_instanced_draw_calls", "d3d12_instanced_indexed_draw_calls", "d3d12_instanced_instances_submitted", "d3d12_instanced_draws_ok=1", "d3d12_instanced_instances_ok=1", "vulkan_instanced_draw_execution_status=ready", "vulkan_instanced_draw_execution_ready=1", "vulkan_instanced_draw_execution_selected=1", "vulkan_instanced_draw_calls", "vulkan_instanced_indexed_draw_calls", "vulkan_instanced_instances_submitted", "vulkan_instanced_draws_ok=1", "vulkan_instanced_instances_ok=1", "rhi_instanced_draw_calls", "rhi_instanced_indexed_draw_calls", "rhi_instanced_instances_submitted")) { Assert-ContainsText $sceneScalePackageText $sceneScalePackageNeedle $sceneScalePackageGuidance } }
foreach ($sceneScaleValidateNeedle in @('"scene_scale_policy_status" = "ready"', '"scene_scale_policy_ready" = "1"', '"scene_scale_policy_diagnostics" = "0"', '"scene_scale_policy_scene_resources_ready" = "1"', '"scene_scale_policy_instanced_draw_calls" = "0"', '"scene_scale_policy_lod_groups" = "0"', '$requiresD3d12InstancedDrawEvidence', '$requiresVulkanInstancedDrawEvidence', '$expectedSceneScaleBackendInstancingEvidence', '"scene_scale_policy_backend_instancing_evidence_required" = $expectedSceneScaleBackendInstancingEvidence', '"scene_scale_policy_backend_instancing_evidence_ready" = $expectedSceneScaleBackendInstancingEvidence', '"d3d12_instanced_draw_execution_status" = "ready"', '"d3d12_instanced_draw_execution_ready" = "1"', '"d3d12_instanced_draw_execution_selected" = "1"', '"d3d12_instanced_draw_execution_expected_instances"', '"d3d12_instanced_draw_calls"', '"d3d12_instanced_indexed_draw_calls"', '"d3d12_instanced_instances_submitted"', '"d3d12_instanced_draws_ok" = "1"', '"d3d12_instanced_instances_ok" = "1"', '"vulkan_instanced_draw_execution_status" = "ready"', '"vulkan_instanced_draw_execution_ready" = "1"', '"vulkan_instanced_draw_execution_selected" = "1"', '"vulkan_instanced_draw_execution_expected_instances"', '"vulkan_instanced_draw_calls"', '"vulkan_instanced_indexed_draw_calls"', '"vulkan_instanced_instances_submitted"', '"vulkan_instanced_draws_ok" = "1"', '"vulkan_instanced_instances_ok" = "1"', '"rhi_instanced_draw_calls"', '"rhi_instanced_indexed_draw_calls"', '"rhi_instanced_instances_submitted"')) { Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $sceneScaleValidateNeedle "tools/validate-installed-desktop-runtime.ps1" }
foreach ($sceneScalePolicyCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $sceneScalePolicyCodeText = Get-AgentSurfaceText $sceneScalePolicyCodeGuidance; foreach ($needle in @("Win32DesktopPresentationSceneScalePolicyReport", "evaluate_win32_desktop_presentation_scene_scale_policy", "Win32DesktopPresentationD3d12InstancedDrawExecutionReport", "evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution", "Win32DesktopPresentationVulkanInstancedDrawExecutionReport", "evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution")) { Assert-ContainsText $sceneScalePolicyCodeText $needle $sceneScalePolicyCodeGuidance } }; foreach ($needle in @("scene_scale_policy_backend_instancing_evidence_ready")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/scene_scale_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/scene_scale_policy.hpp" }; foreach ($needle in @("evaluate_win32_desktop_presentation_scene_scale_policy", "scene_scale_policy_status", "evaluate_win32_desktop_presentation_d3d12_instanced_draw_execution", "evaluate_win32_desktop_presentation_vulkan_instanced_draw_execution", "d3d12_instanced_draw_execution_status", "vulkan_instanced_draw_execution_status", "--require-vulkan-instanced-draw-evidence")) { Assert-ContainsText $sampleDesktopRuntimePostprocessPolicyText $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($gpuMemoryNeedle in @("GPU Memory Policy v1", "GpuMemoryPolicyDesc", "GpuMemoryPolicyPlan", "GpuMemoryResidencyClass", "plan_gpu_memory_policy", "gpu_memory_policy_backend_evidence_ready", "RhiDeviceMemoryDiagnostics", "memory_diagnostics", "transient_texture_heap_allocations", "upload_bytes_written")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $gpuMemoryNeedle "rendering game guidance" }; foreach ($gpuMemoryGuidance in @("engine/agent/manifest.json", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $gpuMemoryText = Get-AgentSurfaceText $gpuMemoryGuidance; foreach ($gpuMemoryNeedle in @("GPU Memory Policy v1", "GpuMemoryPolicyDesc", "plan_gpu_memory_policy")) { Assert-ContainsText $gpuMemoryText $gpuMemoryNeedle $gpuMemoryGuidance } }; Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dGpuMemoryPolicyPackageSmoke) "--require-gpu-memory-policy" "gpu memory policy package guidance"; foreach ($gpuMemoryPackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $gpuMemoryPackageCommandText = Get-AgentSurfaceText $gpuMemoryPackageCommandGuidance; foreach ($gpuMemoryPackageCommandNeedle in @("--require-gpu-memory-policy", "--require-d3d12-gpu-memory-evidence", "--require-vulkan-gpu-memory-evidence")) { Assert-ContainsText $gpuMemoryPackageCommandText $gpuMemoryPackageCommandNeedle $gpuMemoryPackageCommandGuidance } }; foreach ($gpuMemoryPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $gpuMemoryPackageText = Get-AgentSurfaceText $gpuMemoryPackageGuidance; foreach ($gpuMemoryPackageNeedle in @("gpu_memory_policy_status=ready", "gpu_memory_policy_ready=1", "gpu_memory_policy_diagnostics=0", "gpu_memory_policy_requests", "gpu_memory_policy_committed_byte_estimate", "gpu_memory_policy_upload_bytes_written", "gpu_memory_policy_backend_memory_evidence_required=1", "gpu_memory_policy_backend_memory_evidence_ready=1", "d3d12_gpu_memory_execution_status=ready", "d3d12_gpu_memory_execution_ready=1", "d3d12_gpu_memory_execution_selected=1", "d3d12_gpu_memory_execution_budget_ok=1", "d3d12_gpu_memory_execution_transient_heap_ok=1", "vulkan_gpu_memory_execution_status=ready", "vulkan_gpu_memory_execution_ready=1", "vulkan_gpu_memory_execution_selected=1", "vulkan_gpu_memory_execution_budget_ok=1", "vulkan_gpu_memory_execution_transient_heap_ok=1")) { Assert-ContainsText $gpuMemoryPackageText $gpuMemoryPackageNeedle $gpuMemoryPackageGuidance } }; foreach ($gpuMemoryValidateNeedle in @('"gpu_memory_policy_status" = "ready"', '"gpu_memory_policy_ready" = "1"', '"gpu_memory_policy_diagnostics" = "0"', '"gpu_memory_policy_scene_resources_ready" = "1"', '$requiresD3d12GpuMemoryEvidence', '$requiresVulkanGpuMemoryEvidence', '$expectedGpuMemoryBackendEvidence', '$expectedGpuMemoryOsVideoBudgetRequired', '"gpu_memory_policy_backend_memory_evidence_required" = $expectedGpuMemoryBackendEvidence', '"gpu_memory_policy_backend_memory_evidence_ready" = $expectedGpuMemoryBackendEvidence', '"gpu_memory_policy_os_video_memory_budget_required" = $expectedGpuMemoryOsVideoBudgetRequired', '"d3d12_gpu_memory_execution_status" = "ready"', '"d3d12_gpu_memory_execution_ready" = "1"', '"d3d12_gpu_memory_execution_selected" = "1"', '"d3d12_gpu_memory_execution_committed_byte_estimate_available" = "1"', '"d3d12_gpu_memory_execution_budget_ok" = "1"', '"d3d12_gpu_memory_execution_transient_heap_ok" = "1"', '"d3d12_gpu_memory_execution_committed_resources_byte_estimate"', '"d3d12_gpu_memory_execution_upload_bytes_written"', '"vulkan_gpu_memory_execution_status" = "ready"', '"vulkan_gpu_memory_execution_ready" = "1"', '"vulkan_gpu_memory_execution_selected" = "1"', '"vulkan_gpu_memory_execution_committed_byte_estimate_available" = "1"', '"vulkan_gpu_memory_execution_budget_ok" = "1"', '"vulkan_gpu_memory_execution_transient_heap_ok" = "1"', '"vulkan_gpu_memory_execution_committed_resources_byte_estimate"', '"vulkan_gpu_memory_execution_upload_bytes_written"', '"vulkan_gpu_memory_execution_framegraph_barrier_steps_executed"')) { Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $gpuMemoryValidateNeedle "tools/validate-installed-desktop-runtime.ps1" }; foreach ($gpuMemoryPolicyCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $gpuMemoryPolicyCodeText = Get-AgentSurfaceText $gpuMemoryPolicyCodeGuidance; foreach ($needle in @("Win32DesktopPresentationGpuMemoryPolicyReport", "evaluate_win32_desktop_presentation_gpu_memory_policy", "Win32DesktopPresentationD3d12GpuMemoryExecutionReport", "evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution", "Win32DesktopPresentationVulkanGpuMemoryExecutionReport", "evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution")) { Assert-ContainsText $gpuMemoryPolicyCodeText $needle $gpuMemoryPolicyCodeGuidance } }; foreach ($needle in @("gpu_memory_policy_backend_evidence_ready")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/gpu_memory_policy.hpp" }; foreach ($needle in @("Win32DesktopPresentationGpuMemoryPolicyDesc", "evaluate_win32_desktop_presentation_gpu_memory_policy", "evaluate_win32_desktop_presentation_d3d12_gpu_memory_execution", "evaluate_win32_desktop_presentation_vulkan_gpu_memory_execution", "gpu_memory_policy_status", "d3d12_gpu_memory_execution_status", "vulkan_gpu_memory_execution_status", "--require-vulkan-gpu-memory-evidence")) { Assert-ContainsText (Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp") $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($memoryDiagnosticsPackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { Assert-ContainsText (Get-AgentSurfaceText $memoryDiagnosticsPackageCommandGuidance) "--require-memory-diagnostics" $memoryDiagnosticsPackageCommandGuidance }
foreach ($memoryDiagnosticsPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md")) { $memoryDiagnosticsPackageText = Get-AgentSurfaceText $memoryDiagnosticsPackageGuidance; foreach ($memoryDiagnosticsPackageNeedle in @("memory_diagnostics_status=ready", "memory_diagnostics_ready=1", "memory_diagnostics_diagnostics=0", "memory_diagnostics_total_bytes", "memory_diagnostics_package_resident_cpu_bytes", "memory_diagnostics_resident_gpu_bytes", "memory_diagnostics_upload_staging_bytes", "memory_diagnostics_resident_gpu_pressure=nominal", "memory_diagnostics_transient_gpu_aliasing_barriers", "memory_diagnostics_transient_gpu_framegraph_aliasing_ready=1")) { Assert-ContainsText $memoryDiagnosticsPackageText $memoryDiagnosticsPackageNeedle $memoryDiagnosticsPackageGuidance } }
foreach ($memoryDiagnosticsValidateNeedle in @('$requiresMemoryDiagnostics', '$requiresAnyFramegraphMultiqueueEvidence', '"memory_diagnostics_status" = "ready"', '"memory_diagnostics_ready" = "1"', '"memory_diagnostics_diagnostics" = "0"', '"memory_diagnostics_budget_pressure_classes" = "0"', '"memory_diagnostics_budget_exceeded_classes" = "0"', '"memory_diagnostics_invalid_counter" = "0"', '"memory_diagnostics_stale_generation" = "0"', '"memory_diagnostics_use_after_safe_point" = "0"', '"memory_diagnostics_resident_gpu_pressure" = "nominal"', '"memory_diagnostics_transient_gpu_framegraph_aliasing_ready" = if ($requiresAnyFramegraphMultiqueueEvidence) { "1" } else { "0" }', '"memory_diagnostics_total_bytes"', '"memory_diagnostics_package_resident_cpu_bytes"', '"memory_diagnostics_resident_gpu_bytes"', '"memory_diagnostics_upload_staging_bytes"', 'memory_diagnostics_transient_gpu_aliasing_barriers=[1-9]\d*')) { Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $memoryDiagnosticsValidateNeedle "tools/validate-installed-desktop-runtime.ps1" }
foreach ($needle in @("summarize_package_memory_diagnostics", "MemoryCounterRow", "MemoryLifetimeClass::package_resident_cpu", "MemoryLifetimeClass::resident_gpu", "MemoryLifetimeClass::upload_staging", "MemoryLifetimeClass::transient_gpu", "memory_diagnostics_status", "memory_diagnostics_resident_gpu_pressure", "memory_diagnostics_transient_gpu_aliasing_barriers", "memory_diagnostics_transient_gpu_framegraph_aliasing_ready", "--require-memory-diagnostics")) { Assert-ContainsText (Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp") $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($debugProfilingNeedle in @("Debug Profiling Policy v1", "Backend Renderer Parity", "debug_profiling_policy_backend_evidence_ready", "DebugProfilingPolicyDesc", "DebugProfilingPolicyPlan", "DebugProfilingCaptureKind", "plan_debug_profiling_policy", "plan_backend_renderer_parity_policy", "gpu_timestamp_ticks_per_second", "gpu_debug_markers_inserted", "gpu_debug_scopes_begun")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $debugProfilingNeedle "rendering game guidance" }; foreach ($debugProfilingGuidance in @("engine/agent/manifest.json", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $debugProfilingText = Get-AgentSurfaceText $debugProfilingGuidance; foreach ($debugProfilingNeedle in @("Debug Profiling Policy v1", "DebugProfilingPolicyDesc", "plan_debug_profiling_policy", "plan_backend_renderer_parity_policy")) { Assert-ContainsText $debugProfilingText $debugProfilingNeedle $debugProfilingGuidance } }; Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dDebugProfilingPolicyPackageSmoke) "--require-debug-profiling-policy" "debug profiling policy package guidance"; foreach ($debugProfilingPackageCommandGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $debugProfilingPackageCommandText = Get-AgentSurfaceText $debugProfilingPackageCommandGuidance; foreach ($debugProfilingPackageCommandNeedle in @("--require-debug-profiling-policy", "--require-d3d12-debug-profiling-evidence", "--require-vulkan-debug-profiling-evidence")) { Assert-ContainsText $debugProfilingPackageCommandText $debugProfilingPackageCommandNeedle $debugProfilingPackageCommandGuidance } }; foreach ($debugProfilingPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $debugProfilingPackageText = Get-AgentSurfaceText $debugProfilingPackageGuidance; foreach ($debugProfilingPackageNeedle in @("debug_profiling_policy_status=ready", "debug_profiling_policy_ready=1", "debug_profiling_policy_diagnostics=0", "debug_profiling_policy_gpu_timestamp_ticks_per_second", "debug_profiling_policy_gpu_debug_markers_inserted", "debug_profiling_policy_backend_profiling_evidence_required=1", "debug_profiling_policy_backend_profiling_evidence_ready=1", "d3d12_debug_profiling_execution_status=ready", "d3d12_debug_profiling_execution_ready=1", "d3d12_debug_profiling_execution_selected=1", "d3d12_debug_profiling_execution_gpu_timestamps_ok=1", "d3d12_debug_profiling_execution_gpu_debug_markers_ok=1", "d3d12_debug_profiling_execution_frame_diagnostics_ok=1", "vulkan_debug_profiling_execution_status=ready", "vulkan_debug_profiling_execution_ready=1", "vulkan_debug_profiling_execution_selected=1", "vulkan_debug_profiling_execution_gpu_debug_markers_ok=1", "vulkan_debug_profiling_execution_frame_diagnostics_ok=1")) { Assert-ContainsText $debugProfilingPackageText $debugProfilingPackageNeedle $debugProfilingPackageGuidance } }; foreach ($debugProfilingValidateNeedle in @('"debug_profiling_policy_status" = "ready"', '"debug_profiling_policy_ready" = "1"', '"debug_profiling_policy_diagnostics" = "0"', '$requiresD3d12DebugProfilingEvidence', '$requiresVulkanDebugProfilingEvidence', '$expectedDebugProfilingBackendEvidence', '"debug_profiling_policy_backend_profiling_evidence_required" = $expectedDebugProfilingBackendEvidence', '"debug_profiling_policy_backend_profiling_evidence_ready" = $expectedDebugProfilingBackendEvidence', '"d3d12_debug_profiling_execution_status" = "ready"', '"d3d12_debug_profiling_execution_ready" = "1"', '"d3d12_debug_profiling_execution_selected" = "1"', '"d3d12_debug_profiling_execution_gpu_timestamps_ok" = "1"', '"d3d12_debug_profiling_execution_gpu_debug_markers_ok" = "1"', '"d3d12_debug_profiling_execution_frame_diagnostics_ok" = "1"', '"d3d12_debug_profiling_execution_gpu_timestamp_ticks_per_second"', '"d3d12_debug_profiling_execution_gpu_debug_markers_inserted"', '"vulkan_debug_profiling_execution_status" = "ready"', '"vulkan_debug_profiling_execution_ready" = "1"', '"vulkan_debug_profiling_execution_selected" = "1"', '"vulkan_debug_profiling_execution_gpu_debug_markers_ok" = "1"', '"vulkan_debug_profiling_execution_frame_diagnostics_ok" = "1"', '"vulkan_debug_profiling_execution_gpu_debug_markers_inserted"')) { Assert-ContainsText (Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1") $debugProfilingValidateNeedle "tools/validate-installed-desktop-runtime.ps1" }; foreach ($debugProfilingPolicyCodeGuidance in @("engine/runtime_host/win32/include/mirakana/runtime_host/win32/win32_desktop_presentation.hpp", "engine/runtime_host/win32/src/win32_desktop_presentation.cpp")) { $debugProfilingPolicyCodeText = Get-AgentSurfaceText $debugProfilingPolicyCodeGuidance; foreach ($needle in @("Win32DesktopPresentationDebugProfilingPolicyReport", "evaluate_win32_desktop_presentation_debug_profiling_policy", "Win32DesktopPresentationD3d12DebugProfilingExecutionReport", "evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution", "Win32DesktopPresentationVulkanDebugProfilingExecutionReport", "evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution")) { Assert-ContainsText $debugProfilingPolicyCodeText $needle $debugProfilingPolicyCodeGuidance } }; foreach ($needle in @("debug_profiling_policy_backend_evidence_ready")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/debug_profiling_policy.hpp" }; foreach ($needle in @("BackendRendererParityPolicyRequest", "BackendRendererParityProofRow", "BackendRendererParityPolicyPlan", "plan_backend_renderer_parity_policy", "backend_renderer_parity_proof_matches_selected_backend")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp" }; foreach ($needle in @("plan_backend_renderer_parity_policy", "cross_backend_proof_transfer", "unsupported_native_handle_claim", "missing_required_proof")) { Assert-ContainsText (Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp") $needle "tests/unit/renderer_rhi_tests.cpp" }; foreach ($needle in @("Win32DesktopPresentationDebugProfilingPolicyDesc", "evaluate_win32_desktop_presentation_debug_profiling_policy", "evaluate_win32_desktop_presentation_d3d12_debug_profiling_execution", "evaluate_win32_desktop_presentation_vulkan_debug_profiling_execution", "debug_profiling_policy_status", "d3d12_debug_profiling_execution_status", "vulkan_debug_profiling_execution_status", "--require-vulkan-debug-profiling-evidence")) { Assert-ContainsText (Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp") $needle "games/sample_desktop_runtime_game/main.cpp" }
$backendParityHostRecipeText = [string]$manifest.gameCodeGuidance.currentRenderingBackendParityHostRecipeProof; foreach ($backendParityHostRecipeNeedle in @("Backend Renderer Parity Host Recipe Proof v1", "host_validation_recipe_id", "missing_host_validation_recipe", "unreviewed_host_validation_recipe", "shader-toolchain", "mobile-packaging", "renderer-metal-apple-host-evidence", "ios-simulator-smoke", "Metal", "host-gated", "D3D12", "Vulkan")) { Assert-ContainsText $backendParityHostRecipeText $backendParityHostRecipeNeedle "rendering game guidance backend parity host recipe proof" }; foreach ($backendParityHostRecipeGuidance in @("engine/agent/manifest.json", "docs/ai-game-development.md", "docs/current-capabilities.md", "docs/specs/generated-game-validation-scenarios.md", "docs/roadmap.md", "docs/superpowers/plans/README.md", "docs/superpowers/plans/2026-05-29-renderer-backend-parity-host-recipe-proof-v1.md", "docs/superpowers/master-plans/production-completion-v1/04-developer-owned-engine-capability-backlog.md", "docs/superpowers/master-plans/production-completion-v1/05-projections-and-scenarios.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $backendParityHostRecipeGuidanceText = Get-AgentSurfaceText $backendParityHostRecipeGuidance; foreach ($backendParityHostRecipeNeedle in @("host_validation_recipe_id", "missing_host_validation_recipe", "unreviewed_host_validation_recipe", "renderer-metal-apple-host-evidence", "ios-simulator-smoke")) { Assert-ContainsText $backendParityHostRecipeGuidanceText $backendParityHostRecipeNeedle $backendParityHostRecipeGuidance } }; foreach ($needle in @("host_validation_recipe_id", "missing_host_validation_recipe", "unreviewed_host_validation_recipe", "BackendRendererParityProofRow", "BackendRendererParityDiagnosticCode")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp") $needle "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp" }; foreach ($needle in @("host_validation_recipe_ready", "is_reviewed_metal_host_validation_recipe", "missing_host_validation_recipe", "unreviewed_host_validation_recipe", "renderer-metal-apple-host-evidence", "host_validation_recipe_id")) { Assert-ContainsText (Get-AgentSurfaceText "engine/renderer/src/backend_renderer_parity_policy.cpp") $needle "engine/renderer/src/backend_renderer_parity_policy.cpp" }; foreach ($needle in @("requires host validation recipe for Metal host gates", "accepts reviewed Metal host validation recipes", "rejects unreviewed Metal host validation recipes", "missing_host_validation_recipe", "unreviewed_host_validation_recipe", "renderer-metal-apple-host-evidence", "ios-simulator-smoke")) { Assert-ContainsText (Get-AgentSurfaceText "tests/unit/renderer_rhi_tests.cpp") $needle "tests/unit/renderer_rhi_tests.cpp" }; if ([string]$productionLoop.recommendedNextPlan.id -notin @("sandbox-world-package-validation-performance-budgets-v1", "ai-operable-performance-budget-and-evidence-v1", "performance-baseline-v1", "long-running-performance-readiness-v1-phase-1", "long-running-performance-readiness-v1-phase-2", "long-running-performance-readiness-v1-phase-7", "memory-lifetime-taxonomy-v1", "memory-diagnostics-v1", "frame-thread-scratch-v1", "job-scheduling-evidence-v1", "job-execution-worker-pool-v1", "job-execution-topology-policy-v1", "job-execution-work-stealing-v1", "job-execution-placement-policy-v1", "windows-cpu-set-worker-placement-v1", "windows-cpu-set-smt-worker-placement-v1", "simd-dispatch-policy-and-evidence-v1", "avx2-reviewed-target-execution-v1", "native-win32-editor-shell-v1", "first-party-editor-shell-v1", "mavg-research-legal-benchmark-baseline-v1")) { foreach ($needle in @("Renderer Backend Parity Host Recipe Proof v1", "unsupportedProductionGaps = []", "SDL3 stays absent")) { Assert-ContainsText ([string]$productionLoop.recommendedNextPlan.latestCloseoutEvidence) $needle "recommended next plan latest closeout evidence" } }; $geRendererRecentEvidenceText = @($geRendererModule[0].recentEvidence | ForEach-Object { [string]$_ }) -join " "; Assert-ContainsText $geRendererRecentEvidenceText "Renderer Backend Parity Host Recipe Proof v1" "MK_renderer module recent evidence"; if (@($geRendererModule[0].publicHeaders) -notcontains "engine/renderer/include/mirakana/renderer/backend_renderer_parity_policy.hpp") { Write-Error "engine/agent/manifest.json MK_renderer publicHeaders must include backend_renderer_parity_policy.hpp" }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-directional-shadow" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-directional-shadow-filtering" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "--require-d3d12-shadow-cascade-policy" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "directional_shadow_ready=1" "runtime game guidance"
Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) "directional_shadow_filter_mode=fixed_pcf_3x3" "runtime game guidance"
foreach ($directionalShadowPackageGuidance in @("engine/agent/manifest.json", "games/CMakeLists.txt", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $directionalShadowPackageText = Get-AgentSurfaceText $directionalShadowPackageGuidance; foreach ($directionalShadowNeedle in @("--require-directional-shadow", "--require-directional-shadow-filtering", "--require-d3d12-shadow-cascade-policy", "--require-vulkan-shadow-cascade-policy", "--require-framegraph-multiqueue-evidence", "--require-vulkan-framegraph-multiqueue-evidence", "--require-gpu-skinning", "--require-d3d12-gpu-skinning-evidence", "--require-vulkan-gpu-skinning-evidence")) { Assert-ContainsText $directionalShadowPackageText $directionalShadowNeedle $directionalShadowPackageGuidance } }
$shadowCascadePolicyNeedles = @("d3d12_shadow_cascade_policy_ready=1", "d3d12_shadow_cascade_policy_selected=1", "vulkan_shadow_cascade_policy_ready=1", "vulkan_shadow_cascade_policy_selected=1", "directional_shadow_cascade_count=4", "directional_shadow_cascade_tile_width=225", "directional_shadow_atlas_width=900", "directional_shadow_atlas_height=225", "directional_shadow_light_space_cascades=4", "directional_shadow_cascade_splits=5")
foreach ($needle in $shadowCascadePolicyNeedles) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) $needle "runtime game guidance" }
foreach ($shadowCascadePolicyGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md")) { $shadowCascadePolicyText = Get-AgentSurfaceText $shadowCascadePolicyGuidance; foreach ($shadowCascadePolicyNeedle in $shadowCascadePolicyNeedles) { Assert-ContainsText $shadowCascadePolicyText $shadowCascadePolicyNeedle $shadowCascadePolicyGuidance } }
foreach ($shadowCascadePolicyExpectedField in @('$requiresD3d12ShadowCascadePolicy', '"d3d12_shadow_cascade_policy_ready" = "1"', '"d3d12_shadow_cascade_policy_selected" = "1"', '"vulkan_shadow_cascade_policy_ready" = "1"', '"vulkan_shadow_cascade_policy_selected" = "1"', '"directional_shadow_cascade_count" = "4"', '"directional_shadow_cascade_tile_width" = "225"', '"directional_shadow_atlas_width" = "900"', '"directional_shadow_atlas_height" = "225"', '"directional_shadow_light_space_cascades" = "4"', '"directional_shadow_cascade_splits" = "5"')) { Assert-ContainsText $postprocessPolicyValidationText $shadowCascadePolicyExpectedField "tools/validate-installed-desktop-runtime.ps1" }
$sampleDesktopRuntimeDirectionalShadowText = Get-AgentSurfaceText "games/sample_desktop_runtime_game/main.cpp"
foreach ($needle in @("require_d3d12_shadow_cascade_policy", "d3d12_shadow_cascade_policy_ready", "required_d3d12_shadow_cascade_policy_unavailable", "require_vulkan_shadow_cascade_policy", "vulkan_shadow_cascade_policy_ready", "required_vulkan_shadow_cascade_policy_unavailable", "require_framegraph_multiqueue_evidence", "d3d12_framegraph_multiqueue_evidence_ready", "d3d12_framegraph_multiqueue_evidence_selected", "require_vulkan_framegraph_multiqueue_evidence", "vulkan_framegraph_multiqueue_evidence_ready", "vulkan_framegraph_multiqueue_evidence_selected", "require_d3d12_gpu_skinning_evidence", "d3d12_gpu_skinning_evidence_ready", "require_vulkan_gpu_skinning_evidence", "vulkan_gpu_skinning_evidence_ready", "gpu_skinning_evidence_matches")) { Assert-ContainsText $sampleDesktopRuntimeDirectionalShadowText $needle "games/sample_desktop_runtime_game/main.cpp" }
foreach ($directionalShadowPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "tools/validate-installed-desktop-runtime.ps1")) { $directionalShadowPackageText = Get-AgentSurfaceText $directionalShadowPackageGuidance; foreach ($directionalShadowNeedle in @("directional_shadow_cascade_count", "directional_shadow_atlas_width", "directional_shadow_light_space_cascades")) { Assert-ContainsText $directionalShadowPackageText $directionalShadowNeedle $directionalShadowPackageGuidance } }
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dLightingShadowPolicyPackageSmoke) "--require-lighting-shadow-policy" "lighting shadow policy package guidance"
foreach ($lightingShadowPolicyPackageGuidance in @("engine/agent/manifest.json", "games/sample_desktop_runtime_game/game.agent.json", "games/sample_desktop_runtime_game/README.md", "docs/ai-game-development.md", "docs/current-capabilities.md", "docs/workflows.md", ".agents/skills/rendering-change/references/full-guidance.md", ".claude/skills/gameengine-rendering/references/full-guidance.md")) { $lightingShadowPolicyPackageText = Get-AgentSurfaceText $lightingShadowPolicyPackageGuidance; foreach ($lightingShadowPolicyNeedle in @("--require-lighting-shadow-policy", "lighting_shadow_policy_status=ready", "lighting_shadow_policy_ready=1", "lighting_shadow_policy_diagnostics=0", "lighting_shadow_policy_light_rows=1")) { Assert-ContainsText $lightingShadowPolicyPackageText $lightingShadowPolicyNeedle $lightingShadowPolicyPackageGuidance } }
foreach ($lightingShadowPolicyFlagGuidance in @("games/CMakeLists.txt", "tools/validate-installed-desktop-runtime.ps1")) { Assert-ContainsText (Get-AgentSurfaceText $lightingShadowPolicyFlagGuidance) "--require-lighting-shadow-policy" $lightingShadowPolicyFlagGuidance }
foreach ($rendererQualityPackageGuidance in @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $rendererQualityPackageText = Get-AgentSurfaceText $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "--require-renderer-quality-gates" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_status" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_framegraph_execution_budget_ok" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_expected_framegraph_render_passes" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_framegraph_render_passes_ok" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_expected_framegraph_barrier_steps" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "renderer_quality_framegraph_barrier_steps_ok" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "framegraph_render_passes_recorded" $rendererQualityPackageGuidance
    Assert-ContainsText $rendererQualityPackageText "framegraph_barrier_steps_executed" $rendererQualityPackageGuidance
}
foreach ($multiQueuePackageGuidance in @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "docs/ai-game-development.md",
    "docs/roadmap.md",
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md"
)) {
    $multiQueuePackageText = Get-AgentSurfaceText $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "--require-framegraph-multiqueue-evidence" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "--require-vulkan-framegraph-multiqueue-evidence" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "framegraph_multiqueue_command_lists_submitted=4" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "framegraph_multiqueue_queue_waits_recorded=3" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "framegraph_multiqueue_barriers_recorded=4" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "framegraph_multiqueue_aliasing_barriers_recorded=1" $multiQueuePackageGuidance
    Assert-ContainsText $multiQueuePackageText "framegraph_multiqueue_submitted_pass_fences=4" $multiQueuePackageGuidance
}
$installedMultiQueuePackageValidationText = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
foreach ($installedMultiQueueExpectedField in @(
    '"framegraph_multiqueue_command_lists_submitted" = "4"',
    '"framegraph_multiqueue_queue_waits_recorded" = "3"',
    '"framegraph_multiqueue_barriers_recorded" = "4"',
    '"framegraph_multiqueue_aliasing_barriers_recorded" = "1"',
    '"framegraph_multiqueue_pass_callbacks_invoked" = "4"',
    '"framegraph_multiqueue_submitted_pass_fences" = "4"'
)) {
    Assert-ContainsText $installedMultiQueuePackageValidationText $installedMultiQueueExpectedField "tools/validate-installed-desktop-runtime.ps1"
}
Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dFrameGraphMultiQueuePackageSmoke) "--require-vulkan-framegraph-multiqueue-evidence" "frame graph multi-queue package guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dGpuSkinningPackageSmoke) "--require-d3d12-gpu-skinning-evidence" "gpu skinning package guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.desktopRuntime3dGpuSkinningPackageSmoke) "--require-vulkan-gpu-skinning-evidence" "gpu skinning package guidance"
foreach ($needle in @("d3d12_framegraph_multiqueue_evidence_ready=1", "d3d12_framegraph_multiqueue_evidence_selected=1", "vulkan_framegraph_multiqueue_evidence_ready=1", "vulkan_framegraph_multiqueue_evidence_selected=1", "framegraph_multiqueue_command_lists_submitted=4", "framegraph_multiqueue_graphics_waited_for_copy=1", "d3d12_gpu_skinning_evidence_ready=1", "d3d12_gpu_skinning_evidence_selected=1", "vulkan_gpu_skinning_evidence_ready=1", "vulkan_gpu_skinning_evidence_selected=1", "renderer_gpu_skinning_draws", "renderer_skinned_palette_descriptor_binds")) { Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRendering) $needle "rendering game guidance"; Assert-ContainsText ([string]$manifest.gameCodeGuidance.currentRuntime) $needle "runtime game guidance" }
foreach ($d3d12MqField in @('$requiresFrameGraphMultiQueueEvidence', '"d3d12_framegraph_multiqueue_evidence_ready" = "1"', '"d3d12_framegraph_multiqueue_evidence_selected" = "1"', '"framegraph_multiqueue_graphics_waited_for_copy" = "1"')) { Assert-ContainsText $installedMultiQueuePackageValidationText $d3d12MqField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($d3d12GpuSkinningField in @('$requiresD3d12GpuSkinningEvidence', '"d3d12_gpu_skinning_evidence_ready" = "1"', '"d3d12_gpu_skinning_evidence_selected" = "1"')) { Assert-ContainsText $installedMultiQueuePackageValidationText $d3d12GpuSkinningField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($vulkanMqField in @('"vulkan_framegraph_multiqueue_evidence_ready" = "1"', '"vulkan_framegraph_multiqueue_evidence_selected" = "1"', '"vulkan_gpu_skinning_evidence_ready" = "1"', '"vulkan_gpu_skinning_evidence_selected" = "1"')) { Assert-ContainsText $installedMultiQueuePackageValidationText $vulkanMqField "tools/validate-installed-desktop-runtime.ps1" }
foreach ($renderPassPackageGuidance in @(
    ".agents/skills/rendering-change/references/full-guidance.md",
    ".claude/skills/gameengine-rendering/references/full-guidance.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    ".agents/skills/gameengine-game-development/references/full-guidance.md",
    ".claude/skills/gameengine-game-development/references/full-guidance.md",
    "docs/workflows.md",
    "docs/superpowers/master-plans/2026-05-03-production-completion-master-plan-v1.md"
)) {
    $renderPassPackageText = Get-AgentSurfaceText $renderPassPackageGuidance
    Assert-ContainsText $renderPassPackageText "framegraph_render_passes_recorded" $renderPassPackageGuidance
    Assert-ContainsText $renderPassPackageText "renderer_quality_expected_framegraph_render_passes" $renderPassPackageGuidance
    Assert-ContainsText $renderPassPackageText "renderer_quality_framegraph_render_passes" $renderPassPackageGuidance
}
$rendererQualityCMakeText = Get-AgentSurfaceText "games/CMakeLists.txt"
Assert-ContainsText $rendererQualityCMakeText "--require-renderer-quality-gates" "games/CMakeLists.txt"
foreach ($directionalShadowStatusGuidance in @(
    "engine/agent/manifest.json",
    "games/sample_desktop_runtime_game/game.agent.json",
    "games/sample_desktop_runtime_game/README.md",
    "tools/validate-installed-desktop-runtime.ps1"
)) {
    $directionalShadowStatusText = Get-AgentSurfaceText $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_status" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_ready" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_mode" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_taps" $directionalShadowStatusGuidance
    Assert-ContainsText $directionalShadowStatusText "directional_shadow_filter_radius_texels" $directionalShadowStatusGuidance
}
$installedDesktopRuntimeValidation = Get-AgentSurfaceText "tools/validate-installed-desktop-runtime.ps1"
Assert-ContainsText $installedDesktopRuntimeValidation "did not emit the required" "installed desktop runtime validation"
foreach ($directionalShadowGameDevelopmentGuidance in @(
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $directionalShadowGameDevelopmentText = Get-AgentSurfaceText $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "--require-directional-shadow" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "--require-directional-shadow-filtering" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "directional_shadow_ready" $directionalShadowGameDevelopmentGuidance
    Assert-ContainsText $directionalShadowGameDevelopmentText "directional_shadow_filter_mode" $directionalShadowGameDevelopmentGuidance
}
foreach ($stableLightSpaceGuidance in @(
    "docs/testing.md",
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md",
    "games/sample_desktop_runtime_game/README.md"
)) {
    $stableLightSpaceText = Get-AgentSurfaceText $stableLightSpaceGuidance
    Assert-ContainsText $stableLightSpaceText "Stable Directional Light-Space Policy v0" $stableLightSpaceGuidance
}
foreach ($stableLightSpaceApiGuidance in @(
    "docs/rhi.md",
    "docs/architecture.md",
    "docs/roadmap.md",
    "docs/specs/2026-04-27-engine-essential-gap-analysis.md",
    "engine/agent/manifest.json",
    ".agents/skills/rendering-change/SKILL.md",
    ".claude/skills/gameengine-rendering/SKILL.md",
    ".agents/skills/gameengine-game-development/SKILL.md",
    ".claude/skills/gameengine-game-development/SKILL.md"
)) {
    $stableLightSpaceApiText = Get-AgentSurfaceText $stableLightSpaceApiGuidance
    Assert-ContainsText $stableLightSpaceApiText "DirectionalShadowLightSpacePlan" $stableLightSpaceApiGuidance
}
if (-not $manifest.PSObject.Properties.Name.Contains("aiDrivenGameWorkflow")) {
    Write-Error "engine/agent/manifest.json must expose aiDrivenGameWorkflow"
}
foreach ($field in @(
    "sampleGame",
    "inputRendererSample",
    "gameplayFoundationSample",
    "aiNavigationSample",
    "uiAudioAssetsSample",
    "desktopRuntimeShellSample",
    "desktopRuntimeGameSample",
    "generatedDesktopRuntimePackageSample",
    "generatedDesktopRuntimeCookedScenePackageSample",
    "generatedDesktopRuntimeMaterialShaderPackageSample",
    "sample2dDesktopRuntimePackageSample"
)) {
    if (-not $manifest.aiDrivenGameWorkflow.PSObject.Properties.Name.Contains($field)) {
        Write-Error "engine/agent/manifest.json aiDrivenGameWorkflow missing required sample field: $field"
    }
    Resolve-RequiredAgentPath $manifest.aiDrivenGameWorkflow.$field | Out-Null
}

foreach ($requiredAgentPath in @("games/CMakeLists.txt", "docs/README.md", "docs/current-capabilities.md", "docs/roadmap.md", "docs/workflows.md", "games/sample_headless/game.agent.json", "docs/ai-game-development.md", "docs/superpowers/plans/README.md", "docs/specs/README.md", "docs/specs/game-template.md", "docs/specs/generated-game-validation-scenarios.md", "docs/specs/game-prompt-pack.md")) { Resolve-RequiredAgentPath $requiredAgentPath | Out-Null }
$currentEditorGuidance = [string]$manifest.gameCodeGuidance.currentEditor
foreach ($requiredNeedle in @("Windows-native editor/developer shell", "first-party retained", "MK_ui_renderer", "EditorDockLayout", "editor_dock_panel_catalog", "EditorDockCommandPlan", "plan_editor_dock_command", "apply_editor_dock_command", "EditorRichTextDocument", "EditorRichTextViewport", "make_editor_ai_command_panel_rich_text_document", "make_editor_inspector_rich_text_document", "make_editor_rich_text_ai_snapshot", "private Windows SDK DirectWrite text-layout/glyph-raster adapters", "selected text atlas handoff evidence", "private Windows TSF text-input/IME session selection", "ITextShapingAdapter", "IFontRasterizerAdapter", "Console diagnostics", "AI Commands panel status/command/evidence rows", "Inspector property rows as read-only rich-text spans", "input_rebinding remains a workspace panel outside the current native shell panel set", "editor_shell_ui=first_party", "editor_shell_backend=d3d12", "editor_shell_imgui=0", "editor_shell_panels=11", "editor_shell_docking_status=single_window_ready", "editor_shell_dock_tab_headers=11", "editor_shell_dock_split_gutters=3", "editor_shell_dock_active_panels=4", "editor_shell_dock_focusable_controls=11", "editor_shell_viewport_status=d3d12_texture_ready", "editor_shell_viewport_visible_texture_composites", "editor_shell_viewport_native_handles_exposed=0", "editor_shell_material_preview_status=d3d12_texture_ready", "editor_shell_material_preview_visible_texture_composites", "editor_shell_material_preview_native_handles_exposed=0", "editor_shell_text_atlas_handoff_status=glyphs_ready_atlas_handoff_host_gated", "editor_shell_text_font_adapter_invoked=1", "editor_shell_text_font_glyphs_ready=1", "editor_shell_text_font_fallback_used=0", "editor_shell_text_atlas_handoff_host_gated_rows=1", "editor_shell_text_atlas_handoff_unsupported_rows=1", "editor_shell_text_font_native_handles_exposed=0", "editor_shell_ime_status=win32_tsf_selected", "editor_shell_ime_caret_rect_rows=1", "editor_shell_ime_surrounding_text_rows=1", "editor_shell_ime_native_handles_exposed=0", "editor_shell_renderer_boxes_submitted", "editor_shell_renderer_text_runs_available", "--no-user-config", "Resources panel refreshes D3D12 host availability", "private D3D12 texture adapters", "private visible compositor", "broader material-preview GPU parity", "runtime game UI Dear ImGui usage")) { Assert-ContainsText $currentEditorGuidance $requiredNeedle "gameCodeGuidance.currentEditor native shell" }
foreach ($requiredNeedle in @("First-Party Editor Shell v1", "EditorAiOperationSnapshot", "EditorAiCommandCatalog", "EditorAiCommandRequest", "EditorAiCommandDryRunResult", "EditorAiCommandApplyResult", "EditorDockLayout-aware AI operation overloads", "rich_text_rows", "dry-run-before-apply AI panel visibility command rows", "dock layout snapshot rows", "reviewed dock command rows", "editor.panel.resources.show", "editor.panel.ai_commands.show", "editor.panel.profiler.hide", "editor.dock.layout.reset", "editor.dock.panel.<id>.split", "target_stack_id", "source_stack_id", "new_stack_id", "split_axis", "split_ratio", "GameEngine.Workspace.v2", "visual scraping")) { Assert-ContainsText $currentEditorGuidance $requiredNeedle "gameCodeGuidance.currentEditor AI operation surface" }
foreach ($forbiddenNeedle in @("optional SDL3 + Dear ImGui docking shell", "D3D12 shared texture wrapping through SDL3", "GUI-local SDL viewport display texture")) { Assert-DoesNotContainText $currentEditorGuidance $forbiddenNeedle "gameCodeGuidance.currentEditor native shell" }

$editorCmake = Get-AgentSurfaceText "editor/CMakeLists.txt"
foreach ($nativeEditorNeedle in @("add_library(MK_editor_shell_common", "core/src/editor_dock_layout.cpp", "core/src/editor_rich_text.cpp", "src/first_party_editor_document.cpp", "src/native_editor_launch.cpp", "src/native_editor_text_atlas_handoff.cpp", "src/native_editor_text_input.cpp", "src/native_editor_tsf_text_input.cpp", "src/native_editor_text_font_adapters.cpp", "add_library(MK_editor_shell_win32", "src/win32_first_party_editor_host.cpp", "MK_renderer", "MK_ui_renderer", "add_executable(MK_editor", "MK_platform_win32", "dwrite", "MK_ENABLE_DESKTOP_EDITOR is Windows-only", "MK_ENABLE_DESKTOP_EDITOR requires MK_platform_win32", "Native MK_editor shell is SDL3-free")) { Assert-ContainsText $editorCmake $nativeEditorNeedle "native editor shell contract" }
Assert-DoesNotContainText $editorCmake "src/first_party_editor_docking.cpp" "native editor shell contract"
Assert-DoesNotContainText $editorCmake "src/first_party_editor_rich_text.cpp" "native editor shell contract"
Assert-ContainsText $editorCmake "core/src/ai_operation_surface.cpp" "editor core AI operation source list"
$editorMainText = Get-AgentSurfaceText "editor/src/main.cpp"
foreach ($requiredNeedle in @("native_editor_launch.hpp", "native_editor_app.hpp", "win32_first_party_editor_host.hpp", "parse_native_editor_launch", "validate_native_editor_launch", "native_editor_launch_usage_error_exit_code", "editor_shell_status=ready", "editor_shell_ui=first_party", "editor_shell_backend=", "editor_shell_imgui=0", "editor_shell_panels=", "editor_shell_docking_status=", "editor_shell_dock_tab_headers=", "editor_shell_dock_split_gutters=", "editor_shell_dock_active_panels=", "editor_shell_dock_focusable_controls=", "editor_shell_sdl3=0", "editor_shell_material_preview_status=", "editor_shell_material_preview_native_handles_exposed=", "editor_shell_text_atlas_handoff_status=", "editor_shell_text_font_adapter_invoked=", "editor_shell_text_font_glyphs_ready=", "editor_shell_text_font_fallback_used=", "editor_shell_text_atlas_handoff_host_gated_rows=", "editor_shell_text_atlas_handoff_unsupported_rows=", "editor_shell_text_font_native_handles_exposed=", "editor_shell_ime_status=", "editor_shell_ime_caret_rect_rows=", "editor_shell_ime_surrounding_text_rows=", "editor_shell_ime_native_handles_exposed=", "editor_shell_renderer_boxes_submitted=", "editor_shell_renderer_text_runs_available=")) { Assert-ContainsText $editorMainText $requiredNeedle "native editor shell main" }
foreach ($forbiddenNeedle in @("SDL3", "SDL_", "ImGui_Impl", "imgui.h", "windows.h", "d3d12.h", "dxgi")) { Assert-DoesNotContainText $editorMainText $forbiddenNeedle "native editor shell main" }
$editorHostText = Get-AgentSurfaceText "editor/src/win32_first_party_editor_host.cpp"
foreach ($requiredNeedle in @("NativeEditorWin32Services", "select_adapter_kind", "D3D12CreateDevice", "make_first_party_editor_document", "submit_ui_renderer_submission", "record_native_resource_device_ready", "record_native_material_preview_d3d12_host_ready", "record_native_text_atlas_handoff_evidence", "record_native_panels_rendered", "record_native_docking_frame")) { Assert-ContainsText $editorHostText $requiredNeedle "native editor shell host" }
