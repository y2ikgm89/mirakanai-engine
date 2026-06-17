#requires -Version 7.0
#requires -PSEdition Core

# Chapter 5 for check-json-contracts.ps1 static contracts.

# Runtime UI/tools surface texts owned by this chapter.
$geUiHeaderText = Get-JsonContractSurfaceText "engine/ui/include/mirakana/ui/ui.hpp"
$geUiSourceText = Get-JsonContractSurfaceText "engine/ui/src/ui.cpp"
$sourceImageDecodeHeaderText = Get-JsonContractSurfaceText "engine/tools/include/mirakana/tools/source_image_decode.hpp"
$sourceImageDecodeSourceText = Get-JsonContractSurfaceText "engine/tools/asset/source_image_decode.cpp"
$uiAtlasToolHeaderText = Get-JsonContractSurfaceText "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp"
$uiAtlasToolSourceText = Get-JsonContractSurfaceText "engine/tools/asset/ui_atlas_tool.cpp"
$toolsTestsText = Get-JsonContractSurfaceText "tests/unit/tools_tests.cpp"
$uiRendererTestsText = Get-JsonContractSurfaceText "tests/unit/ui_renderer_tests.cpp"

$checkJsonContract040Text = Get-Content -LiteralPath (Join-Path $root "tools/check-json-contracts-040-agent-surfaces.ps1") -Raw
foreach ($forbiddenJsonContract040UiAssignment in @(
        '$geUiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/ui/include/mirakana/ui/ui.hpp")',
        '$geUiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/ui/src/ui.cpp")',
        '$sourceImageDecodeHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/source_image_decode.hpp")',
        '$sourceImageDecodeSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/source_image_decode.cpp")',
        '$uiAtlasToolHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp")',
        '$uiAtlasToolSourceText = Get-Content -LiteralPath (Join-Path $root "engine/tools/asset/ui_atlas_tool.cpp")',
        '$toolsTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/tools_tests.cpp")',
        '$uiRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/ui_renderer_tests.cpp")'
    )) {
    if ($checkJsonContract040Text.Contains($forbiddenJsonContract040UiAssignment)) {
        Write-Error "tools/check-json-contracts-040-agent-surfaces.ps1 must not preload runtime UI/tools surface texts; load them in tools/check-json-contracts-050-generated-games.ps1 instead: $forbiddenJsonContract040UiAssignment"
    }
}

foreach ($needle in @(
    "AccessibilityPublishPlan",
    "AccessibilityPublishResult",
    "plan_accessibility_publish",
    "publish_accessibility_payload",
    "IAccessibilityAdapter",
    "ImeCompositionPublishPlan",
    "ImeCompositionPublishResult",
    "plan_ime_composition_update",
    "publish_ime_composition",
    "IImeAdapter",
    "PlatformTextInputSessionPlan",
    "PlatformTextInputSessionResult",
    "PlatformTextInputEndPlan",
    "PlatformTextInputEndResult",
    "begin_platform_text_input",
    "end_platform_text_input",
    "IPlatformIntegrationAdapter",
    "TextShapingRequestPlan",
    "TextShapingResult",
    "plan_text_shaping_request",
    "shape_text_run",
    "ITextShapingAdapter",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan",
    "FontRasterizationResult",
    "plan_font_rasterization_request",
    "rasterize_font_glyph",
    "IFontRasterizerAdapter",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan",
    "ImageDecodeDispatchResult",
    "ImageDecodePixelFormat",
    "plan_image_decode_request",
    "decode_image_request",
    "IImageDecodingAdapter",
    "invalid_image_decode_result"
)) {
    if (-not $geUiHeaderText.Contains($needle)) {
        Write-Error "engine/ui/include/mirakana/ui/ui.hpp missing runtime UI publish API: $needle"
    }
}
foreach ($needle in @(
    "AccessibilityPublishPlan::ready",
    "AccessibilityPublishResult::succeeded",
    "adapter.publish_nodes",
    "invalid_accessibility_bounds",
    "ImeCompositionPublishPlan::ready",
    "ImeCompositionPublishResult::succeeded",
    "adapter.update_composition",
    "invalid_ime_target",
    "invalid_ime_cursor",
    "PlatformTextInputSessionPlan::ready",
    "PlatformTextInputSessionResult::succeeded",
    "PlatformTextInputEndPlan::ready",
    "PlatformTextInputEndResult::succeeded",
    "adapter.begin_text_input",
    "adapter.end_text_input",
    "invalid_platform_text_input_target",
    "invalid_platform_text_input_bounds",
    "TextShapingRequestPlan::ready",
    "TextShapingResult::succeeded",
    "adapter.shape_text",
    "invalid_text_shaping_text",
    "invalid_text_shaping_font_family",
    "invalid_text_shaping_max_width",
    "invalid_text_shaping_result",
    "FontRasterizationRequestPlan::ready",
    "FontRasterizationResult::succeeded",
    "adapter.rasterize_glyph",
    "invalid_font_family",
    "invalid_font_glyph",
    "invalid_font_pixel_size",
    "invalid_font_allocation",
    "ImageDecodeRequestPlan::ready",
    "ImageDecodeDispatchResult::succeeded",
    "adapter.decode_image",
    "invalid_image_decode_uri",
    "empty_image_decode_bytes",
    "invalid_image_decode_result"
)) {
    if (-not $geUiSourceText.Contains($needle)) {
        Write-Error "engine/ui/src/ui.cpp missing runtime UI publish implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter",
    "ui::IImageDecodingAdapter",
    "decode_image"
)) {
    if (-not $sourceImageDecodeHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/source_image_decode.hpp missing Runtime UI PNG adapter API: $needle"
    }
}
foreach ($needle in @(
    "PngImageDecodingAdapter::decode_image",
    "decode_audited_png_rgba8",
    "ImageDecodePixelFormat::rgba8_unorm",
    "catch (...)"
)) {
    if (-not $sourceImageDecodeSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/source_image_decode.cpp missing Runtime UI PNG adapter implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "runtime UI PNG image decoding adapter fails closed when importers are disabled",
    "runtime UI PNG image decoding adapter returns rgba8 image when importers are enabled",
    "invalid_image_decode_result"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI PNG adapter test evidence: $needle"
    }
}
foreach ($needle in @(
    "PackedUiAtlasAuthoringDesc",
    "PackedUiAtlasPackageUpdateDesc",
    "author_packed_ui_atlas_from_decoded_images",
    "plan_packed_ui_atlas_package_update",
    "apply_packed_ui_atlas_package_update"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI decoded atlas API: $needle"
    }
}
foreach ($needle in @(
    "PackedUiGlyphAtlasAuthoringDesc",
    "PackedUiGlyphAtlasPackageUpdateDesc",
    "author_packed_ui_glyph_atlas_from_rasterized_glyphs",
    "plan_packed_ui_glyph_atlas_package_update",
    "apply_packed_ui_glyph_atlas_package_update",
    "rasterized-glyph-adapter",
    "deterministic-glyph-atlas-rgba8-max-side"
)) {
    if (-not $uiAtlasToolHeaderText.Contains($needle)) {
        Write-Error "engine/tools/include/mirakana/tools/ui_atlas_tool.hpp missing Runtime UI glyph atlas API: $needle"
    }
}
foreach ($needle in @(
    "pack_sprite_atlas_rgba8_max_side",
    "GameEngine.CookedTexture.v1",
    "decoded image must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI decoded atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "rasterized glyph must be RGBA8",
    "ui atlas page output path collides"
)) {
    if (-not $uiAtlasToolSourceText.Contains($needle)) {
        Write-Error "engine/tools/asset/ui_atlas_tool.cpp missing Runtime UI glyph atlas implementation evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI atlas authoring maps decoded images into texture page and metadata",
    "packed runtime UI atlas package update writes texture page metadata and package index",
    "packed runtime UI atlas rejects invalid decoded images and package path collisions",
    "packed runtime UI atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI decoded atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "packed runtime UI glyph atlas authoring maps rasterized glyphs into texture page and metadata",
    "packed runtime UI glyph atlas package update writes texture page metadata and package index",
    "packed runtime UI glyph atlas rejects invalid glyph pixels and package path collisions",
    "packed runtime UI glyph atlas apply leaves existing files unchanged when validation fails"
)) {
    if (-not $toolsTestsText.Contains($needle)) {
        Write-Error "tests/unit/tools_tests.cpp missing Runtime UI glyph atlas test evidence: $needle"
    }
}
foreach ($needle in @(
    "CapturingAccessibilityAdapter",
    "ui accessibility publish plan dispatches validated nodes to adapter",
    "ui accessibility publish plan blocks invalid nodes before adapter",
    "CapturingImeAdapter",
    "ui ime composition publish plan dispatches valid composition to adapter",
    "ui ime composition publish plan blocks invalid composition before adapter",
    "CapturingFontRasterizerAdapter",
    "InvalidFontRasterizerAdapter",
    "ui font rasterization request plan dispatches valid request to adapter",
    "ui font rasterization request plan blocks invalid request before adapter",
    "ui font rasterization result reports invalid adapter allocation",
    "CapturingTextShapingAdapter",
    "ui text shaping request plan dispatches valid request to adapter",
    "ui text shaping request plan blocks invalid request before adapter",
    "ui text shaping result reports invalid adapter runs",
    "CapturingImageDecodingAdapter",
    "ui image decode request plan dispatches valid request to adapter",
    "ui image decode request plan blocks invalid request before adapter",
    "ui image decode result reports missing or invalid adapter output"
)) {
    if (-not $uiRendererTestsText.Contains($needle)) {
        Write-Error "tests/unit/ui_renderer_tests.cpp missing runtime UI publish test evidence: $needle"
    }
}

$geRhiModule = @($engine.modules | Where-Object { $_.name -eq "MK_rhi" })
if ($geRhiModule.Count -ne 1) {
    Write-Error "engine manifest must expose exactly one MK_rhi module"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/resource_lifetime.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include resource_lifetime.hpp"
}
if (@($geRhiModule[0].publicHeaders) -notcontains "engine/rhi/include/mirakana/rhi/upload_staging.hpp") {
    Write-Error "engine manifest MK_rhi publicHeaders must include upload_staging.hpp"
}
if (-not ([string]$geRhiModule[0].purpose).Contains("RhiResourceLifetimeRegistry") -or
    -not ([string]$geRhiModule[0].purpose).Contains("RhiUploadStagingPlan") -or
    -not ([string]$geRhiModule[0].purpose).Contains("FenceValue") -or
    -not ([string]$geRhiModule[0].purpose).Contains("foundation-only") -or
    -not ([string]$geRhiModule[0].purpose).Contains("GPU allocator")) {
    Write-Error "engine manifest MK_rhi purpose must describe foundation-only resource lifetime/upload staging and remaining GPU allocator limits"
}

foreach ($hostGate in $productionLoop.hostGates) {
    Assert-Properties $hostGate @("id", "status", "hosts", "validationRecipes", "notes") "engine manifest aiOperableProductionLoop hostGates"
    foreach ($validationRecipe in @($hostGate.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop host gate '$($hostGate.id)' references unknown validation recipe: $validationRecipe"
        }
    }
}
$vulkanGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "vulkan-strict" })
if ($vulkanGate.Count -ne 1 -or $vulkanGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep vulkan-strict host-gated"
}
$metalGate = @($productionLoop.hostGates | Where-Object { $_.id -eq "metal-apple" })
if ($metalGate.Count -ne 1 -or $metalGate[0].status -ne "host-gated") {
    Write-Error "engine manifest aiOperableProductionLoop must keep metal-apple host-gated"
}

$recipeMapIds = @{}
foreach ($map in $productionLoop.validationRecipeMap) {
    Assert-Properties $map @("recipeId", "validationRecipes") "engine manifest aiOperableProductionLoop validationRecipeMap"
    if (-not $productionRecipeIds.ContainsKey($map.recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap references unknown recipe: $($map.recipeId)"
    }
    $recipeMapIds[$map.recipeId] = $true
    foreach ($validationRecipe in @($map.validationRecipes)) {
        if (-not $validationRecipeNames.ContainsKey($validationRecipe)) {
            Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap '$($map.recipeId)' references unknown validation recipe: $validationRecipe"
        }
    }
}
foreach ($recipeId in $expectedProductionRecipeIds) {
    if (-not $recipeMapIds.ContainsKey($recipeId)) {
        Write-Error "engine manifest aiOperableProductionLoop validationRecipeMap missing recipe id: $recipeId"
    }
}
Assert-Properties $engine.aiDrivenGameWorkflow @("steps", "templateSpec", "validationScenarios", "promptPack", "sampleGame") "engine manifest aiDrivenGameWorkflow"
foreach ($workflowDoc in @($engine.aiDrivenGameWorkflow.templateSpec, $engine.aiDrivenGameWorkflow.validationScenarios, $engine.aiDrivenGameWorkflow.promptPack)) {
    if (-not (Test-Path (Join-Path $root $workflowDoc))) {
        Write-Error "engine manifest aiDrivenGameWorkflow references missing document: $workflowDoc"
    }
}

$desktopRuntimeGameRegistrations = Get-DesktopRuntimeGameRegistrations

foreach ($gameManifestEntry in Get-GameAgentManifests) {
    $relative = $gameManifestEntry.RelativePath
    $game = $gameManifestEntry.Game
    Assert-Properties $game @("schemaVersion", "name", "displayName", "language", "entryPoint", "target", "engineModules", "aiWorkflow", "gameplayContract", "backendReadiness", "importerRequirements", "packagingTargets", "validationRecipes") $relative
    Assert-Properties $game.backendReadiness @("platform", "graphics", "audio", "ui") "$relative backendReadiness"
    Assert-Properties $game.importerRequirements @("sourceFormats", "cookedOnlyRuntime", "externalImportersRequired") "$relative importerRequirements"
    if ($game.language -ne "C++23") {
        Write-Error "$relative must declare language C++23"
    }
    if ($game.name -notmatch "^[a-z][a-z0-9-]*$") {
        Write-Error "$relative has invalid game name: $($game.name)"
    }
    if ($game.target -notmatch "^[a-z][a-z0-9_]*$") {
        Write-Error "$relative has invalid target: $($game.target)"
    }
    if (-not (Test-Path (Join-Path $root $game.entryPoint))) {
        Write-Error "$relative references missing entryPoint: $($game.entryPoint)"
    }
    foreach ($module in $game.engineModules) {
        if (-not $moduleNames.ContainsKey($module)) {
            Write-Error "$relative references unknown engine module: $module"
        }
    }
    if (-not $game.importerRequirements.cookedOnlyRuntime) {
        Write-Error "$relative must keep cookedOnlyRuntime enabled"
    }
    foreach ($target in $game.packagingTargets) {
        if (-not $packagingTargetNames.ContainsKey($target)) {
            Write-Error "$relative references unknown packaging target: $target"
        }
    }
    $selectsDesktopGameRuntime = @($game.packagingTargets) -contains "desktop-game-runtime"
    $selectsDesktopRuntimeRelease = @($game.packagingTargets) -contains "desktop-runtime-release"
    if ($selectsDesktopRuntimeRelease -and -not $selectsDesktopGameRuntime) {
        Write-Error "$relative declares desktop-runtime-release but does not declare desktop-game-runtime"
    }
    if ($selectsDesktopGameRuntime) {
        if (-not $desktopRuntimeGameRegistrations.ContainsKey($game.target)) {
            Write-Error "$relative declares desktop-game-runtime but target '$($game.target)' is not registered with MK_add_desktop_runtime_game in games/CMakeLists.txt"
        }
        $registration = $desktopRuntimeGameRegistrations[$game.target]
        if (-not $registration.hasSmokeArgs) {
            Write-Error "$relative desktop runtime target '$($game.target)' must declare SMOKE_ARGS in MK_add_desktop_runtime_game"
        }
        $registeredManifest = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($game.target)'"
        if ($registeredManifest -ne $relative) {
            Write-Error "$relative desktop runtime target '$($game.target)' is registered with GAME_MANIFEST '$registeredManifest'"
        }
        Assert-DesktopRuntimePackageFileRegistration $game $relative $registration
    }
    if ($selectsDesktopRuntimeRelease) {
        Assert-DesktopRuntimePackageRecipe $game $relative
    }
    $declaredRuntimeFiles = @($game.runtimePackageFiles)
    $hasRuntimeScenePackage = @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".geindex") }).Count -gt 0 -and
        @($declaredRuntimeFiles | Where-Object { ([string]$_).EndsWith(".scene") }).Count -gt 0
    Assert-RuntimeSceneValidationTargets $game $relative $hasRuntimeScenePackage
    $sourceFormats = @($game.importerRequirements.sourceFormats)
    $requiresMaterialShaderTargets = $sourceFormats -contains "first-party-material-source" -and $sourceFormats -contains "hlsl-source"
    Assert-MaterialShaderAuthoringTargets $game $relative $requiresMaterialShaderTargets
    $requiresAtlasTilemapTargets = $game.gameplayContract.productionRecipe -eq "2d-desktop-runtime-package"
    Assert-AtlasTilemapAuthoringTargets $game $relative $requiresAtlasTilemapTargets
    Assert-SpriteAtlasSourceAuthoringTargets $game $relative $requiresAtlasTilemapTargets
    $requiresPrefabScene3dTargets = $game.gameplayContract.productionRecipe -eq "3d-playable-desktop-package"
    Assert-PrefabScenePackageAuthoringTargets $game $relative $requiresPrefabScene3dTargets
    Assert-RegisteredSourceAssetCookTargets $game $relative $requiresPrefabScene3dTargets
    Assert-PackageStreamingResidencyTargets $game $relative $hasRuntimeScenePackage
    $requiresPerformanceBudgets = @(
        "games/sample_2d_desktop_runtime_package/game.agent.json",
        "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
    ) -contains $relative
    Assert-PerformanceBudgets $game $relative $requiresPerformanceBudgets
    foreach ($recipe in $game.validationRecipes) {
        Assert-Properties $recipe @("name", "command") "$relative validationRecipes"
        if ([string]::IsNullOrWhiteSpace($recipe.command)) {
            Write-Error "$relative validation recipe '$($recipe.name)' must declare a command"
        }
    }
}

$sample2dManifestPath = "games/sample_2d_playable_foundation/game.agent.json"
$sample2dManifestEntry = Get-GameAgentManifest $sample2dManifestPath
if ($null -eq $sample2dManifestEntry) {
    Write-Error "2d-playable-source-tree recipe must have a sample game manifest: $sample2dManifestPath"
} else {
    $sample2dManifest = $sample2dManifestEntry.Game
    if ($sample2dManifest.target -ne "sample_2d_playable_foundation") {
        Write-Error "$sample2dManifestPath target must be sample_2d_playable_foundation"
    }
    if ($sample2dManifest.gameplayContract.productionRecipe -ne "2d-playable-source-tree") {
        Write-Error "$sample2dManifestPath gameplayContract.productionRecipe must be 2d-playable-source-tree"
    }
    foreach ($module in @("MK_runtime", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer")) {
        if (@($sample2dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dManifestPath engineModules missing $module"
        }
    }
    if (@($sample2dManifest.packagingTargets) -notcontains "source-tree-default") {
        Write-Error "$sample2dManifestPath must use source-tree-default packaging target"
    }
    if (@($sample2dManifest.packagingTargets) -contains "desktop-game-runtime") {
        Write-Error "$sample2dManifestPath must not claim desktop-game-runtime readiness in the source-tree 2D slice"
    }
}

$sample2dDesktopManifestPath = "games/sample_2d_desktop_runtime_package/game.agent.json"
$sample2dDesktopManifestEntry = Get-GameAgentManifest $sample2dDesktopManifestPath
if ($null -eq $sample2dDesktopManifestEntry) {
    Write-Error "2d-desktop-runtime-package recipe must have a sample game manifest: $sample2dDesktopManifestPath"
} else {
    $sample2dDesktopManifest = $sample2dDesktopManifestEntry.Game
    if ($sample2dDesktopManifest.target -ne "sample_2d_desktop_runtime_package") {
        Write-Error "$sample2dDesktopManifestPath target must be sample_2d_desktop_runtime_package"
    }
    if ($sample2dDesktopManifest.gameplayContract.productionRecipe -ne "2d-desktop-runtime-package") {
        Write-Error "$sample2dDesktopManifestPath gameplayContract.productionRecipe must be 2d-desktop-runtime-package"
    }
    foreach ($module in @("MK_platform_win32", "MK_runtime", "MK_runtime_scene", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_audio", "MK_renderer", "MK_ai", "MK_navigation", "MK_physics")) {
        if (@($sample2dDesktopManifest.engineModules) -notcontains $module) {
            Write-Error "$sample2dDesktopManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample2dDesktopManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample2dDesktopManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @(
        "runtime/sample_2d_desktop_runtime_package.config",
        "runtime/sample_2d_desktop_runtime_package.geindex",
        "runtime/assets/2d/player.texture.geasset",
        "runtime/assets/2d/player.material",
        "runtime/assets/2d/jump.audio.geasset",
        "runtime/assets/2d/level.tilemap", "runtime/assets/2d/hud.uiatlas",
        "runtime/assets/2d/playable.scene"
    )) {
        if (@($sample2dDesktopManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample2dDesktopManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample2dDesktopManifest.target)) {
        Write-Error "$sample2dDesktopManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample2dManifestText = $sample2dDesktopManifestEntry.Text
    foreach ($needle in @(
        "D3D12 package window smoke",
        "Vulkan package window smoke",
        "native 2D sprite package proof",
        "installed-d3d12-window-smoke",
        "installed-vulkan-window-smoke",
        "installed-native-2d-sprite-smoke",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "--require-gameplay-systems",
        "--require-world-region-streaming",
        "--require-entity-scale-culling",
        "--require-scripting-sandbox-policy",
        "--require-networking-foundation-policy",
        "--require-simulation-orchestration", "--require-runtime-ui-renderer-atlas-handoff",
        "gameplay systems package proof", "Runtime UI renderer atlas handoff smoke",
        "world-region streaming package proof",
        "entity scale/culling package proof",
        "scripting sandbox policy proof",
        "networking foundation policy proof",
        "simulation orchestration package proof",
        "sceneGameplayBinding", "gameplay_systems_scene_binding_ready=1",
        "gameplay_systems_scene_interaction_final_session_state", "inputContextRebinding", "input_context_rebinding_ready=1",
        "selected sprite_batch_budget_* world/UI/effects budget profile counters", "sprite_flipbook_direction_sets", "sprite_flipbook_events_sampled",
        "installed-2d-entity-scale-culling-smoke",
        "installed-2d-scripting-sandbox-policy-smoke",
        "installed-2d-networking-foundation-policy-smoke",
        "installed-2d-simulation-orchestration-smoke",
        "installed-2d-long-run-readiness-smoke",
        "host-2d-long-run-readiness-soak",
        "--require-long-run-performance-readiness",
        "long_run_readiness_status=ready",
        "public native or RHI handle access remains unsupported",
        "broad scripting/mod runtime execution remains unsupported",
        "broad multiplayer/networking readiness remains unsupported",
        "broad deterministic multiplayer readiness remains unsupported",
        "broad production sprite batching readiness remains unsupported",
        "general production renderer quality remains unsupported"
    )) {
        if (-not $sample2dManifestText.Contains($needle)) {
            Write-Error "$sample2dDesktopManifestPath missing 2D native window presentation contract text: $needle"
        }
    }
    $sample2dMainPath = Join-Path $root "games/sample_2d_desktop_runtime_package/main.cpp"
    $sample2dMainText = Get-Content -LiteralPath $sample2dMainPath -Raw
    foreach ($needle in @(
        "mirakana/runtime_host/shader_bytecode.hpp",
        "mirakana/renderer/sprite_batch.hpp",
        "mirakana/ai/behavior_tree.hpp",
        "mirakana/navigation/navigation_path_planner.hpp",
        "mirakana/physics/physics2d.hpp",
        "--require-d3d12-shaders",
        "--require-d3d12-renderer",
        "--require-vulkan-shaders",
        "--require-vulkan-renderer",
        "--require-native-2d-sprites",
        "--require-gameplay-systems",
        "--require-world-region-streaming",
        "--require-scripting-sandbox-policy",
        "--require-networking-foundation-policy",
        "--require-simulation-orchestration",
        "--require-long-run-performance-readiness",
        "mirakana/runtime/world_region_streaming.hpp",
        "mirakana/runtime/entity_scale_culling.hpp",
        "mirakana/runtime/scripting_sandbox.hpp",
        "execute_runtime_script_entrypoint",
        "SampleScriptingSandboxAdapter",
        "mirakana/runtime/networking_foundation.hpp",
        "mirakana/runtime/simulation_orchestration.hpp",
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "plan_scene_sprite_batches",
        "plan_sprite_batch_budget_profile",
        "sprite_batch_plan_draws", "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_atlas_backed_batches", "sprite_batch_plan_repeated_atlas_batches",
        "sprite_batch_plan_repeated_atlas_sprites", "sprite_batch_plan_diagnostics",
        "sprite_batch_budget_status", "sprite_batch_budget_ui_max_texture_binds",
        "advance_runtime_sprite_flipbook",
        "advance_runtime_sprite_flipbook_playback",
        "sprite_flipbook_ticks",
        "sprite_flipbook_frames_sampled",
        "sprite_flipbook_frames_applied",
        "sprite_flipbook_selected_frame_sum",
        "sprite_flipbook_diagnostics", "sprite_flipbook_direction_sets", "sprite_flipbook_event_rows",
        "sprite_flipbook_playback_modes", "sprite_flipbook_gameplay_state_rows", "sprite_flipbook_events_sampled", "sprite_flipbook_playback_diagnostics",
        "gameplay_systems_status=",
        "gameplay_systems_physics_contacts=",
        "gameplay_systems_navigation_plan_status=",
        "gameplay_systems_behavior_status=",
        "gameplay_systems_behavior_authoring_ready=",
        "gameplay_systems_behavior_authoring_diagnostics=",
        "gameplay_systems_behavior_authoring_trace_nodes=",
        "gameplay_systems_scene_binding_ready=",
        "gameplay_systems_scene_binding_source_rows=",
        "gameplay_systems_scene_binding_rows=",
        "gameplay_systems_scene_binding_systems=",
        "gameplay_systems_scene_binding_component_rows=",
        "gameplay_systems_scene_binding_diagnostics=",
        "gameplay_systems_scene_interaction_rows=",
        "gameplay_systems_scene_interaction_diagnostics=",
        "gameplay_systems_scene_interaction_final_session_state=",
        "input_context_rebinding_ready=", "input_context_rebinding_layers=", "input_context_rebinding_active_contexts=",
        "input_context_rebinding_capture_active=", "input_context_rebinding_gameplay_consumed=",
        "input_rebinding_profile_overlays_applied=", "input_rebinding_action_capture_status=",
        "input_rebinding_axis_capture_status=", "input_rebinding_focus_consumed=", "input_rebinding_focus_retained=",
        "input_rebinding_presentation_rows=", "input_rebinding_glyph_lookup_keys=", "input_rebinding_diagnostics=",
        "world_region_streaming_status=",
        "world_region_streaming_load_rows=",
        "world_region_streaming_unload_rows=",
        "world_region_streaming_reviewed_package_adoptions=",
        "world_region_streaming_missing_region_diagnostics=",
        "world_region_streaming_safe_point_diagnostics=",
        "entity_scale_culling_status=",
        "entity_scale_culling_rows=",
        "entity_scale_culling_lod_rows=",
        "entity_scale_culling_budget_diagnostics=",
        "scripting_sandbox_status=", "scripting_sandbox_entrypoint_rows=", "scripting_sandbox_denied_permission_rows=",
        "scripting_sandbox_rejected_unsafe_capability_rows=", "scripting_sandbox_budget_diagnostics=",
        "scripting_sandbox_replay_seed_rows=", "scripting_sandbox_diagnostics=", "scripting_sandbox_execution_status=",
        "scripting_sandbox_execution_ready=", "scripting_sandbox_execution_dispatches=",
        "scripting_sandbox_execution_host_api_calls=", "scripting_sandbox_execution_replay_signature=",
        "scripting_sandbox_execution_diagnostics=",
        "networking_foundation_status=",
        "networking_foundation_session_rows=",
        "networking_foundation_transport_rows=",
        "networking_foundation_channel_rows=",
        "networking_foundation_rejected_unsafe_transport_rows=",
        "networking_foundation_replay_prerequisite_rows=",
        "networking_foundation_security_diagnostics=",
        "networking_foundation_diagnostics=",
        "simulation_orchestration_status=",
        "simulation_orchestration_planned_steps=",
        "simulation_orchestration_command_playback_rows=",
        "simulation_orchestration_budget_limited_status=",
        "simulation_orchestration_invalid_command_diagnostics=",
        "simulation_orchestration_diagnostics=",
        "long_run_readiness_status=",
        "long_run_readiness_memory_growth_bytes=",
        "gameplay_authoring_review_status=", "gameplay_authoring_review_ready=",
        "gameplay_authoring_review_feature_rows=", "gameplay_authoring_review_accepted_rows=",
        "gameplay_authoring_review_mutation_ledger_rows=", "gameplay_authoring_review_remediation_rows=",
        "gameplay_authoring_review_missing_required_capability_diagnostics=",
        "gameplay_authoring_review_missing_validation_recipe_diagnostics=",
        "gameplay_authoring_review_missing_package_evidence_diagnostics=",
        "gameplay_authoring_review_unsupported_claim_diagnostics=", "gameplay_authoring_review_diagnostics=",
        "required_long_run_performance_readiness_unavailable",
        "required_gameplay_authoring_review_unavailable",
        "required_simulation_orchestration_unavailable",
        "required_networking_foundation_policy_unavailable",
        "required_scripting_sandbox_policy_unavailable",
        "required_entity_scale_culling_unavailable",
        "required_world_region_streaming_unavailable",
        "required_gameplay_systems_unavailable",
        "required_native_2d_sprites_unavailable",
        "required_d3d12_renderer_unavailable",
        "required_vulkan_renderer_unavailable"
    )) {
        if (-not $sample2dMainText.Contains($needle)) {
            Write-Error "games/sample_2d_desktop_runtime_package/main.cpp missing native presentation smoke field: $needle"
        }
    }
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw
    foreach ($needle in @(
        "sample_2d_desktop_runtime_package_sprite.vs.dxil",
        "sample_2d_desktop_runtime_package_sprite.ps.dxil",
        "sample_2d_desktop_runtime_package_sprite.vs.spv",
        "sample_2d_desktop_runtime_package_sprite.ps.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.dxil",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.vs.spv",
        "sample_2d_desktop_runtime_package_native_sprite_overlay.ps.spv",
        "--require-native-2d-sprites",
        "--require-gameplay-systems",
        "--require-world-region-streaming",
        "--require-entity-scale-culling",
        "--require-scripting-sandbox-policy",
        "--require-networking-foundation-policy",
        "--require-simulation-orchestration",
        "--require-gameplay-authoring-review",
        "MK_ai",
        "MK_navigation",
        "MK_physics",
        "sample_2d_desktop_runtime_package_shaders",
        "sample_2d_desktop_runtime_package_vulkan_shaders",
        "REQUIRES_D3D12_SHADERS"
    )) {
        if (-not $gamesCMakeText.Contains($needle)) {
            Write-Error "games/CMakeLists.txt missing 2D native presentation package metadata: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($needle in @(
        "--require-native-2d-sprites",
        "--require-world-region-streaming",
        "--require-entity-scale-culling",
        "--require-scripting-sandbox-policy",
        "--require-networking-foundation-policy",
        "--require-simulation-orchestration",
        "--require-gameplay-authoring-review",
        "--require-long-run-performance-readiness",
        "Assert-InstalledLongRunReadinessEvidence",
        "native_2d_sprites_status",
        "native_2d_textured_sprites_submitted",
        "native_2d_texture_binds",
        "sprite_batch_plan_draws", "sprite_batch_plan_texture_binds",
        "sprite_batch_plan_atlas_backed_batches", "sprite_batch_plan_repeated_atlas_batches",
        "sprite_batch_plan_repeated_atlas_sprites", "sprite_batch_plan_diagnostics",
        "sprite_batch_budget_status", "sprite_batch_budget_ui_max_texture_binds",
        "sprite_flipbook_ticks",
        "sprite_flipbook_frames_sampled",
        "sprite_flipbook_frames_applied",
        "sprite_flipbook_selected_frame_sum",
        "sprite_flipbook_diagnostics", "sprite_flipbook_direction_sets", "sprite_flipbook_event_rows",
        "sprite_flipbook_playback_modes", "sprite_flipbook_gameplay_state_rows", "sprite_flipbook_events_sampled", "sprite_flipbook_playback_diagnostics",
        "gameplay_systems_status",
        "gameplay_systems_physics_contacts",
        "gameplay_systems_navigation_plan_status",
        "gameplay_systems_behavior_status",
        "gameplay_systems_behavior_authoring_ready",
        "gameplay_systems_behavior_authoring_diagnostics",
        "gameplay_systems_behavior_authoring_trace_nodes",
        "gameplay_systems_scene_binding_ready",
        "gameplay_systems_scene_binding_source_rows",
        "gameplay_systems_scene_binding_rows",
        "gameplay_systems_scene_binding_systems",
        "gameplay_systems_scene_binding_component_rows",
        "gameplay_systems_scene_binding_diagnostics",
        "gameplay_systems_scene_interaction_rows",
        "gameplay_systems_scene_interaction_diagnostics",
        "gameplay_systems_scene_interaction_final_session_state",
        "input_context_rebinding_ready", "input_context_rebinding_layers", "input_context_rebinding_active_contexts",
        "input_context_rebinding_capture_active", "input_context_rebinding_gameplay_consumed",
        "input_rebinding_profile_overlays_applied", "input_rebinding_action_capture_status",
        "input_rebinding_axis_capture_status", "input_rebinding_focus_consumed", "input_rebinding_focus_retained",
        "input_rebinding_presentation_rows", "input_rebinding_glyph_lookup_keys", "input_rebinding_diagnostics",
        "world_region_streaming_status", "world_region_streaming_load_rows", "world_region_streaming_keep_rows",
        "world_region_streaming_unload_rows", "world_region_streaming_reviewed_package_adoptions",
        "world_region_streaming_missing_region_diagnostics", "world_region_streaming_safe_point_diagnostics",
        "world_region_streaming_large_scene_readiness_status", "world_region_streaming_large_scene_readiness_diagnostic", "world_region_streaming_large_scene_readiness_diagnostics",
        "world_region_streaming_navigation_resident_regions", "world_region_streaming_navigation_missing_resident_regions", "world_region_streaming_navigation_path_cache_ready",
        "entity_scale_culling_status",
        "entity_scale_culling_rows",
        "entity_scale_culling_lod_rows",
        "entity_scale_culling_budget_diagnostics",
        "scripting_sandbox_status", "scripting_sandbox_entrypoint_rows", "scripting_sandbox_denied_permission_rows",
        "scripting_sandbox_rejected_unsafe_capability_rows", "scripting_sandbox_budget_diagnostics",
        "scripting_sandbox_replay_seed_rows", "scripting_sandbox_diagnostics", "scripting_sandbox_execution_status",
        "scripting_sandbox_execution_ready", "scripting_sandbox_execution_dispatches",
        "scripting_sandbox_execution_host_api_calls", "scripting_sandbox_execution_replay_signature",
        "scripting_sandbox_execution_diagnostics",
        "networking_foundation_status",
        "networking_foundation_session_rows",
        "networking_foundation_transport_rows",
        "networking_foundation_channel_rows",
        "networking_foundation_rejected_unsafe_transport_rows",
        "networking_foundation_replay_prerequisite_rows",
        "networking_foundation_security_diagnostics",
        "networking_foundation_diagnostics",
        "simulation_orchestration_status",
        "simulation_orchestration_ready",
        "simulation_orchestration_available_steps",
        "simulation_orchestration_planned_steps",
        "simulation_orchestration_step_rows",
        "simulation_orchestration_command_rows",
        "simulation_orchestration_command_playback_rows",
        "simulation_orchestration_consumed_time_us",
        "simulation_orchestration_remaining_time_us",
        "simulation_orchestration_budget_limited_status",
        "simulation_orchestration_budget_limited_available_steps",
        "simulation_orchestration_budget_limited_planned_steps",
        "simulation_orchestration_budget_limited_remaining_time_us",
        "simulation_orchestration_invalid_command_diagnostics",
        "simulation_orchestration_diagnostics",
        "gameplay_authoring_review_status", "gameplay_authoring_review_ready",
        "gameplay_authoring_review_feature_rows", "gameplay_authoring_review_accepted_rows",
        "gameplay_authoring_review_mutation_ledger_rows", "gameplay_authoring_review_remediation_rows",
        "gameplay_authoring_review_missing_required_capability_diagnostics",
        "gameplay_authoring_review_missing_validation_recipe_diagnostics",
        "gameplay_authoring_review_missing_package_evidence_diagnostics",
        "gameplay_authoring_review_unsupported_claim_diagnostics", "gameplay_authoring_review_diagnostics",
        "long_run_readiness_status",
        "long_run_readiness_memory_high_water_bytes",
        "long_run_readiness_native_handles_exposed"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($needle)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 2D native sprite package validation field: $needle"
        }
    }
    $newGameText = (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) +
        "`n" +
        (Get-Content -LiteralPath (Join-Path $root "tools/new-game-templates.ps1") -Raw)
    foreach ($needle in @(
        "shaders/runtime_2d_sprite.hlsl",
        "installed-native-2d-sprite-smoke",
        "installed-2d-entity-scale-culling-smoke",
        "--require-native-2d-sprites",
        "--require-entity-scale-culling",
        "MK_configure_desktop_runtime_2d_sprite_shader_artifacts"
    )) {
        if (-not $newGameText.Contains($needle)) {
            Write-Error "tools/new-game.ps1 missing generated 2D native sprite scaffold contract: $needle"
        }
    }
    if (-not $gamesCMakeText.Contains("function(MK_configure_desktop_runtime_2d_sprite_shader_artifacts)")) {
        Write-Error "games/CMakeLists.txt missing generated 2D sprite shader artifact helper"
    }
}

$spriteBatchHeaderPath = Join-Path $root "engine/renderer/include/mirakana/renderer/sprite_batch.hpp"
$spriteBatchSourcePath = Join-Path $root "engine/renderer/src/sprite_batch.cpp"
if (-not (Test-Path -LiteralPath $spriteBatchHeaderPath)) {
    Write-Error "Missing 2D sprite batch planning header"
}
if (-not (Test-Path -LiteralPath $spriteBatchSourcePath)) {
    Write-Error "Missing 2D sprite batch planning source"
}
$spriteBatchHeaderText = Get-Content -LiteralPath $spriteBatchHeaderPath -Raw
$spriteBatchSourceText = Get-Content -LiteralPath $spriteBatchSourcePath -Raw
$sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
$sceneRendererSourceText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/src/scene_renderer.cpp") -Raw
$sceneRendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/scene_renderer_tests.cpp") -Raw
$rendererTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/renderer_rhi_tests.cpp") -Raw
$frameGraphHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph.hpp") -Raw
$frameGraphSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph.cpp") -Raw
$frameGraphRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/frame_graph_rhi.hpp") -Raw
$frameGraphRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/src/frame_graph_rhi.cpp") -Raw
$rhiUploadStagingHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/include/mirakana/rhi/upload_staging.hpp") -Raw
$rhiUploadStagingSourceText = Get-Content -LiteralPath (Join-Path $root "engine/rhi/src/upload_staging.cpp") -Raw
$rhiUploadStagingTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/rhi_upload_staging_tests.cpp") -Raw
$runtimeRhiUploadHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/include/mirakana/runtime_rhi/runtime_upload.hpp") -Raw
$runtimeRhiUploadSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/src/runtime_upload.cpp") -Raw
$runtimeRhiPackageStreamingHeaderText =
    Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/include/mirakana/runtime_rhi/package_streaming_frame_graph.hpp") -Raw
$runtimeRhiPackageStreamingSourceText =
    Get-Content -LiteralPath (Join-Path $root "engine/runtime_rhi/src/package_streaming_frame_graph.cpp") -Raw
$runtimeSceneRhiHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/include/mirakana/runtime_scene_rhi/runtime_scene_rhi.hpp") -Raw
$runtimeSceneRhiSourceText = Get-Content -LiteralPath (Join-Path $root "engine/runtime_scene_rhi/src/runtime_scene_rhi.cpp") -Raw
$runtimeRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_rhi_tests.cpp") -Raw
$runtimeSceneRhiTestsText = Get-Content -LiteralPath (Join-Path $root "tests/unit/runtime_scene_rhi_tests.cpp") -Raw
$historicalVerdictArchiveText = Get-JsonContractSurfaceText "docs/superpowers/master-plans/production-completion-v1/99-historical-verdict-archive.md"
$runtimeUploadFencePlanText = $historicalVerdictArchiveText
$frameGraphRhiTextureSchedulePlanText = $historicalVerdictArchiveText
$rhiUploadStaleGenerationPlanText = $historicalVerdictArchiveText
$runtimeUploadQueueWaitPlanText = $historicalVerdictArchiveText
$runtimePackageUploadStagingEvidencePlanText = $historicalVerdictArchiveText
$rendererCmakeText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/CMakeLists.txt") -Raw
$engineManifestText = Get-Content -LiteralPath (Join-Path $root "engine/agent/manifest.json") -Raw
foreach ($needle in @("FrameGraphPassExecutionBinding", "FrameGraphExecutionCallbacks", "FrameGraphExecutionResult", "execute_frame_graph_v1_schedule")) {
    if (-not $frameGraphHeaderText.Contains($needle)) {
        Write-Error "Frame Graph callback execution header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph pass callback is missing",
    "frame graph barrier callback is missing",
    "frame graph pass callback threw an exception",
    "frame graph barrier callback threw an exception",
    "frame graph pass callback failed",
    "frame graph barrier callback failed"
)) {
    if (-not $frameGraphSourceText.Contains($needle)) {
        Write-Error "Frame Graph callback execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph v1 dispatches barrier and pass callbacks in schedule order",
    "frame graph v1 callback execution diagnoses missing callbacks before later passes",
    "frame graph v1 callback execution converts thrown callbacks to diagnostics",
    "frame graph v1 callback execution copies pass bindings before dispatch",
    "frame graph v1 callback execution reports returned callback failures",
    "frame graph v1 callback execution converts thrown barrier callbacks to diagnostics"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph callback execution coverage: $needle"
    }
}
foreach ($needle in @("FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiHeaderText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution header missing contract text: $needle"
    }
}
foreach ($needle in @("FrameGraphTransientTextureDesc", "FrameGraphTransientTextureAliasPlan", "plan_frame_graph_transient_texture_aliases")) {
    if (-not $frameGraphRhiHeaderText.Contains($needle)) {
        Write-Error "Frame Graph transient texture alias planning header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution requires a command list",
    "frame graph rhi texture schedule execution cannot use a closed command list",
    "frame graph pass callback is empty",
    "frame graph pass callback is declared more than once",
    "frame graph texture barrier recording failed"
)) {
    if (-not $frameGraphRhiSourceText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "transient texture descriptor targets an imported resource",
    "transient texture descriptor targets an undeclared resource",
    "used transient texture resource has no descriptor",
    "transient texture usage is missing render_target",
    "transient depth texture usage supports only depth_stencil or sampled depth",
    "transient texture byte estimate overflowed"
)) {
    if (-not $frameGraphRhiSourceText.Contains($needle)) {
        Write-Error "Frame Graph transient texture alias planning source missing diagnostic text: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi texture schedule execution interleaves barriers and pass callbacks",
    "frame graph rhi texture schedule execution validates barriers before pass callbacks",
    "frame graph rhi texture schedule execution validates pass callbacks before barriers"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph RHI texture schedule execution coverage: $needle"
    }
}
foreach ($needle in @(
    "frame graph rhi transient texture alias planner reuses exact non overlapping descriptors",
    "frame graph rhi transient texture alias planner keeps incompatible descriptors separate",
    "frame graph rhi transient texture alias planner rejects unsafe descriptors",
    "frame graph rhi transient texture alias planner rejects backend incompatible depth descriptors",
    "frame graph rhi transient texture alias planner rejects byte estimate overflow"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing Frame Graph transient texture alias planning coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "FrameGraphRhiTextureExecutionDesc", "FrameGraphRhiTextureExecutionResult", "execute_frame_graph_rhi_texture_schedule")) {
    if (-not $frameGraphRhiTextureSchedulePlanText.Contains($needle)) {
        Write-Error "Frame Graph RHI texture schedule execution plan missing contract text: $needle"
    }
}
foreach ($needle in @(
    "submitted_fence",
    "RuntimeTextureUploadResult",
    "RuntimeMeshUploadResult",
    "RuntimeSkinnedMeshUploadResult",
    "RuntimeMorphMeshUploadResult",
    "RuntimeMaterialGpuBinding",
    "frame_graph_command_lists_submitted",
    "frame_graph_queue_waits_recorded",
    "frame_graph_barriers_recorded",
    "frame_graph_pass_callbacks_invoked"
)) {
    if (-not $runtimeRhiUploadHeaderText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence header missing contract text: $needle"
    }
}
foreach ($needle in @("submitted_upload_fences", "submitted_upload_fence_count", "last_submitted_upload_fence")) {
    if (-not $runtimeSceneRhiHeaderText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload execution report header missing contract text: $needle"
    }
}
foreach ($needle in @(
    "result.submitted_fence = fence",
    "return RuntimeTextureUploadResult",
    "return RuntimeMeshUploadResult",
    "return RuntimeSkinnedMeshUploadResult",
    "return RuntimeMorphMeshUploadResult",
    "result.frame_graph_command_lists_submitted = frame_graph_execution.command_lists_submitted",
    "result.frame_graph_queue_waits_recorded = frame_graph_execution.queue_waits_recorded",
    "result.frame_graph_barriers_recorded = frame_graph_execution.barriers_recorded",
    "result.frame_graph_pass_callbacks_invoked = frame_graph_execution.pass_callbacks_invoked"
)) {
    if (-not $runtimeRhiUploadSourceText.Contains($needle)) {
        Write-Error "Runtime RHI upload submission fence source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "record_submitted_upload_fence",
    "result.submitted_upload_fences.push_back",
    "upload.submitted_fence",
    "base_upload.submitted_fence",
    "binding.submitted_fence",
    "bindings.submitted_upload_fences"
)) {
    if (-not $runtimeSceneRhiSourceText.Contains($needle)) {
        Write-Error "Runtime scene RHI upload fence aggregation source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "runtime rhi upload reports submitted fence without forcing wait",
    "result.submitted_fence.value != 0",
    "binding.submitted_fence.value != 0",
    "upload.submitted_fence.value != 0",
    "binding.frame_graph_command_lists_submitted == 1",
    "binding.frame_graph_queue_waits_recorded == 0",
    "binding.frame_graph_barriers_recorded == 0",
    "binding.frame_graph_pass_callbacks_invoked == 1"
)) {
    if (-not $runtimeRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "runtime scene rhi upload execution preserves submitted fences in submit order across queues",
    "submitted_upload_fences[0].queue == mirakana::rhi::QueueKind::compute",
    "submitted_upload_fences[1].value == compute_resource.base_position_upload.submitted_fence.value",
    "submitted_upload_fence_count == 3",
    "submitted_upload_fence_count == 4",
    "last_submitted_upload_fence.value != 0"
)) {
    if (-not $runtimeSceneRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_scene_rhi_tests missing upload submission fence coverage: $needle"
    }
}
foreach ($needle in @(
    "**Status:** Completed",
    "Runtime RHI Upload Submission Fence Rows v1",
    "submitted_upload_fences",
    "submitted_upload_fence_count",
    "native async upload execution"
)) {
    if (-not $runtimeUploadFencePlanText.Contains($needle)) {
        Write-Error "Runtime RHI Upload Submission Fence Rows plan missing text: $needle"
    }
}
foreach ($needle in @("RhiUploadDiagnosticCode", "stale_generation", "RhiUploadRing")) {
    if (-not $rhiUploadStagingHeaderText.Contains($needle)) {
        Write-Error "RHI upload staging header missing stale-generation contract text: $needle"
    }
}
foreach ($needle in @(
    "inactive_allocation_code",
    "RHI upload staging allocation generation is stale.",
    "span.allocation_generation",
    "RhiUploadDiagnosticCode::stale_generation"
)) {
    if (-not $rhiUploadStagingSourceText.Contains($needle)) {
        Write-Error "RHI upload staging source missing stale-generation implementation text: $needle"
    }
}
foreach ($needle in @(
    "rhi upload staging diagnoses stale allocation generations",
    "rhi upload ring ownership requires matching allocation generation",
    "RhiUploadDiagnosticCode::stale_generation",
    "!ring.owns_allocation(stale)"
)) {
    if (-not $rhiUploadStagingTestsText.Contains($needle)) {
        Write-Error "MK_rhi_upload_staging_tests missing stale-generation coverage: $needle"
    }
}
foreach ($needle in @("RhiUploadGpuBatchExecutionResult", "execute_upload_gpu_batch_async")) {
    if (-not $rhiUploadStagingHeaderText.Contains($needle)) {
        Write-Error "RHI upload staging header missing async execution contract text: $needle"
    }
}
foreach ($needle in @("RuntimeUploadQueueWaitResult", "wait_for_runtime_uploads_on_queue")) {
    if (-not $runtimeRhiUploadHeaderText.Contains($needle)) {
        Write-Error "runtime RHI upload header missing queue-wait contract text: $needle"
    }
}
foreach ($needle in @("valid_runtime_upload_queue_kind", "device.wait_for_queue(consumer_queue, fence)", "queue_waits_recorded")) {
    if (-not $runtimeRhiUploadSourceText.Contains($needle)) {
        Write-Error "runtime RHI upload source missing queue-wait implementation text: $needle"
    }
}
foreach ($needle in @("upload_queue_waits_recorded", "RuntimePackageStreamingMeshUploadBindingResult")) {
    if (-not $runtimeRhiPackageStreamingHeaderText.Contains($needle)) {
        Write-Error "runtime RHI package streaming header missing queue-wait contract text: $needle"
    }
}
foreach ($needle in @("async_upload_fences", "wait_for_runtime_uploads_on_queue(device, rhi::QueueKind::graphics", "mesh-upload-queue-wait-failed")) {
    if (-not $runtimeRhiPackageStreamingSourceText.Contains($needle)) {
        Write-Error "runtime RHI package streaming source missing queue-wait implementation text: $needle"
    }
}
foreach ($needle in @("runtime package streaming mesh upload transaction waits graphics queue for async copy upload", "transaction.upload_queue_waits_recorded == 1", "device.stats().fence_waits == 0", "device.stats().last_graphics_queue_wait_fence_queue == mirakana::rhi::QueueKind::copy")) {
    if (-not $runtimeRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_rhi_tests missing runtime upload queue-wait coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "Runtime Upload Queue Wait v1", "wait_for_runtime_uploads_on_queue", "upload_queue_waits_recorded")) {
    if (-not $runtimeUploadQueueWaitPlanText.Contains($needle)) {
        Write-Error "Runtime Upload Queue Wait plan missing text: $needle"
    }
}
foreach ($needle in @("RuntimePackageUploadStagingEvidence", "RuntimePackageResourceUpdateReadinessResult", "make_runtime_package_resource_update_readiness", "execute_runtime_package_upload_staging_evidence")) {
    if (-not $runtimeRhiPackageStreamingHeaderText.Contains($needle)) {
        Write-Error "runtime RHI package streaming header missing package upload staging evidence contract text: $needle"
    }
}
foreach ($needle in @("RhiStagingBufferPool", "try_acquire_lease", "upload_runtime_package_streaming_skinned_mesh_gpu_bindings", "make_runtime_package_resource_update_readiness", "package-upload-staging-counters-mismatch")) {
    if (-not $runtimeRhiPackageStreamingSourceText.Contains($needle)) {
        Write-Error "runtime RHI package streaming source missing package upload staging evidence implementation text: $needle"
    }
}
foreach ($needle in @("runtime package upload staging evidence uses pooled async ring for selected package transactions", "runtime package resource update readiness publishes rows after upload fences are graphics ready", "evidence.package_transactions == 4", "evidence.ring_backed_uploads == 4", "evidence.graphics_waited_for_copy", "evidence.resource_updates_ready")) {
    if (-not $runtimeRhiTestsText.Contains($needle)) {
        Write-Error "MK_runtime_rhi_tests missing package upload staging evidence coverage: $needle"
    }
}
foreach ($needle in @("**Status:**", "Selected Package Upload Evidence v1", "execute_runtime_package_upload_staging_evidence", "--require-package-upload-staging")) {
    if (-not $runtimePackageUploadStagingEvidencePlanText.Contains($needle)) {
        Write-Error "Selected Package Upload Evidence plan missing text: $needle"
    }
}
foreach ($needle in @("validate_upload_gpu_batch_execution", "device.begin_command_list(queue)", "mark_pending_allocations_submitted(plan, ring, result.submitted_fence)")) {
    if (-not $rhiUploadStagingSourceText.Contains($needle)) {
        Write-Error "RHI upload staging source missing async execution implementation text: $needle"
    }
}
foreach ($needle in @("rhi upload async execution submits staged buffer batch without waiting", "rhi upload async execution rejects target mismatch before command list creation", "rhi upload async execution rejects unreserved staging before command list creation", "device.stats().fence_waits == 0", "device.stats().queue_waits == 0")) {
    if (-not $rhiUploadStagingTestsText.Contains($needle)) {
        Write-Error "MK_rhi_upload_staging_tests missing async execution coverage: $needle"
    }
}
foreach ($needle in @("**Status:** Completed.", "RHI Upload Stale Generation Diagnostics v1", "stale_generation", "native async upload execution")) {
    if (-not $rhiUploadStaleGenerationPlanText.Contains($needle)) {
        Write-Error "RHI Upload Stale Generation Diagnostics plan missing text: $needle"
    }
}
foreach ($needle in @(
        "SpriteBatchPlan",
        "SpriteBatchPlanDesc",
        "SpriteBatchPlanOptions",
        "SpriteBatchRange",
        "SpriteBatchDiagnosticCode",
        "SpriteBatchBudgetLane", "SpriteBatchBudgetDesc", "SpriteBatchBudgetProfile",
        "atlas_backed_batch_count", "repeated_atlas_batch_count", "repeated_atlas_sprite_count",
        "unsupported_reordering_policy", "untextured_sprite_disallowed",
        "plan_sprite_batches", "plan_sprite_batch_budget_profile"
    )) {
    if (-not $spriteBatchHeaderText.Contains($needle)) {
        Write-Error "2D sprite batch planning header missing contract text: $needle"
    }
}
foreach ($needle in @(
        "append_or_extend_batch",
        "missing_texture_atlas",
        "invalid_uv_rect",
        "allow_sprite_reordering",
        "require_atlas_backed_sprites",
        "texture_bind_count"
    )) {
    if (-not $spriteBatchSourceText.Contains($needle)) {
        Write-Error "2D sprite batch planning source missing contract text: $needle"
    }
}
foreach ($needle in @(
    "sprite batch planner preserves order",
    "sprite batch planner diagnoses invalid texture metadata",
    "SpriteBatchDiagnosticCode::missing_texture_atlas",
    "SpriteBatchDiagnosticCode::invalid_uv_rect"
)) {
    if (-not $rendererTestsText.Contains($needle)) {
        Write-Error "MK_renderer_tests missing 2D sprite batch planning coverage: $needle"
    }
}
foreach ($needle in @("scene sprite batch telemetry", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererTestsText.Contains($needle)) {
        Write-Error "MK_scene_renderer_tests missing 2D sprite batch telemetry coverage: $needle"
    }
}
if (-not $rendererCmakeText.Contains("src/sprite_batch.cpp")) {
    Write-Error "MK_renderer CMake missing sprite_batch.cpp"
}
foreach ($needle in @("mirakana/renderer/sprite_batch.hpp", "plan_scene_sprite_batches")) {
    if (-not $sceneRendererHeaderText.Contains($needle)) {
        Write-Error "MK_scene_renderer header missing 2D sprite batch telemetry contract: $needle"
    }
    if (-not $sceneRendererSourceText.Contains($needle)) {
        Write-Error "MK_scene_renderer source missing 2D sprite batch telemetry contract: $needle"
    }
}
foreach ($needle in @(
    "2d-sprite-batch-planning-contract",
    "2d-sprite-batch-package-telemetry",
    "Sprite Batching Renderer v1",
    "plan_sprite_batches", "SpriteBatchPlanDesc",
    "atlas_backed_batch_count", "unsupported_reordering_policy", "plan_scene_sprite_batches",
    "SpriteBatchBudgetProfile", "plan_sprite_batch_budget_profile", "sprite_batch_budget_status",
    "sprite_batch_plan_atlas_backed_batches",
    "sprite_batch_plan_repeated_atlas_batches",
    "sprite_batch_plan_repeated_atlas_sprites",
    "production sprite batching readiness",
    "native_sprite_batches_executed"
)) {
    if (-not $engineManifestText.Contains($needle)) {
        Write-Error "engine manifest missing sprite batch planning contract text: $needle"
    }
}
if ($engineManifestText.Contains("production sprite batching ready")) {
    Write-Error "engine manifest must not claim production sprite batching ready"
}

$sample3dManifestPath = "games/sample_desktop_runtime_game/game.agent.json"
$sample3dManifestEntry = Get-GameAgentManifest $sample3dManifestPath
if ($null -eq $sample3dManifestEntry) {
    Write-Error "3d-playable-desktop-package recipe must have a sample game manifest: $sample3dManifestPath"
} else {
    $sample3dManifest = $sample3dManifestEntry.Game
    if ($sample3dManifest.target -ne "sample_desktop_runtime_game") {
        Write-Error "$sample3dManifestPath target must be sample_desktop_runtime_game"
    }
    if ($sample3dManifest.gameplayContract.productionRecipe -ne "3d-playable-desktop-package") {
        Write-Error "$sample3dManifestPath gameplayContract.productionRecipe must be 3d-playable-desktop-package"
    }
    foreach ($module in @("MK_platform_win32", "MK_runtime", "MK_runtime_rhi", "MK_runtime_scene", "MK_runtime_scene_rhi", "MK_runtime_host", "MK_runtime_host_win32", "MK_runtime_host_win32_presentation", "MK_scene", "MK_scene_renderer", "MK_ui", "MK_ui_renderer", "MK_renderer")) {
        if (@($sample3dManifest.engineModules) -notcontains $module) {
            Write-Error "$sample3dManifestPath engineModules missing $module"
        }
    }
    foreach ($target in @("desktop-game-runtime", "desktop-runtime-release")) {
        if (@($sample3dManifest.packagingTargets) -notcontains $target) {
            Write-Error "$sample3dManifestPath packagingTargets missing $target"
        }
    }
    foreach ($packageFile in @("runtime/sample_desktop_runtime_game.config", "runtime/sample_desktop_runtime_game.geindex", "runtime/assets/desktop_runtime/base_color.texture.geasset", "runtime/assets/desktop_runtime/environment_presets.gepresetpack", "runtime/assets/desktop_runtime/environment_radiance_exr.texture.geasset", "runtime/assets/desktop_runtime/environment_skybox_basis.texture.geasset", "runtime/assets/desktop_runtime/hud.uiatlas", "runtime/assets/desktop_runtime/skinned_triangle.skinned_mesh", "runtime/assets/desktop_runtime/unlit.material", "runtime/assets/desktop_runtime/packaged_scene.scene")) {
        if (@($sample3dManifest.runtimePackageFiles) -notcontains $packageFile) {
            Write-Error "$sample3dManifestPath runtimePackageFiles missing $packageFile"
        }
    }
    if (-not $desktopRuntimeGameRegistrations.ContainsKey($sample3dManifest.target)) {
        Write-Error "$sample3dManifestPath target must be registered with MK_add_desktop_runtime_game"
    }
    $sample3dManifestText = $sample3dManifestEntry.Text
    foreach ($needle in @(
        "material instance intent",
        "camera/controller movement",
        "HUD diagnostics",
        "runtime source asset parsing remains unsupported",
        "material graph remains unsupported",
        "skeletal animation production path remains unsupported",
        "GPU skinning is host-proven on the D3D12 package smoke lane",
        "sample_desktop_runtime_game --require-gpu-skinning",
        "--require-job-scheduling-evidence",
        "job_scheduling_evidence_status=ready",
        "job_scheduling_evidence_native_threads_started=0",
        "job_scheduling_evidence_thread_pool_started=0",
        "--require-job-execution-foundation", "job_execution_foundation_status=ready", "job_execution_foundation_worker_threads_started=2", "job_execution_foundation_task_side_effects=3", "--require-job-execution-topology-policy", "job_execution_topology_policy_status=ready", "job_execution_topology_policy_selected_worker_count=2",
        "--require-simd-dispatch-policy", "simd_dispatch_policy_status=ready", "simd_dispatch_policy_requested_lane=auto_select", "simd_dispatch_policy_dot_product_result=120", "simd_dispatch_policy_reviewed_avx2_target_available", "gated simd_dispatch_policy_avx2_selected",
        "package streaming remains unsupported",
        "native GPU runtime UI overlay",
        "textured UI sprite atlas",
        "GameEngine.EnvironmentPresetPack.v1",
        "--require-environment-preset-library-package", "environment_preset_library_package_status=ready",
        "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-d3d12-upload", "--require-environment-texture-asset-pipeline-vulkan-upload", "--require-environment-texture-asset-pipeline-d3d12-compressed-upload", "--require-environment-texture-asset-pipeline-vulkan-compressed-upload", "--require-environment-texture-asset-pipeline-metal-compressed-upload", "environment_texture_asset_pipeline_package_status=ready", "environment_texture_asset_pipeline_upload_plan_ready_records=1", "environment_texture_asset_pipeline_upload_plan_gpu_upload_invoked=0", "environment_texture_asset_pipeline_d3d12_upload_ready=1", "environment_texture_asset_pipeline_d3d12_upload_backend_api_invoked=1", "environment_texture_asset_pipeline_d3d12_upload_gpu_upload_invoked=1", "environment_texture_asset_pipeline_d3d12_upload_readback_invoked=1", "environment_texture_asset_pipeline_d3d12_upload_checksum_matched=1", "environment_texture_asset_pipeline_d3d12_upload_descriptor_bound=1", "environment_texture_asset_pipeline_d3d12_upload_row_pitch_bytes=256", "environment_texture_asset_pipeline_d3d12_upload_uploaded_bytes=256", "environment_texture_asset_pipeline_d3d12_upload_readback_bytes=256", "environment_texture_asset_pipeline_d3d12_upload_native_handle_access=0", "environment_texture_asset_pipeline_d3d12_upload_backend_parity_ready=0", "environment_texture_asset_pipeline_d3d12_upload_broad_ready=0", "environment_texture_asset_pipeline_vulkan_upload_ready=1", "environment_texture_asset_pipeline_vulkan_upload_strict_ready=1", "environment_texture_asset_pipeline_vulkan_upload_backend_api_invoked=1", "environment_texture_asset_pipeline_vulkan_upload_gpu_upload_invoked=1", "environment_texture_asset_pipeline_vulkan_upload_readback_invoked=1", "environment_texture_asset_pipeline_vulkan_upload_checksum_matched=1", "environment_texture_asset_pipeline_vulkan_upload_descriptor_bound=1", "environment_texture_asset_pipeline_vulkan_upload_row_pitch_bytes=256", "environment_texture_asset_pipeline_vulkan_upload_uploaded_bytes=256", "environment_texture_asset_pipeline_vulkan_upload_readback_bytes=256", "environment_texture_asset_pipeline_vulkan_upload_native_handle_access=0", "environment_texture_asset_pipeline_vulkan_upload_metal_host_ready=0", "environment_texture_asset_pipeline_vulkan_upload_backend_parity_ready=0", "environment_texture_asset_pipeline_vulkan_upload_broad_ready=0", "environment_texture_asset_pipeline_vulkan_compressed_upload_ready=1", "environment_texture_asset_pipeline_vulkan_compressed_upload_backend_format_support_proven=1", "environment_texture_asset_pipeline_vulkan_compressed_upload_format_support_queries", "environment_texture_asset_pipeline_vulkan_compressed_upload_native_handle_access=0", "environment_texture_asset_pipeline_vulkan_compressed_upload_metal_host_ready=0", "environment_texture_asset_pipeline_vulkan_compressed_upload_backend_parity_ready=0", "environment_texture_asset_pipeline_vulkan_compressed_upload_broad_ready=0", "environment_texture_asset_pipeline_metal_compressed_upload_ready=1", "environment_texture_asset_pipeline_metal_compressed_upload_backend_format_support_proven=1", "environment_texture_asset_pipeline_metal_compressed_upload_texture_usage_shader_resource=1", "environment_texture_asset_pipeline_metal_compressed_upload_texture_usage_copy_source=1", "environment_texture_asset_pipeline_metal_compressed_upload_metal_host_ready=1", "environment_texture_asset_pipeline_metal_compressed_upload_native_handle_access=0", "environment_texture_asset_pipeline_metal_compressed_upload_strict_vulkan_ready=0", "environment_texture_asset_pipeline_metal_compressed_upload_backend_parity_ready=0", "environment_texture_asset_pipeline_metal_compressed_upload_broad_ready=0", "desktop-runtime-sample-game-environment-vulkan-strict-aggregate", "--require-environment-vulkan-strict-aggregate", "environment_vulkan_strict_aggregate_status=ready", "environment_vulkan_strict_aggregate_descriptor_set_bindings=15",
        "desktop-runtime-sample-game-environment-backend-parity", "--require-environment-backend-parity", "environment_backend_parity_status=host_evidence_required", "environment_backend_parity_ready=0", "environment_backend_parity_required_backends=3", "desktop-runtime-sample-game-environment-backend-parity-ready", "--require-environment-backend-parity-ready", "environment_backend_parity_status=ready", "environment_backend_parity_ready_closeout_requested=1", "environment_backend_parity_ready=1", "environment_backend_parity_ready_rows=21", "environment_backend_parity_host_gated_rows=0", "environment_backend_parity_host_validated_backends=3", "environment_backend_parity_metal_evidence_consumed=1", "environment_backend_parity_cross_host_aggregate_ready=1",
        "desktop-runtime-sample-game-environment-platform-readiness", "--require-environment-platform-readiness", "environment_platform_readiness_status=host_evidence_required", "environment_platform_readiness_ready=0", "environment_platform_windows_d3d12_ready=1", "environment_platform_windows_vulkan_ready=0", "environment_all_platform_unconditional_ready=0", "desktop-runtime-sample-game-environment-platform-windows-vulkan-evidence", "--require-environment-platform-windows-vulkan-evidence", "environment_platform_windows_vulkan_evidence_requested=1", "environment_platform_windows_vulkan_strict_aggregate_ready=1", "environment_platform_windows_vulkan_ready=1", "environment_platform_requires_windows_vulkan_host_evidence=0", "desktop-runtime-sample-game-environment-optimization-measurement", "--require-environment-optimization-measurement", "environment_optimization_measurement_status=host_evidence_required", "environment_optimization_measurement_required_workloads=7", "environment_broad_optimization_ready=0", "desktop-runtime-sample-game-environment-weather-simulation-package", "--require-environment-weather-simulation-package", "environment_weather_simulation_package_status=ready", "environment_weather_simulation_package_ready=1", "environment_weather_simulation_solver_budget_status=host_evidence_required", "environment_weather_simulation_cpu_reference_solver_ready=1", "environment_weather_simulation_production_solver_package_counter_review_ready=1", "environment_weather_simulation_production_solver_package_counter_rows=1", "environment_weather_simulation_production_solver_core_review_ready=1", "environment_weather_simulation_production_solver_core_rows=1", "environment_weather_simulation_production_solver_ready=0", "environment_weather_simulation_validation_dataset_status=ready", "environment_weather_simulation_validation_dataset_ready=1", "environment_weather_simulation_validation_image_status=ready", "environment_weather_simulation_validation_images_ready=1", "environment_weather_simulation_artist_control_status=ready", "environment_weather_simulation_artist_controls_ready=1", "environment_physical_weather_simulation_ready=0",
        "payload texture decode and Basis transcode evidence", "one selected RGBA8 upload-plan-ready payload", "environment_texture_asset_pipeline_upload_plan_runtime_codec_invoked=0", "environment_texture_asset_pipeline_upload_plan_runtime_basis_transcode_invoked=0", "environment_texture_asset_pipeline_upload_plan_backend_api_invoked=0", "environment_texture_asset_pipeline_upload_plan_broad_ready=0",
        "zero upload-plan backend/GPU/runtime codec side effects or broad asset-pipeline ready counters",
        "production text/font/image/atlas/accessibility remains unsupported", "public native or RHI handle access remains unsupported", "general production renderer quality remains unsupported"
    )) {
        if (-not $sample3dManifestText.Contains($needle)) {
            Write-Error "$sample3dManifestPath missing 3D boundary text: $needle"
        }
    }
    if ($sample3dManifestText.Contains("native GPU HUD or sprite overlay output remains unsupported")) { Write-Error "$sample3dManifestPath keeps a stale native GPU HUD or sprite overlay unsupported claim" }
    $sample3dUiAtlasPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas"; $sample3dUiAtlasText = Get-Content -LiteralPath $sample3dUiAtlasPath -Raw
    foreach ($needle in @("format=GameEngine.UiAtlas.v1", "source.decoding=unsupported", "atlas.packing=unsupported", "page.count=1", "image.count=1")) {
        if (-not $sample3dUiAtlasText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/hud.uiatlas missing cooked UI atlas metadata text: $needle"
        }
    }
    $sample3dIndexPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex"; $sample3dIndexText = Get-Content -LiteralPath $sample3dIndexPath -Raw
    foreach ($needle in @("kind=ui_atlas", "kind=ui_atlas_texture", "kind=environment_texture", "kind=environment_preset_pack")) {
        if (-not $sample3dIndexText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/sample_desktop_runtime_game.geindex missing sample 3D package row: $needle"
        }
    }
    $sample3dEnvironmentPresetPackPath = Join-Path $root "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/environment_presets.gepresetpack"
    $sample3dEnvironmentPresetPackText = Get-Content -LiteralPath $sample3dEnvironmentPresetPackPath -Raw
    foreach ($needle in @("format=GameEngine.EnvironmentPresetPack.v1", "pack.provenance_id=provenance.environment.sample_commercial_presets", "pack.license_id=LicenseRef-Proprietary", "pack.package_size_budget_bytes=12000", "pack.installed_size_budget_bytes=12000", "pack.decoded_memory_budget_bytes=56000", "pack.gpu_memory_budget_bytes=44000", "pack.required_backend_feature_row.0=environment_platform_windows_d3d12_ready", "preset.0.id=clear_noon", "preset.1.id=overcast_storm", "preset.2.id=night_moonlit", "preset.3.id=snowfield", "preset.4.id=foggy_valley", "preset.5.id=cinematic_sunset", "preset.6.id=indoor_to_outdoor_transition")) {
        if (-not $sample3dEnvironmentPresetPackText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/runtime/assets/desktop_runtime/environment_presets.gepresetpack missing preset governance text: $needle"
        }
    }
    $sample3dMainPath = Join-Path $root "games/sample_desktop_runtime_game/main.cpp"
    $sample3dMainText = Get-Content -LiteralPath $sample3dMainPath -Raw
    foreach ($needle in @(
        "mirakana/ui/ui.hpp",
        "mirakana/ui_renderer/ui_renderer.hpp",
        "--require-native-ui-overlay",
        "--require-native-ui-textured-sprite-atlas",
        "--require-environment-texture-asset-pipeline-package", "--require-environment-texture-asset-pipeline-d3d12-upload", "--require-environment-texture-asset-pipeline-vulkan-upload", "--require-environment-preset-library-package", "--require-environment-vulkan-strict-aggregate", "--require-environment-backend-parity-ready", "--require-environment-platform-readiness", "--require-environment-platform-windows-vulkan-evidence", "--require-environment-optimization-measurement", "--require-environment-weather-simulation-package",
        "environment_preset_library_package_status=", "environment_preset_library_package_ready=",
        "environment_preset_library_package_requested=", "environment_preset_library_package_index_entry=",
        "environment_preset_library_package_file=", "environment_preset_library_preset_count=",
        "environment_preset_library_required_preset_rows=", "environment_preset_library_backend_feature_rows=",
        "environment_preset_library_sample_consumption_evidence=", "environment_preset_library_aaa_ready_claimed=",
        "environment_aaa_preset_library_ready=",
        "environment_texture_asset_pipeline_package_status=",
        "environment_texture_asset_pipeline_backend_policy_rows=", "environment_texture_asset_pipeline_payload_records=", "environment_texture_asset_pipeline_payload_byte_rows=", "environment_texture_asset_pipeline_payload_hash_rows=", "environment_texture_asset_pipeline_upload_plan_records=", "environment_texture_asset_pipeline_upload_plan_ready_records=", "environment_texture_asset_pipeline_upload_plan_diagnostics=", "environment_texture_asset_pipeline_upload_plan_payload_bytes=", "environment_texture_asset_pipeline_upload_plan_source_bytes=", "environment_texture_asset_pipeline_upload_plan_runtime_codec_invoked=", "environment_texture_asset_pipeline_upload_plan_runtime_basis_transcode_invoked=", "environment_texture_asset_pipeline_upload_plan_backend_api_invoked=", "environment_texture_asset_pipeline_upload_plan_gpu_upload_invoked=", "environment_texture_asset_pipeline_upload_plan_broad_ready=", "environment_texture_asset_pipeline_d3d12_upload_ready=", "environment_texture_asset_pipeline_d3d12_upload_backend_api_invoked=", "environment_texture_asset_pipeline_d3d12_upload_gpu_upload_invoked=", "environment_texture_asset_pipeline_d3d12_upload_readback_invoked=", "environment_texture_asset_pipeline_d3d12_upload_checksum_matched=", "environment_texture_asset_pipeline_d3d12_upload_descriptor_bound=", "environment_texture_asset_pipeline_d3d12_upload_source_row_bytes=", "environment_texture_asset_pipeline_d3d12_upload_row_pitch_bytes=", "environment_texture_asset_pipeline_d3d12_upload_uploaded_bytes=", "environment_texture_asset_pipeline_d3d12_upload_readback_bytes=", "environment_texture_asset_pipeline_d3d12_upload_compact_readback_bytes=", "environment_texture_asset_pipeline_d3d12_upload_descriptor_writes=", "environment_texture_asset_pipeline_d3d12_upload_resource_transitions=", "environment_texture_asset_pipeline_d3d12_upload_copy_to_texture_count=", "environment_texture_asset_pipeline_d3d12_upload_copy_to_readback_count=", "environment_texture_asset_pipeline_d3d12_upload_native_handle_access=", "environment_texture_asset_pipeline_d3d12_upload_backend_parity_ready=", "environment_texture_asset_pipeline_d3d12_upload_broad_ready=", "environment_texture_asset_pipeline_d3d12_upload_diagnostics=", "environment_vulkan_strict_aggregate_status=", "environment_vulkan_strict_aggregate_ready=", "environment_vulkan_strict_aggregate_descriptor_set_bindings=", "environment_vulkan_strict_aggregate_synchronization2_barriers=", "environment_vulkan_strict_aggregate_diagnostics=", "environment_backend_parity_status=", "environment_backend_parity_ready=", "environment_backend_parity_required_backends=", "environment_backend_parity_ready_rows=", "environment_backend_parity_host_gated_rows=", "environment_backend_parity_diagnostics=", "environment_backend_parity_all_platform_ready=", "environment_backend_parity_commercial_ready=", "environment_platform_readiness_status=", "environment_platform_readiness_ready=", "environment_platform_windows_vulkan_evidence_requested=", "environment_platform_windows_vulkan_strict_aggregate_ready=", "environment_platform_readiness_rows=", "environment_platform_readiness_host_gated_rows=", "environment_platform_windows_d3d12_ready=", "environment_platform_windows_vulkan_ready=", "environment_platform_linux_vulkan_ready=", "environment_platform_macos_metal_ready=", "environment_platform_ios_metal_ready=", "environment_platform_android_vulkan_ready=", "environment_all_platform_unconditional_ready=", "environment_platform_diagnostics=", "environment_optimization_measurement_status=", "environment_optimization_measurement_required_workloads=", "environment_optimization_measurement_before_after_pairs=", "environment_broad_optimization_ready=", "environment_weather_simulation_package_status=", "environment_weather_simulation_package_ready=", "environment_weather_simulation_steps=", "environment_weather_simulation_cells=", "environment_weather_simulation_effective_timestep_ms=", "environment_weather_simulation_water_conservation_error_mg=", "environment_weather_simulation_water_conservation_error_bound_mg=", "environment_weather_simulation_fallback_cpu_reference_used=", "environment_weather_simulation_invokes_gpu=", "environment_weather_simulation_invokes_backend=", "environment_weather_simulation_native_handle_access=", "environment_weather_simulation_solver_budget_status=", "environment_weather_simulation_solver_budget_ready=", "environment_weather_simulation_cpu_reference_solver_ready=", "environment_weather_simulation_solver_cpu_elapsed_us=", "environment_weather_simulation_solver_cpu_budget_us=", "environment_weather_simulation_solver_cpu_over_budget=", "environment_weather_simulation_gpu_solver_ready=", "environment_weather_simulation_solver_gpu_elapsed_us=", "environment_weather_simulation_solver_gpu_budget_us=", "environment_weather_simulation_d3d12_gpu_solver_ready=", "environment_weather_simulation_d3d12_gpu_solver_cells=", "environment_weather_simulation_d3d12_gpu_solver_dispatches=", "environment_weather_simulation_d3d12_gpu_solver_barriers=", "environment_weather_simulation_d3d12_gpu_solver_native_handle_access=", "environment_weather_simulation_d3d12_gpu_solver_backend_parity_ready=", "environment_weather_simulation_d3d12_gpu_solver_failure_stage=", "environment_weather_simulation_d3d12_gpu_solver_hash=", "environment_weather_simulation_solver_profiler_artifacts=", "environment_weather_simulation_solver_profiler_tool_rows=", "environment_weather_simulation_solver_profiler_backend_rows=", "environment_weather_simulation_solver_profiler_artifact_hash=", "environment_weather_simulation_profiler_budget_ready=", "environment_weather_simulation_production_solver_package_counter_review_ready=", "environment_weather_simulation_production_solver_package_counter_rows=", "environment_weather_simulation_production_solver_core_review_ready=", "environment_weather_simulation_production_solver_core_rows=", "environment_weather_simulation_production_solver_ready=", "environment_weather_simulation_validation_dataset_status=", "environment_weather_simulation_validation_dataset_ready=", "environment_weather_simulation_validation_case_rows=", "environment_weather_simulation_validation_required_cases=", "environment_weather_simulation_validation_ready_cases=", "environment_weather_simulation_validation_supersaturated_condensation_ready=", "environment_weather_simulation_validation_forced_evaporation_precipitation_ready=", "environment_weather_simulation_validation_clamped_mixed_grid_ready=", "environment_weather_simulation_validation_image_status=", "environment_weather_simulation_validation_images_ready=", "environment_weather_simulation_validation_image_rows=", "environment_weather_simulation_validation_required_images=", "environment_weather_simulation_validation_supersaturated_condensation_images_ready=", "environment_weather_simulation_validation_forced_evaporation_precipitation_images_ready=", "environment_weather_simulation_validation_clamped_mixed_grid_images_ready=", "environment_weather_simulation_validation_max_water_error_mg=", "environment_weather_simulation_validation_water_error_bound_mg=", "environment_weather_simulation_validation_diagnostics=", "environment_weather_simulation_validation_image_diagnostics=", "environment_weather_simulation_validation_dataset_hash=", "environment_weather_simulation_validation_image_hash=", "environment_weather_simulation_artist_control_status=", "environment_weather_simulation_artist_controls_ready=", "environment_weather_simulation_artist_control_rows=", "environment_weather_simulation_artist_control_generated_cells=", "environment_weather_simulation_artist_control_raw_solver_internal_access=", "environment_weather_simulation_artist_control_native_handle_access=", "environment_weather_simulation_artist_control_invokes_gpu=", "environment_weather_simulation_artist_control_invokes_backend=", "environment_weather_simulation_artist_control_physical_weather_ready=", "environment_weather_simulation_artist_control_diagnostics=", "environment_weather_simulation_artist_control_hash=", "environment_physical_weather_simulation_ready=",
        "plan_scene_mesh_draws",
        "camera_primary=",
        "camera_controller_ticks=",
        "scene_mesh_plan_draws=",
        "scene_mesh_plan_unique_materials=",
        "scene_mesh_plan_diagnostics=",
        "hud_boxes=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "ui_texture_overlay_requested=",
        "ui_texture_overlay_status=",
        "ui_texture_overlay_atlas_ready=",
        "ui_texture_overlay_sprites_submitted=",
        "ui_texture_overlay_texture_binds=",
        "ui_texture_overlay_draws=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_pages=",
        "ui_atlas_metadata_bindings=",
        "--require-renderer-quality-gates",
        "renderer_quality_status=",
        "renderer_quality_ready=",
        "renderer_quality_diagnostics=",
        "renderer_quality_expected_framegraph_passes=",
        "renderer_quality_expected_framegraph_barrier_steps=",
        "renderer_quality_framegraph_passes_ok=",
        "renderer_quality_framegraph_barrier_steps_ok=",
        "renderer_quality_framegraph_execution_budget_ok=",
        "renderer_quality_scene_gpu_ready=",
        "renderer_quality_postprocess_ready=",
        "renderer_quality_postprocess_depth_input_ready=",
        "renderer_quality_directional_shadow_ready=",
        "renderer_quality_directional_shadow_filter_ready=",
        "--require-d3d12-shadow-cascade-policy",
        "directional_shadow_cascade_count=",
        "directional_shadow_cascade_tile_width=",
        "directional_shadow_atlas_width=",
        "directional_shadow_atlas_height=",
        "directional_shadow_light_space_cascades=",
        "directional_shadow_cascade_splits=",
        "--require-lighting-shadow-policy",
        "lighting_shadow_policy_status=",
        "lighting_shadow_policy_ready=",
        "lighting_shadow_policy_diagnostics=",
        "lighting_shadow_policy_lights=",
        "lighting_shadow_policy_directional_lights=",
        "lighting_shadow_policy_point_lights=",
        "lighting_shadow_policy_spot_lights=",
        "lighting_shadow_policy_shadowed_lights=",
        "lighting_shadow_policy_directional_cascades=",
        "lighting_shadow_policy_shadow_atlas_width=",
        "lighting_shadow_policy_shadow_atlas_height=",
        "lighting_shadow_policy_light_rows=",
        "lighting_shadow_policy_ready_frames=",
        "framegraph_passes_executed=",
        "framegraph_barrier_steps_executed=",
        "--require-framegraph-multiqueue-evidence",
        "--require-job-scheduling-evidence", "--require-job-execution-foundation", "--require-job-execution-topology-policy", "--require-simd-dispatch-policy",
        "job_scheduling_evidence_status=", "job_scheduling_evidence_ready=", "job_scheduling_evidence_worker_count=", "job_scheduling_evidence_queue_rows=", "job_scheduling_evidence_submitted_jobs=", "job_scheduling_evidence_completed_jobs=", "job_scheduling_evidence_execution_rows=", "job_scheduling_evidence_deterministic_merges=",
        "job_execution_foundation_status=", "job_execution_foundation_ready=", "job_execution_foundation_worker_threads_started=", "job_execution_foundation_tasks_submitted=", "job_execution_foundation_tasks_executed=", "job_execution_foundation_task_side_effects=", "job_execution_foundation_deterministic_merges=", "job_execution_topology_policy_status=", "job_execution_topology_policy_ready=", "job_execution_topology_policy_selected_worker_count=", "job_execution_topology_policy_worker_count_limit=", "job_execution_topology_policy_reserved_logical_processors=",
        "simd_dispatch_policy_status=", "simd_dispatch_policy_ready=", "simd_dispatch_policy_requested_lane=", "simd_dispatch_policy_selected_lane=", "simd_dispatch_policy_dot_product_result=", "simd_dispatch_policy_span_inputs_used=", "simd_dispatch_policy_raw_pointers_retained=", "simd_dispatch_policy_reviewed_avx2_target_available=", "simd_dispatch_policy_avx2_selected=", "simd_dispatch_policy_cuda_path_used=", "simd_dispatch_policy_hip_path_used=", "simd_dispatch_policy_sycl_path_used=",
        "framegraph_multiqueue_command_lists_submitted=",
        "framegraph_multiqueue_queue_waits_recorded=",
        "framegraph_multiqueue_barriers_recorded=",
        "framegraph_multiqueue_aliasing_barriers_recorded=",
        "framegraph_multiqueue_pass_callbacks_invoked=",
        "framegraph_multiqueue_submitted_pass_fences=",
        "framegraph_multiqueue_graphics_waited_for_copy=",
        "kRuntimeSceneVulkanComputeMappingShaderPath",
        "sample_desktop_runtime_game_scene_mapping.cs.spv",
        "cs_vulkan_mapping_proof",
        "primary_camera_seen_",
        "hud_boxes_submitted_"
    )) {
        if (-not $sample3dMainText.Contains($needle)) {
            Write-Error "games/sample_desktop_runtime_game/main.cpp missing 3D smoke field or HUD contract: $needle"
        }
    }
    $sample3dCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw; foreach ($needle in @("MK_SAMPLE_DESKTOP_RUNTIME_GAME_VULKAN_COMPUTE_MAPPING_SHADER_OUTPUT", "sample_desktop_runtime_game_scene_mapping.cs.spv", "-E cs_vulkan_mapping_proof -T cs_6_0 -spirv -fspv-target-env=vulkan1.3")) { if (-not $sample3dCMakeText.Contains($needle)) { Write-Error "games/CMakeLists.txt missing sample_desktop_runtime_game Vulkan compute mapping proof: $needle" } }; foreach ($needle in @("--require-job-scheduling-evidence", "--require-job-execution-foundation", "--require-job-execution-topology-policy", "--require-simd-dispatch-policy", "sample_desktop_runtime_game")) { if (-not $sample3dCMakeText.Contains($needle)) { Write-Error "games/CMakeLists.txt missing sample_desktop_runtime_game job scheduling/execution/SIMD smoke evidence: $needle" } }
    $sample3dInstalledValidatorText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($needle in @('$requiresJobSchedulingEvidence', '"job_scheduling_evidence_status" = "ready"', '"job_scheduling_evidence_ready" = "1"', '"job_scheduling_evidence_worker_count" = "2"', '"job_scheduling_evidence_queue_rows" = "2"', '"job_scheduling_evidence_submitted_jobs" = "3"', '"job_scheduling_evidence_completed_jobs" = "3"', '"job_scheduling_evidence_execution_rows" = "3"', '"job_scheduling_evidence_deterministic_merges" = "3"', '"job_scheduling_evidence_native_threads_started" = "0"', '"job_scheduling_evidence_thread_pool_started" = "0"', '"job_scheduling_evidence_affinity_policy_applied" = "0"', '"job_scheduling_evidence_numa_policy_applied" = "0"', '"job_scheduling_evidence_simd_dispatch_applied" = "0"', '"job_scheduling_evidence_gpu_async_overlap_applied" = "0"')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing job scheduling evidence assertion: $needle" } }
    foreach ($needle in @('$requiresJobExecutionFoundation', '$requiresJobExecutionTopologyPolicy', '"job_execution_foundation_status" = "ready"', '"job_execution_foundation_ready" = "1"', '"job_execution_foundation_worker_threads_started" = "2"', '"job_execution_foundation_tasks_submitted" = "3"', '"job_execution_foundation_tasks_executed" = "3"', '"job_execution_foundation_task_side_effects" = "3"', '"job_execution_foundation_deterministic_merges" = "3"', '"job_execution_foundation_work_stealing_applied" = "0"', '"job_execution_foundation_affinity_policy_applied" = "0"', '"job_execution_foundation_numa_policy_applied" = "0"', '"job_execution_foundation_simd_dispatch_applied" = "0"', '"job_execution_foundation_gpu_async_overlap_applied" = "0"', '"job_execution_foundation_cuda_path_used" = "0"', '"job_execution_foundation_hip_path_used" = "0"', '"job_execution_foundation_sycl_path_used" = "0"', '"job_execution_topology_policy_status" = "ready"', '"job_execution_topology_policy_ready" = "1"', '"job_execution_topology_policy_selected_worker_count" = "2"', '"job_execution_topology_policy_worker_count_limit" = "2"', '"job_execution_topology_policy_reserved_logical_processors" = "1"', '"job_execution_topology_policy_affinity_policy_applied" = "0"', '"job_execution_topology_policy_numa_policy_applied" = "0"', '"job_execution_topology_policy_simd_dispatch_applied" = "0"', '"job_execution_topology_policy_gpu_async_overlap_applied" = "0"', '"job_execution_topology_policy_cuda_path_used" = "0"', '"job_execution_topology_policy_hip_path_used" = "0"', '"job_execution_topology_policy_sycl_path_used" = "0"')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing job execution foundation/topology assertion: $needle" } }
    foreach ($needle in @('$requiresSimdDispatchPolicy', '"simd_dispatch_policy_status" = "ready"', '"simd_dispatch_policy_ready" = "1"', '"simd_dispatch_policy_requested_lane" = "auto_select"', '"simd_dispatch_policy_input_count" = "8"', '"simd_dispatch_policy_dot_product_result" = "120"', '"simd_dispatch_policy_reviewed_avx2_target_available"', '"simd_dispatch_policy_avx2_selected"', '"simd_dispatch_policy_span_inputs_used" = "1"', '"simd_dispatch_policy_raw_pointers_retained" = "0"', '"simd_dispatch_policy_native_handles_exposed" = "0"', '"simd_dispatch_policy_numa_allocation_applied" = "0"', '"simd_dispatch_policy_gpu_async_overlap_applied" = "0"', '"simd_dispatch_policy_cuda_path_used" = "0"', '"simd_dispatch_policy_hip_path_used" = "0"', '"simd_dispatch_policy_sycl_path_used" = "0"', 'scalar|sse2|avx2')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing SIMD dispatch policy assertion: $needle" } }
    foreach ($needle in @('$requiresEnvironmentVulkanStrictAggregate', '"environment_vulkan_strict_aggregate_status" = "ready"', '"environment_vulkan_strict_aggregate_ready" = "1"', '"environment_vulkan_strict_aggregate_descriptor_set_bindings" = "15"', '"environment_vulkan_strict_aggregate_diagnostics" = "0"')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing environment Vulkan strict aggregate assertion: $needle" } }
    foreach ($needle in @('$requiresEnvironmentBackendParity', '$requiresEnvironmentBackendParityReady', '$environmentBackendParityExpectedStatus', '$environmentBackendParityExpectedReady', '$environmentBackendParityExpectedReadyRows', '$environmentBackendParityExpectedHostGatedRows', '$environmentBackendParityExpectedHostValidatedBackends', '$environmentBackendParityExpectedMetalHost', '$environmentBackendParityExpectedMetalEvidenceConsumed', '$environmentBackendParityExpectedCrossHostAggregateReady', '$environmentBackendParityExpectedReadyCloseoutRequested', '"environment_backend_parity_status" = $environmentBackendParityExpectedStatus', '"environment_backend_parity_ready_closeout_requested" = $environmentBackendParityExpectedReadyCloseoutRequested', '"environment_backend_parity_ready" = $environmentBackendParityExpectedReady', '"environment_backend_parity_required_backends" = "3"', '"environment_backend_parity_required_features" = "7"', '"environment_backend_parity_ready_rows" = $environmentBackendParityExpectedReadyRows', '"environment_backend_parity_host_gated_rows" = $environmentBackendParityExpectedHostGatedRows', '"environment_backend_parity_host_validated_backends" = $environmentBackendParityExpectedHostValidatedBackends', '"environment_backend_parity_d3d12_primary" = "1"', '"environment_backend_parity_vulkan_strict" = "1"', '"environment_backend_parity_metal_host" = $environmentBackendParityExpectedMetalHost', '"environment_backend_parity_metal_evidence_consumed" = $environmentBackendParityExpectedMetalEvidenceConsumed', '"environment_backend_parity_requires_metal_host_evidence" = "1"', '"environment_backend_parity_cross_host_aggregate_ready" = $environmentBackendParityExpectedCrossHostAggregateReady', '"environment_backend_parity_d3d12_inferred" = "0"', '"environment_backend_parity_vulkan_inferred" = "0"', '"environment_backend_parity_metal_inferred" = "0"', '"environment_backend_parity_diagnostics" = "0"', 'environment_backend_parity_replay_hash=[1-9]\d*')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing environment backend parity assertion: $needle" } }
    foreach ($needle in @('$requiresEnvironmentPlatformReadiness', '$requiresEnvironmentPlatformWindowsVulkanEvidence', '"environment_platform_readiness_status" = "host_evidence_required"', '"environment_platform_readiness_ready" = "0"', '"environment_platform_windows_vulkan_evidence_requested" = "0"', '"environment_platform_windows_vulkan_evidence_requested"] = "1"', '"environment_platform_windows_vulkan_strict_aggregate_ready" = "0"', '"environment_platform_windows_vulkan_strict_aggregate_ready"] = "1"', '"environment_platform_readiness_rows" = "6"', '"environment_platform_readiness_ready_rows" = "1"', '"environment_platform_readiness_host_gated_rows" = "5"', '"environment_platform_windows_d3d12_ready" = "1"', '"environment_platform_windows_d3d12_ready"] = "0"', '"environment_platform_windows_vulkan_ready" = "0"', '"environment_platform_windows_vulkan_ready"] = "1"', '"environment_platform_requires_windows_vulkan_host_evidence" = "1"', '"environment_platform_requires_windows_vulkan_host_evidence"] = "0"', '"environment_platform_linux_vulkan_ready" = "0"', '"environment_platform_macos_metal_ready" = "0"', '"environment_platform_ios_metal_ready" = "0"', '"environment_platform_android_vulkan_ready" = "0"', '"environment_all_platform_unconditional_ready" = "0"', '"environment_platform_diagnostics" = "0"', 'environment_platform_readiness_replay_hash=[1-9]\d*')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing environment platform readiness assertion: $needle" } }
    foreach ($needle in @('$requiresEnvironmentOptimizationMeasurement', '"environment_optimization_measurement_status" = "host_evidence_required"', '"environment_optimization_measurement_ready" = "0"', '"environment_optimization_measurement_workload_rows" = "7"', '"environment_optimization_measurement_required_workloads" = "7"', '"environment_optimization_measurement_measured_workloads" = "7"', '"environment_optimization_measurement_before_after_pairs" = "7"', '"environment_optimization_measurement_backend" = "d3d12"', '"environment_optimization_measurement_profile" = "preset_pack_flythrough"', '"environment_optimization_measurement_profiles" = "preset_pack_flythrough,storm_precipitation,dense_volumetric_fog,volumetric_cloud_sunset,snowfield_material_weathering,weather_simulation_stress,asset_library_cold_load"', '"environment_optimization_preset_pack_flythrough_ready" = "1"', '"environment_optimization_storm_precipitation_ready" = "1"', '"environment_optimization_dense_volumetric_fog_ready" = "1"', '"environment_optimization_volumetric_cloud_sunset_ready" = "1"', '"environment_optimization_snowfield_material_weathering_ready" = "1"', '"environment_optimization_weather_simulation_stress_ready" = "1"', '"environment_optimization_asset_library_cold_load_ready" = "1"', '"environment_optimization_storm_precipitation_cpu_frame_p95_before_us" = "18200"', '"environment_optimization_storm_precipitation_gpu_frame_p95_after_us" = "15800"', '"environment_optimization_dense_volumetric_fog_cpu_frame_p95_before_us" = "17600"', '"environment_optimization_dense_volumetric_fog_gpu_frame_p95_after_us" = "15000"', '"environment_optimization_volumetric_cloud_sunset_cpu_frame_p95_before_us" = "18800"', '"environment_optimization_volumetric_cloud_sunset_gpu_frame_p95_after_us" = "16200"', '"environment_optimization_snowfield_material_weathering_cpu_frame_p95_before_us" = "16900"', '"environment_optimization_snowfield_material_weathering_gpu_frame_p95_after_us" = "14600"', '"environment_optimization_weather_simulation_stress_cpu_frame_p95_before_us" = "19600"', '"environment_optimization_weather_simulation_stress_gpu_frame_p95_after_us" = "16900"', '"environment_optimization_asset_library_cold_load_package_load_before_us" = "98000"', '"environment_optimization_asset_library_cold_load_package_load_after_us" = "78000"', '"environment_optimization_asset_library_cold_load_texture_residency_before_bytes" = "939524096"', '"environment_optimization_asset_library_cold_load_texture_residency_after_bytes" = "872415232"', '"environment_optimization_measurement_regression_budget_rows" = "7"', '"environment_optimization_measurement_over_budget" = "0"', '"environment_broad_optimization_ready" = "0"', '"environment_optimization_measurement_diagnostics" = "0"', 'environment_optimization_measurement_replay_hash=[1-9]\d*')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing environment optimization measurement assertion: $needle" } }
    foreach ($needle in @('$requiresEnvironmentWeatherSimulationPackage', '"environment_weather_simulation_package_status" = "ready"', '"environment_weather_simulation_package_ready" = "1"', '"environment_weather_simulation_steps" = "1"', '"environment_weather_simulation_cells" = "4"', '"environment_weather_simulation_effective_timestep_ms" = "500"', '"environment_weather_simulation_timestep_clamped" = "1"', '"environment_weather_simulation_water_conservation_error_bound_mg" = "1"', 'environment_weather_simulation_water_conservation_error_mg=[01]', '"environment_weather_simulation_fallback_cpu_reference_used" = "1"', '"environment_weather_simulation_invokes_gpu" = "0"', '"environment_weather_simulation_invokes_backend" = "0"', '"environment_weather_simulation_native_handle_access" = "0"', '"environment_physical_weather_simulation_ready" = "0"', '"environment_weather_simulation_diagnostics" = "0"', 'environment_weather_simulation_replay_hash=[1-9]\d*', '"environment_weather_simulation_solver_budget_status" = "host_evidence_required"', '"environment_weather_simulation_solver_budget_ready" = "0"', '"environment_weather_simulation_cpu_reference_solver_ready" = "1"', '"environment_weather_simulation_solver_cpu_budget_us" = "50000"', '"environment_weather_simulation_solver_cpu_over_budget" = "0"', '"environment_weather_simulation_gpu_solver_ready" = "1"', 'environment_weather_simulation_solver_gpu_elapsed_us=[1-9]\d*', '"environment_weather_simulation_solver_gpu_budget_us" = "500000"', '"environment_weather_simulation_d3d12_gpu_solver_ready" = "1"', '"environment_weather_simulation_d3d12_gpu_solver_cells" = "4"', '"environment_weather_simulation_d3d12_gpu_solver_dispatches" = "1"', 'environment_weather_simulation_d3d12_gpu_solver_barriers=[1-9]\d*', '"environment_weather_simulation_d3d12_gpu_solver_native_handle_access" = "0"', '"environment_weather_simulation_d3d12_gpu_solver_backend_parity_ready" = "0"', '"environment_weather_simulation_d3d12_gpu_solver_failure_stage" = "0"', 'environment_weather_simulation_d3d12_gpu_solver_hash=[1-9]\d*', '"environment_weather_simulation_solver_profiler_artifacts" = "2"', '"environment_weather_simulation_solver_profiler_tool_rows" = "2"', '"environment_weather_simulation_solver_profiler_backend_rows" = "1"', 'environment_weather_simulation_solver_profiler_artifact_hash=[1-9]\d*', '"environment_weather_simulation_profiler_budget_ready" = "1"', '"environment_weather_simulation_production_solver_package_counter_review_ready" = "1"', '"environment_weather_simulation_production_solver_package_counter_rows" = "1"', '"environment_weather_simulation_production_solver_core_review_ready" = "1"', '"environment_weather_simulation_production_solver_core_rows" = "1"', '"environment_weather_simulation_production_solver_ready" = "0"', 'environment_weather_simulation_solver_cpu_elapsed_us=\d+', '"environment_weather_simulation_validation_dataset_status" = "ready"', '"environment_weather_simulation_validation_dataset_ready" = "1"', '"environment_weather_simulation_validation_case_rows" = "3"', '"environment_weather_simulation_validation_required_cases" = "3"', '"environment_weather_simulation_validation_ready_cases" = "3"', '"environment_weather_simulation_validation_supersaturated_condensation_ready" = "1"', '"environment_weather_simulation_validation_forced_evaporation_precipitation_ready" = "1"', '"environment_weather_simulation_validation_clamped_mixed_grid_ready" = "1"', '"environment_weather_simulation_validation_image_status" = "ready"', '"environment_weather_simulation_validation_images_ready" = "1"', '"environment_weather_simulation_validation_image_rows" = "12"', '"environment_weather_simulation_validation_required_images" = "12"', '"environment_weather_simulation_validation_supersaturated_condensation_images_ready" = "1"', '"environment_weather_simulation_validation_forced_evaporation_precipitation_images_ready" = "1"', '"environment_weather_simulation_validation_clamped_mixed_grid_images_ready" = "1"', '"environment_weather_simulation_validation_water_error_bound_mg" = "1"', '"environment_weather_simulation_validation_diagnostics" = "0"', '"environment_weather_simulation_validation_image_diagnostics" = "0"', 'environment_weather_simulation_validation_max_water_error_mg=[01]', 'environment_weather_simulation_validation_dataset_hash=[1-9]\d*', 'environment_weather_simulation_validation_image_hash=[1-9]\d*', '"environment_weather_simulation_artist_control_status" = "ready"', '"environment_weather_simulation_artist_controls_ready" = "1"', '"environment_weather_simulation_artist_control_rows" = "4"', '"environment_weather_simulation_artist_control_generated_cells" = "4"', '"environment_weather_simulation_artist_control_raw_solver_internal_access" = "0"', '"environment_weather_simulation_artist_control_native_handle_access" = "0"', '"environment_weather_simulation_artist_control_invokes_gpu" = "0"', '"environment_weather_simulation_artist_control_invokes_backend" = "0"', '"environment_weather_simulation_artist_control_physical_weather_ready" = "0"', '"environment_weather_simulation_artist_control_diagnostics" = "0"', 'environment_weather_simulation_artist_control_hash=[1-9]\d*')) { if (-not $sample3dInstalledValidatorText.Contains($needle)) { Write-Error "tools/validate-installed-desktop-runtime.ps1 missing environment weather simulation package assertion: $needle" } }
    $sceneRendererHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp") -Raw
    foreach ($needle in @(
        "SceneMeshDrawPlan",
        "SceneMeshDrawPlanDiagnosticCode",
        "plan_scene_mesh_draws",
        "SceneLightingShadowPolicyDesc",
        "LightingShadowPolicyPlan",
        "plan_scene_lighting_shadow_policy"
    )) {
        if (-not $sceneRendererHeaderText.Contains($needle)) {
            Write-Error "engine/scene_renderer/include/mirakana/scene_renderer/scene_renderer.hpp missing generated 3D scene renderer API: $needle"
        }
    }
    $rendererShadowHeaderText = Get-Content -LiteralPath (Join-Path $root "engine/renderer/include/mirakana/renderer/shadow_map.hpp") -Raw
    foreach ($needle in @(
        "LightingShadowPolicyDesc",
        "LightingShadowPolicyPlan",
        "LightingShadowPolicyLightRow",
        "plan_lighting_shadow_policy",
        "has_lighting_shadow_policy_diagnostic"
    )) {
        if (-not $rendererShadowHeaderText.Contains($needle)) {
            Write-Error "engine/renderer/include/mirakana/renderer/shadow_map.hpp missing lighting shadow policy API: $needle"
        }
    }
    $installedDesktopRuntimeValidationText = Get-Content -LiteralPath (Join-Path $root "tools/validate-installed-desktop-runtime.ps1") -Raw
    foreach ($field in @(
        "scene_mesh_plan_meshes",
        "scene_mesh_plan_draws",
        "scene_mesh_plan_unique_meshes",
        "scene_mesh_plan_unique_materials",
        "scene_mesh_plan_diagnostics",
        "--require-renderer-quality-gates",
        "renderer_quality_status",
        "renderer_quality_ready",
        "renderer_quality_diagnostics",
        "renderer_quality_expected_framegraph_passes",
        "renderer_quality_expected_framegraph_render_passes",
        "renderer_quality_expected_framegraph_barrier_steps",
        "renderer_quality_framegraph_passes_ok",
        "renderer_quality_framegraph_render_passes_ok",
        "renderer_quality_framegraph_barrier_steps_ok",
        "renderer_quality_framegraph_execution_budget_ok",
        "renderer_quality_scene_gpu_ready",
        "renderer_quality_postprocess_ready",
        "renderer_quality_postprocess_depth_input_ready",
        "renderer_quality_directional_shadow_ready",
        "renderer_quality_directional_shadow_filter_ready",
        "--require-d3d12-shadow-cascade-policy",
        "directional_shadow_cascade_count",
        "directional_shadow_cascade_tile_width",
        "directional_shadow_atlas_width",
        "directional_shadow_atlas_height",
        "directional_shadow_light_space_cascades",
        "directional_shadow_cascade_splits",
        "--require-lighting-shadow-policy",
        "lighting_shadow_policy_status",
        "lighting_shadow_policy_ready",
        "lighting_shadow_policy_diagnostics",
        "lighting_shadow_policy_lights",
        "lighting_shadow_policy_directional_lights",
        "lighting_shadow_policy_point_lights",
        "lighting_shadow_policy_spot_lights",
        "lighting_shadow_policy_shadowed_lights",
        "lighting_shadow_policy_directional_cascades",
        "lighting_shadow_policy_shadow_atlas_width",
        "lighting_shadow_policy_shadow_atlas_height",
        "lighting_shadow_policy_light_rows",
        "lighting_shadow_policy_ready_frames",
        "framegraph_passes_executed",
        "framegraph_render_passes_recorded",
        "framegraph_barrier_steps_executed",
        "--require-framegraph-multiqueue-evidence",
        "framegraph_multiqueue_command_lists_submitted",
        "framegraph_multiqueue_queue_waits_recorded",
        "framegraph_multiqueue_barriers_recorded",
        "framegraph_multiqueue_aliasing_barriers_recorded",
        "framegraph_multiqueue_pass_callbacks_invoked",
        "framegraph_multiqueue_submitted_pass_fences",
        "framegraph_multiqueue_graphics_waited_for_copy",
        "--require-native-ui-overlay",
        "hud_boxes",
        "ui_overlay_requested",
        "ui_overlay_status",
        "ui_overlay_ready",
        "ui_overlay_sprites_submitted",
        "ui_overlay_draws",
        "gameplay_systems_scene_binding_ready",
        "gameplay_systems_scene_binding_source_rows",
        "gameplay_systems_scene_binding_rows",
        "gameplay_systems_scene_binding_systems",
        "gameplay_systems_scene_binding_component_rows",
        "gameplay_systems_scene_binding_diagnostics",
        "gameplay_systems_scene_interaction_rows",
        "gameplay_systems_scene_interaction_diagnostics",
        "gameplay_systems_scene_interaction_final_session_state"
    )) {
        if (-not $installedDesktopRuntimeValidationText.Contains($field)) {
            Write-Error "tools/validate-installed-desktop-runtime.ps1 missing 3D scene mesh package telemetry validation field: $field"
        }
    }
}

$generated3dPackageManifestPath = "games/sample_generated_desktop_runtime_3d_package/game.agent.json"
$generated3dPackageManifestEntry = Get-GameAgentManifest $generated3dPackageManifestPath
if ($null -eq $generated3dPackageManifestEntry) {
    Write-Error "Generated 3D package gameplay systems smoke must have a sample game manifest: $generated3dPackageManifestPath"
} else {
    $generated3dPackageManifest = $generated3dPackageManifestEntry.Game
    $generated3dPackageManifestText = $generated3dPackageManifestEntry.Text
    $generated3dPackageReadmeText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/README.md") -Raw
    $generated3dPackageMainText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/main.cpp") -Raw
    $gamesCMakeText = Get-Content -LiteralPath (Join-Path $root "games/CMakeLists.txt") -Raw

    if ($generated3dPackageManifest.target -ne "sample_generated_desktop_runtime_3d_package") {
        Write-Error "$generated3dPackageManifestPath target must be sample_generated_desktop_runtime_3d_package"
    }
    foreach ($recipe in @($generated3dPackageManifest.validationRecipes)) {
        $isGenerated3dPackageRecipe = [string]$recipe.name -match "installed-d3d12|vulkan"
        $isDirectionalShadowRecipe = [string]$recipe.command -match "--require-directional-shadow"
        $isShadowMorphRecipe = [string]$recipe.command -match "--require-shadow-morph-composition"
        $isSceneCollisionPackageRecipe = [string]$recipe.command -match "--require-scene-collision-package"
        $isEntityScaleCullingRecipe = [string]$recipe.command -match "--require-entity-scale-culling"
        if ($isGenerated3dPackageRecipe -and -not $isDirectionalShadowRecipe -and -not $isShadowMorphRecipe -and -not $isSceneCollisionPackageRecipe -and -not $isEntityScaleCullingRecipe -and [string]$recipe.command -notmatch "--require-gameplay-systems") {
            Write-Error "$generated3dPackageManifestPath package validation recipe missing --require-gameplay-systems: $($recipe.name)"
        }
        if ($recipe.name -match "scene-collision-package" -and [string]$recipe.command -notmatch "--require-scene-collision-package") {
            Write-Error "$generated3dPackageManifestPath scene collision package recipe missing --require-scene-collision-package: $($recipe.name)"
        }
        if ($recipe.name -match "shadow-morph" -and [string]$recipe.command -notmatch "--require-shadow-morph-composition") {
            Write-Error "$generated3dPackageManifestPath shadow-morph recipe missing --require-shadow-morph-composition: $($recipe.name)"
        }
        if ($recipe.name -match "native-ui-overlay" -and [string]$recipe.command -notmatch "--require-native-ui-overlay") {
            Write-Error "$generated3dPackageManifestPath native UI overlay recipe missing --require-native-ui-overlay: $($recipe.name)"
        }
        if ($recipe.name -match "visible-production-proof") {
            if ($recipe.name -match "vulkan") {
                if ([string]$recipe.command -notmatch "--require-vulkan-visible-3d-production-proof") {
                    Write-Error "$generated3dPackageManifestPath Vulkan visible production proof recipe missing --require-vulkan-visible-3d-production-proof: $($recipe.name)"
                }
            }
            elseif ([string]$recipe.command -notmatch "--require-visible-3d-production-proof") {
                Write-Error "$generated3dPackageManifestPath visible production proof recipe missing --require-visible-3d-production-proof: $($recipe.name)"
            }
        }
        if ($recipe.name -match "entity-scale-culling" -and [string]$recipe.command -notmatch "--require-entity-scale-culling") {
            Write-Error "$generated3dPackageManifestPath entity scale/culling recipe missing --require-entity-scale-culling: $($recipe.name)"
        }
    }
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-shadow-morph-composition-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-overlay-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-entity-scale-culling-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-visible-production-proof-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-textured-sprite-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "installed-d3d12-3d-native-ui-text-glyph-atlas-smoke" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime/assets/3d/hud_text.uiatlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "shadow_receiver_shifted.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "runtime_ui_overlay.hlsl" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.dxil" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.vs.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "ui_overlay.ps.spv" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-shadow-morph-composition" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-overlay" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-visible-3d-production-proof" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-textured-sprite-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-native-ui-text-glyph-atlas" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "--require-gameplay-systems" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "sceneGameplayBinding" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "gameplay_systems_scene_binding_ready=1" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageManifestText "gameplay_systems_scene_interaction_final_session_state" $generated3dPackageManifestPath
    Assert-ContainsText $generated3dPackageReadmeText "--require-shadow-morph-composition" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-overlay" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-visible-3d-production-proof" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-textured-sprite-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-native-ui-text-glyph-atlas" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $generated3dPackageReadmeText "--require-gameplay-systems" "games/sample_generated_desktop_runtime_3d_package/README.md"
    Assert-ContainsText $gamesCMakeText "shadow_receiver_shifted.ps.dxil" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "GE_SAMPLE_SHIFTED_SCENE_SHADOW_RECEIVER_PS" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-gameplay-systems" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "UI_OVERLAY_SHADER_SOURCE" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-overlay" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-visible-3d-production-proof" "games/CMakeLists.txt"
    Assert-ContainsText $gamesCMakeText "--require-native-ui-textured-sprite-atlas" "games/CMakeLists.txt"
    $generated3dPackageUiAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    Assert-ContainsText $generated3dPackageUiAtlasText "page.0.asset_uri=runtime/assets/3d/base_color.texture.geasset" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud.uiatlas"
    $generated3dPackageUiTextGlyphAtlasText = Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas") -Raw
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "format=GameEngine.UiAtlas.v1" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.font_family=engine-default" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText $generated3dPackageUiTextGlyphAtlasText "glyph.0.glyph=65" "games/sample_generated_desktop_runtime_3d_package/runtime/assets/3d/hud_text.uiatlas"
    Assert-ContainsText (Get-Content -LiteralPath (Join-Path $root "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex") -Raw) "kind=ui_atlas_texture" "games/sample_generated_desktop_runtime_3d_package/runtime/sample_generated_desktop_runtime_3d_package.geindex"
    $newGameScaffoldText = (Get-Content -LiteralPath (Join-Path $root "tools/new-game.ps1") -Raw) +
        "`n" +
        (Get-Content -LiteralPath (Join-Path $root "tools/new-game-templates.ps1") -Raw)
    Assert-ContainsText $newGameScaffoldText "runtime/assets/3d/hud.uiatlas" "tools/new-game scaffolding"
    Assert-ContainsText $newGameScaffoldText "runtime/assets/3d/hud_text.uiatlas" "tools/new-game scaffolding"
    Assert-ContainsText $newGameScaffoldText "resident_catalog_refresh_failed" "tools/new-game scaffolding"
    Assert-ContainsText $newGameScaffoldText "resident_eviction_plan_failed" "tools/new-game scaffolding"
    foreach ($needle in @("pagePolicy = [ordered]@{", "single-page-tight-rgba8-texture-source", "pivot = [ordered]@{", "sliceBorder = [ordered]@{")) { Assert-ContainsText $newGameScaffoldText $needle "tools/new-game 2D sprite atlas source authoring scaffolding" }
    foreach ($needle in @(
        "--require-shadow-morph-composition",
        "require_shadow_morph_composition",
        "require_graphics_morph_scene",
        "load_packaged_d3d12_shifted_shadow_receiver_scene_shaders",
        "load_packaged_vulkan_shifted_shadow_receiver_scene_shaders",
        "skinned_scene_fragment_shader = mirakana::Win32DesktopPresentationShaderBytecode",
        "--require-gameplay-systems", "require_gameplay_systems",
        "gameplay_systems_status=",
        "gameplay_systems_ready=",
        "gameplay_systems_navigation_plan_status=",
        "gameplay_systems_navigation_navmesh_status=",
        "gameplay_systems_navigation_navmesh_dynamic_obstacles=",
        "gameplay_systems_navigation_crowd_status=",
        "gameplay_systems_navigation_crowd_source_order_ready=",
        "gameplay_systems_navigation_crowd_applied_neighbors=",
        "gameplay_systems_local_avoidance_status=",
        "gameplay_systems_local_avoidance_applied_neighbors=",
        "gameplay_systems_physics_policy_status=",
        "gameplay_systems_physics_policy_dynamic_pushes=",
        "solve_physics_constraints_3d", "PhysicsFixedConstraint3DDesc", "PhysicsLinearAxisConstraint3DDesc", "gameplay_systems_physics_constraints_status=", "gameplay_systems_physics_constraints_diagnostic=", "gameplay_systems_physics_constraints_rows=", "gameplay_systems_physics_constraints_fixed_rows=", "gameplay_systems_physics_constraints_linear_axis_rows=", "gameplay_systems_physics_constraints_axis_limit_clamped=", "plan_physics_simple_vehicle_3d", "PhysicsSimpleVehicle3DDesc", "gameplay_systems_vehicle_status=", "gameplay_systems_vehicle_diagnostic=", "gameplay_systems_vehicle_wheel_rows=", "gameplay_systems_vehicle_grounded_wheels=", "gameplay_systems_vehicle_wheel_probe_hits=",
        "gameplay_systems_blackboard_status=",
        "gameplay_systems_behavior_status=",
        "gameplay_systems_scene_binding_ready=",
        "gameplay_systems_scene_binding_source_rows=",
        "gameplay_systems_scene_binding_rows=",
        "gameplay_systems_scene_binding_systems=",
        "gameplay_systems_scene_binding_component_rows=",
        "gameplay_systems_scene_binding_diagnostics=",
        "gameplay_systems_scene_interaction_rows=",
        "gameplay_systems_scene_interaction_diagnostics=",
        "gameplay_systems_scene_interaction_final_session_state=",
        "gameplay_systems_audio_status=", "gameplay_systems_audio_first_sample=",
        "--require-native-ui-overlay",
        "--require-visible-3d-production-proof",
        "--require-native-ui-textured-sprite-atlas",
        "--require-native-ui-text-glyph-atlas",
        "require_native_ui_overlay",
        "require_visible_3d_production_proof",
        "require_native_ui_textured_sprite_atlas",
        "require_native_ui_text_glyph_atlas",
        "build_ui_renderer_image_palette_from_runtime_ui_atlas",
        "build_ui_renderer_glyph_atlas_palette_from_runtime_ui_atlas",
        "hud_boxes=",
        "hud_images=",
        "hud_text_glyphs=",
        "text_glyphs_resolved=",
        "ui_atlas_metadata_status=",
        "ui_atlas_metadata_glyphs=",
        "ui_texture_overlay_status=",
        "ui_overlay_requested=",
        "ui_overlay_status=",
        "ui_overlay_ready=",
        "ui_overlay_sprites_submitted=",
        "ui_overlay_draws=",
        "visible_3d_status=",
        "visible_3d_presented_frames=",
        "visible_3d_d3d12_selected=",
        "resident_catalog_refresh_failed",
        "resident_eviction_plan_failed",
        "load_packaged_d3d12_native_ui_overlay_shaders",
        "load_packaged_vulkan_native_ui_overlay_shaders",
        "native_ui_overlay_vertex_shader",
        "enable_native_ui_overlay",
        "enable_native_ui_overlay_textures"
    )) {
        Assert-ContainsText $generated3dPackageMainText $needle "games/sample_generated_desktop_runtime_3d_package/main.cpp"
    }
}

foreach ($registration in $desktopRuntimeGameRegistrations.Values) {
    $manifestRelativePath = Assert-DesktopRuntimeGameManifestPath $registration.gameManifest "desktop runtime target '$($registration.target)'"
    $manifest = Get-GameAgentManifest $manifestRelativePath
    if ($null -eq $manifest) {
        Write-Error "desktop runtime target '$($registration.target)' references missing GAME_MANIFEST: $manifestRelativePath"
        continue
    }
    $game = $manifest.Game
    if ($game.target -ne $registration.target) {
        Write-Error "$manifestRelativePath target '$($game.target)' does not match desktop runtime registration '$($registration.target)'"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-game-runtime")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-game-runtime"
    }
    if (-not (@($game.packagingTargets) -contains "desktop-runtime-release")) {
        Write-Error "$manifestRelativePath is registered with MK_add_desktop_runtime_game but does not declare desktop-runtime-release"
    }
    Assert-DesktopRuntimePackageFileRegistration $game $manifestRelativePath $registration
    Assert-DesktopRuntimePackageRecipe $game $manifestRelativePath
}

Write-Information "json-contract-check: ok" -InformationAction Continue
